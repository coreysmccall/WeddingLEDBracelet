/*
 * testHardware.ino
 * Tests hardware by blinking LEDs and printing accelerometer samples to the serial terminal.
 *
 * IMPORTANT: <When using SparkFun_ADXL345.h>, you must set the following in SparkFun_ADXL345.cpp:
 * #define ADXL345_DEVICE (0x1D) //use alternate bus address
 *
 * Corey McCall
 */


#include <SparkFun_ADXL345.h>

#define UPDATE_PERIOD_MS 100  //update period for LEDs

//hardware handles
const byte numLEDs = 7;
const byte LEDPins[numLEDs] = { PIN_PA5, PIN_PA4, PIN_PA3, PIN_PB4, PIN_PB2, PIN_PB3, PIN_PC0 };
ADXL345 adxl = ADXL345();

//State variables
byte currentLED = 0;
byte nextLED = 0;
unsigned long updateTimer = 0;

void setup() {
  initHardware();

  Serial.println("Test Wedding LED Bracelet Bracelet");
}

void loop() {

  //Output data to to serial
  printAccelSample();
  printTime();

  //animate LEDs
  processAnimationIncrement();  // or use processAnimationRandomize();
  updateLEDs(); //actual LED update happens here


  //maintain update rate
  if (UPDATE_PERIOD_MS >= (millis() - updateTimer)) {
    delay(UPDATE_PERIOD_MS - (millis() - updateTimer));
  }
  updateTimer = millis();
}

void printTime() {
  Serial.print("Time [ms]: ");
  Serial.println(millis());
}

void initHardware() {
  //LEDs
  for (int i = 0; i < numLEDs; i++) {
    pinMode(LEDPins[i], OUTPUT);
    digitalWrite(LEDPins[i], LOW);
  }

  //Serial
  Serial.swap(1);  //PA1
  Serial.begin(115200);

  //Accel
  adxl.powerOn();
  adxl.setRangeSetting(2);
}

//poll accelerometer and print sample
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