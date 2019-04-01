#include<iostream>
#include "typedefs.h"

#define shift(x, n) (((x) << (n)) | ((x) >> (32-(n))))//���Ƶ�ʱ�򣬸�λһ��Ҫ���㣬�����ǲ������λ
														//����Ϊѭ������

#define FF(a ,b ,c ,d ,Mj ,s ,t ,i ,j) (a = b + ( (a + F(b,c,d) + M[j] + t[i]) << s[i]))//�Ĳ���������������Ҫ��
#define GG(a ,b ,c ,d ,Mj ,s ,t , i, j) (a = b + ( (a + G(b,c,d) + M[j] + t[i]) << s[i]))
#define HH(a ,b ,c ,d ,Mj ,s ,t, i, j) (a = b + ( (a + H(b,c,d) + M[j] + t[i]) << s[i]))
#define II(a ,b ,c ,d ,Mj ,s ,t, i, j) (a = b + ( (a + I(b,c,d) + M[j] + t[i]) << s[i]))

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))//�ĸ������Ժ���
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define A 0x67452301//��С�˱仯��
#define B 0xefcdab89
#define C 0x98badcfe
#define D 0x10325476

//strBaye�ĳ���
unsigned int strlength;
//A,B,C,D����ʱ����
unsigned int atemp;
unsigned int btemp;
unsigned int ctemp;
unsigned int dtemp;

//����ti unsigned int(abs(sin(i+1))*(2pow32))

const unsigned int t[]={//ti��������
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

const unsigned int s[]={7,12,17,22,7,12,17,22,7,12,17,22,7,//��λ����
        12,17,22,5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
        4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,6,10,
        15,21,6,10,15,21,6,10,15,21,6,10,15,21};

const char str16[]="0123456789abcdef";

/*
*��亯��
*�����Ӧ����bits��448(mod512),�ֽھ���bytes��56��mode64)
*��䷽ʽΪ�ȼ�һ��1,����λ����
*������64λ��ԭ������
*/
unsigned int* add(uint_8 *s, uint_16 len)
{
    unsigned int num=((len+8)/64)+1;//��512λ,64���ֽ�Ϊһ��
    unsigned int *strByte=new unsigned int[num*16];    //64/4=16,������16������
    strlength=num*16;
    for (unsigned int i = 0; i < num*16; i++)
        strByte[i]=0;
    for (unsigned int i=0; i <len; i++)
    {
        strByte[i>>2]|=(s[i])<<((i%4)*8);//һ�������洢�ĸ��ֽڣ�i>>2��ʾi/4 һ��unsigned int��Ӧ4���ֽڣ�����4���ַ���Ϣ
    }
    strByte[len>>2]|=(0x80)<<(((len%4))*8);//β�����1 һ��unsigned int����4���ַ���Ϣ,������128����
    /*
    *���ԭ���ȣ�����ָλ�ĳ��ȣ�����Ҫ��8��Ȼ����С�������Է��ڵ����ڶ���,���ﳤ��ֻ����32λ
    */
    strByte[num*16-2]=len*8;
    return strByte;
}

void mainLoop(unsigned int M[])
{
    unsigned int f,j;
    unsigned int a=atemp;
    unsigned int b=btemp;
    unsigned int c=ctemp;
    unsigned int d=dtemp;
    for (unsigned int i = 0; i < 64; i++)
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
		unsigned int tmp=a;
		a=d;//��һ�ֵ�aΪ��һ�ֵ�d
		d=c;
		c=b;
		b=tmp;
    }

    atemp=a+atemp;//���մ���
    btemp=b+btemp;
    ctemp=c+ctemp;
    dtemp=d+dtemp;

}

/*
uint_8* changeHex(int a)//�仯Ϊstring
{
    int b;
    uint_8 s[8];
	uint_8 s1[2];
    for(int i=0;i<4;i++)
    {
        b=((a>>i*8)%(1<<8))&0xff;//����intÿ�����Ƶ�����λ�ú�ȡ���ֽ�
        for (int j = 0; j < 2; j++)
        {
            s1[1-j]=(uint_8)str16[b%16];//str16Ϊӳ���
            b=b/16;
        }
        s[i*2]=s1[0];
		s[i*2+1]=s1[1];
    }
    return s;
}

void getMD5(uint_8 *source,uint_16 length,uint_8 *md5)
{
    atemp=A;//��ʼ��
    btemp=B;
    ctemp=C;
    dtemp=D;

    unsigned int *strByte=add(source, length);//���������ݽ�����䴦��

    for(unsigned int i=0;i<strlength/16;i++)//ÿ16λ�ָ�
    {
        unsigned int num[16];
        for(unsigned int j=0;j<16;j++)
            num[j]=strByte[i*16+j];
        mainLoop(num);//numֻ�Ǹ��������������ݣ������ı����ȫ�ֱ���a(bcd)temp
    }

	unsigned int t[]={atemp,btemp,ctemp,dtemp};

	for(int i=0; i<4; i++)
		for(int j=0; j<8; j++)
			md5[i*8 + j]=changeHex(t[i])[j];//��С�˱仯��

    //return 0;
}
*/

uint_32 new_changeHex(uint_32 a)
{
    uint_32 b;
    b=(a>>24)|((a&0x00FF0000)>>8)|((a&0x0000FF00)<<8)|(a<<24);
    return b;
}

void getMD5(uint_8 *source,uint_8 length,uint_8 *md5)
{
    atemp=A;//��ʼ��
    btemp=B;
    ctemp=C;
    dtemp=D;

    uint_32 *strByte=add(source, length);//���������ݽ�����䴦��

    for(uint_8 i=0;i<strlength/16;i++)//ÿ16λ�ָ�
    {
        uint_32 num[16];
        for(uint_8 j=0;j<16;j++)
		{
            num[j]=strByte[i*16+j];
		}
        mainLoop(num);//numֻ�Ǹ��������������ݣ������ı����ȫ�ֱ���a(bcd)temp
    }

    uint_32 t[]={atemp,btemp,ctemp,dtemp};

    for(uint_8 i=0; i<4; i++)
    {
      uint_32 a=new_changeHex(t[i]);
      for(uint_8 j=0; j<4; j++)
      {
        md5[i*4 + j]=(uint_8)(a>>(8*(3-j))&0x000000FF);//��С�˱仯��
      }
    }
    //return 0;
}