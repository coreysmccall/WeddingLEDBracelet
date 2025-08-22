/*
 * TestLEDMaxCurrent.ino
 * Turns on all LEDs to test battery output
 *
 *
 * Corey McCall
 */

//select mode
#define STEP_MODE 1 //set to 1 to step instead of turn on all LEDs

//hardware handles
#define LED1 PIN_PA5
#define LED2 PIN_PA4
#define LED3 PIN_PA3
#define LED4 PIN_PB4
#define LED5 PIN_PB2
#define LED6 PIN_PB3
#define LED78 PIN_PC0
const byte numLEDPins = 7;
const pin_size_t LEDPins[numLEDPins] = { LED1, LED2, LED3, LED4, LED5, LED6, LED78 };


void setup() {
  initHardware();

  Serial.println("LEDCurrentStep.ino");
}

void loop() {
  static byte LEDsOn = 0;

  //step up the LED power in a sawtooth
  for (int i = 0; i < numLEDPins; i++)
    digitalWrite(LEDPins[i], (i < LEDsOn));

#if (STEP_MODE)
  LEDsOn = (LEDsOn + 1) % (numLEDPins + 1);
#else
  LEDsOn = 7;
#endif

  delay(200);
}

//initializes R3 hardware
void initHardware() {

  //Serial
  Serial.swap(1);  //PA1
  Serial.begin(115200);

  for (int i = 0; i < numLEDPins; i++) {
    pinMode(LEDPins[i], OUTPUT);
    digitalWrite(LEDPins[i], LOW);
  }
}
