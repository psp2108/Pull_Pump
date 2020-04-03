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
 * 
 * 10000 Write Cycles in total so it is recommended to use put() function in 5 or 10 minutes interval
 */

/////////////////////// PIN SECTION ///////////////////////
// LCD interface pins
const int rs = 12;
const int en = 11;
const int d4 = 10;
const int d5 = 9;
const int d6 = 8;
const int d7 = 7;

// Sensor 1
const int primarySensor = 14;//A0;

// Sensor 2
const int secondarySensor = 15;//A1;

// Pump Control
const int pumpControl = 20;//A6;

// reset the pump after dry return
const int resetPump = 13;

// indication LEDs
const int powerLED = 6;
const int pumpRunningLED = 5;
///////////////////////////////////////////////////////////

// Parameters to check the off time of pump
long offCountStart = -1;
long offCountEnd = -1;
const int offInterval = 5000;

      bool pumpReady = true;      // Get from EEPROM
      bool pumpDrained = false;    // Get from EEPROM
      long countStart = -1;
      long countEnd = -1;
      const int resetInterval = 10;
      const int drainOffTime = 5000;    // To check if pump need manual assistant

      String blankText = "";



// Status
String statusCodes[] = {

/* -- *///"OOOOOOOOOOOOOOOO"
/* 00 */  "Pump Drained",
/* 01 */  "Reseting in ", 
/* 02 */  "Turning Pump on",
/* 03 */  "Drain check ",  
/* 04 */  "Pump Idle",  
/* 05 */  "Pump Running",  
/* 06 */  "Time 00:00:00",  
};

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void updatePumpDrained(bool state){
  // Store in to EEPROM
  pumpDrained = state;
}

void updatePumpReady(bool state){
  // Store in to EEPROM
  pumpReady = state;
}

bool getPrimarySensor(){ 
  // Put not (!) if it is active low
  return digitalRead(primarySensor);
}

bool getSecondarySensor(){
  return digitalRead(secondarySensor) == 1;
}

long getSecondsPassed(){
  return( millis() / 1000);
}

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
  
  switch (pos[0]){
    case 't':
      
      lcd.setCursor(0, 0);
      lcd.print(blankText);
      switch (pos[1]) {
        case 'l':
          lcd.setCursor(0, 0);
          lcd.print(text);
          Serial.println("Inside Top Left");
          break;
        case 'm':
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
    
      lcd.setCursor(0, 1);
      lcd.print(blankText);
      switch (pos[1]) {
        case 'l':
          lcd.setCursor(0, 1);
          lcd.print(text);
          Serial.println("Inside Bottom Left");
          break;
        case 'm':
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

bool pumpOn(bool fromOff = false){
  // True means turning pump on
  bool state = true;
  bool currentState = digitalRead(pumpControl);
  if(fromOff){
    state = !state;
  }
  digitalWrite(pumpControl, state);
  return state != currentState;
}
bool pumpOff(){
  return pumpOn(true);
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);


  // Initialize all pins
  pinMode(primarySensor, INPUT);
  pinMode(secondarySensor, INPUT);
  pinMode(pumpControl, OUTPUT); 

  for(int i=0; i<16;i ++){
    blankText += " ";
  }
}

void preStepsPumpOff(){
  if(offCountStart == -1){
    offCountStart = getSecondsPassed();
  }
  else{
    offCountEnd = getSecondsPassed();
    // Condition check for bubbles
    if(offCountEnd - offCountStart > offInterval){
      pumpOff();
      // Contition check to drain off water
      delay(drainOffTime);
      offCountStart = -1;
      offCountEnd = -1;
    }
  }
}

void loop() {
  if(!pumpDrained){
    if(pumpReady){
      if(getPrimarySensor()){
        pumpOn();
        updatePumpReady(false);
        lcdPrint(true, statusCodes[2], "tl");
        
        countStart = getSecondsPassed();

        // Checking pump drain condition
        while(!getSecondarySensor()){
          countEnd = getSecondsPassed();
          int timeLeft = drainOffTime - countEnd + countStart;
          lcdPrint(statusCodes[3] + timeLeft, "bl");
          if (timeLeft < 0){
            // Pump Drained
            updatePumpDrained(true);
            return;
          }
        }
        // LOOP exit means pump running proper
      }
      else{
        // Display time of last run
        lcdPrint(true, statusCodes[4], "tm");
      }
    }
    else{
      //Either pump is running or water is draining
      if(getSecondarySensor()){
        // Pump is running
        lcdPrint(statusCodes[5], "tm");
      }

    }
  }
  else{
    pumpOff();
    lcdPrint(true, statusCodes[0], "tm");
    if(digitalRead(resetPump)){

      digitalWrite(powerLED, 1);
      countStart = getSecondsPassed();;

      while(digitalRead(resetPump)){
        countEnd = getSecondsPassed();
        int timeLeft = resetInterval - countEnd + countStart;
        lcdPrint(statusCodes[1] + timeLeft, "bm");
        if(timeLeft < 0){
          updatePumpDrained(false);
          updatePumpReady(true);
          while(digitalRead(resetPump)){}
        }
      }
    }
    else{
      digitalWrite(powerLED, 0);
    }
  }
}
