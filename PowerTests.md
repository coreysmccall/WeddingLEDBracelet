# Power Tests
Tests related to power consumption using a cheap CR2450 battery ([EEMB from Amazon](https://a.co/d/5ZDkXNy)).

These tests are only intended to give a general idea about the power delivery that can be expected. Only one battery was tested, which had been previously depleted to ~80% capacity.

## Max Current Stress Test
This test measures the maximum current stress by running [TestLEDMaxCurrent.ino](firmware/TestLEDMaxCurrent/TestLEDMaxCurrent.ino) in step mode, which turns the LEDs on one at a time in 200 ms steps until they are all on.

Using a 3.0 V bench supply, the max current drawn is ~53 mA. This aligns with the expected consumption of:

$$
(8 \times LED) \times \frac{(3V_{\text{supply}} - 2.0V_{\text{LED forward}})}{150\Omega} + (1mA_{\text{ATTINY816 @ 1MHz}}) \approx 54mA
$$

<img src="images/tests/PSU_LEDCurrentStep_I.png" alt="PSU_LEDCurrentStep_I" height="500">

Using the battery, it is able to source around ~38 mA at the same maximum step, resulting in a voltage drop of about 200 mV to ~2.7 V:

<img src="images/tests/EEMB_LEDCurrentStep_I.png" alt="EEMB_LEDCurrentStep_I" height="270">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/tests/EEMB_LEDCurrentStep_V.png" alt="EEMB_LEDCurrentStep_V" height="250">

The battery is also able to sustain >30 mA DC drain for several minutes with only slowly decreasing voltage plateauing around ~2.5V. This was also tested with [TestLEDMaxCurrent.ino](firmware/TestLEDMaxCurrent/TestLEDMaxCurrent.ino) (not in step mode):

<img src="images/tests/EEMB_drain_I.png" alt="EEMB_drain_I" height="270">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/tests/EEMB_drain_V.png" alt="EEMB_drain_V" height="250">

## PWM Pattern Operation
The Twinkle pattern in [TestPWMs.ino](firmware/TestPWMs/TestPWMs.ino) is a good indication of a typical LED pattern with reasonable brightness (shown in the [README](README.md) gif). This pattern randomly adjusts LED brightness via PWM without any attention to phase alignment. The individual duty cycles are low, but occasionally both PWM timers align and all 8 LEDs briefly draw max current for up to the max duty cycle (~16% @ ~500 Hz = ~310 µs). This peak is somewhat smoothed by the individual 4.7 µF capacitors on the [LED driver circuits](hardware/Bracelet/Sch_WeddingLEDBracelet_R3.pdf), but not much at this wide of a pulse width.  

On the bench supply, you can see the occasional max draw of ~53 mA when this occurs, agreeing with the stress test above.

<img src="images/tests/PSU_testPWMs_I.png" alt="PSU_testPWMs_I" height="500">

On the battery, the consumption also agrees with the stress test, maxing out at ~35 mA:

<img src="images/tests/EEMB_testPWMs_I.png" alt="EEMB_testPWMs_I" height="270">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/tests/EEMB_testPWMs_V.png" alt="EEMB_testPWMs_V" height="250">

Unlike the DC drain test, the battery voltage can now be easily maintained at its nominal 3V, as the average current is much lower and it only needs to recover from extremely short pulses—the worst being when both PWM timers overlap like the one below:

<img src="images/tests/EEMB_testPWMs_I_zoom.png" alt="EEMB_testPWMs_I_zoom" height="500">

## Non-LED Power Consumption
The LEDs are the main consumers of current. However, the MCU, accelerometer, and touch sensor also consume current. This can be measured from [TestHardware.ino](firmware/TestHardware/TestHardware.ino) before the LED pattern begins. This current is <1 mA with periodic spikes to 1.5 mA from the PTC (MCU touch sensor controller).

<img src="images/tests/EEMB_TestHardware_I.png" alt="EEMB_TestHardware_I" height="500">

## Safe Current Draw Conclusion
- It seems to be safe to draw the maximum current of the circuit, at least in bursts of up to ~310 µs (max PWM duty cycle in the [TestPWMs.ino](firmware/TestPWMs/TestPWMs.ino) pattern).
- Sustained max current draw is possible, with >30 mA delivery for at least a few minutes at a time at the cost of reduced battery voltage.
- MCU brownout happens at 1.8 V, and accelerometer brownout happens at 2.0 V. Neither of these are at risk when using patterns like the ones in these test programs and a reasonably fresh battery like the one measured here.
- There is probably no practical benefit from power-optimizing the accelerometer, PWM phase alignment, or MCU power states (at least at 1 MHz MCU clock), when using features like the ones in these test programs.
