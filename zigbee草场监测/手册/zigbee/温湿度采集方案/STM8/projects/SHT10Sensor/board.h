/**
  ******************************************************************************
  * @file    board.h
  * @author  Microcontroller Division
  * @version V1.2.0
  * @date    09/2010
  * @brief   Input/Output defines
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __BOARD_H
#define __BOARD_H


/* MACROs for SET, RESET or TOGGLE Output port */
/*
#define GPIO_PIN_0        0x01
#define GPIO_PIN_1        0x02
#define GPIO_PIN_2        0x04
#define GPIO_PIN_3        0x08
#define GPIO_PIN_4        0x10
#define GPIO_PIN_5        0x20
#define GPIO_PIN_6        0x40
#define GPIO_PIN_7        0x80
#define GPIO_PIN_LNIB     0x0F
#define GPIO_PIN_HNIB     0xF0
#define GPIO_PIN_ALL      0xFF
*/
  
#define ADDR_LOW_PORT		GPIOB
#define ADDR_LOW_PIN0           GPIO_PIN_0
#define ADDR_LOW_PIN1           GPIO_PIN_1
#define ADDR_LOW_PIN2           GPIO_PIN_2
#define ADDR_LOW_PIN3           GPIO_PIN_3
#define ADDR_LOW_PIN4           GPIO_PIN_4
#define ADDR_LOW_PIN5           GPIO_PIN_5
#define ADDR_LOW_PIN6           GPIO_PIN_6
#define ADDR_LOW_PIN7           GPIO_PIN_7

#define SHT10_SDA_PORT		GPIOC
#define SHT10_SDA_PIN           GPIO_Pin_0

#define SHT10_SCL_PORT		GPIOC
#define SHT10_SCL_PIN           GPIO_Pin_1

#define RST_PORT		GPIOD
#define RST_PIN                 GPIO_Pin_0

#define MODE0_PORT              GPIOA
#define MODE0_PIN               GPIO_Pin_2

#define MODE1_PORT              GPIOA
#define MODE1_PIN               GPIO_Pin_3

#define SENSOR_DATA_PORT        GPIOC
#define SENSOR_DATA_PIN         GPIO_Pin_4


#define DELAY_1US()   nop();nop();
#define DELAY_5US()   DELAY_1US();DELAY_1US();DELAY_1US();DELAY_1US();DELAY_1US();
#define DELAY_10US()  DELAY_5US();DELAY_5US();
#define DELAY_50US()  DELAY_10US();DELAY_10US();DELAY_10US();DELAY_10US();DELAY_10US();
#define DELAY_100US() DELAY_50US();DELAY_50US();
#define DELAY_500US() DELAY_100US();DELAY_100US();DELAY_100US();DELAY_100US();DELAY_100US();

#define SHT10_DELAY_Q() DELAY_1US();    //SHT10四分之一周期时钟的延时，只要指定这个延时就可以一起指定了下面两个
#define SHT10_DELAY_H() SHT10_DELAY_Q();SHT10_DELAY_Q();  //办周期延时
#define SHT10_DELAY_P() SHT10_DELAY_H();SHT10_DELAY_H();  //周期延时


//SHT10的命令
#define SHT10_MEASURE_TEMP    0x03
#define SHT10_MEASURE_HUMI    0x05
#define SHT10_READ_REG        0x07
#define SHT10_WRITE_REG       0x06
#define SHT10_SOFT_RESET      0x1E

//SHT10寄存器配置位
#define SHT10_REG_HEAT_ON     0x04
#define SHT10_REG_NO_OTP      0x02
#define SHT10_REG_ACC_LOW     0x01


#endif

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
