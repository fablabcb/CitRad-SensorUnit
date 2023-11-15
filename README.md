# CityRadar
Citizen Science Traffic Radar 

The hardware presented in this repo should be treated as base for your own amplifier development. It should not be used without further understanding of the circuit for the CityRadar application. Please consider the following lessons learned
1. The expected gain is not needed at all. The chosen audio board for the Teensy also includes an integrated amplifier, which was not considered during the early planning stage.
2. The utilized amplifiers in the VGA circuit are not suited for the high amplification rates. Since a VGA is already included in the audio board, it can be omitted on a dedicated PCB.
3. Power line filtering is the most important part. A filter based on a choke and capacitors is good enough. The filter is placed in the supply of the radar module. However, when the SD card of the Teensy is utilized to log the data, a significant noise is generated during the write cycles. To remove this, a re-design of the Teensy PCB would be required, since the high-frequency data lines of the SD card are routed near the analog lines.
4. A possible way to fix this, is to use dedicated LDO regulators for the Teensy/Audio board and for the radar sensor. An isolated DC/DC converter can also help to decouple the radar supply from the teensy supply.
5. The integrated amplifier of the audio board is fine for most applications. If the signal should be too noisy or a higher amplification is needed, the following options can be considered:
- The transistor-based input stage shows a good, low-noise amplification. However, the gain should be decreased to 20dB.
- The low-pass filter stage can also be used to reduce the noise on the measured signal, but, again, the gain should be reduced to 20dB. A digital filter is also fine for most applications.
- Use a commercially availyble amplifier board. It should not be based on the LM358, but the LMV358 to ensure low noise amplification.
- An amplifier based on a JEFT, such as e.g. presented here Amplify small signals in low-noise circuit with discrete JFET - Planet Analog, could result in a better low noise amplification compared to the transistor.

A finalized and easy to reproduce hardware can be found here: https://community.fablab-cottbus.de/c/projekte/cityradar/43
