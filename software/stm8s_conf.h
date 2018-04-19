#ifndef __STM8S_CONF_H
#define __STM8S_CONF_H

#include "stm8s.h"

#include "delay.h"
#include "stm8s_adc1.h"
//#include "stm8s_adc2.h"
//#include "stm8s_awu.h"
#include "stm8s_beep.h"
//#include "stm8s_can.h"
#include "stm8s_clk.h"
#include "stm8s_exti.h"
#include "stm8s_flash.h"
#include "stm8s_gpio.h"
//#include "stm8s_i2c.h"
//#include "stm8s_itc.h"
//#include "stm8s_iwdg.h"
//#include "stm8s_rst.h"
//#include "stm8s_spi.h"
#include "stm8s_tim1.h"
#include "stm8s_tim2.h"
#include "stm8s_tim3.h"
//#include "stm8s_tim4.h"
//#include "stm8s_tim5.h"
//#include "stm8s_tim6.h"
#ifdef STM8S003
#include "stm8s_uart1.h"
#else
#include "stm8s_uart2.h"
#endif
//#include "stm8s_uart3.h"
//#include "stm8s_uart4.h"
//#include "stm8s_wwdg.h"

#endif /* __STM8S_CONF_H */
