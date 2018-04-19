#define F_CPU 16000000L
#define BAUDR 115200L
#define _MEM_(mem_addr) (*(volatile uint8_t *)(mem_addr))

#include "stm8s_conf.h"
#include "stdio.h"

bool dot;

void delay(uint32_t d) {
	while (d != 0) {
		d--;
	}
}
#include "tm1650.h"

#define DP_TOP GPIO_PIN_7
#define DP_BOT GPIO_PIN_6
#define OVERSAMPLING 2

#define MINUTE   6000
#define HOUR   360000

typedef struct {
	uint16_t charged_voltage;
	uint16_t cutoff_voltage;
	char     name[5];
} battery_voltage_t;

typedef enum {
	MODE_CC,
	MODE_CW,
	MODE_CR,
	MODE_CV,
} sink_mode_t;

typedef enum {
	ERROR_NONE,
	ERROR_UVP,  // Undervoltage protection
	ERROR_OVP,  // Overvoltage protection
	ERROR_OLP,  // Overload protection/warning
	ERROR_OTP,  // Over temperature protection
	ERROR_PWR,  // Insufficient power source
} error_t;

volatile bool     encoder_pressed = 0;
volatile bool     run_pressed     = 0;
volatile bool     calc_fan        = 0;
volatile bool     redraw          = 0;
bool              running         = 0;
bool              cutoff_active   = 0;
bool              beeper_on       = 0;
bool              brightness[]    = {0, 0};
bool              option_changed  = 0;
volatile int8_t   encoder_val     = 0;
sink_mode_t       set_mode        = MODE_CC;
error_t           error           = ERROR_NONE;
char              error_msg[][5]  = {
	"UVP@",
	"OVP@",
	"OLP@",
	"OTP@",
	"PWR@",
};
char              mode_name[][5]  = {
	"AMPS",
	"WATT",
	"OHMS",
	"VOLT"
};
uint16_t          set_values[4]; // CC/CW/CR/CV
uint16_t          cutoff_voltage  = 270;
uint16_t          temperature     = 0;
uint16_t          voltage         = 0;
uint16_t          set_current     = 0;
uint16_t          max_values[4]   = {
	10000, // 10A
	60000, // 60W
	50000, // 50Ω
	28000, // 28V
}
volatile uint32_t millis          = 0;
volatile uint32_t ampere_seconds  = 0;
volatile uint32_t watt_seconds    = 0;

#define MEM_CHECK 0x00
#define MEM_MODE  0x01
#define MEM_BEEP  0x02
#define MEM_CUTO  0x03
#define MEM_CC    0x10
//                0x11
#define MEM_CW    0x20
//                0x21
#define MEM_CR    0x30
//                0x31
#define MEM_CV    0x40
//                0x41
#define MEM_CUTV  0x50
//                0x51
#define MEM_TOUT  0x52
//                0x53
#define MEM_CUTV  0x54
//                0x55
#define MEM_CUTV  0x56
//                0x57
#define MEM_CUTV  0x58
//                0x59

battery_voltage_t voltages[] = {
	{
		150,
		 90,
		"NIMH"
	},
	{
		241,
		193,
		"LEAD"
	},
	{
		365,
		210,
		"LIFE"
	},
	{
		410,
		300,
		"LIPO"
	},
	{
		420,
		330,
		"LIIO"
	}
};

void setupUART(void) {
	uint32_t Mant, Mant100;
	Mant = ((uint32_t)F_CPU / (BAUDR << 4));
	Mant100 = (((uint32_t)F_CPU * 100) / (BAUDR << 4));
#ifdef STM8S003
	UART1->BRR2  = (uint8_t)((uint8_t)(((Mant100 - (Mant * 100)) << 4) / 100) & (uint8_t)0x0F);
	UART1->BRR2 |= (uint8_t)((Mant >> 4) & (uint8_t)0xF0);
	UART1->BRR1  = (uint8_t)Mant;
#else
	UART2->BRR2 = (uint8_t)(((uint8_t)(((Mant100 - (Mant * 100)) << 4) / 100) & (uint8_t)0x0F) | ((Mant >> 4) & (uint8_t)0xF0));
	UART2->BRR1 = (uint8_t)Mant;
#endif
}

void setup(void) {
	CLK->CKDIVR = CLK_PRESCALER_HSIDIV1;
	CLK->ICKR |= CLK_ICKR_LSIEN;
	CLK->ECKR |= CLK_ECKR_HSEEN;
	while ((CLK->ECKR & CLK_FLAG_HSERDY) == 0);
	CLK->SWR = 0xb4;
	//CLK->SWCR = CLK_SWCR_SWEN;*/
	CLK->CKDIVR = 0;

	GPIOB->CR1 |= GPIO_PIN_4; // ENC A PullUp
	GPIOB->CR2 |= GPIO_PIN_4; // ENC A Interrupt
	GPIOB->CR1 |= GPIO_PIN_5; // ENC B PullUp
	GPIOB->CR2 |= GPIO_PIN_5; // ENC B Interrupt

	GPIOC->DDR |= GPIO_PIN_1; // SET I Output
	GPIOC->CR1 |= GPIO_PIN_1; // SET I PUSH PULL
	GPIOC->CR1 |= GPIO_PIN_2; // OL PullUp
	GPIOC->CR2 |= GPIO_PIN_2; // OL Interrupt
	GPIOC->CR1 |= GPIO_PIN_3; // ENC PullUp
	GPIOC->CR2 |= GPIO_PIN_3; // ENC Interrupt
	GPIOC->CR1 |= GPIO_PIN_4; // RUN PullUp
	GPIOC->CR2 |= GPIO_PIN_4; // RUN Interrupt
	GPIOC->DDR |= GPIO_PIN_5; // SCL Output
	GPIOC->CR1 |= GPIO_PIN_5; // SCL PUSH PULL
	GPIOC->CR2 |= GPIO_PIN_5; // SCL 10 MHz
	GPIOC->DDR |= GPIO_PIN_6; // SDA1 Output
	GPIOC->CR1 |= GPIO_PIN_6; // SDA1 PUSH PULL
	GPIOC->CR2 |= GPIO_PIN_6; // SDA1 10 MHz
	GPIOC->DDR |= GPIO_PIN_7; // SDA2 Output
	GPIOC->CR1 |= GPIO_PIN_7; // SDA2 PUSH PULL
	GPIOC->CR2 |= GPIO_PIN_7; // SDA2 10 MHz
	
	GPIOD->DDR |= GPIO_PIN_0; // FAN Output
	GPIOD->CR1 |= GPIO_PIN_0; // FAN PUSH PULL
	GPIOD->DDR |= GPIO_PIN_2; // F Output
	GPIOD->CR1 |= GPIO_PIN_2; // F PUSH PULL
	GPIOD->CR1 |= GPIO_PIN_3; // VOLTAGE OK PullUp
	GPIOD->CR2 |= GPIO_PIN_3; // VOLTAGE OK Interrupt
	GPIOD->DDR |= GPIO_PIN_4; // Buzzer Output
	GPIOD->CR1 |= GPIO_PIN_4; // Buzzer PUSH PULL
	GPIOD->CR1 |= GPIO_PIN_7; // L PullUp
	GPIOD->CR2 |= GPIO_PIN_7; // L Interrupt
	
	GPIOE->DDR |= GPIO_PIN_5; // ENABLE OUTPUT OD
	GPIOE->ODR |= GPIO_PIN_5; // LOAD OFF
	
	ADC1->CR2 |= ADC1_ALIGN_RIGHT;
	ADC1->CSR |= ADC1_CHANNEL_1;
	ADC1->CR1 |= ADC1_CR1_ADON;
	
	setupUART();
#ifdef STM8S003
	UART1->CR2 = UART1_CR2_TEN | UART1_CR2_REN;
#else
	UART2->CR2 = UART2_CR2_TEN | UART2_CR2_REN;
#endif
	
	/*
	BEEP->CSR |= BEEP_CALIBRATION_DEFAULT;
	BEEP->CSR |= BEEP_FREQUENCY_2KHZ;
	BEEP->CSR |= BEEP_CSR_BEEPEN;
	delay(10000000);
	BEEP->CSR &= ~BEEP_CSR_BEEPEN;
	*/
	
	EXTI->CR2    = EXTI_SENSITIVITY_FALL_ONLY; // TLI
	EXTI->CR1   |= EXTI_SENSITIVITY_RISE_FALL << 2; // GPIOB
	EXTI->CR1   |= EXTI_SENSITIVITY_RISE_FALL << 4; // GPIOC
	EXTI->CR1   |= EXTI_SENSITIVITY_RISE_FALL << 6; // GPIOD
	
#ifndef STM8S003
	// FAN
	//TIM3->ARRH   = 0x10;
	//TIM3->ARRL   = 0x00;
	TIM3->CCMR2  = TIM3_OCMODE_PWM1 | TIM3_CCMR_OCxPE;
	TIM3->CCER1  = TIM3_CCER1_CC2E;
	TIM3->PSCR   = TIM3_PRESCALER_1; // Prescaler of 1 gives 16 MHz / 2^16 = 244 Hz
	TIM3->CR1   |= TIM3_CR1_CEN | TIM3_CR1_ARPE;
	TIM3->CCR2H  = 0;
	TIM3->CCR2L  = 0;
#endif
	
	// I-SET
	// 28000 gives a frequency of about 571 Hz on 16 MHz
	TIM1->ARRH   = 0x6D;
	TIM1->ARRL   = 0x60;
	TIM1->PSCRH  = 0;
	TIM1->PSCRL  = 0;
	
	TIM1->CCMR1  = TIM1_OCMODE_PWM1 | TIM1_CCMR_OCxPE;
	TIM1->CCER1  = TIM1_CCER1_CC1E;
	TIM1->CCR1H  = 0;
	TIM1->CCR1L  = 0;
	TIM1->CR1   |= TIM1_CR1_CEN | TIM1_CR1_ARPE;
	TIM1->BKR   |= TIM1_BKR_MOE;
	
	// 20000 gives an interrupt frequency of 100 Hz
	TIM2->ARRH   = 0x4E;
	TIM2->ARRL   = 0x20;
	TIM2->PSCR   = TIM2_PRESCALER_8;
	TIM2->IER    = TIM2_IER_UIE;
	TIM2->CR1   |= TIM2_CR1_CEN;
	
	// Unlock flash
	FLASH->DUKR = FLASH_RASS_KEY2;
	FLASH->DUKR = FLASH_RASS_KEY1;
	//while (!(FLASH->IAPSR & (1 << FLASH_IAPSR_DUL)));
}

uint8_t read8(uint16_t address) {
	return _MEM_(address + 0x4000);
}

uint16_t read16(uint16_t address) {
	return (read8(address) << 8) | read8(address + 1);
}

void write8(uint16_t address, uint8_t data) {
	_MEM_(address + 0x4000) = data;
}

void write16(uint16_t address, uint16_t data) {
	write8(address, data >> 8);
	write8(address + 1, data & 0xFF);
}

void showText(char text[], uint8_t display) {
	disp_write(digits[0], chars[text[0] - CHAR_OFFSET], display);
	disp_write(digits[1], chars[text[1] - CHAR_OFFSET], display);
	disp_write(digits[2], chars[text[2] - CHAR_OFFSET], display);
	if (display == DP_TOP) {
		disp_write(digits[3], chars[text[3] - CHAR_OFFSET], display);
	}
}

void showNumber(uint16_t num, uint8_t dot, uint8_t display) {
	while (num >= 10000) {
		num /= 10;
		dot--;
	}
	if (display == DP_TOP) {
		disp_write(digits[0], chars[num / 1000] | (0x80 & (0x10 << dot)), DP_TOP);
		disp_write(digits[1], chars[(num / 100) % 10] | (0x80 & (0x20 << dot)), DP_TOP);
		disp_write(digits[2], chars[(num / 10) % 10] | (0x80 & (0x40 << dot)), DP_TOP);
		disp_write(digits[3], chars[num % 10], DP_TOP);
	} else {
		if (num >= 1000) {
			num /= 10;
			dot--;
		}
		disp_write(digits[0], chars[(num / 100) % 10] | (0x80 & (0x20 << dot)), DP_BOT);
		disp_write(digits[1], chars[(num / 10) % 10] | (0x80 & (0x40 << dot)), DP_BOT);
		disp_write(digits[2], chars[num % 10], DP_BOT);
	}
}

uint16_t analogRead(ADC1_Channel_TypeDef ch) {
	uint8_t adcH, adcL;
	ADC1->CSR &= (uint8_t)(~ADC1_CSR_CH);
	ADC1->CSR |= (uint8_t)(ch);
	
	ADC1->CR1 |= ADC1_CR1_ADON;
	while (!(ADC1->CSR & ADC1_IT_EOC));
	adcL = ADC1->DRL;
	adcH = ADC1->DRH;
	ADC1->CSR &= ~ADC1_IT_EOC;
	return (adcL | (adcH << 8));
}

uint16_t analogRead12(ADC1_Channel_TypeDef ch) {
	uint16_t val = 0;
	uint8_t i;
	for (i = 0; i < 4 * OVERSAMPLING; i++) {
		val += analogRead(ch);
	}
	return val / OVERSAMPLING;
}

void getTemp(void) {
	uint16_t tmp = (10720 - analogRead(ADC1_CHANNEL_0) * 10) >> 3;
	temperature = tmp;
}

void getVoltage(void) {
	uint16_t v1, v_ref, v2, v_load;
	v1 = analogRead12(ADC1_CHANNEL_1);
	v_load = 1.02217839986557 * v1 - 81.5878664441528;
	v2 = analogRead12(ADC1_CHANNEL_2);
	v_ref = 0.891348658196074 * v2 - 80.4250357289787;
	if (v1 > 20) {
		voltage = v_load;
		if (v_ref >= v_load && v_ref < v_load + 100) {
			voltage = v_ref;
		}
	}
}

void calcPWM(void) {
	uint16_t pwm;
	uint32_t current;
	switch (set_mode) {
		case MODE_CC:
			current = set_values[MODE_CC];
			break;
		case MODE_CW: // I = P / U
			current = set_values[MODE_CW];
			current *= 100;
			current /= voltage;
			break;
		case MODE_CR: // I = U / R
			current = voltage;
			current *= 1060; // Resistance mode needs a slightly higher value
			current /= set_values[MODE_CR];
			break;
	}
	if (current > 10000) {
		current = 10000;
	}
	set_current = current;
	current *= 2119;
	current /= 1000;
	pwm = current + 60;
	//pwm = I_PWM_FACTOR * current + I_PWM_OFFSET;
	TIM1->CCR1H = pwm >> 8;
	TIM1->CCR1L = (uint8_t) pwm;
	// CC
	// set_value[0];
	// CW
	// set_value[1] / voltage;
	// CR
	// voltage / set_value[2]
}

void setFan() {
	if (temperature > 850) { // Over temperature protection
#ifdef STM8S003
		GPIOD->ODR |= GPIO_PIN_0;
#else
		TIM3->CCR2H  = 0xFF;
		TIM3->CCR2L  = 0xFF;
#endif
		error = ERROR_OTP;
	}
	if (temperature > 400) {
#ifdef STM8S003
		GPIOD->ODR |= GPIO_PIN_0;
#else
		TIM3->CCR2H  = (temperature * 54) >> 8;
		TIM3->CCR2L  = (uint8_t)(temperature * 54);
#endif
	} else if (~GPIOE->ODR & GPIO_PIN_5) { // Set minimum pwm of 1/3 if switched on
#ifdef STM8S003
		GPIOD->ODR |= GPIO_PIN_0;
#else
		TIM3->CCR2H  = 0x55;
		TIM3->CCR2L  = 0x55;
#endif
	} else {
#ifdef STM8S003
		GPIOD->ODR &= ~GPIO_PIN_0;
#else
		TIM3->CCR2H  = 0;
		TIM3->CCR2L  = 0;
#endif
	}
}

void testFan() {
	uint16_t i;
	for (i = 0; i < 0xFFFF; i++) {
		TIM3->CCR2H  = i >> 8;
		TIM3->CCR2L  = i & 0xFF;
		delay(50);
	}
	delay(1500000);
	TIM3->CCR2H  = 0;
	TIM3->CCR2L  = 0;
}

void tempFan() {
	if (calc_fan) {
		getTemp();
		setFan();
		calc_fan = 0;
	}
}

uint8_t select(char *opts, uint8_t num_opts, uint8_t selected) {
	uint8_t old_opt = -1;
	while (1) {
		if(encoder_val < 0){if(!selected){selected=num_opts;}selected--;}
		else {if(encoder_val > 0){selected++;if(selected == num_opts)selected=0;}}
		if (selected != old_opt) {
			disp_write(digits[0], chars[*(opts + selected * 3 + 0) - CHAR_OFFSET], DP_BOT);
			disp_write(digits[1], chars[*(opts + selected * 3 + 1) - CHAR_OFFSET], DP_BOT);
			disp_write(digits[2], chars[*(opts + selected * 3 + 2) - CHAR_OFFSET], DP_BOT);
			old_opt = selected;
			encoder_val = 0;
		}
	}
}

void blinkDisplay(uint8_t disp) {
	bool dptop = disp == DP_TOP;
	if (brightness[dptop] != ((millis >> 5) & 1)) {
		brightness[dptop] = !brightness[dptop];
		setBrightness(1 + brightness[dptop], disp);
	}
	tempFan();
}

uint8_t change_u8(uint8_t var, uint8_t max) {
	if (encoder_val < 0) {
		option_changed = 1;
		if (var) {
			return var - 1;
		} else {
			return max;
		}
	} else if (encoder_val > 0) {
		option_changed = 1;
		if (var < max) {
			return var + 1;
		} else {
			return 0;
		}
	}
	return var;
}

uint16_t change_u16(uint16_t var, uint16_t max, uint16_t inc) {
	if (encoder_val < 0) {
		option_changed = 1;
		if (var > inc) {
			return var - inc;
		} else {
			return 0;
		}
	} else if (encoder_val > 0) {
		option_changed = 1;
		if (var + inc < max) {
			return var + inc;
		} else {
			return max;
		}
	}
	return var;
}

void selectMode(void) {
	char subopts[][4] = {"CC@","CW@","CR@","CV@"};
	while (1) {
		blinkDisplay(DP_BOT);
		set_mode = change_u8(set_mode, 2);
		if (option_changed) {
			option_changed = 0;
			encoder_val = 0;
			showText(subopts[set_mode], DP_BOT);
		}
		if (encoder_pressed) {
			write8(MEM_MODE, set_mode);
			return;
		}
	}
}

bool selectBool(bool val) {
	char subopts[][4] = {"OFF","ON@"};
	while (1) {
		blinkDisplay(DP_BOT);
		if (encoder_val) {
			val = !val;
			encoder_val = 0;
			showText(subopts[val], DP_BOT);
		}
		if (encoder_pressed) {
			return val;
		}
	}
}

uint16_t selectUInt16(uint16_t val, uint16_t max) {
	bool hl_opt = 0;
	uint16_t inc = 10;
	option_changed = 1;
	while (1) {
		blinkDisplay(DP_BOT);
		if (encoder_pressed) {
			encoder_pressed = 0;
			if (!hl_opt) {
				hl_opt = 1;
				option_changed = 1;
				disp_write(digits[3], LED_LOW, DP_BOT);
			} else {
				return val;
			}
		}
		if (option_changed) {
			option_changed = 0;
			encoder_val = 0;
			showNumber(val, 3, DP_BOT);
			
			if (set_values[set_mode] >= 10000) {
				inc = 100;
			} else if (set_values[set_mode] >= 1000) {
				inc = 10;
			} else {
				inc = 1;
			}
			if (!hl_opt) {
				inc *= 10;
			}
		}
		val = change_u16(val, max, inc);

}

void selectValue(void) {
	bool change = 0;
	bool hl_opt = 0;
	uint16_t inc = 10;
	option_changed = 1;
	showText(opts[set_mode], DP_TOP);
	disp_write(digits[3], LED_HIGH, DP_BOT);
	while (1) {
		blinkDisplay(DP_BOT);
		if (encoder_pressed) {
			encoder_pressed = 0;
			if (!hl_opt) {
				hl_opt = 1;
				option_changed = 1;
				disp_write(digits[3], LED_LOW, DP_BOT);
			} else {
				write16((set_mode + 1) << 4, set_values[set_mode]);
				return;
			}
		}
		if (option_changed) {
			option_changed = 0;
			encoder_val = 0;
			showNumber(set_values[set_mode], 3, DP_BOT);
			
			/*if (set_values[set_mode] >= 100000) {
				inc = 1000;
			} else */if (set_values[set_mode] >= 10000) {
				inc = 100;
			} else if (set_values[set_mode] >= 1000) {
				inc = 10;
			} else {
				inc = 1;
			}
			if (!hl_opt) {
				inc *= 10;
			}
		}
		set_values[set_mode] = change_u16(set_values[set_mode], 10000, inc);
	}
}

void selectCutoff(void) {
	bool change = 0;
	bool hl_opt = 0;
	uint16_t inc = 10;
	option_changed = 1;
	disp_write(digits[3], LED_HIGH, DP_BOT);
	while (1) {
		blinkDisplay(DP_BOT);
		if (encoder_pressed) {
			encoder_pressed = 0;
			if (!hl_opt) {
				hl_opt = 1;
				option_changed = 1;
				disp_write(digits[3], LED_LOW, DP_BOT);
			} else {
				write16(MEM_CUTV, cutoff_voltage);
				return;
			}
		}
		if (option_changed) {
			option_changed = 0;
			encoder_val = 0;
			showNumber(cutoff_voltage / 10, 1, DP_BOT);
		}
		cutoff_voltage = change_u16(cutoff_voltage, 2900, hl_opt ? 10 : 100);
	}
}

void showMenu(void) {
	uint8_t old_opt = 255, opt = 0;
	char opts[][5] = {"MODE", "VAL@", "SHDN", "CUTO", "BEEP"};
	char subopts[][4] = {"CC@","CW@","CR@","CV@","OFF","ON@"};
	//char *opts = "MODEVAL@SHDNCUTOBEEP";
	//select(opts, 5, 0);
	while (1) {
		if (encoder_val < 0) {
			if (opt) {
				opt--;
			} else {
				opt = 4;
			}
		} else {
			if (encoder_val > 0) {
				if (opt < 4) {
					opt++;
				} else {
					opt = 0;
				}
			}
		}
		blinkDisplay(DP_TOP);
		if (opt != old_opt) {
			showText(opts[opt], DP_TOP);
			switch (opt) {
				case 0:
					showText(subopts[(uint8_t)set_mode], DP_BOT);
					break;
				case 1:
					showNumber(set_values[(uint8_t)set_mode], 3, DP_BOT);
					break;
				case 2:
					showText(subopts[4 + cutoff_active], DP_BOT);
					break;
				case 3:
					showNumber(cutoff_voltage, 2, DP_BOT);
					break;
				case 4:
					showText(subopts[4 + beeper_on], DP_BOT);
					break;
			}
			old_opt = opt;
			encoder_val = 0;
		}
		if (encoder_pressed) {
			encoder_pressed = 0;
			setBrightness(2, DP_TOP);
			switch (opt) {
				case 0:
					selectMode();
					break;
				case 1:
					showText(mode_name[set_mode], DP_TOP);
					set_values[set_mode] = selectUInt16(uint16_t set_values[set_mode], uint16_t max_values[set_mode]);
					write16((set_mode + 1) << 4, set_values[set_mode]);
					break;
				case 2:
					cutoff_active = selectBool(cutoff_active);
					write8(MEM_CUTO, cutoff_active);
					break;
				case 3:
					selectCutoff();
					break;
				case 4:
					beeper_on = selectBool(beeper_on);
					write8(MEM_BEEP, beeper_on);
					break;
			}
			setBrightness(2, DP_BOT);
			disp_write(digits[3], 0, DP_BOT);
			encoder_pressed = 0;
			run_pressed = 0;
			old_opt = 255;
		}
		if (run_pressed) {
			run_pressed = 0;
			return;
		}
	}
}

void main(void) {
	setup();
	set_mode = read8(MEM_MODE);
	set_values[MODE_CC] = read16(MEM_CC);
	set_values[MODE_CW] = read16(MEM_CW);
	set_values[MODE_CR] = read16(MEM_CR);
	set_values[MODE_CV] = read16(MEM_CV);
	beeper_on = read8(MEM_BEEP);
	cutoff_active = read8(MEM_CUTO);
	cutoff_voltage = read16(MEM_CUTV);
	
	//GPIOE->ODR  &= ~GPIO_PIN_5;
	/*
	while (1) {
		showNumber(analogRead(ADC1_CHANNEL_0), 0, DP_TOP);
		delay(300000);
	}*/
	delay(300000);
	setBrightness(2, DP_BOT);
	setBrightness(2, DP_TOP);
#ifndef STM8S003
	showText("TEST", DP_TOP);
	//testFan();
#endif
	printf("LOAD READY\n");
	__asm__ ("rim");
	while (1) {
		uint32_t start_time;
		showMenu();
		GPIOE->ODR &= ~GPIO_PIN_5;
		running = 1;
		setFan();
		start_time = millis;
		while (!run_pressed && error == ERROR_NONE) {
			getVoltage();
			if (voltage > 3000) {
				error = ERROR_OVP;
				break;
			}
			if (cutoff_active && voltage < cutoff_voltage) {
				error = ERROR_UVP;
				break;
			}
			calcPWM();
			if (redraw) {
				uint8_t s_var = (millis / 500) % 5;
				uint16_t timer;
				switch (s_var) {
					case 0:
						showNumber(voltage, 2, DP_TOP);
						break;
					case 1:
						showNumber(ampere_seconds / 3600, 3, DP_TOP);
						break;
					case 2:
						showNumber(watt_seconds / HOUR, 3, DP_TOP);
						break;
					case 3:
						showNumber(temperature, 1, DP_TOP);
						break;
					case 4:
						timer = (millis - start_time) / MINUTE * 100 + ((millis - start_time) / 100) % 60;
						showNumber(timer, 2, DP_TOP);
				}
				printf("%lu; %u; %u; %lu; %lu; %u\n", (millis - start_time), set_current, voltage, ampere_seconds, watt_seconds, temperature);
				//showNumber(analogRead(ADC1_CHANNEL_0), 4, DP_TOP);
				showNumber(set_current, 3, DP_BOT);
				disp_write(digits[3], LED_RUN | (1 << s_var), DP_BOT);
				redraw = 0;
			}
			tempFan();
		}
		running = 0;
		GPIOE->ODR |= GPIO_PIN_5;
		encoder_pressed = 0;
		if (error != ERROR_NONE) {
			showText("ERR", DP_BOT);
			showText(error_msg[error - 1], DP_TOP);
			while (!encoder_pressed) {
				tempFan();
				//showNumber(temperature, 1, DP_TOP);
				disp_write(digits[3], LED_RUN * ((millis / 50) & 1), DP_BOT);
			}
			error = ERROR_NONE;
			encoder_pressed = 0;
		}
		run_pressed = 0;
		disp_write(digits[3], 0, DP_BOT);
	}
	while (1) {
		showNumber((uint16_t)millis, 2, DP_TOP);
	}
	while (1) {
		uint16_t pwm = 0;
		uint16_t ma = 1000;
		bool chng = 1;
		while (1) {
			if (encoder_val < 0) {
				if (set_values[set_mode])
					set_values[set_mode] -= 100;
				chng = 1;
			} else if (encoder_val > 0) {
				set_values[set_mode] += 100;
				//pwm += 100;
				chng = 1;
			}
			if (run_pressed) {
				if (GPIOE->ODR & GPIO_PIN_5) {
					GPIOE->ODR &= ~GPIO_PIN_5;
				} else {
					GPIOE->ODR |= GPIO_PIN_5;
				}
				run_pressed = 0;
			}
			//showNumber((10720 - analogRead(ADC1_CHANNEL_0) * 10) >> 3, 1, DP_TOP);
			showNumber(set_values[set_mode], 3, DP_TOP);
			//delay(300000);
			disp_write(digits[0], chars[GPIOC->IDR & GPIO_PIN_2], DP_BOT);
			if (chng) {
				/*
				//showNumber(pwm, 1, DP_TOP);
				uint32_t set_pwm = 2119;
				set_pwm *= ma;
				set_pwm += 60043;
				//pwm = I_PWM_FACTOR * ma + I_PWM_OFFSET;
				pwm = set_pwm / 1000;
				TIM1->CCR1H = pwm >> 8;
				TIM1->CCR1L = (uint8_t) pwm;
				//showNumber((TIM1->CCR1H << 8) | TIM1->CCR1L, 0, DP_TOP);
				showNumber(ma, 3, DP_TOP);
				showNumber(pwm, 0, DP_BOT);
				/*
				if (ma < 1000)
					showNumber(ma, 0, DP_BOT);
				else if (ma < 10000)
					showNumber(ma / 10, 2, DP_BOT);
				else if (ma < 100000)
					showNumber(ma / 100, 1, DP_BOT);
				*/
				chng = 0;
				encoder_val = 0;
				printf("%d\n", pwm);
			}
			getVoltage();
			calcPWM();
			getTemp();
			//setFan();
		}
	}
	while (1) {
		bool chng = 0;
		if (encoder_val < 0) {
			if (!TIM1->CCR1L) TIM1->CCR1H--;
			TIM1->CCR1L--;
			chng = 1;
		} else if (encoder_val > 0) {
			if (TIM1->CCR1L == 0xFF) TIM1->CCR1H++;
			TIM1->CCR1L++;
			chng = 1;
		}
		if (chng) {
			showNumber((TIM1->CCR1H << 8) | TIM1->CCR1L, 0, DP_TOP);
			chng = 0;
			encoder_val = 0;
		}
		
	}
	delay(1500000);
	
	//GPIOC->ODR &= ~GPIO_PIN_1;
	GPIOE->ODR |= GPIO_PIN_5; // LOAD OFF
	
	disp_write(digits[0], chars[EXTI->CR1], DP_TOP);
	disp_write(digits[1], chars[EXTI->CR2], DP_TOP);
	disp_write(digits[2], chars[12], DP_TOP);
	disp_write(digits[3], chars[13], DP_TOP);
	disp_write(digits[0], 0, DP_BOT);
	disp_write(digits[1], 0, DP_BOT);
	disp_write(digits[2], 0, DP_BOT);
	disp_write(digits[3], 0, DP_BOT);
	
	//printf("test\nDies ist ein Test!");
	//EXTI->CR1 |= EXTI_SENSITIVITY_FALL_ONLY << 4; //FALL_ONLY
	
	GPIOD->ODR &= ~GPIO_PIN_0;
	__asm__ ("rim"); // interrupts enabled
	//while (1) {
	//	showMenu();
	//}
	/*
	while (1) {
		uint16_t v1 = analogRead12(ADC1_CHANNEL_2);
		showNumber(v1 / 10, 0, DP_TOP);
		showNumber(v1 % 10, 0, DP_BOT);
		delay(300000);
	}
	*/
	while (1) {
		uint16_t v1, v_ref, v2, v_load, v;
		v1 = analogRead12(ADC1_CHANNEL_1);
		v_load = 1.02217839986557 * v1 - 81.5878664441528;
		//v_load = 4.05175 * v1 / 4 - 64.7543;
		//GPIOD->ODR |= GPIO_PIN_0;
		v2 = analogRead12(ADC1_CHANNEL_2);
		v_ref = 0.891348658196074 * v2 - 80.4250357289787;
		//v_ref = 3.52991 * v2 / 4 - 64.78; // ADC2
		//GPIOD->ODR &= ~GPIO_PIN_0;
		v = 0;
		if (v1 > 20) {
			v = v_load;
			//if (v_ref >= v_load && v_ref < v_load + 100) {
				v = v_ref;
				disp_write(digits[2], chars[1], DP_BOT);
			//} else {
			//	disp_write(digits[2], chars[3], DP_BOT);
			//}
		}
		disp_write(digits[0], chars[v / 1000], DP_TOP);
		disp_write(digits[1], chars[(v / 100) % 10] | 0x80, DP_TOP);
		disp_write(digits[2], chars[(v / 10) % 10], DP_TOP);
		disp_write(digits[3], chars[v % 10], DP_TOP);
		delay(300000);
	}
	
}

void putchar(char c) {
#ifdef STM8S003
	UART1->DR = c;
	while (!(UART1->SR & (uint8_t)UART1_FLAG_TXE));
#else
	UART2->DR = c;
	while (!(UART2->SR & (uint8_t)UART2_FLAG_TXE));
#endif
}
uint8_t _encoder_dir = 0xFF;
void GPIOB_Handler() __interrupt(4) {
	uint8_t cur = (GPIOB->IDR >> 4) & 3;
	if (cur == 0) {
		if (_encoder_dir == 2) {
			encoder_val++;
		} else if (_encoder_dir == 1) {
			encoder_val--;
		}
	}
	_encoder_dir = cur;
	dot = !dot;
}
uint8_t input_values = 0xFF;
void GPIOC_Handler() __interrupt(5) {
	input_values &= ~GPIOC->IDR; // store changes (H->L) for buttons
	encoder_pressed |= (input_values >> 3) & 1; // Set flag, that the button was pressed
	run_pressed     |= (input_values >> 4) & 1; // Set flag, that the button was pressed
	input_values = GPIOC->IDR;
}
//Voltage OK interrupt
void GPIOD_Handler() __interrupt(6) {
}
// TIM2 UO (CC = 14)
void TIM2_UPD_OVF_Handler() __interrupt(13) {
	millis++;
	TIM2->SR1 &= ~TIM2_SR1_UIF;
	if (millis % 50 == 0) {
		redraw = 1;
	}
	if (millis % 100 == 0 && running) {
		// watts can be 60000 max.
		uint32_t watt = set_current;
		watt *= voltage;
		watt_seconds += watt;
		ampere_seconds += set_current;
	}
	if (millis % 5000 == 0) {
		calc_fan = 1;
	}
}