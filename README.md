# ZPB30A1 Firmware

This repository is there to build an open-source firmware for the ZPB30A1 electronic load

## Hardware

The ZPB30A1 uses a STM8S003K3 running at 12 MHz

| Pin  | Pin Name | Function          | Note
| ---: | -------- | ----------------- | ---
|   1  | NRST     | NRST              | Connect to ST-Link
|   2  | PA1      | Crystal           | 12 MHz
|   3  | PA2      | Crystal           | 12 MHz
|   4  | VSS      | GND               |
|   5  | VCAP     | VCAP              | 1uF to GND
|   6  | VDD      | Vcc               | 5V
|   7  | VDDIO    | Vcc               | 5V
|   8  | PF4/AIN12| NC                |
|   9  | VDDA     | Vcc               | 5V via PCB inductor
|  10  | VSSA     | GND               |
|  11  | PB5/AIN5 | Encoder 2         | Frontpanel
|  12  | PB4/AIN4 | Encoder 1         | Frontpanel
|  13  | PB3/AIN3 | Input voltage     | `VIN / 3`
|  14  | PB2/AIN2 | Sense voltage     | `0.137565 V * V_SENSE + 0.0965910 V`
|  15  | PB1/AIN1 | Load voltage      | `0.121221 V * V_POWER + 0.0847433 V`
|  16  | PB0/AIN0 | Thermistor input  | <!--`t = 121.104 - 22.8585 * V_TEMP` (±1°C)-->
|  17  | PE5      | Enable            | Must be LOW to enable current regulation
|  18  | PC1/T1C1 | I-Set             | `I = 12.9027 * DUTY_PERCENT + 0.0130276` (800Hz)
|  19  | PC2/T1C2 | Overload detect   | LOW when SET I is higher than the source can deliver
|  20  | PC3/T1C3 | Encoder Button    | Frontpanel
|  21  | PC4/T1C4 | Run Button        | Frontpanel
|  22  | PC5/SCK  | SCK               | Frontpanel
|  23  | PC6/MOSI | SDA1              | Frontpanel
|  24  | PC7/MISO | SDA2              | Frontpanel
|  25  | PD0/T3C2 | FAN               | PWM speed control
|  26  | PD1/SWIM | SWIM              | Connect to ST-Link
|  27  | PD2/T3C1 | F                 | 50 kHz 50% duty
|  28  | PD3/T2C2 | Voltage OK        | LOW when VIN > 9.6 V
|  29  | PD4/T2C1 | Buzzer            | 2.364 kHz 50% duty
|  30  | PD5/U2TX | Tx                | 115200 baud 8n1
|  31  | PD6/U2RX | Rx                | 115200 baud 8n1
|  32  | PD7/TLI  | L                 | Interrupt