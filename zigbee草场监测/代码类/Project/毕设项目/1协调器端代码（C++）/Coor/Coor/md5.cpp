#include<iostream>
#include "typedefs.h"

#define shift(x, n) (((x) << (n)) | ((x) >> (32-(n))))//右移的时候，高位一定要补零，而不是补充符号位
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
unsigned int strlength;
//A,B,C,D的临时变量
unsigned int atemp;
unsigned int btemp;
unsigned int ctemp;
unsigned int dtemp;

//常量ti unsigned int(abs(sin(i+1))*(2pow32))

const unsigned int t[]={//ti常量数组
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

const unsigned int s[]={7,12,17,22,7,12,17,22,7,12,17,22,7,//左位移数
        12,17,22,5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
        4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,6,10,
        15,21,6,10,15,21,6,10,15,21,6,10,15,21};

const char str16[]="0123456789abcdef";

/*
*填充函数
*处理后应满足bits≡448(mod512),字节就是bytes≡56（mode64)
*填充方式为先加一个1,其它位补零
*最后加上64位的原来长度
*/
unsigned int* add(uint_8 *s, uint_16 len)
{
    unsigned int num=((len+8)/64)+1;//以512位,64个字节为一组
    unsigned int *strByte=new unsigned int[num*16];    //64/4=16,所以有16个整数
    strlength=num*16;
    for (unsigned int i = 0; i < num*16; i++)
        strByte[i]=0;
    for (unsigned int i=0; i <len; i++)
    {
        strByte[i>>2]|=(s[i])<<((i%4)*8);//一个整数存储四个字节，i>>2表示i/4 一个unsigned int对应4个字节，保存4个字符信息
    }
    strByte[len>>2]|=(0x80)<<(((len%4))*8);//尾部添加1 一个unsigned int保存4个字符信息,所以用128左移
    /*
    *添加原长度，长度指位的长度，所以要乘8，然后是小端序，所以放在倒数第二个,这里长度只用了32位
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
uint_8* changeHex(int a)//变化为string
{
    int b;
    uint_8 s[8];
	uint_8 s1[2];
    for(int i=0;i<4;i++)
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

void getMD5(uint_8 *source,uint_16 length,uint_8 *md5)
{
    atemp=A;//初始化
    btemp=B;
    ctemp=C;
    dtemp=D;

    unsigned int *strByte=add(source, length);//对输入数据进行填充处理

    for(unsigned int i=0;i<strlength/16;i++)//每16位分割
    {
        unsigned int num[16];
        for(unsigned int j=0;j<16;j++)
            num[j]=strByte[i*16+j];
        mainLoop(num);//num只是辅助传入输入数据，真正改变的是全局变量a(bcd)temp
    }

	unsigned int t[]={atemp,btemp,ctemp,dtemp};

	for(int i=0; i<4; i++)
		for(int j=0; j<8; j++)
			md5[i*8 + j]=changeHex(t[i])[j];//大小端变化回

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
    atemp=A;//初始化
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