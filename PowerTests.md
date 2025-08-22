# Power Tests
Tests related to power consumption. In all tests,the MCU clock is left active (Clock = 1MHz), Accelerometer is connected but not powered on with `powerOn()`. When a battery is used, it is a cheap CR2450 ([EEMB from Amazon](https://a.co/d/5ZDkXNy))

These tests are only intended to give a general idea about the power delivery that can be expected. Only one battery was tested that was previous depleted to ~80% capacity.

## Max Current Stress Test
This tests the max current stress by running [TestLEDMaxCurrent.ino](firmware/TestLEDMaxCurrent/TestLEDMaxCurrent.ino) in step mode, which turns the LEDs on one at a time in 200ms steps until they are all on.

Using a 3.0V bench supply, the max current drawn is ~53mA. This aligns with the expected consumption of:

$$
(8 \times LED) \times \frac{(3V_{\text{supply}} - 2.0V_{\text{LEDforward}})}{150\Omega} + (1mA_{\text{ATTINY816 @ 1MHz}}) \approx 54mA
$$

<img src="images/tests/PSU_LEDCurrentStep_I.png" alt="PSU_LEDCurrentStep_I" width="300">

Using the battery, it is able to source around ~38mA at the same maximum step, resulting in a voltage drop of about 200mV to ~2.7V 

<img src="images/tests/EEMB_LEDCurrentStep_I.png" alt="EEMB_LEDCurrentStep_I" width="300">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="images/tests/EEMB_LEDCurrentStep_V.png" alt="EEMB_LEDCurrentStep_V" width="300">


 
