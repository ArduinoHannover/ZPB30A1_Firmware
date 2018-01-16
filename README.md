# ZPB30A1 Firmware

This repository is there to build an open-source firmware for the ZPB30A1 electronic load.

The original firmware is protected and therefore could not been read and improved partially. :warning: They also activated the write protection, so unfortunately **you have to replace the IC on the board**. But the processors are available at almost no cost on the same platform you bought the load on ;-) (or up to a quadruple of the price at regular distributors)

So if you plan to buy a load like this and want to "upgrade", consider ordering the processor and a cheap STLink V2 too (if you haven't got one already).

![Hacked](https://github.com/ArduinoHannover/ZPB30A1_Firmware/raw/master/images/hacked.jpg)

## Hardware

The ZPB30A1 uses a STM8S005K6 running at 16 MHz with an external 12 MHz crystal and internal 128 kHz.

### Processor specifications

- 32 kByte Flash
- 2 kByte RAM
- 128 Byte EEPROM
- External interrupts in almost all pins
- Up to 6 analog inputs in LQFP32 package
- 2x 16 Bit, 1x 8 Bit timers
- UART, SPI, I<sup>2</sup>C

### Pinout

| Pin  | Pin Name | Direction | Function                 | Note
| ---: | -------- | --------- | ------------------------ | ---
|   1  | NRST     | IN PUP    | NRST                     | Connect to ST-Link
|   2  | PA1      | XIN       | Crystal                  | 12 MHz
|   3  | PA2      | XOUT      | Crystal                  | 12 MHz
|   4  | VSS      | PWR       | GND                      | Ground
|   5  | VCAP     | PAS       | VCAP                     | 1uF for internal 1.8V regulator
|   6  | VDD      | PWR       | Vcc                      | 5V
|   7  | VDDIO    | PWR       | Vcc                      | 5V
|   8  | PF4/AIN12| -/-       | NC                       | Ground
|   9  | VDDA     | PWR       | Vcc                      | 5V (via PCB inductor)
|  10  | VSSA     | PWR       | GND                      | Ground
|  11  | PB5/AIN5 | IN PUP    | Encoder 2                | Frontpanel
|  12  | PB4/AIN4 | IN PUP    | Encoder 1                | Frontpanel
|  13  | PB3/AIN3 | IN Analog | Input voltage            | `VIN / 3`
|  14  | PB2/AIN2 | IN Analog | Sense voltage            | `0.137565 V * V_SENSE + 0.0965910 V`
|  15  | PB1/AIN1 | IN Analog | Load voltage             | `0.121221 V * V_POWER + 0.0847433 V` (real offset voltage by 510k : 10k would be ~0.0961538 V) 
|  16  | PB0/AIN0 | IN Analog | Thermistor input         | `135.5 - .12778 * ADC`
|  17  | PE5      | OUT       | Enable                   | Must be LOW to enable load regulation, otherwise MOSFET stays off (no load)
|  18  | PC1/T1C1 | OUT PWM   | I-Set                    | `I = 12.9027 * DUTY_PERCENT + 0.0130276` (800Hz)
|  19  | PC2/T1C2 | IN PUP    | Overload detect          | LOW when SET I is higher than the source can deliver
|  20  | PC3/T1C3 | IN PUP    | Encoder Button           | Frontpanel
|  21  | PC4/T1C4 | IN PUP    | Run Button               | Frontpanel
|  22  | PC5/SCK  | OUT       | Soft-I<sup>2</sup>C SCK  | Frontpanel
|  23  | PC6/MOSI | I/O       | Soft-I<sup>2</sup>C SDA1 | Frontpanel
|  24  | PC7/MISO | I/O       | Soft-I<sup>2</sup>C SDA2 | Frontpanel
|  25  | PD0/T3C2 | OUT PWM   | FAN                      | PWM speed control, 50 kHz, 1/3 duty on idle
|  26  | PD1/SWIM | PROG      | SWIM                     | Connect to ST-Link
|  27  | PD2/T3C1 | OUT PWM   | F                        | 50 kHz 50% duty
|  28  | PD3/T2C2 | IN PUP    | Voltage OK               | LOW when VIN > 9.6 V
|  29  | PD4/T2C1 | OUT PWM   | Buzzer                   | 2.364 kHz 50% duty
|  30  | PD5/U2TX | OUT       | Tx                       | 115200 baud 8n1
|  31  | PD6/U2RX | IN        | Rx                       | 115200 baud 8n1
|  32  | PD7/TLI  | IN        | L                        | Interrupt

I-SET runs now at 571 Hz, which gives also good results.

## Software

It seems like they have activated the Read Out Protection, so when running a flash readout, it just dumps zeros. So there is no real software reverse engineering possible. Software needs to be build from scratch.

As there are just a few compilers out there and just one open source solution, we'll be using [Small Device C Compiler (SDCC)](http://sdcc.sourceforge.net/).

### Functions

Development started using a STM8S Discovery, which is using a STM8S003K3. This is incompatible with the 005 on the load, as UART2 is UART1, PWM on FAN is not available and there are no internal PullUps on the encoder pins.

- [ ] Different modes:
 - [x] CC (Constant Current, as default)
 - [x] CW (Constant Power)
 - [x] CR (Constant Resistance)
 - [ ] BAT (Battery capacity test)
- [ ] Continous output of data via UART
- [ ] Logging of Ah, Wh, J (?) in every mode
- [x] Adjusting shunt resistance (if replaced with e.g. 100 mΩ for an offset as low as 20 mA instead of 200 mA, not the resistance itself but the value in software)
 - :information_source: Not needed, we can safely trim the current down to almost 0 mA by PWM
- [ ] Toggle beeper, auto shutdown
 - Currently no beeper active
- [x] Nice menus even though we got just that small seven segments


### Specifications

| Param           | Range    | Value
| --------------- | -------- | ---
| Voltage Input   | 0-30 V   | ± 2 digits (oversampling the 10 bit ADC to get 12 bit)
| Current control | 0-999 mA | ± 0.2 mA
|                 | 1-10 A   | ± 1 mA
| Temperature     | 30-90°C  | ± 1.5 °C


## Original Software

### Error codes

| Code   | Meaning
| ------ | ---
| `Err1` | Ultra-high voltage for battery capacity testing. [whatever this should be]
| `Err2` | V<sub>BATT</sub> &lt; V<sub>THRESHOLD</sub>, reversed or not connected
| `Err3` | Line resistance to high; The set current can't be delivered
| `Err4` | Circuit failed somewhere somehow. [Unknown error]
| `Err6` | Input voltage is inapropiate (12V/0.5A), if using a correct power supply, check PB3 and PD3 for shorts or open contacts
| `Otp`  | Over temperature protection [if everything is cool, check the thermistor value @ 25°C / 77°F]
| `Ert`  | Temperature sensore failure or temperature too low
| `ouP`  | Overvoltage Protection [Sense can measure 35 V max]
| `oPP`  | Transient power protection [Peaks?]