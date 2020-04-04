#include <LiquidCrystal.h>
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

// Sensor 3
const int mainTankSensor = 16;//A2;

// Pump Control
const int pumpControl = A3;

// reset the pump after dry return
// const int resetPump = 13;

// indication LEDs
const int powerLED = 6;
const int pumpRunningLED = 3;
const int drainLED = 2;
///////////////////////////////////////////////////////////

// Parameters to check the off time of pump
long offCountStart = -1;
long offCountEnd = -1;
const int offInterval = 10;

bool pumpReady = true;      // Get from EEPROM
bool pumpDrained = false;   // Get from EEPROM <Update dont get>
bool pumpRunning = false;   // Get from EEPROM
long pumpRunTime = 0;       // Get from EEPROM
int drainCounter = 0;       // Get from EEPROM
long pumpRunCountStart = -1;
const int writeLimitInterval = 15 * 60;// EEPROM has write cycle limit so write is done in every 15 minutes

long countStart = -1;
long countEnd = -1;
const int waterEmptyTime = 10;
const int mainTankEmptyDelay = 15;
const int drainOffTime = 10;// To check if pump need manual assistant

String blankText = "";

String currentTextOnLCD = "";
bool bottomFill = false;

// Status
String statusCodes[] = {
/* -- *///"OOOOOOOOOOOOOOOO"
/* 00 */  "Priming fault",
/* 01 */  "Reseting in ",   // Not used
/* 02 */  "Water Detected",
/* 03 */  "Dry Running ",
/* 04 */  "Pump Ready",  
/* 05 */  "Pump Running",  
/* 06 */  "RunTime ",  //"RunTime 00:00:00"
/* 07 */  "Drain Mode ",
/* 08 */  "LastRun ",   //"LastRun 00:00:00"
/* 09 */  "No Water ",
/* 10 */  "Main Tank Full"
/* -- *///"OOOOOOOOOOOOOOOO"
};
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void updatePumpDrained(bool state){
  // Store in to EEPROM
  // 04-04-2020 2:30 AM UPDATE -> Dont store it on EEPROM
  pumpDrained = state;
}

void updatePumpReady(bool state){
  // Store in to EEPROM
  pumpReady = state;
}

void updatePumpRunning(bool state){
  // Store in to EEPROM
  pumpRunning = state;
}

void updatePumpRunTime(long runTime, bool force = false){
  pumpRunTime = runTime;
  if(force){
    pumpRunCountStart = -1;
  }
  if (pumpRunTime % writeLimitInterval == 0 || force){
    // Store in to EEPROM (condition check)

  }
}

void updateDrainCounter(int counter){
  drainCounter = counter;

  if (drainCounter <= 2){
    // Store in to EEPROM (condition check)

  }
}

void setPumpDrained(){
  // Load from EEPROM
  // 04-04-2020 2:30 AM UPDATE -> Dont load from EEPROM
}

void setPumpReady(){
  // Load from EEPROM
}

void setPumpRunning(){
  // Load from EEPROM
}

void setPumpRunTime(){
  // Load EEPROM 
}

void setDrainCounter(){
  // Load EEPROM 
}

String extraZero(byte num){
  if(num < 10)
    return "0";
  else
    return "";
}

String getFormattedTime(long temp, int limit = 3){
  int seconds = temp % 60;
  temp = temp / 60;
  int minutes = temp % 60;
  int hours = temp / 60;

  String s = "";
  if(limit > 0)
    s += (extraZero(hours) + hours);
  if(limit > 1)
    s += (":" + extraZero(minutes) + minutes);
  if(limit > 2)
    s += (":" + extraZero(seconds) + seconds);

  return s;
}

bool getPrimarySensor(bool force = false){
  // Put not (!) if it is active low
  if(force){
    return digitalRead(primarySensor);
  }
  if(!digitalRead(primarySensor)){
    return false;
  }
  else{
    return !getMainTankSensor();
  }
}

bool getSecondarySensor(){
  return digitalRead(secondarySensor);
  // if(!digitalRead(secondarySensor)){
  //   return false;
  // }
  // else{
  //   return !getMainTankSensor();
  // }

}

bool getMainTankSensor(){
  return digitalRead(mainTankSensor);
}

long getSecondsPassed(){
  return( millis() / 1000);
}

String getN(int n, String dummy = " "){
  String text = "";
  while (text.length() < n){
    text = text + dummy;
  }
  return text;
}

/*
 * tl    tm      tr
 * bl    bm      br
*/
void lcdPrint(String text, String pos="tl"){
  int len = text.length() <= 16 ? text.length() : 16;
  text = text + blankText;
  String temp = "";
  
  switch (pos[0]){
    case 't':
      
      lcd.setCursor(0, 0);
      switch (pos[1]) {
        case 'l':
          temp = text;
          lcd.print(temp);
          break;
        case 'm':
          temp = getN((len % 2 == 0? 8 - (len/2): 7 - (len/2))) + text;
          lcd.print(temp);
          break;
        case 'r':
          temp = getN(16-len) + text;
          lcd.print(temp);
          break;
        default:
          break;
      }
      break;
    case 'b':
      bottomFill = true;
      lcd.setCursor(0, 1);
      switch (pos[1]) {
        case 'l':
          temp = text;
          lcd.print(temp);
          break;
        case 'm':
          temp = getN((len % 2 == 0? 8 - (len/2): 7 - (len/2))) + text;
          lcd.print(temp);
          break;
        case 'r':
          temp = getN(16-len) + text;
          lcd.print(temp);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void lcdClearPrint(String text, String pos="tl"){
  Serial.println("Second LCD Print");
  if (text != currentTextOnLCD || bottomFill){
    Serial.println("Cleared");
    lcd.clear();
    currentTextOnLCD = text;
    bottomFill = false;
  }
  lcdPrint(text, pos);
}

bool pumpOn(bool fromOff = false){
  // True means turning pump on
  bool state = true;
  bool currentState = digitalRead(pumpControl);
  if(fromOff){
    state = !state;
  }
  digitalWrite(pumpControl, state);
  digitalWrite(pumpRunningLED, !fromOff);
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
  pinMode(mainTankSensor, INPUT);
  pinMode(pumpControl, OUTPUT);
  pinMode(powerLED, OUTPUT);
  pinMode(pumpRunningLED, OUTPUT);
  pinMode(drainLED, OUTPUT);

  for(int i=0; i<16; i ++){
    blankText += " ";
  }

  // Load EEPROM and set values
  setPumpDrained();
  setPumpReady();
  setPumpRunning();
  setPumpRunTime();
  setDrainCounter();

  if (pumpRunning){
    pumpOn();
    pumpRunCountStart = -pumpRunTime;
  }
  else{
    pumpOff();
  }
}

void loop() {
  if(!pumpDrained){
    if(pumpReady){
      if(getPrimarySensor()){
        pumpOn();
        pumpRunCountStart = getSecondsPassed();
        updatePumpRunTime(0);
        lcdPrint(statusCodes[2], "tm");
        
        countStart = getSecondsPassed();

        // Checking pump drain condition
        while(!getSecondarySensor() && drainCounter == 0){
          countEnd = getSecondsPassed();
          int timeLeft = drainOffTime - countEnd + countStart;
          lcdPrint(statusCodes[3] + timeLeft, "bm");
          if (timeLeft < 0){
            // Pump Drained
            updatePumpDrained(true);
            return;
          }
        }
        updatePumpRunning(true);
        updatePumpReady(false);
        updatePumpRunTime(getSecondsPassed() - pumpRunCountStart);
        // LOOP exit means pump running proper
      }
      else{
        // Ideal state
        if (getMainTankSensor()){
          lcdPrint(statusCodes[10], "tm");
        }
        else{
          lcdPrint(statusCodes[4], "tm");
        }
        // Display time of last run
        lcdPrint(statusCodes[8] + getFormattedTime(pumpRunTime), "bm");
      }
    }
    else{
      //Either pump is running or water is draining
      updatePumpRunTime(getSecondsPassed() - pumpRunCountStart);
      lcdPrint(statusCodes[6] + getFormattedTime(pumpRunTime), "bm");
      
      if(getMainTankSensor()){
        // Main Tank is full
        pumpOff();
        Serial.println("Pump Off");
        updatePumpRunTime(getSecondsPassed() - pumpRunCountStart, true);
        lcdPrint(statusCodes[8] + getFormattedTime(pumpRunTime), "bm");
        Serial.println("LCD Updated");
        lcdPrint(statusCodes[7], "tm");

        updateDrainCounter(0);
        while(getPrimarySensor(true) && drainCounter < mainTankEmptyDelay){
          lcdPrint(statusCodes[7] + drainCounter++, "tm");
          updateDrainCounter(drainCounter);
          delay(1000);
        }
        if(!getPrimarySensor(true)){
          updateDrainCounter(0);
          Serial.println("Water Drained");
        }
        updatePumpReady(true);
      }
      else if(getSecondarySensor()){
        offCountStart = -1;
        // Pump is running
        lcdPrint(statusCodes[5], "tm");
      }
      else{
        // Wait some time to drain water and then turn pump off
        if(offCountStart == -1){
          offCountStart = getSecondsPassed();
        }
        else{
          offCountEnd = getSecondsPassed();
          int timeLeft = offInterval - offCountEnd + offCountStart;
          lcdPrint(statusCodes[9] + timeLeft, "tm");
          // Condition check for bubbles
          Serial.println(offCountEnd - offCountStart);
          if(timeLeft < 0){
            pumpOff();
            Serial.println("Pump Off");
            updatePumpRunTime(getSecondsPassed() - pumpRunCountStart, true);
            lcdPrint(statusCodes[8] + getFormattedTime(pumpRunTime), "bm");
            Serial.println("LCD Updated");
            // Contition check to empty water
            lcdPrint(statusCodes[7], "tm");

            // Should not be like this
            // Serial.print("Will wait for waterEmpty, delay ");
            updateDrainCounter(0);
            while(getPrimarySensor(true)){
              lcdPrint(statusCodes[7] + drainCounter++, "tm");
              updateDrainCounter(drainCounter);
              delay(1000);
            }
            updateDrainCounter(0);
            Serial.println("Water Drained");
            updatePumpReady(true);
            offCountStart = -1;
            offCountEnd = -1;
          }
        }
      }
    }
  }
  else{
    pumpOff();
    lcdClearPrint(statusCodes[0], "tm");
    updatePumpRunning(false);
    updatePumpReady(true);
    while (true) {
      digitalWrite(drainLED, 1);
      delay(500);
      digitalWrite(drainLED, 0);
      delay(500);
    }
  }
}
