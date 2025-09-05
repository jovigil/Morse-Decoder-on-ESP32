# Morse-Decoder-on-ESP32

This program allows the ESP32 to real-time decode a Morse code signal given by an LED or other light source. Consequently, this program's function is dependent on connecting a photoresistor to one of the ESP32 DevKit's ADC pins. Be sure to configure the macros defined in the top of the main file accordingly. There are also macros for adjusting the symbol rate and pause widths (though the user would need to match these to the rates of their light-based Morse encoder).

Make sure you have the following dependencies, as well as the esp-idf library.

```bash
sudo apt-get install fish neovim g++ git wget \
   flex bison gperf python3 python3-venv cmake \
   ninja-build ccache libffi-dev libssl-dev \
   dfu-util libusb-1.0-0
```