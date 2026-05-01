# Game Boy Advance: Bluetooth HID Gamepad

### DESCRIPTION
This simple circuit allows you to connect an **ESP32** to the Link Cable port of the **Game Boy Advance** or **Game Boy Advance SP**, giving it **bluetooth** connectivity and turning it into a Human Interface Device (**HID**) so it can be used as a **bluetooth gamepad** and connected to a computer or phone to control games from the GBA.

![GBA-BT-HID_6](./5.%20Photos/4.%20Completed/GBA-BT-HID_6.jpg)

### REQUIRED
*   1x Game Boy Advance Wireless Adapter (shell & connector)
*   1x PCB (Download the Gerber files and order it yourself, PCB Thickness: 1mm)
*   1x ESP-WROOM-32 ESP32-WROOM-NARROW
*   1x Resistor 0603 1K
*   2x Resistor 0603 10K
*   1x LED 0603
*   2x Switch bottom 4.6\*2.8mm
*   1x USB UART-CP2102 **(1)**

**(1)** This [USB UART adapter](https://www.aliexpress.com/item/1005009534233685.html) can be connected directly to the board without the need for cables or soldering. Just connect it and keep both boards at a 45-degree angle so all the pins make electrical contact. Hold them together with a rubber band and, after programming the code, disconnect the boards.


### NOTES
I had a lot of difficulties in 2020 trying to flash the code onto the ESP32, and I eventually gave up without succeeding. Now, in 2026, I gave it another try and finally managed to do it, but it is still quite complicated for anyone not familiar with ESP32 development tools.

**Shyri Villar**’s code needs to be compiled to generate the .bin file, and once you have it, it must be flashed onto the ESP32. To simplify the process, I uploaded the already compiled .bin file to my repository, so the only remaining step is flashing it onto the ESP32. I also uploaded instructions to help with that task.


### ATTRIBUTIONS
The original idea is not mine, but [**Shyri Villar**](https://github.com/Shyri)’s, who designed it for a GBA to NGC cable and developed the software. I only adapted it to work with the GBA wireless adapter.


### LICENSE
This project is licensed under a **Creative Commons** license:
**[Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0) ](https://creativecommons.org/licenses/by-nc-sa/4.0/)**

Check the [LICENSE.md](LICENSE.md) for more information.
