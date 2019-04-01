
#include <DHT.h>              //for tempture sensor
#include<SoftwareSerial.h>  //define softSerial
#include<Enerlib.h>           //low power lib

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef short s16;
typedef long s32;
typedef unsigned char uint_8;
typedef unsigned char byte;
typedef unsigned short uint_16;
typedef unsigned long uint_32;

#define DHTTYPE DHT11
#define DHTPIN 8
#define rxPin 5
#define txPin 6
#define turn_on 3
#define lowPower 2 //arduino pro mini int0 is pin d2
#define dataSize 15
#define zbReset 8  
#define frPin 9

#define WORK_TO_WAKE_RATIO  1  //工作与唤醒次数比例，每WORK_TO_WAKE_RATIO次唤醒（一般为30s）进行一次采集与上传，设置值1到32767//暂时为1，最后为10
#define RESET_ZIGBEE_TO_WAKE_RATIO  2880  //重启zigbee模块与唤醒次数比例，每RESET_ZIGBEE_TO_WAKE_RATIO次唤醒（一般为30s）重启一次zigbee模块，zigbee没开看门狗，避免zigbee模块突发异常大量掉电
#define WAKE_MAX 2880

#define rand8() (u8)(rand() & (u8)0xFF)

/* My ---------------------------------------------------------*/

//变量
u8 UUID[12];
u8 UUIDlen=12;
u8 ID[14];//暂定2+12
u8 IDlen=14;
u8 situ[8];//暂定4+4
u8 situlen=8;
u8 datalen=9;
u8 randnum1;
u8 randnum2;
u8 md5id[16];
u8 md5key[16];//未来可以将所有md5合并
u8 md5len=16;
u8 situaes[16],situaeslen=16;//ResetZigbeeCount大小确定
u8 UARTRecDataBuf[32];//暂定
bool isRec=false;
//u8 hex_chars[16];
u16 changedrandnum16;
u16 randnum16;
u8 timeaes8len=16;
s32 timeaes32;

//状态
u8 notreallyimportantn=0;
bool datarec=false;
bool isalarm=false;
bool isemergency=false;

/* Private defines -----------------------------------------------------------*/
u8 i;
u16 Temprature;
u8  TH,TL,Config;
u8  AddrHi,AddrLo;
u16 Addr,SettingTemp;
u8  UARTSendDataBuf[48];//改为48！！！
u8  UARTRcvDataIdx=0;     //接收数据缓存指针
u8  WakeFromData=false;   //是否由接收数据唤醒
s32 WakeCount=-1;   //用于统计唤醒次数以实现每WORK_TO_WAKE_RATIO次唤醒一次采集上传
s32 ResetZigbeeCount=1;   //跳过=0时的第一次重启


/*
  blow *the struct of data transform
*/
typedef union
{
  int No;
  uint8_t bytes[4];
} NO_t; //transform the No. to byte

typedef union
{
  float data;
  uint8_t bytes[4];
} Data_t; //transform the data to byte

/*
  blow *define the gpio
*/
SoftwareSerial software_Serial(rxPin , txPin);
DHT dht(DHTPIN, DHTTYPE);

/*
 * *define the variable
*/
int Identity;
int index;
float humidity;
float temperature;
float heatindex;
NO_t iden;
byte identity;
byte recData;
Data_t hu, te, he;
byte sed_byte[dataSize] = {170, 00, 00, 85, 255, 00};
byte sendbyte[6]={170, 00, 00, 85, 56, 128};
volatile byte state = 0;   //MEMORY THE STATE OF SLEEP
Energy energy;     //define the low energy varie
u8 count=0;
/*
   DEFINE THE SLEEP FUNCTION
*/
void wakeISR() {
  if (energy.WasSleeping()) {
    state = 1;
  } else {
    state = 2;
  }
}

void send_message0(void)//发送基本信息，假定初始信息发送必然成功
{
  //FLASH_SetProgrammingTime(FLASH_ProgramTime_Standard);
  //FLASH_Unlock(FLASH_MemType_Program);
  for(u8 i=0; i<UUIDlen-1; i++)
    UUID[i]=0;//未来用设备号替代
  UUID[UUIDlen-1]=1;
    //FLASH_ReadByte(0x4926 + (s16)i);  //4926~4931
  //FLASH_Lock(FLASH_MemType_Program);
  
  ID[0]=(u8)'A';
  ID[1]=(u8)'0';//每个都要不一样
  for(u8 i=0; i<UUIDlen; i++)
    ID[i + 2]=UUID[i];
  
  setSitu();
  
  addHead(0);
  
  for(u8 i=0; i<IDlen; i++)
    UARTSendDataBuf[i + 7]=ID[i];
  
  for(u8 i=0; i<situlen; i++)
    UARTSendDataBuf[i + 7 + IDlen]=situ[i];
  
  u16 seed;
  seed=(UUID[UUIDlen - 1]<<8) | ID[1];//伪随机生成第一个种子
  srand((unsigned)(seed));
  randnum1=(u8)rand8();
  UARTSendDataBuf[7 + IDlen + situlen]=randnum1;

  delay(10);
  software_Serial.write(UARTSendDataBuf, 8 + IDlen + situlen);
    digitalWrite(turn_on , HIGH);
  delay(30);
  digitalWrite(turn_on , LOW);
}

void send_message1(void)//普通数据
{
  //将采集到的普通温度数据发送到主机
  addHead(1);
  
  getData(7);
   
  delay(10);
  software_Serial.write(UARTSendDataBuf, 7 + datalen); //将数据发送到主机

  digitalWrite(turn_on , HIGH);
  delay(30);
  digitalWrite(turn_on , LOW);
}

void send_message2(void)//重要数据
{
  srand((unsigned)((u8)(ResetZigbeeCount & 0x0000000F) | (u8)(randnum1 & 0xF0)));//可以利用ADC或者自带时钟产生随机数，暂时使用Reset值和randnum1做为时间种子
  randnum2=(u8)rand8();//头两位需不同//暂时没做！！！
  
  u8 RID[14 + 2];//IDlen + 2
  RID[0]=randnum1;
  RID[1]=randnum2;
  for(u8 i=0; i<IDlen; i++)
    RID[i + 2]=ID[i];

  getMD5(RID, IDlen + 2, md5id);
  
  u8 RUUID[12 + 1];//UUIDlen + 1
  RUUID[0]=randnum1;
  for(u8 i=0; i<UUIDlen; i++)
    RUUID[i + 1]=UUID[i];
  
  getMD5(RUUID, UUIDlen + 1, md5key);
  
  //extendU8(ResetZigbeeCount, situaes);//s32//改situ
  //setSitu(ResetZigbeeCount, situaes);//此时reCount必为正数
  setSituAes();
  //aes(situaes, situaeslen, md5key, md5len);
  
  addHead(2);
  
  getData(7);
  
  UARTSendDataBuf[7 + datalen]=randnum2;
  
  for(u8 i=0; i<md5len; i++)
      UARTSendDataBuf[i + 8 + datalen]=md5id[i];
  
  for(u8 i=0; i<situaeslen; i++)
      UARTSendDataBuf[i + 8 + datalen + md5len]=situaes[i];

  delay(10);
  software_Serial.write(UARTSendDataBuf, 8 + datalen + md5len +situaeslen);
    digitalWrite(turn_on , HIGH);
  delay(30);
  digitalWrite(turn_on , LOW);
}

void send_message4(void)//重要数据双向认证回复
{
  //暂定随采集数据更新，且类型既不普通也不重要
  addHead(4);
  
  getData(7);

  delay(10);
  software_Serial.write(UARTSendDataBuf, 7 + datalen);
    digitalWrite(turn_on , HIGH);
  delay(30);
  digitalWrite(turn_on , LOW);
  
  notreallyimportantn--;
}

void send_message6(void)//定时更新2
{
  R16UUIDAes();
  
  addHead(6);
  
  for(u8 i=0; i<situaeslen; i++)
    UARTSendDataBuf[i + 7]=situaes[i];

  delay(10);
  software_Serial.write(UARTSendDataBuf, 7 + situaeslen);
    digitalWrite(turn_on , HIGH);
  delay(30);
  digitalWrite(turn_on , LOW);
}

void send_message8(void)//带数据请求的定时更新2
{
  R16UUIDAes();
  
  addHead(8);
  
  for(u8 i=0; i<situaeslen; i++)
    UARTSendDataBuf[i + 7]=situaes[i];
  
  getData(7 + situaeslen);

  delay(10);
  software_Serial.write(UARTSendDataBuf, 7 + situaeslen + datalen);
    digitalWrite(turn_on , HIGH);
  delay(30);
  digitalWrite(turn_on , LOW);
  
  datarec=false;
}

void send_message10(void)//带随机数更新请求的定时更新2
{
  R16UUIDAes();
  
  addHead(10);
  
  for(u8 i=0; i<situaeslen; i++)
    UARTSendDataBuf[i + 7]=situaes[i];

  delay(10);
  software_Serial.write(UARTSendDataBuf, 7 + situaeslen);
    digitalWrite(turn_on , HIGH);
  delay(30);
  digitalWrite(turn_on , LOW);
  
  randnum1=((changedrandnum16 | 0x0F00)>>4) | (changedrandnum16 | 0x000F);//暂定的历史随机数更新秘密算法
}

void send_message12(void)//带报警接收的定时更新2
{
  R16UUIDAes();
  
  addHead(12);
  
  for(u8 i=0; i<situaeslen; i++)
    UARTSendDataBuf[i + 7]=situaes[i];

  delay(10);
  software_Serial.write(UARTSendDataBuf, 7 + situaeslen);
    digitalWrite(turn_on , HIGH);
  delay(30);
  digitalWrite(turn_on , LOW);
  
  isalarm=false;
  isemergency=false;//暂定可以通过报警解除消息解除紧急报警
}

void send_message13(void)//定时更新时间失效
{
  R16UUIDAes();
  
  addHead(13);
  
  for(u8 i=0; i<situaeslen; i++)
    UARTSendDataBuf[i + 7]=situaes[i];

  delay(10);
  software_Serial.write(UARTSendDataBuf, 7 + situaeslen);
    digitalWrite(turn_on , HIGH);
  delay(30);
  digitalWrite(turn_on , LOW);
}

void send_message14(void)//报警错误
{
  R16UUIDAes();
  
  addHead(14);
  
  for(u8 i=0; i<situaeslen; i++)
    UARTSendDataBuf[i + 7]=situaes[i];

  delay(10);
  software_Serial.write(UARTSendDataBuf, 7 + situaeslen);
    digitalWrite(turn_on , HIGH);
  delay(30);
  digitalWrite(turn_on , LOW);
}

void send_message15(void)//伪装基站报警
{
  //暂时随采集数据一起发送
  addHead(15);
  
  getData(7);

  delay(10);
  software_Serial.write(UARTSendDataBuf, 7 + datalen);
    digitalWrite(turn_on , HIGH);
  delay(30);
  digitalWrite(turn_on , LOW);
  
  isalarm=true;
}

void send_message16(void)//紧急报警
{
  //紧急警报未来可以考虑减少发送间隔，无视短时间内能量消耗
  addHead(16);
  
  delay(10);
  software_Serial.write(UARTSendDataBuf, 7);
    digitalWrite(turn_on , HIGH);
  delay(30);
  digitalWrite(turn_on , LOW);
  
  isemergency=true;
}

void ifRec(void)
{
  u8 j=0;
  while(software_Serial.available() > 0)//开始串口接收数据
  {
    Serial.println("rec data!");
    UARTRecDataBuf[j] = software_Serial.read();
    delay(2);
    j++;
  }

  software_Serial.flush();
  
  if(1)//长度判断
  {
    //判断消息类型
    //0~1发送方地址，2消息类型
      if((UARTRecDataBuf[0] == 0x00) && (UARTRecDataBuf[1] == 0x00))
      {
        if(UARTRecDataBuf[2] == 0x0D)
        {
          if(isalarm)//暂时不加紧急报警！！！
          {
            send_message15();
          }
          else if(datarec)
          {
            send_message8();
          }
          else
          {
            if(notreallyimportantn > 0)
            {
              send_message4();
            }
            else
            {
              if(testFire() < 30)//为了方便暂定30度== 0x01)//将火焰为01时当作普通，结合项目改！！！
              {
                send_message1();
              }
              else
                send_message2();
            }
          } 
          ResetZigbeeCount++;
          if(ResetZigbeeCount == WAKE_MAX)
            ResetZigbeeCount=0;
        }
        else if(UARTRecDataBuf[2] == 0x03)
          handle_message3();
        else if(UARTRecDataBuf[2] == 0x05)
          handle_message5();
        else if(UARTRecDataBuf[2] == 0x07)
          handle_message7();
        else if((UARTRecDataBuf[2] & 0x3F) == 0x09)
          handle_message9();
        else if(UARTRecDataBuf[2] == 0x0B)
          handle_message11();
        else if(UARTRecDataBuf[2] == 0x0C)
        {
            Serial.println("CC");
            digitalWrite(zbReset, HIGH);
            delay(100);
            digitalWrite(zbReset, LOW);
        }
        else
        {
           software_Serial.flush();
        }
        for(u8 i=0; i<32; i++){
          UARTRecDataBuf[i]=0x00;
        }
      }
  }
}

void handle_message3(void)
{
  u8 R8ID[14 + 1];//idlen + 1
  R8ID[0]=randnum1;
  for(u8 i=0; i<IDlen; i++)
    R8ID[i + 1]=ID[i];
  
  getMD5(R8ID, IDlen + 1,md5id);
  
  bool r=true;
  for(u8 i=0; i<md5len; i++)
    if(md5id[i] != UARTRecDataBuf[i + 3])
      r=false;
  
  if(r)
  {
    notreallyimportantn=144;//之后1小时+数据采集改为message4
    randnum1=randnum2;//预先更新随机数
  }
  else
    send_message15();
}

void handle_message5(void)
{
  if(isR16md5Right())
  {
    send_message6();
  }
  else
  {
    send_message15();//伪装基站报警
  }
}

void handle_message7(void)
{
  if(isR16md5Right())
  {
    //send_message8();//应该进入下一轮
    datarec=true;
  }
  else
  {
    send_message15();//伪装基站报警
  }
}

void handle_message9(void)
{
  u8 r=isMessageRight(9);
  if(r == 0)
  {
    randnum1=(u16)((u16)(changedrandnum16 & 0x0F00)>>4) | (u16)(changedrandnum16 & 0x000F);//更新随机数
    Serial.println(changedrandnum16, HEX);
    Serial.println(randnum1, HEX);
    send_message10();
  }
  else if(r == 1)
  {
    send_message13();//伪时间戳失效回复
  }
  else if(r == 2)
  {
    send_message15();//伪装基站报警
  }
}

void handle_message11(void)
{
  u8 r=isMessageRight(11);
  if(r == 0)
  {
    /*if(!isalarm)//此时不是报警状态
      send_message14();
    else
    {*/
      send_message12();//暂时不做是否在报警状态的判断，减少系统出错率！！！
      isalarm=false;
    //}
  }
  else if(r == 1)
  {
    send_message13();//伪时间戳失效回复
  }
  else if(r == 2)
  {
    send_message15();//伪装基站报警
  }
}

void addHead(u8 n)
{
  UARTSendDataBuf[0]=0xAA;      //4字节目的地址包头，0xAA 目的地址高位 目的地址低位 0x55，主机目的地址高低位均为0x00
  UARTSendDataBuf[1]=0x00;
  UARTSendDataBuf[2]=0x00;
  UARTSendDataBuf[3]=0x55;
  
  UARTSendDataBuf[4]=AddrHi;    //发送给主机本机地址以方便主机的寻址，地址高位，这里我们放空了地址高位，模块的3位地址高位默认下拉设置为0
  UARTSendDataBuf[5]=AddrLo;    //发送给主机本机地址低位
  
  if(n == 4)
    UARTSendDataBuf[6]=((randnum1 & 0xC0) | (0x04));
  else if(n == 10)
    UARTSendDataBuf[6]=n;//未来改为前两位为随机数，使更新的随机数更没有规律
  else
    UARTSendDataBuf[6]=n;//消息类别
}
/*
void hextoU8(u8 u[], u8 *len)
{
  hex_chars[0]='0';hex_chars[1]='1';hex_chars[2]='2';hex_chars[3]='3';
  hex_chars[4]='4';hex_chars[5]='5';hex_chars[6]='6';hex_chars[7]='7';
  hex_chars[8]='8';hex_chars[9]='9';hex_chars[10]='A';hex_chars[11]='B';
  hex_chars[12]='C';hex_chars[13]='D';hex_chars[14]='E';hex_chars[15]='F';//最后想办法改成真正hex型！！！

  u8 ret[16];//一般都为16

  for(int i=*len-1; i>=0; i-=2)
  {
    ret[(*len-i)/2]=((hex_chars_Find(u[i-1]) << 4) | hex_chars_Find(u[i]));
    //2个16进制数做1次处理
  }
  
  *len/=2;

  for(u8 i=0; i<*len;  i++)
    u[i]=ret[i];
}

u8 hex_chars_Find(u8 c)
{
  for(u8 i=0; i<16; i++)
    if(c == hex_chars[i])
      return i;
  return (u8)0xFF;
}
*/

void R16UUIDAes()
{ 
  u8 R16UUID[12 + 2];//UUIDlen + 2
  R16UUID[0]=(u8)(changedrandnum16>>8);
  R16UUID[1]=(u8)(changedrandnum16 & 0xFF);
  for(u8 i=0; i<UUIDlen; i++)
    R16UUID[i + 2]=UUID[i];
  
  getMD5(R16UUID, UUIDlen + 2, md5key);
  
  //situaes=extendU8(ResetZigbeeCount);//s32//改situ
  setSituAes();//此时reCount必为正数
  //aes(situaes, situaeslen, md5key, md5len);
}

bool isR16md5Right(void)//以后为节省代码存储量可以和R16TimeAes合并
{
  randnum16=(((u16)UARTRecDataBuf[3]<<8) | (u16)(UARTRecDataBuf[4]));
  
  changedrandnum16=changeR16(randnum16);
  
  u8 R16ID[14 + 2];//IDlen + 2
  R16ID[0]=(u8)((changedrandnum16>>8) & 0x00FF);
  R16ID[1]=(u8)((changedrandnum16) & 0x00FF);
  for(u8 i=0; i<IDlen; i++)
    R16ID[i + 2]=ID[i];
  getMD5(R16ID, IDlen + 2, md5id);//是否需要再设一个变量？？？

  bool r=true;
  for(u8 i=0; i<md5len; i++)//确认IDmd5是否正确
     if(md5id[i] != UARTRecDataBuf[i + 5])
       r=false;
  
  /*  
  bool r=FALSE;
  addHead(14);
  for(u8 i=0; i<md5len; i++)
    UARTSendDataBuf[i + 7]=md5id[i];
  UART_Send_Data(UARTSendDataBuf, 7 + md5len);
  
  Delay(0xFFFF);
  */
  
  return r;
}

bool isR16TimeAesRight(u8 n)//解密则不需要验证md5
{
  randnum16=(((u16)UARTRecDataBuf[3]<<8) | (UARTRecDataBuf[4]));

  changedrandnum16=changeR16(randnum16);
  
  u8 R16ID[14 + 2];//IDlen + 2
  if(n == 11)
  {
    R16ID[0]=(u8)(changedrandnum16>>8);
    R16ID[1]=(u8)((changedrandnum16) & 0x00FF);
    for(u8 i=0; i<IDlen; i++)
      R16ID[i + 2]=ID[i];
    getMD5(R16ID, IDlen + 2, md5key);//是否需要再设一个变量？？？
  }
  else if(n == 9)
  {
    u8 R16ID3[14 + 3];//IDlen + 3
    R16ID3[0]=(u8)(changedrandnum16>>8);
    R16ID3[1]=(u8)((changedrandnum16) & 0x00FF);
    R16ID3[2]=UARTRecDataBuf[2];//将带两位随机数的类型全部作为第三位随机数
    for(u8 i=0; i<IDlen; i++)
      R16ID3[i + 3]=ID[i];
    getMD5(R16ID3, IDlen + 3, md5key);//是否需要再设一个变量？？？
  }
  
  u8 timeaes8[16];//可以改成局部
  for(u8 i=0; i<timeaes8len; i++)//假定正确的16位
    timeaes8[i]=UARTRecDataBuf[i + 5];
  
  //u8 isRight=false;
  u8 isRight=0;//不加密，默认正确！！！
    
  //isRight=deAes(timeaes8, timeaes8len, md5key, md5len);
  
  if(isRight == 0)//u8!!!
    for(u8 i=0; i<timeaes8len - 4; i++)
      if(timeaes8[timeaes8len - i - 1] != (u8)'0')//暂定填充0
        isRight=101;//任意大于2的数
  
   timeaes32=((s32)timeaes8[0]<<24) | ((s32)timeaes8[1]<<16) | ((s32)timeaes8[2]<<8) | (s32)timeaes8[3];

  if(isRight == 0)
    return true;
  else
    return false;
}

bool isTimeRight(void)
{
  if((((ResetZigbeeCount - timeaes32) < 144) && (ResetZigbeeCount - timeaes32) > 0) || ((timeaes32 - ResetZigbeeCount) > 2736) || (ResetZigbeeCount < 144))//暂定1小时定时更新一次，每次消息时效1小时//第三条防止自重启
    return true;
  else
    return false;
}

u8 isMessageRight(u8 n)
{
  if(isR16TimeAesRight(n))
  {
    if(isTimeRight())
      return 0;
    else
      return 1;
  }
  else
    return 2;
}

void setSitu()
{
  situ[0]=(u8)((((u32)ResetZigbeeCount)>>24) & 0x000000FF);
  situ[1]=(u8)((((u32)ResetZigbeeCount)>>16) & 0x000000FF);
  situ[2]=(u8)((((u32)ResetZigbeeCount)>>8) & 0x000000FF);
  situ[3]=(u8)(ResetZigbeeCount & 0x000000FF);
  
  for(u8 i=4; i<situlen; i++)
    situ[i]=(u8)'0';//situ剩余位0填充
}

void setSituAes(void)
{
  setSitu();
  
  for(u8 i=0; i<situlen; i++)
    situaes[i]=situ[i];
  for(u8 i=situlen; i<situaeslen; i++)
    situaes[i]=(u8)'0';//所有加密'0'填充
}

u16 changeR16(u16 u)//暂定翻转前后的u8
{
  u16 tmp=u;
  u8 u1,u2;
  u1=(u8)((tmp>>8) & 0x00FF);
  u2=(u8)(tmp & 0x00FF);
  u=((u16)u2)<<8 | (u16)u1;
  return u;
}

u8 testFire()
{
  u8 TF;
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  heatindex = dht.computeHeatIndex(temperature, humidity, false);
  for ( i = 0 ; i < 10 ; i ++)
    count += digitalRead( frPin );
  if( count < 8) TF = 0x01;
  else if( count > 7) TF = 0x00; 

  Serial.println(count);
  Serial.println(temperature);
  Serial.println((u8)((int)temperature));
  //return TF;
  return (u8)((int)temperature);
}

void getData(u8 n)
{
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  heatindex = dht.computeHeatIndex(temperature, humidity, false);
  for ( i = 0 ; i < 10 ; i ++)
    count += digitalRead( frPin );
  if( count < 8) UARTSendDataBuf[n] = 0x01;
  else if( count > 7) UARTSendDataBuf[n] = 0x00; 
    
  hu.data = humidity;
  te.data = temperature;
  
  for (index = 0; index < 4; index++) {
    UARTSendDataBuf[n + index + 1] = hu.bytes[index];
  }

  for (index = 0; index < 4; index++) {
    UARTSendDataBuf[n + 4 + index + 1] = te.bytes[index];
  }
}

/*
   debug data
*/
char ss[20];
float *f;
char st[20];
int value;
void setup() {
  Serial.begin(115200);
//  while (!Serial) { //wait serial test
//    ;
//  }
  software_Serial.begin(115200);
  Serial.println("DHT11 Started!");
  Serial.println("CC2530 Started.");
  pinMode(turn_on , OUTPUT);
  pinMode(zbReset , OUTPUT);
  pinMode(frPin , INPUT);
  digitalWrite(turn_on , LOW);
  digitalWrite(zbReset, HIGH);
  attachInterrupt(0 , wakeISR , RISING);//触发模式有4种类型：LOW（低电平触发）、CHANGE（变化时触发）、RISING（低电平变为高电平触发）、FALLING（高电平变为低电平触发）
  index = 0;
  //Identity = 2; //the no.2 en
  // iden.No = Identity;
  identity = 0x02;
  sed_byte[5] = identity;
  // sed_byte[0] = 170;
  // sed_byte[1] = 00;
  // sed_byte[2] = 00;//the address of receiver
  // sed_byte[3] = 85;
  // sed_byte[5] = iden.bytes[0];
  // sed_byte[6] = iden.bytes[1];
  // sed_byte[7] = iden.bytes[2];
  // sed_byte[8] = iden.bytes[3];
  i = 0;

  AddrHi=0x00;
  AddrLo=0x02;//地址每个都要手动改！！！
  send_message0();//真实环境下可能需要多发几次！！！

}

void loop() {

 ifRec();
 
  //energy.PowerDown();//时灵时不灵

}

#define shift(x, n) ((u32)((x) << (n)) | ((x) >> (32-(n))))//右移的时候，高位一定要补零，而不是补充符号位
                            //左移为循环左移

#define FF(a ,b ,c ,d ,Mj ,s ,t ,i ,j) (a = b + ( (a + F(b,c,d) + M[j] + t[i]) << s[i]))//四步操作，但并不需要用
#define GG(a ,b ,c ,d ,Mj ,s ,t , i, j) (a = b + ( (a + G(b,c,d) + M[j] + t[i]) << s[i]))
#define HH(a ,b ,c ,d ,Mj ,s ,t, i, j) (a = b + ( (a + H(b,c,d) + M[j] + t[i]) << s[i]))
#define II(a ,b ,c ,d ,Mj ,s ,t, i, j) (a = b + ( (a + I(b,c,d) + M[j] + t[i]) << s[i]))

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))//四个非线性函数
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define A 0x67452301//大小端变化后
#define B 0xefcdab89
#define C 0x98badcfe
#define D 0x10325476

//strBaye的长度
uint_32 strlength;
//A,B,C,D的临时变量
uint_32 atemp;
uint_32 btemp;
uint_32 ctemp;
uint_32 dtemp;

//常量ti unsigned int(abs(sin(i+1))*(2pow32))

const uint_32 t[]={//ti常量数组
        0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,
        0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,0x698098d8,
        0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,
        0xa679438e,0x49b40821,0xf61e2562,0xc040b340,0x265e5a51,
        0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
        0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,
        0xfcefa3f8,0x676f02d9,0x8d2a4c8a,0xfffa3942,0x8771f681,
        0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,
        0xbebfbc70,0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,
        0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,0xf4292244,
        0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,
        0xffeff47d,0x85845dd1,0x6fa87e4f,0xfe2ce6e0,0xa3014314,
        0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391};

const uint_8 s[]={7,12,17,22,7,12,17,22,7,12,17,22,7,//左位移数
        12,17,22,5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
        4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,6,10,
        15,21,6,10,15,21,6,10,15,21,6,10,15,21};
/*
*填充函数
*处理后应满足bits≡448(mod512),字节就是bytes≡56（mode64)
*填充方式为先加一个1,其它位补零
*最后加上64位的原来长度
*/
uint_32* add(uint_8 *s, uint_8 len)
{
    uint_8 num=((len+8)/64)+1;//以512位,64个字节为一组
    uint_32 *strByte=(uint_32*)malloc(num * 16 * (sizeof(uint_32)));    //64/4=16,所以有16个整数
    strlength=num*16;
    for (uint_8 i = 0; i < num*16; i++)
        strByte[i]=0;
    for (uint_8 i=0; i <len; i++)
    {
        strByte[i>>2]|=(u32)(s[i])<<((i%4)*8);//一个整数存储四个字节，i>>2表示i/4 一个unsigned int对应4个字节，保存4个字符信息
    }

    strByte[len>>2]|=(u32)((u8)0x80)<<(((len%4))*8);//尾部添加1 一个unsigned int保存4个字符信息,所以用128左移
    //注意！arduino编译器常数位移默认有符号数！

    /*
    *添加原长度，长度指位的长度，所以要乘8，然后是小端序，所以放在倒数第二个,这里长度只用了32位
    */
    strByte[num*16-2]=len*8;

    return strByte;
}

void mainLoop(uint_32 M[])
{
    uint_32 f,j;
    uint_32 a=atemp;
    uint_32 b=btemp;
    uint_32 c=ctemp;
    uint_32 d=dtemp;
    for (uint_8 i = 0; i < 64; i++)
    {
        if(i<16){
      f=F(b,c,d);
      j=i;

        }
    else if (i<32)
    {
      f=G(b,c,d);
            j=(5*i+1)%16;
        }
    else if(i<48)
    {
      f=H(b,c,d);
            j=(3*i+5)%16;
        }
    else
    {
      f=I(b,c,d);
            j=(7*i)%16;
        }

    /*
        unsigned int tmp=d;
        d=c;
        c=b;
        b=b+shift((a+ f +t[i]+M[j]),s[i]);
        a=tmp;
    */
    
    a=b+shift((a+ f +t[i]+M[j]),s[i]);
    uint_32 tmp=a;
    a=d;//下一轮的a为这一轮的d
    d=c;
    c=b;
    b=tmp;
    }

    atemp=a+atemp;//最终处理
    btemp=b+btemp;
    ctemp=c+ctemp;
    dtemp=d+dtemp;

}

/*
uint_8* changeHex(uint_32 a)//变化为string
{
    uint_8 b;
    uint_8 s[8];
    uint_8 s1[2];
    
    for(uint_8 i=0;i<4;i++)
    {
        b=((a>>i*8)%(1<<8))&0xff;//整型int每次右移到合适位置后取低字节
        for (int j = 0; j < 2; j++)
        {
            s1[1-j]=(uint_8)str16[b%16];//str16为映射表
            b=b/16;
        }
        s[i*2]=s1[0];
        s[i*2+1]=s1[1];
    }
    return s;
}
*/

uint_32 new_changeHex(uint_32 a)//变化为string
{
    uint_32 b;
    b=(u32)((u32)(a>>24)|(u32)((a&0x00FF0000)>>8)|(u32)((a&0x0000FF00)<<8)|(u32)(a<<24));
    return b;
}

void getMD5(uint_8 *source,uint_8 length,uint_8 *md5)
{
    atemp=A;//大小端变化后
    btemp=B;
    ctemp=C;
    dtemp=D;

    uint_32 *strByte=add(source, length);//对输入数据进行填充处理

    for(uint_8 i=0;i<strlength/16;i++)//每16位分割
    {
        uint_32 num[16];
        for(uint_8 j=0;j<16;j++)
        {
            num[j]=strByte[i*16+j];
        }
        mainLoop(num);//num只是辅助传入输入数据，真正改变的是全局变量a(bcd)temp
    }

    uint_32 t[]={atemp,btemp,ctemp,dtemp};

    for(uint_8 i=0; i<4; i++)
    {
      uint_32 a=new_changeHex(t[i]);
      for(uint_8 j=0; j<4; j++)
      {
        md5[i*4 + j]=(uint_8)(a>>(8*(3-j))&0x000000FF);//大小端变化回
      }
    }

    //return 0;
}
