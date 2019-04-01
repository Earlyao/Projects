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

static string char_to_hex="0123456789ABCDEF";//δ����md5��ת��

u8 UUIDlen=12;
u8 IDlen=14;
u8 situlen=8;
u8 situaes[16];
u8 md5id[16];
u8 md5key[16];
u8 md5len=16;
u8 situaeslen=16;//ResetZigbeeCount��Сȷ��
u8 UARTRecDataBuf[4][48];//�ݶ�
u8 UARTSendDataBuf[32];
bool isRec=false;
u8 reclen=0;
u8 hex_chars[16];
u8 datalen=9;

struct Node{//���Ըĳ���������ŵ�������������оƬ�У�����
  u8 ID[14];
  u8 situ[8];
  u16 addr;
  //u16 SHT10;
  u8 F;//����ϵ��
  u8 T[4];//�¶�//�����һ��������Ի�ԭΪfloat
  u8 H[4];//ʪ��
  u8 randnum8;
  u8 randnum8temp;//�����洢���滻����randnum8
  u16 randnum16;
  u8 time;//��ʱ��
  //bool isimportant;//�ڵ��Ƿ���״̬4��//�ݲ����룡����
  bool hasalarm;//�Ƿ����˾������
  bool needupdate;//�����������������
  bool isfree;//�Ƿ����
  u8 islost;//�Ƿ�ʧ
};
Node node[100];//��ʱ����100���ڵ�λ��
u8 nodei=0;

void addHead(u8 n, u8 ni)
{
  UARTSendDataBuf[0]=0xAA;      //����Ŀ��ڵ����õ�ַ
  UARTSendDataBuf[1]=(u8)(node[ni].addr>>8);
  UARTSendDataBuf[2]=(u8)(node[ni].addr & 0x00FF);
  UARTSendDataBuf[3]=0x55;
  
  UARTSendDataBuf[4]=0x00;    //δ�����Բ�����ַ�����Ƽ���Ҫ����������
  UARTSendDataBuf[5]=0x00;
  
  if(n == 9)
  {
	srand(time(NULL));
    UARTSendDataBuf[6]=(rand8() & 0xC0) | n;//����λ�����������ֹ��ͷ
  }
  else
    UARTSendDataBuf[6]=n;//��Ϣ���
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

void setSituAes(u8 ni)//����16λ��
{
  for(u8 i=0; i<situlen; i++)
    situaes[i]=node[ni].situ[i];
  for(u8 i=situlen; i<situaeslen; i++)
    situaes[i]=(u8)'0';//���м���'0'���
}

u8 checkSituaes(u8 ni, u8 s[])
{
	u8 situtemp[8];
	s32 time;//�ݶ�ֻ�Ƚ�ǰ4λ����ʱ��
	s32 nitime;

	for(u8 i=0; i<situlen; i++)
		situtemp[i]=s[i];

	bool isright=true;
	//�Լ�situ�еĹ̶�����
	for(u8 i=situlen; i < situaeslen; i++)
		if(s[i] != (u8)'0')
			isright=false;

	time=((u32)situtemp[0]<<24) | ((u32)situtemp[1]<<16) | ((u32)situtemp[2]<<8) | (u32)situtemp[3];
	nitime=((u32)node[ni].situ[0]<<24) | ((u32)node[ni].situ[1]<<16) | ((u32)node[ni].situ[2]<<8) | (u32)node[ni].situ[3];

	bool istimeright=false;
	if(((time-nitime) < 144) || ((time < 144) && (nitime > 2636)) || (time < 144))//������Ϊ��ֹ�������ڵ��������
		istimeright=true;

	if(isright && istimeright)
	{
		for(u8 i=0; i<situlen; i++)//����״̬��Ϣ
			node[ni].situ[i]=s[i];
		return 0;
	}
	if(!isright)
		return 1;
	if(!istimeright)
		return 2;

	//return 0;//Ϊ�˷�ֹ�������ֱ�Ӳ���״̬��Ϣ�жϣ���return 0���ɣ�����
}

void R16UUIDDeaes(u8 n, u8 ni, u8 situaes[])//״̬��Ϣ���ܺ�����ȷ���������������ٿ��������鲻����������
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

u16 changeR16(u16 u)//�ݶ���תǰ���u8
{
  u16 tmp=u;
  u8 u1,u2;
  u1=(u8)((tmp>>8) & 0x00FF);
  u2=(u8)(tmp & 0x00FF);
  u=((u16)u2)<<8 | (u16)u1;
  return u;
}

void UART_Send_Data(u8 s[], u8 n)//���հ��Ϊͨ��zigbee���ͣ����﷽������ã�����
{
	for(u8 i=0; i<n; i++)
		printf("%02X", s[i]);

	printf("\n");
}

void do_timing(u8 n, u8 ni)
{
	//srand(time(NULL));
	srand(233);//Ϊ�����arduino����һ���̶��֣����ϵͳ�ı䣡����
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

	  u8 timeaes[16];//�ô���������������
	  for(u8 i=0; i<4; i++)
		  timeaes[i]=node[ni].situ[i];
	  for(u8 i=4; i<16; i++)
		  timeaes[i]=(u8)'0';//���м���'0'���

	  //aes(timeaes, 16, md5id, md5len);

	  for(u8 i=0; i<16; i++)
		UARTSendDataBuf[i + 9]=timeaes[i];
	  
	  UART_Send_Data(UARTSendDataBuf, 9 + 16); 
  }
  else if(n == 11)
  {
	  u8 timeaes[16];//�ô���������������
	  for(u8 i=0; i<4; i++)
		  timeaes[i]=node[ni].situ[i];
	  for(u8 i=4; i<16; i++)
		  timeaes[i]=(u8)'0';//���м���'0'���

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

void send_message3(u8 ni)//����send��Ҫ������ʱ���⣬�������ӷ��ͻ������飡����
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
  
  //�˴�����һ���̣߳�����
  //�ò��ֲ����벻Ӱ���������У�����һmessage4�����ϵͳ����󣡣���
//  thread(){
//	  ;//��ʱ����ʱ��10������1Сʱ���ɣ�����
//	  if(node[ni].needupdate == true)
//		  send_message9(ni);
//  }
}

void send_message5(u8 ni)//��ʱ������֤����
{
  do_timing(5, ni);

  node[ni].islost++;//�ݶ�ֻ�ڶ�ʱ������֤ʧ��ʱ�ż�һ�ζ�ʧ

  if(node[ni].islost == 12)
  {
	  node[ni].isfree =true;//12��ȷ�Ͻڵ㶪ʧ����12Сʱ�������ڵ�洢λ���ͷţ��ɸ���ʵ������ı�ʱ�䣡����
	  ;//alarm6();�ڵ㶪ʧ����
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
  node[ni].hasalarm=true;//�ýڵ�����ѷ��ͱ���ȷ������״̬������
}

void handle_message0(u8 n)//���ͻ�����Ϣ
{
	printf("�յ�ע����Ϣ����\n");

	/*
	while(node[nodei].isfree == false)//��Ҫ��ʼ�趨isfree=true��Ѱ�ҵ�һ���յĽڵ�洢λ�ã�����
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
  //node[nodei].isimportant=false;//�ݲ����룡����
  node[nodei].hasalarm=false;
  node[nodei].needupdate=false;
  node[nodei].isfree=false;//�ýڵ㳤��ʧ�����Ϊ����
  node[nodei].islost=0;

  nodei++;
}

void handle_message1(u8 n, u8 ni)//��ͨ����
{
  setData(ni, n, 3);//תΪu8[]�ͣ��������д���
  
  //node[ni].isimportant=false;//�ݶ�ֻҪ�յ�һ����ͨ���ݾͽ���message4״̬���÷����Խڵ�����������
  //�ݲ����룡����

  /*if((node[ni].time++) == 120)//�յ�120�����ݲɼ���ʱ���£��ö�ʱ�����棡����
    send_message5(ni);
	*/
}

void handle_message2(u8 n, u8 ni)//��Ҫ����
{
	printf("�յ���Ҫ������֤���󣡣�\n");
  setData(ni, n, 3);//���������д���
  
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
  for(u8 i=0; i<md5len; i++)//δ���ĳ�checkMd5id()
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
	  printf("��Ҫ������֤������֤�ɹ�����\n");

      for(u8 i=0; i<situlen; i++)
        node[ni].situ[i]=situaes[i];//ǰ��λ������Ϣ

	  /*if(T < 30)//����Ҫ�����ж��Ƿ���ȷ�жϣ���׼��Ҫ�ʹ������ڵ���ͬ�����տɲ��ӣ�����ִ���Ѳ���
	  {
		  send_message7(ni);
		  printf("���ʹ���������Ķ�ʱ������֤��������֤�ڵ�״̬����\n");
		  printf("�������ڵ��޷���ȷ�ж���Ҫ��Ϣ��������\n");//alarm5();
	  }
	  else
	  {*/
		  send_message3(ni);
		  printf("��Ҫ������֤�ظ�����\n");
	      
		  node[ni].randnum8temp=UARTRecDataBuf[n][3 + datalen];
		  //node[ni].isimportant=true;//=144
		  //�ݲ����룡����
	  //}
    }
	else if(issituaesright == 1)
		printf("αװ�������ڵ㱨������\n");//alarm1();
    else if(issituaesright == 2)
      printf("�������ڵ������𻵱�������\n");//alarm3();
  }
  else
    printf("αװ�������ڵ㱨������\n");//alarm1();
}

void handle_message4(u8 n, u8 ni)//��Ҫ����˫����֤�ظ�
{
  //if((UARTRecDataBuf[n][2]>>6) != (node[ni].randnum8>>6))//����һ�������ǰ��λ����ʱ�Ÿ��£���ʱ����������
    node[ni].randnum8=node[ni].randnum8temp;
	//printf("%X\n",node[ni].randnum8);
	printf("�յ���Ҫ����˫����֤�����ͬ��֡����\n");
  
  //if(node[ni].isimportant)//��if�ж�����������Ҫ��Ϣ���ʱ������Ϣ4������Ϊ���ⲻ��Ҫ������ʱ�����룡����
  {
    setData(ni, n, 3);
	node[ni].needupdate=false;
    
    /*if((node[ni].time++) == 120)//�յ�120�����ݲɼ���ʱ���£����հ��ö�ʱ������
    send_message5(ni);*/
  }
  //else
    ;//alarm1();
}

//���Ǻϲ�����handle_message6~10
void handle_message6(u8 n, u8 ni)//��ʱ����2
{
	printf("�յ���ʱ������֤�ظ�����\n");
	u8 situaes[16];
  //R16UUIDDeaes(n, ni, situaes);
  //�˴�Ӧ��deAesʧ�ܺ��ֱ���жϣ�ͬissituaesright=1

	for(u8 i=0; i<situaeslen; i++)
		situaes[i]=UARTRecDataBuf[n][i+3];
  
  u8 issituaesright;
  issituaesright=checkSituaes(ni, situaes);
  
  if(issituaesright == 0)
  {
    for(u8 i=0; i<situlen; i++)
      node[ni].situ[i]=situaes[i];//ǰ��λ������Ϣ
	printf("��ʱ������֤�ɹ�����\n");

	node[ni].islost=0;
  }
  else if(issituaesright == 1)
	printf("αװ�������ڵ㱨������\n");//alarm1();
  else if(issituaesright == 2)
  {
    printf("�������ڵ������𻵱�������\n");//alarm3();
	node[ni].islost=0;
  }
}

void handle_message8(u8 n, u8 ni)//����������Ķ�ʱ����2
{
  u8 situaes[16];
  //R16UUIDDeaes(n, ni, situaes);
  for(u8 i=0; i<situaeslen; i++)
  {
	  situaes[i]=UARTRecDataBuf[n][i+5];//����λ����
  }
  
  u8 issituaesright;
  issituaesright=checkSituaes(ni, situaes);
  
  if(issituaesright == 0)
  {
    for(u8 i=0; i<situlen; i++)
      node[ni].situ[i]=situaes[i];//ǰ��λ������Ϣ
    
    int t=(s16)node[nodei].T[4] - (s16)UARTRecDataBuf[n][11];
    if(t>-2 && t<2)//�����ʱ�������������
      ;//alarm5();
    else
	{
	  printf("��ֹ�������ڵ��޷���ȷ�ж���Ҫ���ݱ�������\n");//stopAlarm5();
      printf("αװ�������ڵ㱨������\n");//alarm1();
	}
  }
  else if(issituaesright == 1)
	;//alarm1();
  else if(issituaesright == 2)
    ;//alarm3();
}

void handle_message10(u8 n, u8 ni)//���������������Ķ�ʱ����2
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
    printf("���������������֤�ɹ�����\n");
    for(u8 i=0; i<situlen; i++)
      node[ni].situ[i]=situaes[i];//ǰ��λ������Ϣ
    
    node[ni].randnum8=((node[ni].randnum16 & 0x0F00)>>4) | (node[ni].randnum16 & 0x000F);//��change
  }
  else if(issituaesright == 1)
	printf("���������αװ�ڵ㱨������\n");//alarm1();
  else if(issituaesright == 2)
	printf("��������½ڵ������𻵱�������\n");//alarm3();
}

void handle_message12(u8 n, u8 ni)//���������յĶ�ʱ����2
{
	printf("�յ�����ȷ�ϣ���\n");

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
        node[ni].situ[i]=situaes[i];//ǰ��λ������Ϣ
      
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

void handle_message13(u8 n, u8 ni)//��ʱ����ʱ��ʧЧ
{
  u8 situaes[16];
  R16UUIDDeaes(n, ni, situaes);
  
  bool issituaesright;
  issituaesright=checkSituaes(ni, situaes);
  
  if(issituaesright)
  {
    for(u8 i=0; i<situlen; i++)
      node[ni].situ[i]=situaes[i];//ǰ��λ������Ϣ
  }
  else if(issituaesright == 1)
  {
	;//alarm1();
	;//alarm2();
  }
  else if(issituaesright == 2)
    ;//alarm3();

  if(node[ni].hasalarm)//�ط�����ʱ�������Ҫ��ʱ��������
    send_message11(ni);
  else
    send_message9(ni);
}

void handle_message14(u8 ni)//�����������
{
  ;//alarm1(ni);
  ;//alarm2();
}

void handle_message15(u8 ni)//αװ��վ����
{
  ;//alarm1(ni);
  ;//alarm2();
  send_message11(ni);
}

void handle_message16(u8 ni)//��������
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
  //�ж���Ϣ����
  //0~1���ͷ���ַ��2��Ϣ����
  if(isRec)
  {
	if(UARTRecDataBuf[n][2] == (u8)0x00)//δ���������⸽�����������֤
		handle_message0(n);

	u8 ni=find_node(((u16)UARTRecDataBuf[n][0]<<8) | (u16)UARTRecDataBuf[n][1]);

	if(ni != (u8)0xFF)//��ַ�Ƿ��ж�
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
	;//�쳣��Ϣ
  }
}

int main(){//������������
	//while(1)
	//{
		string s;
		getline(cin, s);        //?????
		printf("�յ�����\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);
		//0002004130000000000000000000000001000000013030303050


		getline(cin, s);//????
		printf("�յ�����\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);
		//00020201000004420000F041D79EB8470CA1960EE92A16F4B5A35D1EFF00000001303030303030303030303030

		getline(cin, s);
		printf("�յ�����\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);
		//0002C401000014420000F041
		//��Ҫ����˫����֤���������ͬ��֡��֮��144�����ݲɼ����ͽ�Ϊ������Ȳ�����ͨ����Ҳ������Ҫ���ݣ�
		//��������Ҫ����˫����֤�������ʱ������֤�������ͨ���ݲɼ��������������Ҫ�޸������������Ȼ��Ҫ�����ĳ���

		//������Ҫ����˫����֤��ȷ���̲���ͨ��

		//��ʼ���Զ�ʱ������֤

		printf("���Ͷ�ʱ������֤���󣡣�\n");
		send_message5(0);//��ʼ��ʱ������֤
		//AA000255000005031FBEAD6E22C19DFBD5801425DAD118E2B9

		getline(cin, s);
		printf("�յ�����\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);//��ʱ������֤�ظ�
		//00020600000003303030303030303030303030

		//���˶�ʱ������֤��ȷ���̲���ͨ��

		//��ʱ�����Դ���������Ķ�ʱ������֤����Ϊ����ȫ�治����ù��̣�Ҫ���þ���û�����ٿ��������ԣ�����
		//֮ǰ�Ĳ��Խ����ʾӦ��û�д���

		//��ʼ���������������֤

		printf("���ʹ��������������Ķ�ʱ������֤���󣡣�\n");
		send_message9(0);//��ʼ���������
		//AA0002550000C9031F00000000303030303030303030303030

		getline(cin, s);
		printf("�յ�����\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);//�������������ظ�
		//00020A00000003303030303030303030303030
		//printf("%X\n",node[0].randnum16);//0x1F03
		//printf("%X\n",node[0].randnum8);//0xF3

		//����֤������ѳɹ�ͬ������
		//���������������֤��ȷ���̲���ͨ��

		/*
		for(u8 i=0; i<situlen; i++)
			printf("%X",node[0].situ[i]);
		*/
		//������״̬��Ϣ������������

		//��ʼ���Ա���ȷ����֤

		//00020201000004420000F041D79EB8470CA1960EE92A16F4B5A35D1EFF00000001303030303030303030303030
		//�������ط���Ҫ������֤�ظ���Э�����˲��ö�����������������

		getline(cin, s);
		printf("�յ�����\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);//�յ�����
		//00020F01000034420000F841

		//����֤������������

		//���ͱ���ȷ������
		//AA00025500000B031F00000003303030303030303030303030

		getline(cin, s);
		printf("�յ�����\n");
		for(u8 i=0; i<s.length()/2; i++)
		{
			UARTRecDataBuf[0][i]=(((u8)char_to_hex.find(s[2*i]))<<4) | ((u8)char_to_hex.find(s[2*i+1]));
		}
		ifRec(0);
		Sleep(1000);//�յ�����ȷ��
		//00020C00000003303030303030303030303030

		//����AA025500000D��������
		//����Ϊ04���Ͷ��Ǳ���0F���ͣ�����֤��������ɹ�

		//���˱���ȷ����ȷ���̲���ͨ��

		//����������Ҫ���ܲ��Խ�����

		getline(cin, s);

	return 0;
}

/*
int main(){//��������������α����

	while(1){
		ifRec(nodei);//ֻ�������ղ���ʱ��ע����Ϣ
		
		if(nodei == 101)//�ﵽ�ڵ��������ޣ��ڵ��������޽����ĿҪ�����б仯������
			��//nodei��0��ʼѰ��isfree�Ľڵ�λ��

		��//���ÿ���ڵ㶨ʱ��1����5���Ӻ��Ͳɼ�������Ϣ

		��//���ÿ���ڵ㶨ʱ��2����1��Сʱ���Ͷ�ʱ������֤��Ϣ

		Sleep(100);//˯��ʱ���׼���ܱض����յ�ע����ϢΪ׼������
	}
}
*/