/*
 * TestInputsMobile.ino
 *
 * Test the double tap and activity signals using the LEDs as output instaed of the serial terminal.
 * This makes it easier to test while moving.
 *
 *  LED1: error display (mainly to show PTC error)
 *  LED2: on when heart is touched
 *  LED3-6: counts double taps
 *  LED7-8: turns on when motion was detected recently 
 *
 * IMPORTANT: When using <SparkFun_ADXL345.h>, you must set the following in SparkFun_ADXL345.cpp:
 * #define ADXL345_DEVICE (0x1D) //use alternate bus address
 *
 * Corey McCall
 */

/***************** sensitivity thresholds  *****************/
//accelerometer (max 255 for accelerometer values)
#define ACTIVITY_THRESHOLD 30   //motion magnitude. 62.5 mg per increment
#define TAP_THRESHOLD 250       //motion magnitude of tap. 62.5 mg per increment. Keep large to prevent false positives.
#define TAP_DURATION 60         //duration of tap. 625 μs per increment
#define DOUBLE_TAP_LATENCY 100  //delay before second-tap is checked. 1.25 ms per increment.
#define DOUBLE_TAP_WINDOW 180   //duration that second tap will be checked for. 1.25 ms per increment.

//touch button
#define TOUCH_PRESS_LEVEL 400   //sensitivity to activate touch button
#define TOUCH_RELEASE_LEVEL 350 //sensitivity to release touch button (set lower than TOUCH_PRESS_LEVEL for some hysteresis)

/***********************************************************/

#include <SparkFun_ADXL345.h>
#include <ptc.h>

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

//event counters
byte doubleTapsDetected = 0;

//touch button state variables
bool heartPressed = false;
bool PTC_error = false;

//timers
#define MOTION_TIMEOUT_MS 2000
uint32_t lastMotionDetected_ms = 0;
#define DOUBLE_TAP_DEBOUNCE_MS 1000
uint32_t lastDoubleTapDetected_ms = 0;

void setup() {
  initHardware();

  configureADXL();
  configurePTC();
}

void loop() {
  checkInterrupts();
  PTCHandler();

  updateLEDs();
}

//configures PTC settings
void configurePTC() {
  // Set the threshold for touch detection and away from touch for a node. 0= no change.
  ptc_node_set_thresholds(&heartButton, TOUCH_PRESS_LEVEL, TOUCH_RELEASE_LEVEL);
  //ptc_node_set_prescaler(&heartButton, PTC_PRESC_DIV2_gc); //PTC_PRESC_DIV2_gc,PTC_PRESC_DIV4_gc,PTC_PRESC_DIV8_gc,PTC_PRESC_DIV16_gc,PTC_PRESC_DIV32_gc,PTC_PRESC_DIV64_gc,PTC_PRESC_DIV128_gc,PTC_PRESC_DIV256_gc
  ptc_node_set_gain(&heartButton, PTC_GAIN_1); //PTC_GAIN_1,PTC_GAIN_2,PTC_GAIN_4,PTC_GAIN_8,PTC_GAIN_16,PTC_GAIN_32,PTC_GAIN_MAX
  //ptc_node_set_oversamples(&heartButton, 0); //0-6
  //ptc_node_set_charge_share_delay(&heartButton, 0); //0-15
}

//handles PTC
void PTCHandler() {
  //PTC will not run if there are errors...so simply clear them!
  if (heartButton.state.error) {
    heartButton.state.error = 0;
    PTC_error = true;
  } else {
    PTC_error = false;
  }

  //handle PTC
  ptc_process(millis());
}

//PTC touch callback
void ptc_event_cb_touch(const ptc_cb_event_t eventType, cap_sensor_t *node) {
  if (PTC_CB_EVENT_TOUCH_DETECT == eventType) {
    heartPressed = true;
  } else if (PTC_CB_EVENT_TOUCH_RELEASE == eventType) {
    heartPressed = false;
  }
}

//updates the LEDs based on current state
void updateLEDs() {

  //motion
  digitalWrite(LED78, lastMotionDetected_ms && (millis() - lastMotionDetected_ms < MOTION_TIMEOUT_MS));

  //double taps
  for (int i = 1; i < 5; i++)
    digitalWrite(LEDPins[i + 1], doubleTapsDetected == i);

  //heart button
  digitalWrite(LED2, heartPressed);
  
  //show the errors
  digitalWrite(LED1, PTC_error);
}

//flash to indicate a permanent problem.
void error() {
  for (int i = 0; i < numLEDs; i++) {
    digitalWrite(LEDPins[i], LOW);
  }
  while (1) {
    digitalWrite(LED1, !digitalRead(LED2));
    delay(250);
  }
}

//configures accelerometer settings
void configureADXL() {

  //adxl.setRangeSetting(2);  //the range setting does not effect the tap/activity detectors.

  //double tap
  adxl.setTapDetectionOnXYZ(0, 0, 1);  //tap on the sensor (z-axis)
  adxl.setTapThreshold(TAP_THRESHOLD);
  adxl.setTapDuration(TAP_DURATION);
  adxl.setDoubleTapLatency(DOUBLE_TAP_LATENCY);
  adxl.setDoubleTapWindow(DOUBLE_TAP_WINDOW);
  adxl.doubleTapINT(true);

  //activity
  adxl.setActivityXYZ(1, 1, 1);
  adxl.setActivityThreshold(ACTIVITY_THRESHOLD);
  adxl.ActivityINT(true);

  //clear any interrupts
  adxl.getInterruptSource();
}

//clears interrupts and does interrupt action
void checkInterrupts() {

  //poll for motion interrupts
  byte interrupts = adxl.getInterruptSource();

  if (adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)) {
    countDoubleTaps();
  }

  if (adxl.triggered(interrupts, ADXL345_ACTIVITY)) {
    lastMotionDetected_ms = millis();
  }
}

//counts number of double taps detected with debounce
void countDoubleTaps() {
  if ((millis() - lastDoubleTapDetected_ms) > DOUBLE_TAP_DEBOUNCE_MS) {
    doubleTapsDetected = (doubleTapsDetected + 1) % 5;
    lastDoubleTapDetected_ms = millis();
  }
}

//initializes hardware
void initHardware() {

  //LEDs
  for (int i = 0; i < numLEDs; i++) {
    pinMode(LEDPins[i], OUTPUT);
    digitalWrite(LEDPins[i], LOW);
  }

  //Accel
  adxl.powerOn();
  if (adxl.get_bw_code() != 10) {  //check default register to know if it is actually working
    error();
  }

  //PTC (cap touch heart)
  ptc_add_selfcap_node(&heartButton, 0, PIN_TO_PTC(TOUCH_HEART));
}