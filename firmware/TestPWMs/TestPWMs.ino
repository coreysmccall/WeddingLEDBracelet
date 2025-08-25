/*
 * TestPWMs.ino
 * Tests PWMs with twinkle pattern. There are four timers on ATTINY816: TCA, TCB, TCD, and RTC. LED1-6 (red) 
 * are used with the 6 independent outputs of TCA. LED78 controls two separate LEDs (orange) from the single
 * output of TCB. TCD is left to control millis() and micros() timekeeping, and RTC is not used. At least this
 * is how it is supposed to be setup. TCB appears to have some issues that are fixed in analogWriteLEDs().
 *
 *
 * Corey McCall
 */

//definitions
#define ASCENDING 1
#define DESCENDING 0

//twinkle settings
#define PWM_MAX 40             //max brightness (8 bits)
#define PWM_MIN 1              //min brightness (8 bits)
#define PWM_STEP PWM_MAX / 10  //PWM steps per update
#define UPDATE_PERIOD_MS 66    //update period for LEDs

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


//State variables
int PWMStates[numLEDPins];
bool PWMDirection[numLEDPins];
uint32_t updateTimer = 0;


void setup() {
  initHardware();

  Serial.println("Congratulations Patrick and Allison!!");
  Serial.println("testPWMs.ino");
}

void loop() {
  //Update LEDs
  if ((millis() - updateTimer) >= UPDATE_PERIOD_MS) {
    updateTimer = millis();
    stepAnimationTwinkleLEDs();
  }
}

//executes one step of twinkle animation
void stepAnimationTwinkleLEDs() {
  //initialize
  static bool initialized = false;
  if (!initialized) {
    for (int i = 0; i < numLEDPins; i++) {
      PWMStates[i] = random(PWM_MIN + 1, PWM_MAX - 1);
      PWMDirection[i] = random(0, 1);
    }
    initialized = true;
  }

  for (int i = 0; i < numLEDPins; i++) {
    //skip sometimes to add randomness
    if (random(1, 10) <= 2)
      continue;

    //oscillate LED brightness
    PWMStates[i] = constrain(PWMStates[i] + ((PWMDirection[i] == ASCENDING) ? PWM_STEP : (-PWM_STEP)), PWM_MIN, PWM_MAX);
    PWMDirection[i] = (PWMStates[i] == PWM_MAX)   ? DESCENDING
                      : (PWMStates[i] == PWM_MIN) ? ASCENDING
                                                  : PWMDirection[i];
    analogWriteLEDs(LEDPins[i], PWMStates[i]);
  }
}

//initializes R3 hardware
void initHardware() {

  //Serial
  Serial.swap(1);  //PA1
  Serial.begin(115200);

  //PWM
  initPWM();
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

  for (int i = 0; i < numLEDPins; i++) {
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