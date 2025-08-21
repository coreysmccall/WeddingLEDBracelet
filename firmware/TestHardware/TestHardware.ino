/*
 * testHardware.ino
 *
 * Tests hardware:
 * - Cycles all LEDs
 * - Polls and prints accelerometer reading
 * - Detects motion and double taps via external interrupt from ADXL
 * - Detects press and releases on the "heart" pad using Programmable Touch Controller (PTC)
 *   (Note: Touch sensor accuracy is not expected to be good considering the arm is directly below
 *    the pad. You should probably use double-tap detection for user interaction instead.)
 *
 * IMPORTANT: <When using SparkFun_ADXL345.h>, you must set the following in SparkFun_ADXL345.cpp:
 * #define ADXL345_DEVICE (0x1D) //use alternate bus address
 *
 *
 * Corey McCall
 */

#include <SparkFun_ADXL345.h>
#include <ptc.h>

//refresh rates
#define LED_UPDATE_PERIOD_MS 100   //update period for LEDs
#define ACCEL_POLL_PERIOD_MS 3000  //update period for Accel data printing

//hardware handles
#define LED1 PIN_PA5
#define LED2 PIN_PA4
#define LED3 PIN_PA3
#define LED4 PIN_PB4
#define LED5 PIN_PB2
#define LED6 PIN_PB3
#define LED78 PIN_PC0
#define INT_ACTIVITY PIN_PA6
#define INT_DOUBLETAP PIN_PB5
#define TOUCH_HEART PIN_PA7
const byte numLEDs = 7;
const pin_size_t LEDPins[numLEDs] = { LED1, LED2, LED3, LED4, LED5, LED6, LED78 };

//sensors
cap_sensor_t heartButton;
ADXL345 adxl = ADXL345();

//LED state variables
byte currentLED = 0;
byte nextLED = 0;
unsigned long accelPollingTimer = 0;
unsigned long LEDUpdateTimer = 0;

//sensor state variables
bool doubleTapDetected = false;
bool activityDetected = false;
bool heartTouchDetected = false;
bool heartReleaseDetected = false;

void setup() {
  Serial.swap(1);  //alt TX on PA1
  Serial.begin(115200);
  Serial.println("Test Wedding LED Bracelet Bracelet");

  initHardware();

  configureADXL();
  configurePTC();
}

void loop() {
  checkInterrupts(true);   //notifications from interrupts
  PTCHandler(true, true);  //touch button handler

  //Print Accelerometer Samples
  if ((millis() - accelPollingTimer) >= ACCEL_POLL_PERIOD_MS) {
    accelPollingTimer = millis();
    printAccelSample();
  }

  //Update LEDs
  if ((millis() - LEDUpdateTimer) >= LED_UPDATE_PERIOD_MS) {
    LEDUpdateTimer = millis();
    calculateAnimationIncrement();  // or use calculateAnimationRandomize();
    updateLEDs();                   //actual LED port update happens here
  }
}

void printTime() {
  Serial.print("Time [ms]: ");
  Serial.println(millis());
}

//configures PTC settings
void configurePTC() {
  //PTC settings
  //ptc_node_set_thresholds(&heartButton, 0, 0);
  //ptc_node_set_prescaler(&heartButton, 0);
  //ptc_node_set_gain(&heartButton, 0);
  //ptc_node_set_oversamples(&heartButton, 0);
  //ptc_node_set_charge_share_delay(&heartButton, 0);
}

//handles PTC and prints notifications and/or errors
void PTCHandler(bool printNotifications, bool printErrors) {

  //PTC will not run if there are errors...so simply clear them!
  if (heartButton.state.error) {
    if (printErrors)
      Serial.println("PTC recovered from error.");
    heartButton.state.error = 0;
  }

  //handle PTC
  ptc_process(millis());

  //notifications from PTC touch callback
  if (heartTouchDetected) {
    if (printNotifications)
      Serial.println("*HEART TOUCHED*");
    heartTouchDetected = false;
  }
  if (heartReleaseDetected) {
    if (printNotifications)
      Serial.println("*HEART RELEASED*");
    heartReleaseDetected = false;
  }
}

//PTC touch callback
void ptc_event_cb_touch(const ptc_cb_event_t eventType, cap_sensor_t *node) {
  if (PTC_CB_EVENT_TOUCH_DETECT == eventType) {
    heartTouchDetected = true;
  } else if (PTC_CB_EVENT_TOUCH_RELEASE == eventType) {
    heartReleaseDetected = true;
  }
}

//PTC calibration callback to check for calibration errors
/*void ptc_event_cb_calibration(const ptc_cb_event_t eventType, cap_sensor_t *node) {
  if (eventType == PTC_CB_EVENT_ERR_CALIB_LOW
      || eventType == PTC_CB_EVENT_ERR_CALIB_HIGH
      || eventType == PTC_CB_EVENT_ERR_CALIB_TO)
    Serial.println("PTC Calibration Error");
}*/

//configures accelerometer settings and attaches MCU interrupts
void configureADXL() {

  adxl.setRangeSetting(2);  //range in Gs

  //attach interrupts to functions: interrupt1=double tap, interrupt2=activity
  adxl.setImportantInterruptMapping(0, 1, 0, 2, 0);

  //double tap
  adxl.setTapDetectionOnXYZ(0, 0, 1);  //tap on the sensor (z-axis)
  adxl.setTapThreshold(100);           // 62.5 mg per increment
  adxl.setTapDuration(15);             // 625 μs per increment
  adxl.setDoubleTapLatency(80);        // 1.25 ms per increment
  adxl.setDoubleTapWindow(500);        // 1.25 ms per increment
  adxl.doubleTapINT(true);
  attachInterrupt(digitalPinToInterrupt(INT_DOUBLETAP), ISR_DoubleTap, RISING);

  //activity
  adxl.setActivityXYZ(1, 1, 1);
  adxl.setActivityThreshold(50);  // 62.5mg per increment
  adxl.ActivityINT(true);
  attachInterrupt(digitalPinToInterrupt(INT_ACTIVITY), ISR_Activity, RISING);
}

//print accelerometer sample
void printAccelSample() {
  int x, y, z;
  adxl.readAccel(&x, &y, &z);
  Serial.print("Accel [x,y,z]: ");
  Serial.print(x);
  Serial.print(",");
  Serial.print(y);
  Serial.print(",");
  Serial.print(z);
  Serial.println();
}

//ISR for double tap detection
void ISR_DoubleTap() {
  doubleTapDetected = true;
}

//ISR for activity detection
void ISR_Activity() {
  activityDetected = true;
}

//clears interrupt and prints notification if desired
void checkInterrupts(bool printNotifications) {
  //there were no interrupts
  if (!(digitalRead(INT_ACTIVITY) || digitalRead(INT_DOUBLETAP)))
    return;

  //clear interrupts
  adxl.getInterruptSource();


  if (doubleTapDetected) {
    if (printNotifications)
      Serial.println("*DOUBLE-TAP DETECTED*");
    doubleTapDetected = false;
  }
  if (activityDetected) {
    if (printNotifications)
      Serial.println("*ACTIVITY DETECTED*");
    activityDetected = false;
  }
}

//increment LEDs in order
void calculateAnimationIncrement() {
  static bool initialized = false;
  if (!initialized) {
    currentLED = 6;
    initialized = true;
  }
  nextLED = (currentLED + 1) % numLEDs;
}

//increment LEDs randomly
void calculateAnimationRandomize() {
  nextLED = currentLED;
  while ((nextLED = random(0, 7)) == currentLED)
    ;
}

//update the LEDs
void updateLEDs() {
  digitalWrite(LEDPins[currentLED], LOW);
  digitalWrite(LEDPins[nextLED], HIGH);
  currentLED = nextLED;
}

//initializes hardware
void initHardware() {

  //LEDs
  for (int i = 0; i < numLEDs; i++) {
    pinMode(LEDPins[i], OUTPUT);
    digitalWrite(LEDPins[i], LOW);
  }

  //ADXL interrupts
  pinMode(INT_ACTIVITY, INPUT);
  pinMode(INT_DOUBLETAP, INPUT);

  //Accel
  adxl.powerOn();
  if (adxl.get_bw_code() != 10) {  //check default register to know if it is actually working
    Serial.println("ADXL COM ERROR: Check connection and ADXL345_DEVICE ID is 0x1D in SparkFun_ADXL345.cpp");
  }

  //PTC (cap touch heart)
  ptc_add_selfcap_node(&heartButton, 0, PIN_TO_PTC(TOUCH_HEART));
}