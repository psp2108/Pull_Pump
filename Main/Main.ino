#include <LiquidCrystal.h>
/*
 * lcd.setCursor(<col>, <row>)
 * lcd.print()
 * 
 * ---------------------
 * 
 *  AOOOOOOOOOOOOOOB
 *  COOOOOOOOOOOOOOD
 *  
 *  A = 0,0
 *  B = 15,0
 *  C = 0,1
 *  D = 15,1
 */
#include <EEPROM.h>
/*
 * https://www.arduino.cc/en/Tutorial/EEPROMRead
 * 1024 bytes of storage
 * int        = 2 Bytes
 * long       = 4 Bytes
 * long long  = 8 Bytes
 */

// initialize the library by associating any needed LCD interface pin with the arduino pin number it is connected to
const int rs = 12;
const int en = 11;
const int d4 = 5;
const int d5 = 4;
const int d6 = 3;
const int d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Mode Selector
const int mode = 6;

// Timmer Control
const int increment = 7;
const int decrement = 8;
const int setTime = 9;

// Sensor 1
const int primarySensor = 10;

// Sensor 2
const int secondarySensor0 = 11;
const int secondarySensor1 = 12;

// Pump Control
const int pumpControl = 13;

// solenoid Control
const int solenoidControl = A0;

void lcdPrint(bool clearScr, String text, String pos="tl"){
  if (clearScr){
    lcd.clear();
  }
  lcdPrint(text, pos);
}
/*
 * tl    tm      tr
 * bl    bm      br
*/
void lcdPrint(String text, String pos="tl"){
  int len = text.length();
  String blankText = "";
  for(int i=0; i<16;i ++){
    blankText += " ";
  }
  
  switch (pos[0]){
    case 't':
      switch (pos[1]) {
        case 'l':
          lcd.setCursor(0, 0);
          lcd.print(text);
          Serial.println("Inside Top Left");
          break;
        case 'm':
          lcd.setCursor(0, 0);
          lcd.print(blankText);
          lcd.setCursor((len % 2 == 0? 8 - (len/2): 7 - (len/2)), 0);
          lcd.print(text);
          Serial.println("Inside Top Middle");
          break;
        case 'r':
          lcd.setCursor(16-len, 0);
          lcd.print(text);
          Serial.println("Inside Top Right");
          break;
        default:
          Serial.println("Default");
          break;
      }
      break;
    case 'b':
      switch (pos[1]) {
        case 'l':
          lcd.setCursor(0, 1);
          lcd.print(text);
          Serial.println("Inside Bottom Left");
          break;
        case 'm':
          lcd.setCursor(0, 1);
          lcd.print(blankText);
          lcd.setCursor((len % 2 == 0? 8 - (len/2): 7 - (len/2)), 1);
          lcd.print(text);
          Serial.println("Inside Bottom Middle");
          break;
        case 'r':
          lcd.setCursor(16-len, 1);
          lcd.print(text);
          Serial.println("Inside Bottom Right");
          break;
        default:
          Serial.println("Default");
          break;
      }
      break;
    default:
      Serial.println("Default");
      break;
  }
}

void pumpOn(bool fromOff = false){
  // True means turning pump on
  bool state = true;
  if(fromOff){
    state = !state;
  }
  digitalWrite(pumpControl, state);
}
void pumpOff(){
  pumpOn(true);
}

void solenoidOn(bool fromOff = false){
  // True means turning pump on
  bool state = true;
  if(fromOff){
    state = !state;
  }
  digitalWrite(solenoidControl, state);
}
void solenoidOff(){
  solenoidOn(true);
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);


  // Initialize all pins
  pinMode(mode, INPUT);
  pinMode(increment, INPUT);
  pinMode(decrement, INPUT);
  pinMode(setTime, INPUT);
  pinMode(primarySensor, INPUT);
  pinMode(secondarySensor0, INPUT);
  pinMode(secondarySensor1, INPUT);
  pinMode(pumpControl, OUTPUT);
  pinMode(solenoidControl, OUTPUT);  
}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1); //Column, Row
  // print the number of seconds since reset:
  lcd.print(millis() / 1000);
}


















