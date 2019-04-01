/**
  ******************************************************************************
  * @file project\main.c
  * @brief  ����׿����ӿƼ����޹�˾
  *         ZAuZx_Txϵ�д���͸���������
  *         SHT10/DS18B20�͹��Ĳɼ����䷶��
  *         ʹ����ο���Ӧ�ֲ�
  *         ��Ʒ��������� http://zatech.taobao.com
  * @author ZATech
  * @version V1.0.0
  * @date    09/15/2014
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "stm8l10x.h"
#include "board.h"

#define USE_FULL_ASSERT

//#define WATCHDOG

#define WORK_TO_WAKE_RATIO  1  //�����뻽�Ѵ���������ÿWORK_TO_WAKE_RATIO�λ��ѣ�һ��Ϊ30s������һ�βɼ����ϴ�������ֵ1��32767
#define RESET_ZIGBEE_TO_WAKE_RATIO  2880  //����zigbeeģ���뻽�Ѵ���������ÿRESET_ZIGBEE_TO_WAKE_RATIO�λ��ѣ�һ��Ϊ30s������һ��zigbeeģ�飬zigbeeû�����Ź�������zigbeeģ��ͻ���쳣��������

#define USE_SEC_ADDR    //����ʹ�õ�ַ���õڶ�����-�������õ�ַ�������IO�ڵ�ַ���÷�ʽ����ע�͵��˾�
//#define SETTING_PANID     4372  //PANID����Χ1��65535��ע�͵��˾�������
//#define SETTING_TX_POWER  19    //TX_POWER����Χ0~19��13.3.6����ܵ�21����ע�͵��˾�������
//#define SETTING_CHANNEL   8192  //CHANNEL����Χ2048��134215680��ע�͵��˾�������
//#define SETTING_POLL_RATE 3000  //POLL_RATE����Χ0��65535��ע�͵��˾�������

#define UI_STRING   //����Ϊ�ַ����������

/* Private defines -----------------------------------------------------------*/
volatile u8 i;
u16 Temprature;
u8  TH,TL,Config;
u8  AddrHi,AddrLo;
u16 Addr,SettingTemp;
u8  UARTSendDataBuf[32];
s32 WakeCount=-1;   //����ͳ�ƻ��Ѵ�����ʵ��ÿWORK_TO_WAKE_RATIO�λ���һ�βɼ��ϴ�
s32 ResetZigbeeCount=1;   //����=0ʱ�ĵ�һ������

u8  DS18B20_Exist=FALSE;    //DS18B20�Ƿ����
u8  SHT10_Exist=FALSE;      //SHT10�Ƿ����

u16 SHT10_TempData;   //�������������¶�ԭʼ����
u16 SHT10_HumiData;   //������������ʪ��ԭʼ����
float SHT10_Temp;   //���������¶�����
float SHT10_Humi;   //��������ʪ������
s16 SHT10_TempWord;   //�¶ȸ�����ת��Ϊ2�ֽ����ͣ����ֽ����������ֽ�С��
s16 SHT10_HumiWord;   //ʪ�ȸ�����ת��Ϊ2�ֽ����ͣ����ֽ����������ֽ�С��

/* Private function prototypes -----------------------------------------------*/
void Delay(uint16_t nCount);

u8 DS18B20_Init(void);
u8 DS18B20_Read(void);
void DS18B20_Write(u8 Data);
void DS18B20_Data_Send(void);

void UART_Send_Data(u8 DataBuf[], u8 DataLength);


void SHT10_WriteStart(void);
u8 STH10_Write(u8 Data);
u16 STH10_ReadData(void);
u8 STH10_WriteReg(u8 Reg);
u8 STH10_ReadReg(void);
void SHT10_Wait(void);
float SHT10_CalTemp(u16 Data);
float SHT10_CalHumi(u16 Data, float Temp);
s16 SHT10_ConvertTemp(float Temp);
s16 SHT10_ConvertHumi(float Humi);
void SHT10_Data_Send(void);



/* Private functions ---------------------------------------------------------*/

void main(void)
{
  /*----------IO������----------*/
  GPIO_Init(ADDR_LOW_PORT, GPIO_Pin_All, GPIO_Mode_Out_PP_Low_Slow);        //8λ��ַ
  
  GPIO_Init(RST_PORT, RST_PIN, GPIO_Mode_Out_PP_High_Slow);    //����zigbeeģ��RST������Ϊ����ߵ�ƽ
  
  //SDA��SCL�ڵ�PC0��PC1ΪTrue open drain��������ڲ����������Ĵ���λ��������
  //�����SDA��GPIO_Mode_Out_OD_Low_Fast�൱��0xA0
  //������ⲿ��������
  //��תΪ����ʱֻ��ҪSHT10_SDA_PORT->DDR &= (uint8_t)(~(SHT10_SDA_PIN));��ʱ����0x20���븡�����ж�
  //SCL��GPIO_Mode_Out_OD_Low_Slow�൱��0x80
  //��תΪ����ʱֻ��ҪSHT10_SCL_PORT->DDR &= (uint8_t)(~(SHT10_SCL_PIN));��ʱ����0x00���븡�����ж�
  //��˵SCL����Զ�����������^^
  GPIO_Init(SHT10_SDA_PORT, SHT10_SDA_PIN, GPIO_Mode_Out_OD_Low_Fast); 
  EXTI_SetPinSensitivity(EXTI_Pin_0, EXTI_Trigger_Falling);   //�½��ش�����SHT10���͵�ʱ����
  GPIO_Init(SHT10_SCL_PORT, SHT10_SCL_PIN, GPIO_Mode_Out_OD_Low_Slow); 
  SHT10_SDA_PORT->ODR |= (uint8_t)SHT10_SDA_PIN;    //����ߵ�ƽ������ⲿ�������費ƥ�䵼��©��
  SHT10_SCL_PORT->ODR |= (uint8_t)SHT10_SCL_PIN;
  
  //P1.3/mode0 - ģ������˯�ߣ�stm8���˯��
  GPIO_Init( MODE0_PORT, MODE0_PIN, GPIO_Mode_Out_PP_Low_Slow);
  
  //P1.5/mode1 - ģ�����˯�ߣ�stm8����˯��
  //Zigbee͸��ģ�������stm8�Ļ����ź�Ϊ�ߵ�ƽ������10ms��ʼ���ʹ����ź�
  //����stm8�������ڲ�����ѡ����ֻ������Ϊ��������
  //������Zigbeeģ�鵥������stm8�������ʱ���������뽫���ܵ��³��������жϣ���������ؽ���Zigbeeģ����ڲ�����ģ��ʱ��Ϊ��������
  GPIO_Init( MODE1_PORT, MODE1_PIN, GPIO_Mode_In_FL_IT);
  EXTI_SetPinSensitivity(EXTI_Pin_3, EXTI_Trigger_Rising);
  
  GPIO_Init(SENSOR_DATA_PORT, SENSOR_DATA_PIN, GPIO_Mode_Out_PP_High_Fast);   //���������ݿ�����
  //��ЧΪ��������
  //SENSOR_DATA_PORT->ODR |= SENSOR_DATA_PIN;   //����ߵ�ƽ
  //SENSOR_DATA_PORT->CR1 |= SENSOR_DATA_PIN;   //�������
  //SENSOR_DATA_PORT->CR2 &= (uint8_t)(~(SENSOR_DATA_PIN));   //10MHz�������
  //SENSOR_DATA_PORT->DDR |= SENSOR_DATA_PIN;   //����Ϊ���
  
  //GPIO_Init(SENSOR_DATA_PORT, SENSOR_DATA_PIN, GPIO_Mode_In_PU_No_IT);   //���������ݿ������������ж�
  //��ЧΪ��������
  //SENSOR_DATA_PORT->ODR |= SENSOR_DATA_PIN;   //����ߵ�ƽ������������ʱ�޹ؽ�Ҫ
  //SENSOR_DATA_PORT->CR1 |= SENSOR_DATA_PIN;   //��������
  //SENSOR_DATA_PORT->CR2 &= (uint8_t)(~(SENSOR_DATA_PIN));   //���ж�
  //SENSOR_DATA_PORT->DDR &= (uint8_t)(~(SENSOR_DATA_PIN));   //����Ϊ����
  //�ɴ˿ɼ���������18B20�ĵ�������������л�ʱ��ֻ��Ҫ���һ������DDR�����Ը����л��������Ϳ��Ծ�ȷ����ʱ��
  
  
  /*----------ϵͳ�����������ڲ�ģ��ʹ��----------*/
  CLK_DeInit();
  CLK_PeripheralClockConfig(CLK_Peripheral_AWU, ENABLE);      //ʹ�ܻ���
  CLK_MasterPrescalerConfig(CLK_MasterPrescaler_HSIDiv8);     //ʱ��8��Ƶ��2MHz
  
  
  /*----------���ѳ�ʼ��----------*/
  AWU_DeInit();
  
  
  /*----------���ڳ�ʼ��----------*/
  CLK_PeripheralClockConfig(CLK_Peripheral_USART, ENABLE);   //ʹ�ܴ���
  GPIO_ExternalPullUpConfig(GPIOC,GPIO_Pin_2|GPIO_Pin_3, ENABLE);   //���ߵ�ƽ
  USART_DeInit();
  USART_Init(115200,                            //������115200
            USART_WordLength_8D,                //8λ����λ
            USART_StopBits_1,                   //1λֹͣλ
            USART_Parity_No,                    //��У��
            USART_Mode_Rx | USART_Mode_Tx);     //���պͷ���ʹ��
  //USART_ITConfig(USART_IT_TXE, ENABLE);         //ʹ�ܷ����ж�
  USART_Cmd(ENABLE);    //���ڿ�ʼ����
  
  
  
  
  /*----------���Ź���ʼ��----------*/
  //ע�⣡�����Ź����ι��ʱ�޽�1~2�룬���������е�Ƭ������ʱ�������Ϊ30��
  //������Ҫ��Option Byte�е�OPT4��Ĭ�ϵ�0x00��Ϊ0x01����ʹ����ʱ���Ź���ͣ
  //Option Byte�޷��ڳ������޸ģ�ֻ��ͨ����д�����STVP����дʱ��SWIMЭ���ⲿд��
  //����ڵ���ʱ���Ź������޷�ʵ��
  //��SWIM�ⲿ��дʱ�����к궨��WATCHDOG��ʹ���Ź���Ч
#ifdef  WATCHDOG
  IWDG_Enable();
  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_SetPrescaler(IWDG_Prescaler_256);    //���Ź�ʱ����Ϊ���1724.63ms
  IWDG_SetReload(0xFF);
  IWDG_ReloadCounter();
#endif
  
  
  /*----------���½���Zigbee����͸��ģ���ַ������----------*/
  //������STM8L 96bitΨһ���к��е�8bit��Ϊ��ַ��λ����ַ��3bit�ſ�Ϊ0x000��ʵ��Ӧ���п��Բ��ò��뿪�صȷ�ʽ�����趨
  //ע�⣡�������IO�����õ�ַ��IO�ڵ�ƽ�ܴ�̶Ƚ�Ӱ���豸���ģ�Ϊ�������¿���
  //Zigbee����͸��ģ�������Ϊ�ڲ���������STM8L���ڽ�IO�����ڸߵ�ƽ���н��������ĵ������
  //���Zigbeeģ���ڿ�����1��ʱ����IO�����õ�ַ������ʱ��Ӧ��IO�����ڵ͵�ƽ�Խ��͹��ģ���ģ���ֲᣩ
  //����STM8L����Ӧ�ڿ���ʱ���õ�ַIO�ڣ�����0.5���IO�ڻָ��͵�ƽ
  //����ò������õ�ַ�ķ�ʽ��������Ӧ�������õ�ַ��Ȼ���ϵ磬����0.5��󽫿��ز�����0
  
  //������ô������õ�ַģʽ���򽫵�ַ����IO��ȫ��������д�봮�����õ�ֵַ������ģ�鼴������ɵ�ַ����
  
  AddrHi=0;   //��ַ��λ��0  ע�⣡��������ſ���ģ���ַ��λ���ÿڣ��ʵ�ַ��λΪ0�������Ϊ�Լ��ĳ�����ǵ����øõ�ַ��������ĳ��������ö�Ӧ�ĵ�ַIO
  FLASH_SetProgrammingTime(FLASH_ProgramTime_Standard);
  FLASH_Unlock(FLASH_MemType_Program);
  //AddrLo=FLASH_ReadByte(0x4926);    //����X-coordinator
  AddrLo=FLASH_ReadByte(0x4928);    //����Y-coordinator
  //AddrLo=(FLASH_ReadByte(0x4926)<<4) | (FLASH_ReadByte(0x4928)&0x0F);   //X-coordinator��Y-coordinator��ռ4bit
  FLASH_Lock(FLASH_MemType_Program);
  Addr = ((u16)AddrHi<<8) | AddrLo; //���ߵ�λ��ַ��ϳ�������ַ
  
  //�ȴ�ģ���ʼ�����������Ӧ������������
  //ע�⣡������
  for(u8 i=0; i<6; i++)
  {
    Delay(0xFFFF);    //ÿ��DelayԼ200ms������ʱ1�����ϣ�����Ҫ�������ȴ�ʱ�䣬��Ϊģ���ϵ�һ��ʱ��󴮿ڲſ�ʼ������������
  }
  
#ifndef USE_SEC_ADDR    //δ����ʹ�ô������÷����������IO�����÷���

  //GPIO_Write(ADDR_HIGH_PORT, AddrHi);   //ע�⣡��������ſ���ģ���ַ��λ���ÿڣ�����������������IO�ڣ����Ϊ�Լ��ĳ�����ǵ����ø�λ�ĵ�ַ���ڴ�����IO
  GPIO_Write(ADDR_LOW_PORT, AddrLo);    //д���8λ��ַ����3λ�ſ�
  for(u8 i=0; i<3; i++)
  {
    Delay(0xFFFF);    //ÿ��DelayԼ200ms������ʱ0.5�����ϣ���Ϊ��0.5��ʱ�����ַIO�ڣ�֮���ַIO��״̬��Ӱ���ַ����
  }
  //GPIO_Write(ADDR_HIGH_PORT, 0);
  GPIO_Write(ADDR_LOW_PORT, 0x00);    //����ַIO���û�0�Խ��͹���

#else   //�����˴������õ�ַ���������ô������õ�ַ

  //���ô������õ�ַ��ʽ���������е�ַIO��ʹ֮Ϊ0x0000
  //������IO�ڳ�ʼ��ʱ��Ϊ�����͵�ƽ��������ĵ�ƽ��ע����������κε�ַIO�ڣ��򴮿ڵ�ַ��Ч���Զ�����IO�������õĵ�ַ
  //GPIO_Write(ADDR_LOW_PORT, 0);   //��ַ��λд��͵�ƽ����ʼ����Ϊ�͵�ƽ������д��
  //GPIO_Write(ADDR_HIGH_PORT, 0);  //��ַ��λд��͵�ƽ����ʼ����Ϊ�͵�ƽ������д��
  
  //���½���ַת��Ϊ�������������ַ���������������������UNI_SEC_ADDR������������ֵ֮��Ŀո�
  UARTSendDataBuf[0]='U';UARTSendDataBuf[1]='N';UARTSendDataBuf[2]='I';UARTSendDataBuf[3]='_';UARTSendDataBuf[4]='S';
  UARTSendDataBuf[5]='E';UARTSendDataBuf[6]='C';UARTSendDataBuf[7]='_';UARTSendDataBuf[8]='A';UARTSendDataBuf[9]='D';
  UARTSendDataBuf[10]='D';UARTSendDataBuf[11]='R';UARTSendDataBuf[12]=' ';
  //Ȼ��13bit��ַת��Ϊ���ֽ��ַ�������0x0789ת��Ϊ��1929����0x0089ת��Ϊ��0137��������ֵ����UARTSendDataBuf[13]~UARTSendDataBuf[16]
  SettingTemp=Addr;
  for(u8 i=0; i<4; i++)
  {
    UARTSendDataBuf[16-i] = '0' + (SettingTemp % 10);
    SettingTemp /= 10;
  }
  //�������õ�ַ����
  UART_Send_Data(UARTSendDataBuf, 17);
  
  Delay(0x3000);    //�����ȴ�
  
#endif
  
  /*----------���½��������������ã�����35�д�ȡ����Ҫ����ֵ��#defineע�Ͳ�������Ӧֵ----------*/
  
#ifdef  SETTING_PANID   //PANID���ã�ע�����õ�PANIDҪ��������·����һ�²ſ���������
  assert_param(SETTING_PANID>=1 && SETTING_PANID<=65535);
  
  UARTSendDataBuf[0]='P';UARTSendDataBuf[1]='A';UARTSendDataBuf[2]='N';UARTSendDataBuf[3]='I';UARTSendDataBuf[4]='D';
  UARTSendDataBuf[5]=' ';
  //Ȼ�����ֽ�PANIDת��Ϊ���ֽ��ַ���������ֵ����UARTSendDataBuf[6]~UARTSendDataBuf[10]
  SettingTemp=SETTING_PANID;
  for(u8 i=0; i<5; i++)
  {
    UARTSendDataBuf[10-i] = '0' + (SettingTemp % 10);
    SettingTemp /= 10;
  }
  //������������
  UART_Send_Data(UARTSendDataBuf, 11);
  
  Delay(0x3000);    //�����ȴ�  
#endif  //SETTING_PANID
  
#ifdef  SETTING_TX_POWER   //TX_POWER���书������
  assert_param(SETTING_TX_POWER>=0 && SETTING_TX_POWER<=21);
  
  UARTSendDataBuf[0]='T';UARTSendDataBuf[1]='X';UARTSendDataBuf[2]='_';UARTSendDataBuf[3]='P';UARTSendDataBuf[4]='O';
  UARTSendDataBuf[5]='W';UARTSendDataBuf[6]='E';UARTSendDataBuf[7]='R';UARTSendDataBuf[8]=' ';
  //Ȼ��TX_POWERת��Ϊ���ֽ��ַ���������ֵ����UARTSendDataBuf[9]~UARTSendDataBuf[10]
  SettingTemp=SETTING_TX_POWER;
  for(u8 i=0; i<2; i++)
  {
    UARTSendDataBuf[10-i] = '0' + (SettingTemp % 10);
    SettingTemp /= 10;
  }
  //������������
  UART_Send_Data(UARTSendDataBuf, 11);
  
  Delay(0x3000);    //�����ȴ�  
#endif  //SETTING_TX_POWER
  
#ifdef  SETTING_CHANNEL   //CHANNEL���ã�ע�����õ�CHANNELҪ��������·����һ�²ſ���������
  assert_param(SETTING_CHANNEL>=2048 && SETTING_CHANNEL<=134215680);
  
  UARTSendDataBuf[0]='C';UARTSendDataBuf[1]='H';UARTSendDataBuf[2]='A';UARTSendDataBuf[3]='N';UARTSendDataBuf[4]='N';
  UARTSendDataBuf[5]='E';UARTSendDataBuf[6]='L';UARTSendDataBuf[7]=' ';
  //Ȼ��CHANNELת��Ϊʮ�ֽ��ַ���������ֵ����UARTSendDataBuf[8]~UARTSendDataBuf[17]
  SettingTemp=SETTING_CHANNEL;
  for(u8 i=0; i<10; i++)
  {
    UARTSendDataBuf[17-i] = '0' + (SettingTemp % 10);
    SettingTemp /= 10;
  }
  //������������
  UART_Send_Data(UARTSendDataBuf, 18);
  
  Delay(0x3000);    //�����ȴ�  
#endif  //SETTING_CHANNEL
  
#ifdef  SETTING_POLL_RATE   //POLL_RATE�ն˶��ڻ��Ѳ�ѯ����ʱ�� 
  assert_param(SETTING_POLL_RATE>=0 && SETTING_POLL_RATE<=65535);
  
  UARTSendDataBuf[0]='P';UARTSendDataBuf[1]='O';UARTSendDataBuf[2]='L';UARTSendDataBuf[3]='L';UARTSendDataBuf[4]='_';
  UARTSendDataBuf[5]='R';UARTSendDataBuf[6]='A';UARTSendDataBuf[7]='T';UARTSendDataBuf[8]='E';UARTSendDataBuf[9]=' ';
  //Ȼ��POLL_RATEת��Ϊ���ֽ��ַ���������ֵ����UARTSendDataBuf[10]~UARTSendDataBuf[14]
  SettingTemp=SETTING_POLL_RATE;
  for(u8 i=0; i<5; i++)
  {
    UARTSendDataBuf[14-i] = '0' + (SettingTemp % 10);
    SettingTemp /= 10;
  }
  //������������
  UART_Send_Data(UARTSendDataBuf, 15);
  
  Delay(0x3000);    //�����ȴ�  
#endif  //SETTING_POLL_RATE
  
  
  //д������ģ������PW_RESET 1
  UARTSendDataBuf[0]='P';UARTSendDataBuf[1]='W';UARTSendDataBuf[2]='_';UARTSendDataBuf[3]='R';UARTSendDataBuf[4]='E';
  UARTSendDataBuf[5]='S';UARTSendDataBuf[6]='E';UARTSendDataBuf[7]='T';UARTSendDataBuf[8]=' ';UARTSendDataBuf[9]='1';
  //������������
  UART_Send_Data(UARTSendDataBuf, 10);
  
  //�ȴ�ģ������������0.5��ʱ�����ַ����IO��Ϊ0x0000�����봮�����õ�ַģʽ����ȡ�ղ����Ǵ��������õĵ�ַ�������ڸõ�ַ
  for(u8 i=0; i<6; i++)
  {
    Delay(0xFFFF);    //ÿ��DelayԼ200ms������ʱ1�����ϣ�����Ҫ�������ȴ�ʱ�䣬��Ϊģ���ϵ�һ��ʱ��󴮿ڲſ�ʼ������������
  }
    
  
  //���½���18B20������������
  DS18B20_Exist = DS18B20_Init();
  if(DS18B20_Exist)
  {
    DS18B20_Write(0xCC);  //����ROM����
    DS18B20_Write(0xBE);  //������
    //����TH��TLֵ�Ա�����ʱ����ԭ��д�벻���иı�
    Temprature=DS18B20_Read();    //��8λ
    Temprature=Temprature | (DS18B20_Read()<<8);  //��8λ
    TH=DS18B20_Read();
    TL=DS18B20_Read();
    Config=DS18B20_Read();
    
    DS18B20_Init();
    DS18B20_Write(0xCC);  //����ROM����
    DS18B20_Write(0x4E);  //д����
    DS18B20_Write(TH);    //ԭ��д�벻�ı�
    DS18B20_Write(TL);    //ԭ��д�벻�ı�
    DS18B20_Write(0x1F);  //���ýϵͷֱ����ܴ�����͹��� 1F��9λ��93.75ms 3F��10λ��187.5ms 5F��11λ��375ms 7F��12λ��750ms
  }
  
  //SHT10�ĳ�ʼ��
  SHT10_WriteStart();
  SHT10_Exist = STH10_WriteReg(SHT10_REG_ACC_LOW);  //����Ϊ�;���80����ɼ�ʱ�䣬SHT10����0.5�� 4.5%RH��12bit/8bit ADC�����ѹ�
  SHT10_Wait();                                     //����ʵ�⣬�ɼ�������ADC�����ȣ��;������¶Ȳɼ�80���룬ʪ�Ȳɼ�20���룬�߾������¶Ȳɼ�320���룬ʪ�Ȳɼ�80����
  
  disableInterrupts();
  
  /* Infinite loop */
  while (1)
  {
#ifdef  WATCHDOG
    IWDG_ReloadCounter();   //ι��
#endif
    
    if(WakeCount<=0)    //ÿWORK_TO_WAKE_RATIO�λ��Ѳɼ�һ�δ��������ݲ��ϴ���֮���Բ��ó�ֵ-1����<=0����++��%WORK_TO_WAKE_RATIO�ķ�ʽ����Ϊ���ڿ���ʱ����������ʹ��������������
    {
      
      //DS18B20����뷢������
      if(DS18B20_Exist)
      {
        DS18B20_Exist = DS18B20_Init();   //�κ�ʱ���ʼ��ʧ�ܶ����ΪDS18B20�����ڲ������Խ�Լ����
        DS18B20_Write(0xCC);  //����ROM����
        DS18B20_Write(0x44);  //�¶�ת��
        
        //9λ�ֱ���93.75ms����ʱ������£�����˯��128ms���ȴ�18B20�������
        //ע�⣡���������߷ֱ��ʻ��ӳ�����ʱ�䣬��ʱӦ�ӳ�˯��ʱ��ʹ֮���ڲ���ʱ�䣬��������
        enableInterrupts();
        AWU_Init(AWU_Timebase_128ms);
        AWU_ReInitCounter();
        AWU_Cmd(ENABLE);
        halt();
        disableInterrupts();
        
#ifdef  WATCHDOG
        IWDG_ReloadCounter();   //ι��
#endif
    
        while(GPIO_ReadInputDataBit(SENSOR_DATA_PORT, SENSOR_DATA_PIN)==RESET);   //��18B20��δ���������ȴ�
        
        DS18B20_Exist = DS18B20_Init();   //�κ�ʱ���ʼ��ʧ�ܶ����ΪDS18B20�����ڲ������Խ�Լ����
        DS18B20_Write(0xCC);  //����ROM����
        DS18B20_Write(0xBE);  //��ȡRAM
        Temprature=DS18B20_Read();    //��8λ
        Temprature=Temprature | (DS18B20_Read()<<8);  //��8λ
        TH=DS18B20_Read();
        TL=DS18B20_Read();
        Config=DS18B20_Read();
        DS18B20_Init();       //reset����ֹ��д�����Ĵ���
        
        DS18B20_Data_Send();  //DS18B20��������
      }
      
      //SHT10����뷢������
      if(SHT10_Exist)
      {
        //�¶ȼ��
        SHT10_WriteStart();
        SHT10_Exist = STH10_Write(SHT10_MEASURE_TEMP);
        if(SHT10_Exist)
        {
          //��ʱ�����ڣ�����SCK������©�磨�ֲ��������ǵ͵�ƽ�ȴ������ⲿ����������©�磬�����øߵ�ƽ�ȴ���
          SHT10_Wait();
          
          //8bit/12bit�ֱ����¶�80ms����ʱ������£�����˯��128ms���ȴ�SHT10������ɣ��������жϻ���
          //ע�⣡���������߷ֱ��ʻ��ӳ��¶Ȳ���ʱ����320ms����ʱӦ�ӳ�˯��ʱ����512ms����ʹ֮���ڲ���ʱ�䣬����ᵼ��STM8���������ȴ�SHT10�˷ѵ�
          EXTI_ClearITPendingBit(EXTI_IT_Pin0);   //���ж�ǰҪ��SDA���жϱ�־����Ȼ�����ϴβ���һ��ȥ�ʹ���
          enableInterrupts();
          AWU_Init(AWU_Timebase_128ms);
          AWU_ReInitCounter();
          AWU_Cmd(ENABLE);
          halt();
          disableInterrupts();
          
#ifdef  WATCHDOG
          IWDG_ReloadCounter();   //ι��
#endif
          
          SHT10_TempData = STH10_ReadData();    //����ԭʼ����������
          SHT10_Wait();
          SHT10_Temp = SHT10_CalTemp(SHT10_TempData);   //ת��Ϊ������
          SHT10_TempWord = SHT10_ConvertTemp(SHT10_Temp);   //ת��Ϊ2�ֽ�����
        }
        
        //ʪ�ȼ��
        SHT10_WriteStart();
        SHT10_Exist = STH10_Write(SHT10_MEASURE_HUMI);
        if(SHT10_Exist)
        {
          //��ʱ�����ڣ�����SCK������©�磨�ֲ��������ǵ͵�ƽ�ȴ������ⲿ����������©�磬�����øߵ�ƽ�ȴ���
          SHT10_Wait();
          
          //8bit/12bit�ֱ���ʪ��20ms����ʱ������£�����˯��32ms���ȴ�SHT10������ɣ��������жϻ���
          //ע�⣡���������߷ֱ��ʻ��ӳ�ʪ�Ȳ���ʱ����80ms����ʱӦ�ӳ�˯��ʱ����128ms����ʹ֮���ڲ���ʱ�䣬����ᵼ��STM8���������ȴ�SHT10�˷ѵ�
          EXTI_ClearITPendingBit(EXTI_IT_Pin0);   //���ж�ǰҪ��SDA���жϱ�־����Ȼ�����ϴβ���һ��ȥ�ʹ���
          enableInterrupts();
          AWU_Init(AWU_Timebase_32ms);
          AWU_ReInitCounter();
          AWU_Cmd(ENABLE);
          halt();
          disableInterrupts();
          
#ifdef  WATCHDOG
          IWDG_ReloadCounter();   //ι��
#endif
          
          SHT10_HumiData = STH10_ReadData();    //����ԭʼ����������
          SHT10_Wait();
          SHT10_Humi = SHT10_CalHumi(SHT10_HumiData, SHT10_Temp);   //ת��Ϊ������
          SHT10_HumiWord = SHT10_ConvertHumi(SHT10_Humi);   //ת��Ϊ2�ֽ�����
        }
        
        SHT10_Data_Send();  //SHT10��������
      }
      
    }

#ifdef  WATCHDOG
    IWDG_ReloadCounter();   //ι��
#endif
    
    WakeCount++;
    WakeCount = WakeCount % WORK_TO_WAKE_RATIO;   //ÿWORK_TO_WAKE_RATIO�λ��Ѳɼ�һ�δ��������ݲ��ϴ���ԼWORK_TO_WAKE_RATIO*30��ɼ��ϴ�һ�Σ�

    if(ResetZigbeeCount==0)
    {
      //�ȴ�zigbee�������һ�����ݺ�������
      enableInterrupts();
      AWU_Init(AWU_Timebase_128ms);
      AWU_ReInitCounter();
      AWU_Cmd(ENABLE);
      halt();
      disableInterrupts();
      
      GPIO_WriteBit(RST_PORT, RST_PIN, RESET);    //��������zigbeeģ��
      DELAY_5US();
      GPIO_WriteBit(RST_PORT, RST_PIN, SET);      //��ԭ
    }
    ResetZigbeeCount++;
    ResetZigbeeCount = ResetZigbeeCount % RESET_ZIGBEE_TO_WAKE_RATIO;
    
    enableInterrupts();
    AWU_Init(AWU_Timebase_30s);   //30��˯�ߣ�ע�⣡��ʵ��˯��ʱ�����ϴ�
    AWU_ReInitCounter();
    AWU_Cmd(ENABLE);
    halt();
    disableInterrupts();
    
  }

}

/**
  * @brief  Delay.
  * @param  nCount
  * @retval None
  */
void Delay(uint16_t nCount)   //ʵ��ÿ��count��Ӧ6ָ�����ڣ�2MHz�¶�Ӧ3us
{
    /* Decrement nCount value */
    while (nCount != 0)
    {
        nCount--;
    }
}

/**
  * @brief  18B20 Init.
  * @param  None
  * @retval TRUE - DS18B20���ڲ���ʼ���ɹ��� FALSE - DS18B20������
  */
u8 DS18B20_Init(void)
{
  SENSOR_DATA_PORT->ODR &= (uint8_t)(~(SENSOR_DATA_PIN));   //�������ߣ�����͵�ƽ
  SENSOR_DATA_PORT->DDR |= SENSOR_DATA_PIN;
  //DELAY_500US();DELAY_100US();    //����600us�����Ҫ1200��nop��ռ��1.2K ROM��̫�ķ���Դ���ĳ��������delay
  Delay(200);   //ʵ��2M��ÿ�����ڶ�Ӧ3us
  SENSOR_DATA_PORT->DDR &= (uint8_t)(~(SENSOR_DATA_PIN));   //�ͷ����ߣ�����Ϊ����
  DELAY_50US();DELAY_10US();       //�ȴ�60΢��
  if(GPIO_ReadInputDataBit(SENSOR_DATA_PORT, SENSOR_DATA_PIN)==RESET)       //���18B20�������߱�������
  {
    while(GPIO_ReadInputDataBit(SENSOR_DATA_PORT, SENSOR_DATA_PIN)==RESET);   //�ȴ�18B20�ͷ�����
    DELAY_50US();    //�ȴ�RX�����������
    
    return TRUE;
  }
  
  //����һ��˵��������
  return FALSE;
}

/**
  * @brief  18B20 Read.
  * @param  None
  * @retval None
  */
u8 DS18B20_Read(void)
{
  u8 Data;
  
  for(u8 i=0;i<8;i++)
  {
    Data >>= 1;
    
    SENSOR_DATA_PORT->ODR &= (uint8_t)(~(SENSOR_DATA_PIN));   //�������ߣ�����͵�ƽ
    SENSOR_DATA_PORT->DDR |= SENSOR_DATA_PIN;
    DELAY_1US();DELAY_1US();                 //��ʱ2us
    SENSOR_DATA_PORT->DDR &= (uint8_t)(~(SENSOR_DATA_PIN));   //�ͷ����ߣ�����Ϊ����
    DELAY_10US(); DELAY_1US();DELAY_1US();   //�ȴ�12us
    
    if(GPIO_ReadInputDataBit(SENSOR_DATA_PORT, SENSOR_DATA_PIN)==SET)   //��ȡ��ƽ
    {
      Data |= 0x80;
    }
    
    DELAY_10US();DELAY_10US();DELAY_10US();DELAY_10US();DELAY_5US();    //�ȴ�45us
  }
  
  return Data;
}

/**
  * @brief  18B20 Write.
  * @param  None
  * @retval None
  */
void DS18B20_Write(u8 Data)
{
  for(u8 i=0;i<8;i++)
  {
    SENSOR_DATA_PORT->ODR &= (uint8_t)(~(SENSOR_DATA_PIN));   //�������ߣ�����͵�ƽ
    SENSOR_DATA_PORT->DDR |= SENSOR_DATA_PIN;
    DELAY_10US();DELAY_5US();     //�ȴ�15us
    
    if(Data & 0x01)     //���д��1
    {
      SENSOR_DATA_PORT->DDR &= (uint8_t)(~(SENSOR_DATA_PIN));   //�ͷ����ߣ�����Ϊ����
    }
    DELAY_10US();DELAY_10US();DELAY_10US();DELAY_10US();DELAY_5US();     //�ȴ�45us
    
    SENSOR_DATA_PORT->DDR &= (uint8_t)(~(SENSOR_DATA_PIN));   //�ͷ����ߣ�����Ϊ����
    DELAY_5US();    //�ָ�ʱ��
    
    Data >>= 1;
  }
}


/**
  * @brief  18B20 ���ݷ���zigbee.
  * @param  None
  * @retval None
  */
void DS18B20_Data_Send(void)
{
#ifndef UI_STRING
  //���ɼ������¶����ݷ��͵�����
  UARTSendDataBuf[0]=0xAA;      //4�ֽ�Ŀ�ĵ�ַ��ͷ��0xAA Ŀ�ĵ�ַ��λ Ŀ�ĵ�ַ��λ 0x55������Ŀ�ĵ�ַ�ߵ�λ��Ϊ0x00
  UARTSendDataBuf[1]=0x00;
  UARTSendDataBuf[2]=0x00;
  UARTSendDataBuf[3]=0x55;
  UARTSendDataBuf[4]=AddrHi;    //���͸�����������ַ�Է���������Ѱַ����ַ��λ���������Ƿſ��˵�ַ��λ��ģ���3λ��ַ��λĬ����������Ϊ0
  UARTSendDataBuf[5]=AddrLo;    //���͸�����������ַ��λ
  UARTSendDataBuf[6]=(u8)(Temprature>>8);     //�¶ȸ�λ
  UARTSendDataBuf[7]=(u8)(Temprature&0x00FF); //�¶ȵ�λ
  UART_Send_Data(UARTSendDataBuf, 8);         //�����ݷ��͵�����
#else
  //�ַ�����ʽ
  //��ʽ ID:1234 T:+025.0
  //0~3��AA XX XX 55��Ŀ�ĵ�ַ
  //4~6, 3�ֽ�ͷ
  //7~10, 4�ֽڵ�ַ
  //11~13, 3�ֽ���
  //14������λ
  //15~17������λ
  //18��С����
  //19��С��λ
  //20��1�ֽ�β
  UARTSendDataBuf[0]=0xAA;UARTSendDataBuf[1]=0x00;UARTSendDataBuf[2]=0x00;UARTSendDataBuf[3]=0x55;
  UARTSendDataBuf[4]='I';UARTSendDataBuf[5]='D';UARTSendDataBuf[6]=':';
  //��������ַת��Ϊʮ�ֽ��ַ���������ֵ����UARTSendDataBuf[10]~UARTSendDataBuf[19]
  SettingTemp=Addr;
  for(u8 i=0; i<4; i++)
  {
    UARTSendDataBuf[10-i] = '0' + (SettingTemp % 10);
    SettingTemp /= 10;
  }
/*  //��ͷ��������0�滻Ϊ�ո�
  i=7;
  while(UARTSendDataBuf[i]=='0')
  {
    UARTSendDataBuf[i++]=' ';
  }
*/  
  UARTSendDataBuf[11]=' ';UARTSendDataBuf[12]='T';UARTSendDataBuf[13]=':';
  UARTSendDataBuf[18]='.';
  UARTSendDataBuf[20]=' ';
  
  //��������ɱ����ݲ���
  
  UARTSendDataBuf[14]='+';
  if(Temprature & 0x8000)   //���λΪ1�����ģ�ȡ����һ
  {
    UARTSendDataBuf[14]='-';
    Temprature = (~Temprature)+1;
  }
  //С������
  UARTSendDataBuf[19]='0';
  if(Temprature & 0x000F)
  {
    UARTSendDataBuf[19]='5';
  }
  //��������
  SettingTemp=Temprature>>4;
  for(u8 i=0; i<3; i++)
  {
    UARTSendDataBuf[17-i] = '0' + (SettingTemp % 10);
    SettingTemp /= 10;
  }
  UART_Send_Data(UARTSendDataBuf, 21);         //�����ݷ��͵�����
#endif
}


/**
  * @brief  SHT10 ���ݷ���zigbee.
  * @param  None
  * @retval None
  */
void SHT10_Data_Send(void)
{
#ifndef UI_STRING
  //���ɼ������¶����ݷ��͵�����
  UARTSendDataBuf[0]=0xAA;      //4�ֽ�Ŀ�ĵ�ַ��ͷ��0xAA Ŀ�ĵ�ַ��λ Ŀ�ĵ�ַ��λ 0x55������Ŀ�ĵ�ַ�ߵ�λ��Ϊ0x00
  UARTSendDataBuf[1]=0x00;
  UARTSendDataBuf[2]=0x00;
  UARTSendDataBuf[3]=0x55;
  UARTSendDataBuf[4]=AddrHi;    //���͸�����������ַ�Է���������Ѱַ����ַ��λ���������Ƿſ��˵�ַ��λ��ģ���3λ��ַ��λĬ����������Ϊ0
  UARTSendDataBuf[5]=AddrLo;    //���͸�����������ַ��λ
  UARTSendDataBuf[6]=(u8)(SHT10_TempWord>>8);     //�¶ȸ�λ
  UARTSendDataBuf[7]=(u8)(SHT10_TempWord&0x00FF); //�¶ȵ�λ
  UARTSendDataBuf[8]=(u8)(SHT10_HumiWord>>8);     //ʪ�ȸ�λ
  UARTSendDataBuf[9]=(u8)(SHT10_HumiWord&0x00FF); //ʪ�ȵ�λ
  UART_Send_Data(UARTSendDataBuf, 10);         //�����ݷ��͵�����
#else
  //�ַ�����ʽ
  //��ʽ ID:1234 T:+025.0 H:100.0% 
  //0~3��AA XX XX 55��Ŀ�ĵ�ַ
  //4~6, 3�ֽ�ͷ
  //7~10, 4�ֽڵ�ַ
  //11~13, 3�ֽ���
  //14������λ
  //15~17������λ
  //18��С����
  //19��С��λ
  //20~22, 3�ֽ���
  //23~25, ����λ
  //26, С����
  //27, С��λ
  //28~29��2�ֽ�β
  UARTSendDataBuf[0]=0xAA;UARTSendDataBuf[1]=0x00;UARTSendDataBuf[2]=0x00;UARTSendDataBuf[3]=0x55;
  UARTSendDataBuf[4]='I';UARTSendDataBuf[5]='D';UARTSendDataBuf[6]=':';
  //��������ַת��Ϊʮ�ֽ��ַ���������ֵ����UARTSendDataBuf[10]~UARTSendDataBuf[19]
  SettingTemp=Addr;
  for(u8 i=0; i<4; i++)
  {
    UARTSendDataBuf[10-i] = '0' + (SettingTemp % 10);
    SettingTemp /= 10;
  }
/*  //��ͷ��������0�滻Ϊ�ո�
  i=7;
  while(UARTSendDataBuf[i]=='0')
  {
    UARTSendDataBuf[i++]=' ';
  }
*/  
  UARTSendDataBuf[11]=' ';UARTSendDataBuf[12]='T';UARTSendDataBuf[13]=':';
  UARTSendDataBuf[18]='.';
  UARTSendDataBuf[20]=' ';UARTSendDataBuf[21]='H';UARTSendDataBuf[22]=':';
  UARTSendDataBuf[26]='.';
  UARTSendDataBuf[28]='%';UARTSendDataBuf[29]=' ';
  
  //��������ɱ����ݲ���
  
  //�¶�
  UARTSendDataBuf[14]='+';
  if(SHT10_TempWord & 0x8000)   //���λΪ1�����ģ�ȡ����һ
  {
    UARTSendDataBuf[14]='-';
    SHT10_TempWord = (~SHT10_TempWord)+1;
  }
  //С������
  SettingTemp = (SHT10_TempWord & 0x00FF) * 10 / 256;
  UARTSendDataBuf[19]='0' + SettingTemp;
  
  //��������
  SettingTemp = SHT10_TempWord>>8;
  for(u8 i=0; i<3; i++)
  {
    UARTSendDataBuf[17-i] = '0' + (SettingTemp % 10);
    SettingTemp /= 10;
  }
  
  //ʪ��
  //С������
  SettingTemp = (SHT10_HumiWord & 0x00FF) * 10 / 256;
  UARTSendDataBuf[27]='0' + SettingTemp;
  
  //��������
  SettingTemp = SHT10_HumiWord>>8;
  for(u8 i=0; i<3; i++)
  {
    UARTSendDataBuf[25-i] = '0' + (SettingTemp % 10);
    SettingTemp /= 10;
  }
  
  
  UART_Send_Data(UARTSendDataBuf, 30);         //�����ݷ��͵�����
#endif
}


/**
  * @brief  ���͸�ģ�����ݻ�����
  * @param  DataBuf - ���͸�ģ������ݣ�DataLength - ���ͳ���
  * @retval None
  */
void UART_Send_Data(u8 DataBuf[], u8 DataLength)
{
  u8 idx=0;
  
  USART_Cmd(ENABLE);    //ʹ�ܴ���
  
  //ѡ���������ֻ��ѷ�ʽ֮һ
  
  //��һ�֣�IO�ڻ��ѣ������ѿ����ߺ�ȴ�3ms����ʼ���ʹ����ź�
  //�ڷ�����һ�����ݰ���ȴ�20ms���Ϸ�����һ�����ݰ��������д����źŷ�����Ϻ����ͻ��ѿ�
  GPIO_WriteBit(MODE0_PORT, MODE0_PIN, SET);    //���Ѵ���
  Delay(0x400);         //�ȴ�3ms
  
  //�ڶ��֣�����ֱ�ӻ��ѣ�����һ�ֽ�0x55���Ѵ��ں�ȴ�10ms���ʹ������ݣ��ڷ�����һ�����ݰ���ȴ�20~150ms������һ�����ݰ�
  //�������150msδ�������ݰ�������Ҫ���½��л���
  //USART_SendData8(0x55);    //���ͻ����ֽ�
  //while((USART->SR & 0x80) == 0);      //�ȴ����ͻ����
  //Delay(0xD00);         //�ȴ�10ms
  
  for(idx=0; idx<DataLength; idx++)
  {
    USART_SendData8(DataBuf[idx]);    //���͵�ǰ�ַ�
    while((USART->SR & 0x80) == 0);   //�ȴ����ͻ����
  }
  
  while((USART->SR & 0x40) == 0);       //�ȴ��������
  GPIO_WriteBit(MODE0_PORT, MODE0_PIN, RESET);    //��ģ�����½���˯��
  
  //USART_Cmd(DISABLE);
}

/**
  * @brief  SHT10д��ʼ
  * @param  None
  * @retval None
  */
void SHT10_WriteStart(void)
{
  //��Ϊ���
  SHT10_SDA_PORT->DDR |= (uint8_t)SHT10_SDA_PIN;    SHT10_SCL_PORT->DDR |= (uint8_t)SHT10_SCL_PIN;
  
  //���������λ���ڣ�����DATA=1����תSCK 10��
  for(i=0;i<9;i++)
  {
    //SCK=0;��ʱ������
    SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;   SHT10_DELAY_H();
    //SCK=1;��ʱ������
    SHT10_SCL_PORT->ODR |= (uint8_t)SHT10_SCL_PIN;    SHT10_DELAY_H();
  }
  
  //DATA=1;SCK=0;��ʱ������
  SHT10_SDA_PORT->ODR |= (uint8_t)SHT10_SDA_PIN;    SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;   SHT10_DELAY_H();
  //SCK=1;��ʱ1/4����
  SHT10_SCL_PORT->ODR |= (uint8_t)SHT10_SCL_PIN;    SHT10_DELAY_Q();
  //DATA=0;��ʱ1/4����
  SHT10_SDA_PORT->ODR &= ~(uint8_t)SHT10_SDA_PIN;   SHT10_DELAY_Q();
  //SCK=0;��ʱ������
  SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;   SHT10_DELAY_H();
  //SCK=1;��ʱ1/4����
  SHT10_SCL_PORT->ODR |= (uint8_t)SHT10_SCL_PIN;    SHT10_DELAY_Q();
  //DATA=1;��ʱ1/4����
  SHT10_SDA_PORT->ODR |= (uint8_t)SHT10_SDA_PIN;    SHT10_DELAY_Q();
  //SCK=0;��ʱ������
  SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;   SHT10_DELAY_H();
  
  //ע��������SCK�͵�ƽ�ȴ����������������©�磬Ӧ���Ͻ���һ��ָ�����ָ��Ӧ����SHT10_Wait()����SCK
}

/**
  * @brief  SHT10д�����ֽ�
  * @param  Data - д������
  * @retval TURE - ��ȷ  FALSE - ����
  */
u8 STH10_Write(u8 Data)
{
  u8 status=0;
  
  //assert_param(Data == SHT10_MEASURE_TEMP || Data == SHT10_MEASURE_HUMI || Data == SHT10_READ_REG || Data ==SHT10_WRITE_REG || Data == SHT10_SOFT_RESET);
  
  //��Ϊ���
  SHT10_SDA_PORT->DDR |= (uint8_t)SHT10_SDA_PIN;
  //DATA=1;SCK=0;��ʱ1/4����
  SHT10_SDA_PORT->ODR |= (uint8_t)SHT10_SDA_PIN;    SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;   SHT10_DELAY_Q();
  
  for(i=0; i<8; i++)
  {
    //SCK=0ʱд������
    if(Data & 0x80)
    {
      SHT10_SDA_PORT->ODR |= (uint8_t)SHT10_SDA_PIN;    //����������
    }
    else
    {
      SHT10_SDA_PORT->ODR &= ~(uint8_t)SHT10_SDA_PIN;   //����������
    }
    SHT10_DELAY_Q();
    
    //����SCK���룬��ʱ������
    SHT10_SCL_PORT->ODR |= (uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_H();
    
    //����SCK����ʱ1/4����
    SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_Q();
    
    Data <<= 1;
  }
  
  //��Ϊ���룬��ʱ1/4����
  SHT10_SDA_PORT->DDR &= ~(uint8_t)SHT10_SDA_PIN;   SHT10_DELAY_Q();
  
  //�ж��Ƿ��д��󣬴˴�ӦΪSHT10����ACK��Ҳ����Ϊ0
  status = SHT10_SDA_PORT->IDR & (uint8_t)SHT10_SDA_PIN;
  
  //����SCK����ʱ������
  SHT10_SCL_PORT->ODR |= (uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_H();
  
  //����SCK����ʱ1/4����
  SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_Q();
  
  //�ж��Ƿ��д��󣬴˴�STH10�ͷ�Ϊ�ߣ�Ҳ����1��������Ӧ����0x01
  status <<= 1;
  status += SHT10_SDA_PORT->IDR & (uint8_t)SHT10_SDA_PIN;
  
  //��Ϊ�����Ϊ�˷���ɼ���ʱ���ģ����������Է���ɼ���ϵ���������ź�
  //SHT10_SDA_PORT->DDR |= (uint8_t)SHT10_SDA_PIN;
  
  //ע��������SCK�͵�ƽ�ȴ����������������©�磬����ǲɼ������ȴ�80�������Ϻ��ȡ
  //Ӧ�����ⲿ����SHT10_Wait()����SCK����©��
  
  if(status == 0x01)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

/**
  * @brief  SHT10������
  * @param  None
  * @retval ��������
  */
u16 STH10_ReadData(void)
{
  u8  DataHi=0,DataLo=0;
  u16 Data;
  u32 status=0;
  
  //��Ϊ���룬�ȴ�ֱ���͵�ƽ
  SHT10_SDA_PORT->DDR &= ~(uint8_t)SHT10_SDA_PIN;
  while(SHT10_SDA_PORT->IDR & (uint8_t)SHT10_SDA_PIN);
  
  //SCK=0;��ʱ1/4����
  //SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;   SHT10_DELAY_Q();
  //������䲻�ɼ�
  //SHT10����SDAʱ��������׼���ã�����������SCK�ߵ�ƽ�ȴ���͵�ƽ�ȴ����ֲ��ǵ͵�ƽ�ȴ���������Ϊʡ���Ϊ�ߵ�ƽ�ȴ���
  //SDA���ݶ���׼����Ϊ0����ʱֻҪSCK��������SHT10�ͻ�׼����һ�����ݣ���������ֱ�Ӷ����ͺ�
  
  //��λ
  for(i=0; i<8; i++)
  {
    DataHi <<= 1;
    
    //��ʱ1/4���ڵȴ�����׼����
    SHT10_DELAY_Q();
    
    //��������
    if(SHT10_SDA_PORT->IDR & (uint8_t)SHT10_SDA_PIN)
    {
      DataHi += 1;
    }
    
    //����������SCK����ʱ������
    SHT10_SCL_PORT->ODR |= (uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_H();
    
    //����SCK����ʱ1/4����
    SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_Q();
  }
  
  //��Ϊ����������Ի�Ӧ����ʱ1/4����
  SHT10_SDA_PORT->DDR |= (uint8_t)SHT10_SDA_PIN;  SHT10_SDA_PORT->ODR &= ~(uint8_t)SHT10_SDA_PIN;   SHT10_DELAY_Q();
  
  //����SCK����ʱ������
  SHT10_SCL_PORT->ODR |= (uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_H();
  
  //����SCK����ʱ1/4����
  SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_Q();
  
  //��Ϊ����
  SHT10_SDA_PORT->DDR &= ~(uint8_t)SHT10_SDA_PIN;
  
  //��λ
  for(i=0; i<8; i++)
  {
    DataLo <<= 1;
    
    //��ʱ1/4���ڵȴ�����׼����
    SHT10_DELAY_Q();
    
    //�������ݺ�����SCK
    if(SHT10_SDA_PORT->IDR & (uint8_t)SHT10_SDA_PIN)
    {
      DataLo += 1;
    }
    
    //����������SCK����ʱ������
    SHT10_SCL_PORT->ODR |= (uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_H();
    
    //����SCK����ʱ1/4����
    SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_Q();
  }
  
  //��Ϊ��������߱�ʾ��CRCУ�飬��ʱ1/4����
  SHT10_SDA_PORT->DDR |= (uint8_t)SHT10_SDA_PIN;  SHT10_SDA_PORT->ODR |= (uint8_t)SHT10_SDA_PIN;   SHT10_DELAY_Q();
  
  //����SCK����ʱ������
  SHT10_SCL_PORT->ODR |= (uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_H();
  
  //����SCK����ʱ1/4����
  SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_Q();
  
  Data = (uint16_t)DataHi;
  Data = (Data<<8) + (uint16_t)DataLo;
  
  //ע��������SCK�͵�ƽ�ȴ����������������©�磬Ӧ���Ͻ���һ��ָ�����ָ��Ӧ����SHT10_Wait()����SCK
  
  return Data;
}

/**
  * @brief  SHT10д�Ĵ���
  * @param  Reg - д��Ĵ�������
  * @retval None
  */
u8 STH10_WriteReg(u8 Reg)
{
  if(STH10_Write(SHT10_WRITE_REG))
  {
    if(STH10_Write(Reg))
    {
      return TRUE;
    }
  }
  return FALSE;
}


/**
  * @brief  SHT10���Ĵ���
  * @param  Reg - �����Ĵ�������
  * @retval None
  */
u8 STH10_ReadReg(void)
{
  u8 status=0,Reg=0;
  
  //ע�⣡��ĳЩSHT10����ȫ���������ö�ȡ�Ĵ������������Ĵ�������û��ACK���أ�
  //Ŀǰ�������õ�SHT10�����ɶ��Ĵ��������ǿ�д�����ã�����ԭ���в���ȷ�����Ĵ�����д���ܶ����������˴���
  status=STH10_Write(SHT10_READ_REG);
  if(status == FALSE)return 0xFF;
  
  //����SCK����ʱ1/4����
  SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_Q();
  
  //��Ϊ����
  SHT10_SDA_PORT->DDR &= ~(uint8_t)SHT10_SDA_PIN;
  
  for(i=0; i<8; i++)
  {
    Reg <<= 1;
    
    //��ʱ1/4���ڵȴ�����׼����
    SHT10_DELAY_Q();
    
    //��������
    if(SHT10_SDA_PORT->IDR & (uint8_t)SHT10_SDA_PIN)
    {
      Reg += 1;
    }
    
    //����������SCK����ʱ������
    SHT10_SCL_PORT->ODR |= (uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_H();
    
    //����SCK����ʱ1/4����
    SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_Q();
  }
  
  //��Ϊ��������߱�ʾ��CRCУ�飬��ʱ1/4����
  SHT10_SDA_PORT->DDR |= (uint8_t)SHT10_SDA_PIN;  SHT10_SDA_PORT->ODR |= (uint8_t)SHT10_SDA_PIN;   SHT10_DELAY_Q();
  
  //����SCK����ʱ������
  SHT10_SCL_PORT->ODR |= (uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_H();
  
  //����SCK����ʱ1/4����
  SHT10_SCL_PORT->ODR &= ~(uint8_t)SHT10_SCL_PIN;  SHT10_DELAY_Q();
  
  
  //ע��������SCK�͵�ƽ�ȴ����������������©�磬Ӧ���Ͻ���һ��ָ�����ָ��Ӧ����SHT10_Wait()����SCK
  
  return Reg;
}


/**
  * @brief  �����Լ�����ģ�����SHT10��������ΪSCK�ǿ�©������ⲿ�������裬��Ҫ����������SCK�Ա���©��
  * @param  None
  * @retval None
  */
void SHT10_Wait(void)
{
  //��ʱ1/2���ڣ�����SCK������©�磬�����´�д�����ʱ���µ���SHT10_WriteStart();
  SHT10_DELAY_H();  SHT10_SCL_PORT->ODR |= (uint8_t)SHT10_SCL_PIN; 
}


/**
  * @brief  �ɴ��������ݼ����¶�
  * @param  Data - ����������
  * @retval �¶�����
  */
float SHT10_CalTemp(u16 Data)
{
  float SOT=0.04;   //12bit�¶�
  float Temp;
  
  Temp = (float)Data*SOT-39.6;
  
  return Temp;
}


/**
  * @brief  �ɴ��������ݺ��¶����ݼ���ʪ��
  * @param  Data - ����������
  * @retval �¶�����
  */
float SHT10_CalHumi(u16 Data, float Temp)
{
  //float c1=-4.0,c2=0.648,c3=-0.00072;   //�;���ʪ��
  float c1=-2.0468,c2=0.5872,c3=-0.00040845;   //�°�;���ʪ��
  float t1=0.01,t2=0.00128;   //�;����¶Ȳ���
  float RHlinear;
  float RHtrue;
  
  RHlinear=c1+c2*(float)Data+c3*(float)Data*(float)Data;
  
  RHtrue=(Temp-25.0)*(t1+t2*(float)Data)+RHlinear;
  
  return RHtrue;
}


/**
  * @brief  �¶ȸ�����ת��Ϊ2�ֽ����ͣ����ֽ����������ֽ�С��
  * @param  Temp - �¶ȸ���
  * @retval �¶�����
  */
s16 SHT10_ConvertTemp(float Temp)
{
  s16 TempWord;
  
  TempWord=(int)(Temp*256);
  
  return TempWord;
}


/**
  * @brief  ʪ�ȸ�����ת��Ϊ2�ֽ����ͣ����ֽ����������ֽ�С��
  * @param  Humi - ʪ�ȸ���
  * @retval ʪ������
  */
s16 SHT10_ConvertHumi(float Humi)
{
  s16 HumiWord;
  
  HumiWord=(int)(Humi*256);
  
  return HumiWord;
}


#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/