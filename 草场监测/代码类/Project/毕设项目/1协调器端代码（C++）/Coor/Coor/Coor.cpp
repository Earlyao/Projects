#include "stdio.h"
#include "stdlib.h"
#include <iostream>
#include <string>
#include "windows.h"
#include "time.h"
#include "md5.h"
#include "aes128.h"
#define u8 unsigned char
#define u16 unsigned short
#define s16 short
#define u32 unsigned long
#define s32 long

#define rand8() (u8)(rand()&0xFF)
#define rand16() (u16)(rand()&0xFFFF)

using namespace std;

static string char_to_hex="0123456789ABCDEF";//未来将md5做转换

u8 UUIDlen=12;
u8 IDlen=14;
u8 situlen=8;
u8 situaes[16];
u8 md5id[16];
u8 md5key[16];
u8 md5len=16;
u8 situaeslen=16;//ResetZigbeeCount大小确定
u8 UARTRecDataBuf[4][48];//暂定
u8 UARTSendDataBuf[32];
bool isRec=false;
u8 reclen=0;
u8 hex_chars[16];
u8 datalen=9;

struct Node{//可以改成链表，建议放到大容量高性能芯片中！！！
  u8 ID[14];
  u8 situ[8];
  u16 addr;
  //u16 SHT10;
  u8 F;//火焰系数
  u8 T[4];//温度//如需进一步处理可以还原为float
  u8 H[4];//湿度
  u8 randnum8;
  u8 randnum8temp;//用来存储待替换的新randnum8
  u16 randnum16;
  u8 time;//定时器
  //bool isimportant;//节点是否处在状态4中//暂不加入！！！
  bool hasalarm;//是否发送了警报解除
  bool needupdate;//用来触发随机数更新
  bool isfree;//是否空闲
  u8 islost;//是否丢失
};
Node node[100];//暂时设置100个节点位置
u8 nodei=0;

void addHead(u8 n, u8 ni)
{
  UARTSendDataBuf[0]=0xAA;      //根据目标节点设置地址
  UARTSendDataBuf[1]=(u8)(node[ni].addr>>8);
  UARTSendDataBuf[2]=(u8)(node[ni].addr & 0x00FF);
  UARTSendDataBuf[3]=0x55;
  
  UARTSendDataBuf[4]=0x00;    //未来可以不带地址，但推荐不要作死！！！
  UARTSendDataBuf[5]=0x00;
  
  if(n == 9)
  {
	srand(time(NULL));
    UARTSendDataBuf[6]=(rand8() & 0xC0) | n;//有两位随机数用来防止换头
  }
  else
    UARTSendDataBuf[6]=n;//消息类别
}

u8 find_node(u16 addr)
{
	for(u8 i=0; i<nodei; i++)
		if(node[i].addr == addr)
			return i;
	
	return (u8)0xFF;
}

void setData(u8 ni, u8 n, u8 j)
{
	node[ni].F=UARTRecDataBuf[n][j];
	for(u8 i=0; i<4; i++)
	{
		node[ni].H[i]=UARTRecDataBuf[n][j + 1 + i];
	}
	for(u8 i=0; i<4; i++)
	{
		node[ni].T[i]=UARTRecDataBuf[n][j + 5 + i];
	}
}

void setSituAes(u8 ni)//凑齐16位用
{
  for(u8 i=0; i<situlen; i++)
    situaes[i]=node[ni].situ[i];
  for(u8 i=situlen; i<situaeslen; i++)
    situaes[i]=(u8)'0';//所有加密'0'填充
}

u8 checkSituaes(u8 ni, u8 s[])
{
	u8 situtemp[8];
	s32 time;//暂定只比较前4位重启时间
	s32 nitime;

	for(u8 i=0; i<situlen; i++)
		situtemp[i]=s[i];

	bool isright=true;
	//以及situ中的固定部分
	for(u8 i=situlen; i < situaeslen; i++)
		if(s[i] != (u8)'0')
			isright=false;

	time=((u32)situtemp[0]<<24) | ((u32)situtemp[1]<<16) | ((u32)situtemp[2]<<8) | (u32)situtemp[3];
	nitime=((u32)node[ni].situ[0]<<24) | ((u32)node[ni].situ[1]<<16) | ((u32)node[ni].situ[2]<<8) | (u32)node[ni].situ[3];

	bool istimeright=false;
	if(((time-nitime) < 144) || ((time < 144) && (nitime > 2636)) || (time < 144))//第三条为防止传感器节点错误重启
		istimeright=true;

	if(isright && istimeright)
	{
		for(u8 i=0; i<situlen; i++)//更新状态信息
			node[ni].situ[i]=s[i];
		return 0;
	}
	if(!isright)
		return 1;
	if(!istimeright)
		return 2;

	//return 0;//为了防止出错可以直接不加状态信息判断，恒return 0即可！！！
}

void R16UUIDDeaes(u8 n, u8 ni, u8 situaes[])//状态信息解密函数，确定程序绝对无误后再开启，建议不开启！！！
{
  for(u8 i=0; i<situaeslen; i++)
	  situaes[i]=UARTRecDataBuf[n][i + 4];
  
  u8 R16UUID[12 + 2];
  R16UUID[0]=(u8)(node[ni].randnum16>>8);
  R16UUID[1]=(u8)(node[ni].randnum16 & 0x00FF);
  for(u8 i=0; i<UUIDlen; i++)
    R16UUID[i + 2]=node[ni].ID[i + 2];
  
  getMD5(R16UUID, UUIDlen + 2, md5key);
  
  deAes(situaes, situaeslen, md5key, md5len);
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

void UART_Send_Data(u8 s[], u8 n)//最终版改为通过zigbee发送，这里方便测试用！！！
{
	for(u8 i=0; i<n; i++)
		printf("%02X", s[i]);

	printf("\n");
}

void do_timing(u8 n, u8 ni)
{
	//srand(time(NULL));
	srand(233);//为了配合arduino设置一个固定种，结合系统改变！！！
  node[ni].randnum16=rand16();
  
  UARTSendDataBuf[7]=(u8)((node[ni].randnum16>>8) & 0x00FF);
  UARTSendDataBuf[8]=(u8)(node[ni].randnum16 & 0x00FF);
  
  u16 R16temp=node[ni].randnum16;
  node[ni].randnum16=changeR16(R16temp);
  
  u8 R16ID[14 + 2];
  
  R16ID[0]=(u8)((node[ni].randnum16>>8) & 0x00FF);
  R16ID[1]=(u8)(node[ni].randnum16 & 0x00FF);
  for(u8 i=0; i<IDlen; i++)
    R16ID[i + 2]=node[ni].ID[i];


  getMD5(R16ID, IDlen + 2, md5id);

  addHead(n, ni);
	  
  if(n == 9)
  {
	  u8 R16ID[14 + 3];
  
	  R16ID[0]==(u8)((node[ni].randnum16>>8) & 0x00FF);
	  R16ID[1]=(u8)(node[ni].randnum16 & 0x00FF);
	  R16ID[2]=UARTSendDataBuf[6];
	  for(u8 i=0; i<IDlen; i++)
		R16ID[i + 3]=node[ni].ID[i];
	  
	  getMD5(R16ID, IDlen + 3, md5id);

	  u8 timeaes[16];//用代码量换变量数量
	  for(u8 i=0; i<4; i++)
		  timeaes[i]=node[ni].situ[i];
	  for(u8 i=4; i<16; i++)
		  timeaes[i]=(u8)'0';//所有加密'0'填充

	  //aes(timeaes, 16, md5id, md5len);

	  for(u8 i=0; i<16; i++)
		UARTSendDataBuf[i + 9]=timeaes[i];
	  
	  UART_Send_Data(UARTSendDataBuf, 9 + 16); 
  }
  else if(n == 11)
  {
	  u8 timeaes[16];//用代码量换变量数量
	  for(u8 i=0; i<4; i++)
		  timeaes[i]=node[ni].situ[i];
	  for(u8 i=4; i<16; i++)
		  timeaes[i]=(u8)'0';//所有加密'0'填充

	  //aes(timeaes, 16, md5id, md5len);

	  for(u8 i=0; i<16; i++)
		UARTSendDataBuf[i + 9]=timeaes[i];
	  
	  UART_Send_Data(UARTSendDataBuf, 9 + 16); 
  }
  else
  {
	  for(u8 i=0; i<md5len; i++)
		UARTSendDataBuf[i + 9]=md5id[i];
	  
	  UART_Send_Data(UARTSendDataBuf, 9 + md5len); 
  }
}

void send_message3(u8 ni)//多条send需要做发送时互斥，或者增加发送缓冲数组！！！
{
  u8 R8ID[14 + 1];
  
  R8ID[0]=node[ni].randnum8;
  for(u8 i=0; i<IDlen; i++)
    R8ID[i + 1]=node[ni].ID[i];
  
  getMD5(R8ID, IDlen + 1,md5id);
  
  addHead(3, ni);
  
  for(u8 i=0; i<md5len; i++)
    UARTSendDataBuf[i + 7]=md5id[i];
  
  UART_Send_Data(UARTSendDataBuf, 7 + md5len); 

  node[ni].needupdate=true;
  
  //此处增加一个线程！！！
  //该部分不加入不影响整体运行，但万一message4出错后系统会错误！！！
//  thread(){
//	  ;//定时器计时，10分钟至1小时均可！！！
//	  if(node[ni].needupdate == true)
//		  send_message9(ni);
//  }
}

void send_message5(u8 ni)//定时更新认证请求
{
  do_timing(5, ni);

  node[ni].islost++;//暂定只在定时更新认证失败时才计一次丢失

  if(node[ni].islost == 12)
  {
	  node[ni].isfree =true;//12次确认节点丢失（即12小时），将节点存储位置释放，可根据实际情况改变时间！！！
	  ;//alarm6();节点丢失报警
  }
}

void send_message7(u8 ni)
{
  do_timing(7, ni);
}

void send_message9(u8 ni)
{
  do_timing(9, ni);
}

void send_message11(u8 ni)
{
  do_timing(11, ni);
  node[ni].hasalarm=true;//该节点进入已发送报警确认请求状态！！！
}

void handle_message0(u8 n)//发送基本信息
{
	printf("收到注册信息！！\n");

	/*
	while(node[nodei].isfree == false)//需要初始设定isfree=true，寻找第一个空的节点存储位置！！！
	{
		nodei++;
	}
	*/

	node[nodei].addr=((u16)UARTRecDataBuf[n][0]<<8) | (UARTRecDataBuf[n][1]);

	//printf("addr:%X\n",node[nodei].addr);

  for(u8 i=0; i<IDlen; i++)
    node[nodei].ID[i]=UARTRecDataBuf[n][i+3];
  
  for(u8 i=0; i<situlen; i++)
    node[nodei].situ[i]=UARTRecDataBuf[n][i + 3 + IDlen];
  
  node[nodei].randnum8=UARTRecDataBuf[n][3 + IDlen + situlen];
  
  node[nodei].time=0;
  //node[nodei].isimportant=false;//暂不加入！！！
  node[nodei].hasalarm=false;
  node[nodei].needupdate=false;
  node[nodei].isfree=false;//该节点长期失联后改为空闲
  node[nodei].islost=0;

  nodei++;
}

void handle_message1(u8 n, u8 ni)//普通数据
{
  setData(ni, n, 3);//转为u8[]型，后续自行处理
  
  //node[ni].isimportant=false;//暂定只要收到一次普通数据就结束message4状态，该方案对节点给予更多信任
  //暂不加入！！！

  /*if((node[ni].time++) == 120)//收到120次数据采集后定时更新，用定时器代替！！！
    send_message5(ni);
	*/
}

void handle_message2(u8 n, u8 ni)//重要数据
{
	printf("收到重要数据认证请求！！\n");
  setData(ni, n, 3);//后续请自行处理
  
  u8 RID[14 + 2];
  u8 RUUID[12 + 2];
  u8 situaes[16];
  u8 situaeslen=16;
  
  RID[0]=node[ni].randnum8;
  RID[1]=UARTRecDataBuf[n][3 + datalen];
  for(u8 i=0; i<IDlen; i++)
    RID[i + 2]=node[ni].ID[i];
  
  RUUID[0]=node[ni].randnum8;
  for(u8 i=0; i<UUIDlen; i++)
    RUUID[i + 1]=node[ni].ID[i + 2];
  
  getMD5(RID, IDlen + 2, md5id);
  
  bool ismd5idright=true;
  for(u8 i=0; i<md5len; i++)//未来改成checkMd5id()
    if(md5id[i] != UARTRecDataBuf[n][i + 3 + datalen + 1])
      ismd5idright=false;
  
  if(ismd5idright)
  {
    getMD5(RUUID, UUIDlen + 1, md5key);
    
    for(u8 i=0; i<situaeslen; i++)
      situaes[i]=UARTRecDataBuf[n][i + 3 + datalen + md5len + 1];
    //deAes(situaes, situaeslen, md5key, md5len);
    
    u8 issituaesright;
    issituaesright=checkSituaes(ni, situaes);
    
    if(issituaesright == 0)
    {
	  printf("重要数据认证请求认证成功！！\n");

      for(u8 i=0; i<situlen; i++)
        node[ni].situ[i]=situaes[i];//前几位有用信息

	  /*if(T < 30)//做重要数据判断是否正确判断，标准需要和传感器节点相同，最终可不加，后续执行已测试
	  {
		  send_message7(ni);
		  printf("发送带数据请求的定时更新认证请求，来认证节点状态！！\n");
		  printf("传感器节点无法正确判断重要信息报警！！\n");//alarm5();
	  }
	  else
	  {*/
		  send_message3(ni);
		  printf("重要数据认证回复！！\n");
	      
		  node[ni].randnum8temp=UARTRecDataBuf[n][3 + datalen];
		  //node[ni].isimportant=true;//=144
		  //暂不加入！！！
	  //}
    }
	else if(issituaesright == 1)
		printf("伪装传感器节点报警！！\n");//alarm1();
    else if(issituaesright == 2)
      printf("传感器节点物理损坏报警！！\n");//alarm3();
  }
  else
    printf("伪装传感器节点报警！！\n");//alarm1();
}

void handle_message4(u8 n, u8 ni)//重要数据双向认证回复
{
  //if((UARTRecDataBuf[n][2]>>6) != (node[ni].randnum8>>6))//和上一个随机数前两位不等时才更新，暂时不做！！！
    node[ni].randnum8=node[ni].randnum8temp;
	//printf("%X\n",node[ni].randnum8);
	printf("收到重要数据双向认证随机数同步帧！！\n");
  
  //if(node[ni].isimportant)//该if判断用来做非重要信息间隔时发送消息4报警，为避免不必要错误暂时不加入！！！
  {
    setData(ni, n, 3);
	node[ni].needupdate=false;
    
    /*if((node[ni].time++) == 120)//收到120次数据采集后定时更新，最终版用定时器代替
    send_message5(ni);*/
  }
  //else
    ;//alarm1();
}

//考虑合并下面handle_message6~10
void handle_message6(u8 n, u8 ni)//定时更新2
{
	printf("收到定时更新认证回复！！\n");
	u8 situaes[16];
  //R16UUIDDeaes(n, ni, situaes);
  //此处应有deAes失败后的直接判断，同issituaesright=1

	for(u8 i=0; i<situaeslen; i++)
		situaes[i]=UARTRecDataBuf[n][i+3];
  
  u8 issituaesright;
  issituaesright=checkSituaes(ni, situaes);
  
  if(issituaesright == 0)
  {
    for(u8 i=0; i<situlen; i++)
      node[ni].situ[i]=situaes[i];//前几位有用信息
	printf("定时更新认证成功！！\n");

	node[ni].islost=0;
  }
  else if(issituaesright == 1)
	printf("伪装传感器节点报警！！\n");//alarm1();
  else if(issituaesright == 2)
  {
    printf("传感器节点物理损坏报警！！\n");//alarm3();
	node[ni].islost=0;
  }
}

void handle_message8(u8 n, u8 ni)//带数据请求的定时更新2
{
  u8 situaes[16];
  //R16UUIDDeaes(n, ni, situaes);
  for(u8 i=0; i<situaeslen; i++)
  {
	  situaes[i]=UARTRecDataBuf[n][i+5];//多两位数据
  }
  
  u8 issituaesright;
  issituaesright=checkSituaes(ni, situaes);
  
  if(issituaesright == 0)
  {
    for(u8 i=0; i<situlen; i++)
      node[ni].situ[i]=situaes[i];//前几位有用信息
    
    int t=(s16)node[nodei].T[4] - (s16)UARTRecDataBuf[n][11];
    if(t>-2 && t<2)//允许短时间内有两度误差
      ;//alarm5();
    else
	{
	  printf("终止传感器节点无法正确判断重要数据报警！！\n");//stopAlarm5();
      printf("伪装传感器节点报警！！\n");//alarm1();
	}
  }
  else if(issituaesright == 1)
	;//alarm1();
  else if(issituaesright == 2)
    ;//alarm3();
}

void handle_message10(u8 n, u8 ni)//带随机数更新请求的定时更新2
{
  u8 situaes[16];
  //R16UUIDDeaes(n, ni, situaes);

  for(u8 i=0; i<situaeslen; i++)
  {
	  situaes[i]=UARTRecDataBuf[n][i+3];
  }
  
  u8 issituaesright;
  issituaesright=checkSituaes(ni, situaes);
  
  if(issituaesright == 0)
  {
    printf("随机数更新请求认证成功！！\n");
    for(u8 i=0; i<situlen; i++)
      node[ni].situ[i]=situaes[i];//前几位有用信息
    
    node[ni].randnum8=((node[ni].randnum16 & 0x0F00)>>4) | (node[ni].randnum16 & 0x000F);//已change
  }
  else if(issituaesright == 1)
	printf("随机数更新伪装节点报警！！\n");//alarm1();
  else if(issituaesright == 2)
	printf("随机数更新节点物理损坏报警！！\n");//alarm3();
}

void handle_message12(u8 n, u8 ni)//带报警接收的定时更新2
{
	printf("收到报警确认！！\n");

  if(node[ni].hasalarm)
  {
    u8 situaes[16];
	//R16UUIDDeaes(n, ni, situaes);

	for(u8 i=0; i<situaeslen; i++)
	{
	  situaes[i]=UARTRecDataBuf[n][i+3];
	}
    
    bool issituaesright;
    issituaesright=checkSituaes(ni, situaes);
    
    if(issituaesright == 0)
    {
      for(u8 i=0; i<situlen; i++)
        node[ni].situ[i]=situaes[i];//前几位有用信息
      
      node[ni].hasalarm=false;
    }
    else if(issituaesright == 1)
	  ;//alarm1();
	else if(issituaesright == 2)
	  ;//alarm3();
  }
  else
    ;//alarm1();
}

void handle_message13(u8 n, u8 ni)//定时更新时间失效
{
  u8 situaes[16];
  R16UUIDDeaes(n, ni, situaes);
  
  bool issituaesright;
  issituaesright=checkSituaes(ni, situaes);
  
  if(issituaesright)
  {
    for(u8 i=0; i<situlen; i++)
      node[ni].situ[i]=situaes[i];//前几位有用信息
  }
  else if(issituaesright == 1)
  {
	;//alarm1();
	;//alarm2();
  }
  else if(issituaesright == 2)
    ;//alarm3();

  if(node[ni].hasalarm)//重发更新时间戳的重要定时更新请求
    send_message11(ni);
  else
    send_message9(ni);
}

void handle_message14(u8 ni)//报警解除错误
{
  ;//alarm1(ni);
  ;//alarm2();
}

void handle_message15(u8 ni)//伪装基站报警
{
  ;//alarm1(ni);
  ;//alarm2();
  send_message11(ni);
}

void handle_message16(u8 ni)//紧急报警
{
  ;//isalarm9(ni);
}

void ifRec(u8 n)
{
  //while(USART_GetFlagStatus(USART_FLAG_RXNE) == SET)
  //{
  //  UARTRecDataBuf[i]=USART_ReceiveData8();
  //  reclen++;
  //  isRec=true;
  //}
  isRec=true;
  //判断消息类型
  //0~1发送方地址，2消息类型
  if(isRec)
  {
	if(UARTRecDataBuf[n][2] == (u8)0x00)//未来可以在这附近加入接入认证
		handle_message0(n);

	u8 ni=find_node(((u16)UARTRecDataBuf[n][0]<<8) | (u16)UARTRecDataBuf[n][1]);

	if(ni != (u8)0xFF)//地址非法判断
	{  
		if(UARTRecDataBuf[n][2] == (u8)0x01)
		  handle_message1(n, ni);
		else if(UARTRecDataBuf[n][2] == (u8)0x02)
		  handle_message2(n, ni);
		else if((UARTRecDataBuf[n][2] & (u8)0x3F) == (u8)0x04)
		  handle_message4(n, ni);
		else if(UARTRecDataBuf[n][2] == (u8)0x06)
		  handle_message6(n, ni);
		else if(UARTRecDataBuf[n][2] == (u8)0x08)
		  handle_message8(n, ni);
		else if(UARTRecDataBuf[n][2] == (u8)0x0A)
		  handle_message10(n, ni);
		else if(UARTRecDataBuf[n][2] == (u8)0x0C)
		  handle_message12(n, ni);
		else if(UARTRecDataBuf[n][2] == (u8)0x0D)
		  handle_message13(n, ni);
		else if(UARTRecDataBuf[n][2] == (u8)0x0E)
		  handle_message14(ni);
		else if(UARTRecDataBuf[n][2] == (u8)0x0F)
		  handle_message15(ni);
		else if(UARTRecDataBuf[n][2] == (u8)0x10)
		  handle_message16(ni);
	}
  }
  else
  {
	;//异常消息
  }
}

int main(){//测试用主函数
	//while(1)
	//{
		string s;
		getline(cin, s);        //?????
		printf("收到！！\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);
		//0002004130000000000000000000000001000000013030303050


		getline(cin, s);//????
		printf("收到！！\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);
		//00020201000004420000F041D79EB8470CA1960EE92A16F4B5A35D1EFF00000001303030303030303030303030

		getline(cin, s);
		printf("收到！！\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);
		//0002C401000014420000F041
		//重要数据双向认证随机数更新同步帧（之后144次数据采集发送皆为这个，既不是普通数据也不是重要数据，
		//如果想改重要数据双向认证间隔及定时更新认证间隔与普通数据采集间隔比例至少需要修改这个！！！显然需要慢慢改程序）

		//至此重要数据双向认证正确过程测试通过

		//开始测试定时更新认证

		printf("发送定时更新认证请求！！\n");
		send_message5(0);//开始定时更新认证
		//AA000255000005031FBEAD6E22C19DFBD5801425DAD118E2B9

		getline(cin, s);
		printf("收到！！\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);//定时更新认证回复
		//00020600000003303030303030303030303030

		//至此定时更新认证正确过程测试通过

		//暂时不测试带数据请求的定时更新认证，因为非完全版不加入该过程，要觉得绝对没问题再开启并测试！！！
		//之前的测试结果显示应该没有错误

		//开始测试随机数更新认证

		printf("发送带随机数更新请求的定时更新认证请求！！\n");
		send_message9(0);//开始随机数更新
		//AA0002550000C9031F00000000303030303030303030303030

		getline(cin, s);
		printf("收到！！\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);//随机数更新请求回复
		//00020A00000003303030303030303030303030
		//printf("%X\n",node[0].randnum16);//0x1F03
		//printf("%X\n",node[0].randnum8);//0xF3

		//经验证随机数已成功同步更新
		//至此随机数更新认证正确过程测试通过

		/*
		for(u8 i=0; i<situlen; i++)
			printf("%X",node[0].situ[i]);
		*/
		//经测试状态信息更新运行正常

		//开始测试报警确认认证

		//00020201000004420000F041D79EB8470CA1960EE92A16F4B5A35D1EFF00000001303030303030303030303030
		//攻击者重放重要数据认证回复，协调器端不用动，发给传感器即可

		getline(cin, s);
		printf("收到！！\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);//收到报警
		//00020F01000034420000F841

		//已验证报警连续发送

		//发送报警确认请求
		//AA00025500000B031F00000003303030303030303030303030

		getline(cin, s);
		printf("收到！！\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);//收到报警确认
		//00020C00000003303030303030303030303030

		//发送AA025500000D接受数据
		//数据为04类型而非报警0F类型，已验证报警解除成功

		//至此报警确认正确过程测试通过

		//至此所有主要功能测试结束！

		getline(cin, s);

	return 0;
}

/*
int main(){//运行用主函数，伪代码

	while(1){
		ifRec(nodei);//只用来接收不定时的注册消息
		
		if(nodei == 101)//达到节点数量上限，节点数量上限结合项目要求自行变化！！！
			；//nodei从0开始寻找isfree的节点位置

		；//检测每个节点定时器1，到5分钟后发送采集数据消息

		；//检测每个节点定时器2，到1个小时后发送定时更新认证消息

		Sleep(100);//睡眠时间标准以能必定接收到注册消息为准！！！
	}
}
*/