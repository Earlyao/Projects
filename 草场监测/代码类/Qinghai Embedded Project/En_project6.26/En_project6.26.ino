
#include <DHT.h>              //for tempture sensor
#include<SoftwareSerial.h>  //define softSerial
#include<Enerlib.h>           //low power lib

#define DHTTYPE DHT11
#define DHTPIN 8
#define rxPin 10
#define txPin 11
#define turn_on 3
#define lowPower 2 //arduino pro mini int0 is pin d2
#define dataSize 15
#define zbReset 8  
#define frPin 9
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

/*
   debug data
*/
char s[20];
float *f;
char st[20];
int i;
int count;
int value;
void setup() {
  Serial.begin(115200);
//  while (!Serial) { //wait serial test
//    ;
//  }
  software_Serial.begin(115200);
  Serial.println("DHT11 Started!");
  Serial.println("CC2530 Started.");
digitalWrite(turn_on , HIGH);
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
  count = 0;
}

void loop() {
  if ( software_Serial.available()>0) {
    Serial.println("rec data!");
    recData = software_Serial.read();
    Serial.println(recData, HEX);
    value = (int) recData & 0xff;
    Serial.println(value);
    if (value == 204) {
      Serial.println("CC");
      digitalWrite(zbReset, HIGH);
      delay(100);
      digitalWrite(zbReset, LOW);
    }
   else if (value == 170)
   {
       Serial.println("data:");
      // put your main code here, to run repeatedly:
      //delay(2000);
      humidity = dht.readHumidity();
      temperature = dht.readTemperature();
      heatindex = dht.computeHeatIndex(temperature, humidity, false);
      for ( i = 0 ; i < 10 ; i ++)
        count += digitalRead( frPin );
        if( count <8 ) sed_byte[6] = 0x01;
        else if( count > 7) sed_byte[6] = 0x00; 
      dtostrf(humidity, 2, 3, s);
      // Serial.println("data:");
      // Serial.print(s);
      // Serial.print( '\t');
      dtostrf(temperature, 2, 3, s);
      // Serial.print(s);
      // Serial.print( '\t');
      dtostrf(heatindex, 2, 3, s);
      // Serial.print(s);
      // Serial.println();
      // Serial.println("transform & raw:");
      hu.data = humidity;
      // for (int i=0; i<4; i++)
      //{
      // Serial.print(hu.bytes[i], HEX); // Print the hex representation of the float
      // Serial.print(' ');
      //}
      //
      //for (int i=0; i<4; i++)
      //{
      // Serial.print(hu.bytes[i], HEX); // Print the hex representation of the float
      // Serial.print(' ');
      //}
      //f = (float * )&hu.bytes;
      //dtostrf(*f,2,3,st);
      //Serial.println(st);
       te.data = temperature;
      // for (int i=0; i<4; i++)
      //{
      // Serial.print(te.bytes[i], HEX); // Print the hex representation of the float
      // Serial.print(' ');
      //}
      //f = (float * )&te.bytes;
      //dtostrf(*f,2,3,st);
      //Serial.println(st);
      // he.data = heatindex;
      // for (int i=0; i<4; i++)
      //{
      // Serial.print(he.bytes[i], HEX); // Print the hex representation of the float
      // Serial.print(' ');
      //}
      //f = (float * )&he.bytes;
      //dtostrf(*f,2,3,st);
      //Serial.println(st);
      //for (int i=0; i<4; i++)
      //{
      // Serial.print(hu.bytes[i], HEX); // Print the hex representation of the float
      // Serial.print(' ');
      //}
      //Serial.println("in the output bytes:");
      for (index = 0; index < 4; index++) {
        sed_byte[7 + index] = hu.bytes[index];
        //   Serial.print(hu.bytes[index], HEX); Serial.print(' ');
        //   Serial.print(sed_byte[8+index], HEX); Serial.print(' ');
      }
      // for (int i=0; i<4; i++)
      //{
      // Serial.print(hu.bytes[i], HEX); // Print the hex representation of the float
      // Serial.print(' ');
      //}
      //Serial.println();
      for (index = 0; index < 4; index++) {
        sed_byte[11 + index] = te.bytes[index];
        //   Serial.print(te.bytes[index], HEX); Serial.print(' ');
        //   Serial.print(sed_byte[12+index], HEX); Serial.print(' ');
      }
      // Serial.write(te.bytes,4);
      // Serial.println();
    //  for (index = 0; index < 4; index++) {
    //    sed_byte[14 + index] = he.bytes[index];
        //  Serial.print(he.bytes[index], HEX); Serial.print(' ');
        //  Serial.print(sed_byte[16+index], HEX); Serial.print(' ');
      //}
      // Serial.write(he.bytes,4);
      // Serial.println();
      // Serial.println("outputdata:");
       for (int i=0; i<dataSize; i++)
      {
       Serial.print(sed_byte[i], HEX); // Print the hex representation of the float
      }
      Serial.println();
      delay(10);
       software_Serial.write(sed_byte, dataSize); 
      // software_Serial.write(sendbyte,6);
       Serial.println("send completely!");
      digitalWrite(turn_on , HIGH);
     // delay(100);
     
      delay(30);
      digitalWrite(turn_on , LOW);
    }
    else
      software_Serial.flush();
  }
  // energy.PowerDown();
}
