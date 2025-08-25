/*
 * Corey.ino
 * Corey's firmware for wedding bracelet.
 * 
 * There are two modes:
 * - TWINKLE: The default mode makes the bracelet twinkle
 * - DANCE: The second mode is for “dancing”: LEDs brightly flash or twinkle only around the time you are moving. When you are still, all LEDs are very dim to indicate you are in this mode.
 * Change mode by firmly double tapping the bracelet near the space needle where it says “tap tap”
 *
 * IMPORTANT: When using <SparkFun_ADXL345.h>, you must set the following in SparkFun_ADXL345.cpp:
 * #define ADXL345_DEVICE (0x1D) //use alternate bus address
 *
 * TODO: Change brightness beween low and high by holding the heart?
 *
 * Corey McCall
 */

#include <SparkFun_ADXL345.h>

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
/***********************************************************/


/************************ settings  ************************/
//twinkle mode settings
#define TWINKLE_MIN_BRIGHTNESS 1                  //min brightness (max 255)
#define TWINKLE_MAX_BRIGHTNESS 40                 //max brightness (max 255)
#define TWINKLE_STEP TWINKLE_MAX_BRIGHTNESS / 10  //PWM steps per update
#define TWINKLE_ANIMATION_PERIOD_MS 66            //update period for LEDs

//dance mode settings
#define DANCE_MIN_BRIGHTNESS 1         //brightness (max 255)
#define DANCE_MAX_BRIGHTNESS 150       //brightness (max 255)
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

//system state variables
byte currentMode = MODE_TWINKLE;

//twinkle animation state variables
byte TwinkleState[numLEDs];
bool TwinkleDirection[numLEDs];

//dance animation state variables
byte currentLED = 0;
byte nextLED = 1;

//timers
uint32_t animationTimer_ms = 0;
uint32_t lastMotionDetected_ms = 0;
uint32_t lastModeChange_ms = 0;

void setup() {
  //enable serial if needed (uses ~3% of program memory)
  /*Serial.swap(1);  //alt TX on PA1
  Serial.begin(115200);
  Serial.println("begin");*/

  initHardware();
}

void loop() {
  //check for accelerometer input
  checkADXLInterrupts();

  //run mode
  switch (currentMode) {
    case MODE_TWINKLE:
      runTwinkleMode();
      break;
    case MODE_DANCE:
      runDanceMode();
      break;
  }
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

  //initialize mode
  lastModeChange_ms = millis();
  setAllLEDs((currentMode == MODE_DANCE) ? DANCE_MIN_BRIGHTNESS : TWINKLE_MIN_BRIGHTNESS);
}

//flash to indicate a permanent problem.
void error() {
  while (1) {
    digitalWrite(LED1, !digitalRead(LED1));
    delay(250);
  }
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
void checkADXLInterrupts() {

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
    error();
  }
  configureADXL();
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
