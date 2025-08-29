/*
 * Corey.ino
 * Corey's firmware for wedding bracelet.
 * 
 * There are two modes:
 * - TWINKLE: The default mode makes the bracelet twinkle
 * - DANCE: The second mode is for “dancing”: LEDs brightly flash or twinkle only around the time you are moving. 
 *  When you are still, all LEDs are very dim to indicate you are in this mode.
 * Change mode by firmly double tapping the bracelet near the space needle where it says “tap tap”
 *
 * You can also make the bracelet got to "sleep" by touching the heart for 5 seconds. This turns the bracelet off
 * except for a single dim LED. Touch the heart again to wake it back up.
 *
 * IMPORTANT: When using <SparkFun_ADXL345.h>, you must set the following in SparkFun_ADXL345.cpp:
 * #define ADXL345_DEVICE (0x1D) //use alternate bus address
 *
 *
 * Corey McCall
 */

#include <SparkFun_ADXL345.h>
#include <ptc.h>

//definitions
#define ASCENDING 1
#define DESCENDING 0
#define MODE_TWINKLE 0
#define MODE_DANCE 1

/***************** sensitivity thresholds  *****************/
//accelerometer (max 255 for accelerometer values)
#define MOTION_THRESHOLD 30          //motion magnitude. 62.5 mg per increment
#define TAP_THRESHOLD 250            //motion magnitude of tap. 62.5 mg per increment. Keep large to prevent false positives.
#define TAP_DURATION 60              //duration of tap. 625 μs per increment
#define DOUBLE_TAP_LATENCY 100       //delay before second-tap is checked. 1.25 ms per increment.
#define DOUBLE_TAP_WINDOW 180        //duration that second tap will be checked for. 1.25 ms per increment.
#define DOUBLE_TAP_DEBOUNCE_MS 1000  //minimum time between double-tap presses (for debounce)

//touch button
#define TOUCH_PRESS_LEVEL 400              //sensitivity to activate touch button
#define TOUCH_RELEASE_LEVEL 350            //sensitivity to release touch button (set lower than TOUCH_PRESS_LEVEL for some hysteresis)
#define SLEEP_MODE_PRESS_DURATION_MS 4000  //time to hold button before sleep is engaged. Max 4000 (the PTC calibration fails afer holding for 4 seconds)
/***********************************************************/

/******************** animation settings ********************/
//twinkle mode settings
#define TWINKLE_MIN_BRIGHTNESS 1                  //min brightness (max 255)
#define TWINKLE_MAX_BRIGHTNESS 30                 //max brightness (max 255)
#define TWINKLE_STEP TWINKLE_MAX_BRIGHTNESS / 10  //PWM steps per update
#define TWINKLE_ANIMATION_PERIOD_MS 66            //update period for LEDs

//dance mode settings
#define DANCE_MIN_BRIGHTNESS 1         //brightness (max 255)
#define DANCE_MAX_BRIGHTNESS 80       //brightness (max 255)
#define DANCE_ANIMATION_PERIOD_MS 80   //update period for LEDs
#define DANCE_MOTION_COOLDOWN_MS 4000  //cooldown after no movement is detected
#define DANCE_MOTION_WARMUP_MS 1000    //warmup period after mode is changed
/***********************************************************/

//hardware handles
#define LED1 PIN_PA5
#define LED2 PIN_PA4
#define LED3 PIN_PA3
#define LED4 PIN_PB4
#define LED5 PIN_PB2
#define LED6 PIN_PB3
#define LED78 PIN_PC0
#define TOUCH_HEART PIN_PA7
const byte numLEDs = 7;
const pin_size_t LEDPins[numLEDs] = { LED1, LED2, LED3, LED4, LED5, LED6, LED78 };

//sensors
ADXL345 adxl = ADXL345();
cap_sensor_t heartButton;

//system state variables
byte currentMode = MODE_TWINKLE;  //default mode
byte isAsleep = false;

//twinkle animation state variables
byte TwinkleState[numLEDs];
bool TwinkleDirection[numLEDs];

//dance animation state variables
byte currentLED = 0;
byte nextLED = 1;

//touch button state variables
bool heartPressed = false;

//timers
uint32_t animationTimer_ms = 0;
uint32_t lastMotionDetected_ms = 0;
uint32_t lastModeChange_ms = 0;
uint32_t lastHeartPressed_ms = 0;


void setup() {
  delay(100);
  //setup serial for debugging
  Serial.swap(1);  //alt TX on PA1
  Serial.begin(115200);
  Serial.println("Congratulations Patrick and Allison!");

  //setup hardware
  initHardware();
  configurePTC();
}

void loop() {
  //check for touch events for sleep control
  PTCHandler();

  //don't run animations or respond to inputs when asleep
  if (!isAsleep) {
    //check for accelerometer inputs  inputs
    ADXLHandler();

    //run current mode
    switch (currentMode) {
      case MODE_TWINKLE:
        runTwinkleMode();
        break;
      case MODE_DANCE:
        runDanceMode();
        break;
    }
  }
}

//configures PTC settings
void configurePTC() {
  ptc_node_set_thresholds(&heartButton, TOUCH_PRESS_LEVEL, TOUCH_RELEASE_LEVEL);
  ptc_node_set_gain(&heartButton, PTC_GAIN_1);  //use minimum gain to reduce false positives
}

//handles PTC and does PTC action
void PTCHandler() {
  //PTC will not run if there are errors. Clear them so it doesn't hang
  //Calibration errors will occur after touching for 4 seconds. Release will not register after that.
  if (heartButton.state.error) {
    heartButton.state.error = 0;
    Serial.println("PTC Error");
  }
  //handle PTC sensor
  ptc_process(millis());

  if (heartPressed && (millis() - lastHeartPressed_ms > SLEEP_MODE_PRESS_DURATION_MS)) {
    isAsleep = true;
    heartPressed = false;
    enterSleep();
    Serial.println("Sleep");
  }
}

//PTC touch callback
void ptc_event_cb_touch(const ptc_cb_event_t eventType, cap_sensor_t *node) {
  if (PTC_CB_EVENT_TOUCH_DETECT == eventType) {
    Serial.println("Touch");
    if (isAsleep) {
      Serial.println("Wake");
      isAsleep = false;
    }

    heartPressed = true;
    lastHeartPressed_ms = millis();
  } else if (PTC_CB_EVENT_TOUCH_RELEASE == eventType) {
    Serial.println("Release");
    heartPressed = false;
  }
}

//enter sleep mode
void enterSleep() {
  setAllLEDs(0);
  analogWriteLEDs(LED1, 1);
}

//changes the mode and debounces double-tap detection
void toggleMode() {

  //debounce
  static uint32_t lastDoubleTapDetected_ms = 0;
  if ((millis() - lastDoubleTapDetected_ms) < DOUBLE_TAP_DEBOUNCE_MS)
    return;
  lastDoubleTapDetected_ms = millis();

  //update mode
  currentMode = !currentMode;
  Serial.print("Switched to ");
  Serial.println((currentMode == MODE_TWINKLE) ? "Twinkle" : "Dance");

  //initialize mode
  lastModeChange_ms = millis();
  setAllLEDs((currentMode == MODE_DANCE) ? DANCE_MIN_BRIGHTNESS : TWINKLE_MIN_BRIGHTNESS);
}

//flash to indicate a permanent problem and reset.
void fatalError() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED1, !digitalRead(LED1));
    delay(250);
  }
  Serial.println("Fatal Error.");
  delay(10);
  _PROTECTED_WRITE(RSTCTRL.SWRR, 1);  //reset
}

//runs twinkle mode
void runTwinkleMode() {
  if ((millis() - animationTimer_ms) >= TWINKLE_ANIMATION_PERIOD_MS) {
    animationTimer_ms = millis();
    stepAnimationTwinkleLEDs();
  }
}

//executes one step of twinkle animation
void stepAnimationTwinkleLEDs() {
  //initialize randomly
  static bool initialized = false;
  if (!initialized) {
    for (int i = 0; i < numLEDs; i++) {
      TwinkleState[i] = random(TWINKLE_MIN_BRIGHTNESS + 1, TWINKLE_MAX_BRIGHTNESS - 1);
      TwinkleDirection[i] = random(0, 1);
    }
    initialized = true;
  }

  //step
  for (int i = 0; i < numLEDs; i++) {
    //skip sometimes to add randomness
    if (random(1, 10) <= 2)
      continue;

    //increase brightness by one step in current direction
    TwinkleState[i] = constrain(TwinkleState[i] + ((TwinkleDirection[i] == ASCENDING) ? TWINKLE_STEP : (-TWINKLE_STEP)), TWINKLE_MIN_BRIGHTNESS, TWINKLE_MAX_BRIGHTNESS);

    //switch directions when at the min or max value
    TwinkleDirection[i] = (TwinkleState[i] == TWINKLE_MAX_BRIGHTNESS)   ? DESCENDING
                          : (TwinkleState[i] == TWINKLE_MIN_BRIGHTNESS) ? ASCENDING
                                                                        : TwinkleDirection[i];

    //update the LED
    analogWriteLEDs(LEDPins[i], TwinkleState[i]);
  }
}

//runs dance mode
void runDanceMode() {
  //step the animation if the warmup period is over and motion was detected recently.
  if ((lastMotionDetected_ms > (lastModeChange_ms + DANCE_MOTION_WARMUP_MS)) && (millis() - lastMotionDetected_ms < DANCE_MOTION_COOLDOWN_MS)) {
    if ((millis() - animationTimer_ms) >= DANCE_ANIMATION_PERIOD_MS) {
      animationTimer_ms = millis();
      stepAnimationRandomizeLEDs();
    }
  } else {
    setAllLEDs(DANCE_MIN_BRIGHTNESS);
  }
}

//executes one step of randmize animation
void stepAnimationRandomizeLEDs() {
  //find random next LED
  while ((nextLED = random(0, numLEDs)) == currentLED)
    ;

  //move LED to next
  analogWriteLEDs(LEDPins[currentLED], DANCE_MIN_BRIGHTNESS);
  analogWriteLEDs(LEDPins[nextLED], DANCE_MAX_BRIGHTNESS);
  currentLED = nextLED;
}

//sets all LEDs to the same value
void setAllLEDs(byte brightness) {
  for (int i = 0; i < numLEDs; i++) {
    analogWriteLEDs(LEDPins[i], brightness);
  }
}

//configures accelerometer settings
void configureADXL() {
  //double tap
  adxl.setTapDetectionOnXYZ(0, 0, 1);  //tap on the sensor (z-axis)
  adxl.setTapThreshold(TAP_THRESHOLD);
  adxl.setTapDuration(TAP_DURATION);
  adxl.setDoubleTapLatency(DOUBLE_TAP_LATENCY);
  adxl.setDoubleTapWindow(DOUBLE_TAP_WINDOW);
  adxl.doubleTapINT(true);

  //activity
  adxl.setActivityXYZ(1, 1, 1);
  adxl.setActivityThreshold(MOTION_THRESHOLD);
  adxl.ActivityINT(true);

  //clear any interrupts
  adxl.getInterruptSource();
}

//clears interrupts from accelerometer and does interrupt action
void ADXLHandler() {

  //poll for motion interrupts
  byte interrupts = adxl.getInterruptSource();

  //attempt to change mode when double-tap is detected
  if (adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)) {
    toggleMode();
  }

  //keep track of last motion seen
  if (adxl.triggered(interrupts, ADXL345_ACTIVITY)) {
    lastMotionDetected_ms = millis();
  }
}

//initializes hardware
void initHardware() {

  //LEDs
  initPWM();

  //Accel
  adxl.powerOn();
  if (adxl.get_bw_code() != 10) {  //check default register to know if it is actually working
    Serial.println("Accelerometer problem. Check that I2C address = 0x1D");
    fatalError();
  }
  configureADXL();

  //PTC (cap touch heart)
  ptc_add_selfcap_node(&heartButton, 0, PIN_TO_PTC(TOUCH_HEART));
}

//Attaches PWM hardware timers and port muxes
void initPWM() {
  //default PWM frequency for all channels is clock / 2048 (~488Hz @ 1MHz clock)

  //set alternate ports LED4 and LED6
  PORTMUX.CTRLC =
    (1 << PORTMUX_TCA00_bp) |  // TCA0 WO0 -> PB3 (ALT) (LED6)
    (1 << PORTMUX_TCA01_bp);   // TCA0 WO1 -> PB4 (ALT) (LED4)

  //enable pins
  TCA0.SPLIT.CTRLB |= TCA_SPLIT_LCMP1EN_bm;  //enable LED4 PWM
  TCA0.SPLIT.CTRLB |= TCA_SPLIT_LCMP0EN_bm;  //enable LED6 PWM
  for (int i = 0; i < numLEDs; i++) {
    pinMode(LEDPins[i], OUTPUT);
  }
}

//analogWrite that works with this  hardware configuration
void analogWriteLEDs(pin_size_t LEDPin, byte brightness) {
  switch (LEDPin) {
    case LED4:  //LED4 and LED6 use alternate pins
      TCA0.SPLIT.LCMP1 = brightness;
      break;
    case LED6:
      TCA0.SPLIT.LCMP0 = brightness;
      break;
    case LED78:  //LED78 needs to be forced to zero for some reason
      if (brightness)
        analogWrite(LEDPin, brightness);
      else digitalWrite(LEDPin, LOW);
      break;
    default:
      analogWrite(LEDPin, brightness);
  }
}
