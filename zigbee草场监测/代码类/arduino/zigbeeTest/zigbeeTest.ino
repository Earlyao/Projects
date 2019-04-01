#include<SoftwareSerial.h>

SoftwareSerial software_Serial(5 ,6);

char recChar;
String reString;
byte rbyte[6] = {170,00,00,85,56,128};
byte bytes[6];
int i;
int num;
void setup() {
  // put your setup code here, to run once:
 Serial.begin(115200);
 while(!Serial) { 
  ;
 }
 software_Serial.begin(115200);
 delay(2000);
 i = 0;
 Serial.println("Device Started.");
}

void loop() {
 //while(software_Serial.available()>0)
 if (software_Serial.available()>0)
 {
 //reString += char(software_Serial.read());
 num = software_Serial.readBytes(bytes,6);
 Serial.print("num:");
 Serial.print(num);
 Serial.println();
  delay(2);
  for(i = 0;i<num;i++)
  reString += char(bytes[i]);
 Serial.print("bytes:");
 Serial.write(bytes,num);
 Serial.println(reString);
 delay(2);
 }
 if(reString.length()>0)
 {
  for(int i=0;i<reString.length();i++){
       Serial.print("reString[");
       Serial.print(i,DEC);
       Serial.print("]=");
       Serial.print(reString[i],HEX);  
     }
 }
//  Serial.println();
 // reString = "";
 // while(software_Serial.available()){
//    recChar = software_Serial.read();
//    reString += recChar;
//      rbyte[i] = software_Serial.read();
//      i++;
//      if(i == 6) {
//        i =0;
//        Serial.write(rbyte,6);
//      }
     
 //  }
 // Serial.println(reString);
 //   Serial.write(rbyte,6);
// }
 //  reString = "AA 00 00 55123";
//    while(Serial.available())
//    {
//      recChar = Serial.read();
//      reString += recChar; //makes the string readString
//    }
     //reString.getBytes(rbyte,18);
//     Serial.write(rbyte,6);
//    software_Serial.write(rbyte,6); 
//    delay(2000);
 // }
 reString = "";
 }
