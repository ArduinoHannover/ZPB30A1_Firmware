#ifndef TM1650_H
#define TM1650_H

#define I2C_DELAY 0

#define CHAR_OFFSET 54
#define LED_V       0x01
#define LED_AH      0x02
#define LED_WH      0x04
#define LED_A       0x08
#define LED_RUN     0x10
#define LED_HIGH    0x20
#define LED_LOW     0x40
 
uint8_t digits[] = {
	0x68,
	0x6A,
	0x6C,
	0x6E
};

uint8_t chars[] = {
	0x3F, // 0
	0x06, // 1
	0x5B, // 2
	0x4F, // 3
	0x66, // 4
	0x6D, // 5
	0x7D, // 6
	0x07, // 7
	0x7F, // 8
	0x6F, // 9
	0x00, // @ as SPACE
	0x77, // A
	0x7C, // b
	0x39, // C
	0x5E, // d
	0x79, // E
	0x71, // F
	0x3D, // G
	0x74, // h
	0x30, // I
	0x1E, // J
	0x70, // K
	0x38, // L
	0x37, // m
	0x54, // n
	0x5C, // o
	0x73, // P
	0x67, // q
	0x50, // r
	0x2D, // S
	0x78, // t
	0x3E, // U
	0x1C, // v
	0x14, // w
	0x76, // x
	0x6E, // y
	0x1B, // Z
	0x40, // -
};

void i2c_write(uint8_t data, uint8_t pin);
void disp_write(uint8_t addr, uint8_t data, uint8_t pin);
void setBrightness(uint8_t brightness, uint8_t pin);

void i2c_write(uint8_t data, uint8_t pin) {
	uint8_t i;
	GPIOC->DDR |= pin;
	for (i = 7; i < 255; i--) {
		if (data & (1 << i)) {
			GPIOC->ODR |= pin;
		} else {
			GPIOC->ODR &= ~pin;
		}
		// Clock H/L
		GPIOC->ODR |= GPIO_PIN_5;
		GPIOC->ODR &= ~GPIO_PIN_5;
	}
	// We don't need the ACK, so just do a single clock H/L without reading
	GPIOC->ODR |= GPIO_PIN_5;
	GPIOC->ODR &= ~GPIO_PIN_5;
}

void disp_write(uint8_t addr, uint8_t data, uint8_t pin) {
	// Start sequence
	GPIOC->ODR |= pin;         // SDA HIGH
	GPIOC->ODR |= GPIO_PIN_5;  // SCL HIGH
	GPIOC->ODR &= ~pin;        // SDA LOW
	GPIOC->ODR &= ~GPIO_PIN_5; // SCL LOW

	i2c_write(addr, pin);
	i2c_write(data, pin);

	// Stop sequence
	GPIOC->ODR &= ~pin;        // SDA LOW
	GPIOC->ODR |= GPIO_PIN_5;  // SCL HIGH
	GPIOC->ODR |= pin;         // SDA HIGH
}

void setBrightness(uint8_t brightness, uint8_t pin) {
	if (!brightness) disp_write(0x48, 0x00, pin); // OFF
	else disp_write(0x48, ((brightness & 7) << 4) | 0x01, pin); // 8 brightness levels
}

#endif
