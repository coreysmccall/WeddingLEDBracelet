/*
 * testHardware.ino
 * Tests hardware by blinking LEDs and printing accelerometer samples to the serial terminal.
 *
 * IMPORTANT: <When using SparkFun_ADXL345.h>, you must set the following in SparkFun_ADXL345.cpp:
 * #define ADXL345_DEVICE (0x1D) //use alternate bus address
 *
 * TODO:
 * - heart button
 *
 * Corey McCall
 */

#include <SparkFun_ADXL345.h>

#define UPDATE_PERIOD_MS 100  //update period for LEDs

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
const byte numLEDs = 7;
const pin_size_t LEDPins[numLEDs] = { LED1, LED2, LED3, LED4, LED5, LED6, LED78 };

ADXL345 adxl = ADXL345();

//State variables
byte currentLED = 0;
byte nextLED = 0;
unsigned long updateTimer = 0;
bool doubleTapDetected = false;
bool activityDetected = false;

void setup() {
  byte errorCode = initHardware();
  Serial.println("Test Wedding LED Bracelet Bracelet");
  if (errorCode)
    error(errorCode);

  configureADXL();
}

void loop() {
  //Output data to to serial
  printAccelSample();

  //interrupt notifications
  checkInterrupts(true);

  //animate LEDs
  processAnimationIncrement();  // or use processAnimationRandomize();
  updateLEDs();                 //actual LED update happens here

  //maintain update rate
  if (UPDATE_PERIOD_MS >= (millis() - updateTimer)) {
    delay(UPDATE_PERIOD_MS - (millis() - updateTimer));
  }
  updateTimer = millis();
  //printTime();
}

void printTime() {
  Serial.print("Time [ms]: ");
  Serial.println(millis());
}

//configures accelerometer settings and attaches MCU interrupts
void configureADXL() {

  adxl.setRangeSetting(2); //range in Gs

  //attach interrupts to functions: interrupt1=double tap, interrupt2=activity
  adxl.setImportantInterruptMapping(0, 1, 0, 2, 0);

  //double tap
  adxl.setTapDetectionOnXYZ(0, 0, 1);  //tap on the sensor (z-axis)
  adxl.setTapThreshold(50); // 62.5 mg per increment
  adxl.setTapDuration(15); // 625 μs per increment
  adxl.setDoubleTapLatency(80); // 1.25 ms per increment
  adxl.setDoubleTapWindow(500); // 1.25 ms per increment
  adxl.doubleTapINT(true);
  attachInterrupt(digitalPinToInterrupt(INT_DOUBLETAP), ISR_DoubleTap, RISING);

  //activity
  adxl.setActivityXYZ(1, 1, 1);
  adxl.setActivityThreshold(20); // 62.5mg per increment
  adxl.ActivityINT(true);
  attachInterrupt(digitalPinToInterrupt(INT_ACTIVITY), ISR_Activity, RISING);
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

  if (printNotifications) {
    if (doubleTapDetected) {
      Serial.println("**********DOUBLE-TAP DETECTED**********");
      doubleTapDetected = false;
    }
    if (activityDetected) {
      Serial.println("**********ACTIVITY DETECTED**********");
      activityDetected = false;
    }
  }
}

//initializes hardware and returns error code if issue detected
byte initHardware() {
  //Serial
  Serial.swap(1);  //alt TX on PA1
  Serial.begin(115200);

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
    return 1;
  }

  return 0;
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

//increment LEDs in order
void processAnimationIncrement() {
  static bool initialized = false;
  if (!initialized) {
    currentLED = 6;
    initialized = true;
  }
  nextLED = (currentLED + 1) % numLEDs;
}

//increment LEDs randomly
void processAnimationRandomize() {
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

//error display
void error(byte code) {
  switch (code) {
    case 1:
      Serial.println("ADXL communication error: Check connection and ADXL345_DEVICE ID is 0x1D in SparkFun_ADXL345.cpp");
      break;
  }

  while (1) {
    digitalWrite(LEDPins[0], HIGH);
    delay(200);
    digitalWrite(LEDPins[0], LOW);
    delay(200);
  };
}