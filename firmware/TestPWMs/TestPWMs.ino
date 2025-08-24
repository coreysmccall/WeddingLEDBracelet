/*
 * TestPWMs.ino
 * Tests PWMs with twinkle pattern. There are four timers on ATTINY816: TCA, TCB, TCD, and RTC. LED1-6 (red) 
 * are used with the 6 independent outputs of TCA. LED78 controls two separate LEDs (orange) from the single
 * output of TCB. TCD is left to control millis() and micros() timekeeping, and RTC is not used.
 * 
 * Ideally, TCA and TCB should be phase-offset so that the two orange LEDS do not draw current at the same time
 * as any of the red LEDs. This seems to be easier said than done though. Currently the phase shift between 
 * TCA and TCB is not consistant and occationally overlap
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
  twinkleLEDs();
  twinkleLEDs();


  //maintain update rate
  if (UPDATE_PERIOD_MS >= (millis() - updateTimer)) {
    delay(UPDATE_PERIOD_MS - (millis() - updateTimer));
  }
  updateTimer = millis();
}

void twinkleLEDs_lowpower() {
  ;
}


void twinkleLEDs() {
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
    if (PWMDirection[i] == ASCENDING) {
      PWMStates[i] = min(PWMStates[i] + PWM_STEP, PWM_MAX);
      if (PWMStates[i] == PWM_MAX)
        PWMDirection[i] = DESCENDING;
    } else {
      PWMStates[i] = max(PWMStates[i] - PWM_STEP, PWM_MIN);
      if (PWMStates[i] == PWM_MIN)
        PWMDirection[i] = ASCENDING;
    }
    analogWriteWithMUX(LEDPins[i], PWMStates[i]);
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
  //set alternate ports LED4 and LED6
  PORTMUX.CTRLC =
    (1 << PORTMUX_TCA00_bp) |  // TCA0 WO0 -> PB3 (ALT) (LED6)
    (1 << PORTMUX_TCA01_bp);   // TCA0 WO1 -> PB4 (ALT) (LED4)

  //enable pins
  TCA0.SPLIT.CTRLB |= TCA_SPLIT_LCMP0EN_bm;  //enable LED6 PWM
  TCA0.SPLIT.CTRLB |= TCA_SPLIT_LCMP1EN_bm;  //enable LED4 PWM
  for (int i = 0; i < numLEDPins; i++) {
    pinMode(LEDPins[i], OUTPUT);
  }
}

//analogWrite that works with portmuxed pins
void analogWriteWithMUX(pin_size_t pin, byte duty) {
  switch (pin) {  //LED4 and LED6 use alternate pins
    case LED4:
      TCA0.SPLIT.LCMP1 = duty;
      break;
    case LED6:
      TCA0.SPLIT.LCMP0 = duty;
      break;
    default:
      analogWrite(pin, duty);
  }
}