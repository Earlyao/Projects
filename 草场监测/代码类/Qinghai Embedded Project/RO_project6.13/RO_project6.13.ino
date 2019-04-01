#include<SoftwareSerial.h>

#define stcSize 15//56//19 //85
#define steSize 5
#define enCount 1 // 8
#define dataSize 11
#define maxSize
#define roReset 2
#define vcc0 9
#define sleeptime 3000
SoftwareSerial software_Serial(5 , 6);

byte identity;
byte co_Data[stcSize] = {170, 00, 00, 85, 255};//, 01};
byte en_Data[steSize] = {170, 00, 02, 85, 170};
byte enList[8] = {
  0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09
 // 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19
};

byte rfc;
byte recData[dataSize];
int i;
int numfc;
int numfe;
typedef struct { //the function is using to judge if the en working
  byte enNo;
  bool flag;
} En;
En en[enCount];
byte tempBuf[85];
int readBuffer(byte temp[]);
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  software_Serial.begin(115200);
 // while (!Serial) ;
  Serial.println("start!");
  for ( i = 0 ; i < enCount ; i++)
  {
    en[i].enNo = enList[i];
    en[i].flag = true;
  }
  pinMode( roReset, OUTPUT );
  digitalWrite(roReset, HIGH);
   pinMode( vcc0, OUTPUT );
  digitalWrite(vcc0, HIGH);
  //Serial.println("start!");
  //by[0] = 0x02;
  //  by[1] = 0x01;
  //  // put your main code here, to run repeatedly:
  // Serial.write(by,2);
  Serial.println("send data:");
}

void loop() {
  en_Data[0] = 0xAA;en_Data[1] = 0x00;en_Data[2] = 0x00;
  en_Data[3] = 0x55;en_Data[4] = 0xAA;
  if (software_Serial.available() > 0)
  {
    numfc = readBuffer( tempBuf );
    Serial.println(numfc);
    //    software_Serial.readBytes(recData, dataSize);
    if (numfc == 1) {
      rfc = tempBuf[0];
      Serial.println(rfc , HEX);
      if (rfc == 0xAF)
      {
        software_Serial.flush();
        Serial.println("rec data!");
        for ( i = 0; i < enCount; i++) {
          if ( en[i].flag == true ) {
            en_Data[2] = enList[i];
            //  Serial.write(en_Data, dataSize);
            //            co_Data[2] = 0X00;
            //            software_Serial.write(co_Data, stcSize);
            //    Serial.write(co_Data, stcSize);
            //co_Data[2] = 0X02;
            software_Serial.write(en_Data, steSize);
            //   Serial.write(en_Data, stcSize);
            Serial.println("compelete!");
            //  Serial.write(en_Data, 5);
            delay(sleeptime);
            Serial.println(en[0].flag);
            Serial.println("now accept the data!");
            if (software_Serial.available() > 0)
            {
              Serial.println("rec data from en");
              // software_Serial.read();
              // software_Serial.readBytes(recData, dataSize);
              numfe = readBuffer(tempBuf);
              if (numfe == dataSize) {
                for ( int j = 0; j < dataSize; j++)
                  recData[j] = tempBuf[j];
                if (recData[0] == 0xFF)
                {
                  Serial.println("rec succeseefully!");
                  for (int j = 0; j < dataSize; j++)
                    co_Data[ 5 + i * 13 + j ] = recData[ j + 1 ];
                  //software_Serial.flush();
                  //  Serial.write(co_Data, stcSize);
                 // software_Serial.write(co_Data, stcSize);
                }
//                else {//二次确认
//                  Serial.println("ensure the second time!");
//                  software_Serial.flush();
//                  software_Serial.write(en_Data, steSize);//ensure the en alive
//                  delay(sleeptime);
//                  if (software_Serial.available() > 0)
//                  {
//
//                    numfe = readBuffer(tempBuf);
//                    if (numfe == dataSize) {
//                      for ( int j = 0; j < dataSize; j++)
//                        recData[j] = tempBuf[j];
//                      if (recData[0] == 0xFF)
//                      {
//                        Serial.println("sencod time successfully!");
//                        for (int j = 0; j < dataSize; j++)
//                          co_Data[ 5 + i * 13 + j ] = recData[ j + 1 ];
//                       // software_Serial.flush();
//                        // Serial.write(co_Data, stcSize);
//                       // software_Serial.write(co_Data, stcSize);
//                      }
//                    }
//                  }
//                  else {
//                    Serial.println("sencod time failed!");
//                    en[i].flag = false;
//                  }
//                }
              }
              else {
                Serial.println("reset!");
                software_Serial.flush();
                software_Serial.write(en_Data, steSize);//ensure the en alive
                delay(sleeptime);
                if (software_Serial.available() > 0)
                {
                  numfe = readBuffer(tempBuf);
                  if (numfe == dataSize) {
                    Serial.println("reset rec successfully!");
                    for ( int j = 0; j < dataSize; j++)
                      recData[j] = tempBuf[j];
                    if (recData[0] == 0xFF)
                    {
                      for (int j = 0; j < dataSize; j++)
                        co_Data[ 5 + i * 13 + j ] = recData[ j + 1 ];
                      //software_Serial.flush();
                      // Serial.write(co_Data, stcSize);
                   //   software_Serial.write(co_Data, stcSize);
                    }
                  }
                }
                else {
                  Serial.println("reset recfailed!");
                  en[i].flag = false;
                }
              }
            }
            else { //重启
              Serial.println("reset en board !");
              en_Data[4] = 0xCC;//reset the en
              software_Serial.write(en_Data, steSize);//重启en板 
              delay(100);
              en_Data[4] = 0xAA;
               software_Serial.write(en_Data, steSize);//继续请求 
              delay(sleeptime);//wait for rec the data again
              if (software_Serial.available() > 0)
              {
                numfe = readBuffer(tempBuf);
                if (numfe == dataSize) {
                  for ( int j = 0; j < dataSize; j++)
                    recData[j] = tempBuf[j];
                  if (recData[0] == 0xFF)
                  {
                    for (int j = 0; j < dataSize; j++)
                      co_Data[ 5 + i * 13 + j ] = recData[ j + 1 ];
                   // software_Serial.flush();
                   Serial.println("after reset  is successful!");
                    //Serial.write(co_Data, stcSize);
                  //  software_Serial.write(co_Data, stcSize);
                  }
                }
              }
              else {
                 en[i].flag = false;
              }
            }
          }
          else if ( en[i].flag == false){
           // en[i].flag = true;
            co_Data[ 5 + i * 13 ] =  enList[i];
            co_Data[ 5 + i * 13 + 1 ] =  0xFE;
             for (int j = 2; j < dataSize; j++)
               co_Data[ 5 + i * 13 + j ] = 0x00;
          }
          
          //send the data to co
        }
         for (int j = 0; j < dataSize; j++)
         Serial.print(co_Data[j] ,HEX);
         Serial.println();
        software_Serial.write(co_Data, stcSize);
        software_Serial.flush();
      }
      // if (en[i].flag) Serial.println("true");
      // else Serial.println("false");
      // delay(2000);//debug time
      else if (rfc == 0xBF) {
        Serial.println("reset the board!");
        digitalWrite(roReset, LOW);
        delay(100);
        digitalWrite(roReset, HIGH);
      }
    }

  }
}

int readBuffer(byte readBuf[])
{
  int k = 0;
  byte temp;
  while (software_Serial.available() > 0) {
    temp = software_Serial.read();
    readBuf[k] = temp;
    k++;
  }
  return k;
}

