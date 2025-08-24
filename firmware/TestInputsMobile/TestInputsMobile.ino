/*
 * TestInputsMobile.ino
 *
 * Test the double tap and activity signals using the LEDs as output instaed of the serial terminal.
 * This makes it easier to test while moving.
 *
 * IMPORTANT: When using <SparkFun_ADXL345.h>, you must set the following in SparkFun_ADXL345.cpp:
 * #define ADXL345_DEVICE (0x1D) //use alternate bus address
 *
 *  LED1: error display
 *
 *  LED3-6: counts double taps
 *  LED7-8: turns on when motion was detected recently 
 *
 * Corey McCall
 */

//sensitivity thresholds (max 255 for all values)
#define ACTIVITY_THRESHOLD 50  // 62.5 mg per increment
#define TAP_THRESHOLD 250      // 62.5 mg per increment. Keep large to prevent false positives.
#define TAP_DURATION 60        // 625 μs per increment
#define DOUBLE_TAP_LATENCY 100  //1.25 ms per increment.
#define DOUBLE_TAP_WINDOW 180  //1.25 ms per increment.


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

//event counters
byte doubleTapsDetected = 0;

//timers
#define MOTION_TIMEOUT_MS 2000
unsigned long lastMotionDetected_ms = 0;
#define DOUBLE_TAP_DEBOUNCE_MS 1000
unsigned long lastDoubleTapDetected_ms = 0;

void setup() {
  Serial.swap(1);  //alt TX on PA1
  Serial.begin(115200);
  Serial.println("Mobile sensor test");

  initHardware();

  configureADXL();
}

void loop() {
  checkInterrupts();

  updateLEDs();
}

//updates the LEDs based on current state
void updateLEDs() {
  digitalWrite(LED78, lastMotionDetected_ms && (millis() - lastMotionDetected_ms < MOTION_TIMEOUT_MS));

  for (int i = 1; i < 5; i++)
    digitalWrite(LEDPins[i + 1], doubleTapsDetected == i);
}

//flash error code forever to indicate a problem.
void error() {
  Serial.println("Error.");
  for (int i = 0; i < numLEDs; i++) {
    digitalWrite(LEDPins[i], LOW);
  }
  while (1) {
    digitalWrite(LED2, !digitalRead(LED2));
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
    Serial.println("raw double-tap detected");
    countDoubleTaps();
  }

  if (adxl.triggered(interrupts, ADXL345_ACTIVITY)) {
    Serial.println("raw motion detected");
    lastMotionDetected_ms = millis();
  }
}

//counts number of double taps detected with debounce
void countDoubleTaps() {
  if ((millis() - lastDoubleTapDetected_ms) > DOUBLE_TAP_DEBOUNCE_MS) {
    doubleTapsDetected = (doubleTapsDetected + 1) % 5;
    lastDoubleTapDetected_ms = millis();
    Serial.println("***debounced double-tap detected***");
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
    Serial.println("ADXL COM CHECK: Check connection and ADXL345_DEVICE ID is 0x1D in SparkFun_ADXL345.cpp");
    error();
  }
}