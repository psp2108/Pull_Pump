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

//////////////////////// MEMORY RELATED ////////////////////////

// Default States
byte stateByte = 1;

const int addressStateByte = 5;   // Address to store pumpReady, pumpRunning, drainCounter
bool pumpReady = true;      // Get from EEPROM (-------x)
bool pumpRunning = false;   // Get from EEPROM (------x-)
int drainCounter = 0;       // Get from EEPROM (-----x--)

const int addressPumpRunTime = addressStateByte + 1;
unsigned long pumpRunTime = 0;       // Get from EEPROM

////////////////////////////////////////////////////////////////
///////////////////////// PIN  SECTION /////////////////////////

// To ERASE MEMORY
const int _throw = 9;
const int _catch = 10;

// LCD interface pins
const int rs = 8;
const int en = 6;
const int d4 = 5;
const int d5 = 4;
const int d6 = 3;
const int d7 = 2;

// Sensor 1
const int primarySensor = A0;

// Sensor 2
const int secondarySensor = A1;

// Sensor 3
const int mainTankSensor = A2;

// Pump Control
const int pumpControl = A3;

// Force Pump On (Active Low)
const int forcePumpOn = 10;

// indication LEDs
const int pumpRunningLED = 13;
const int primingLED = 12;
const int readyLED = 12;
const int drainingLED = 12;

////////////////////////////////////////////////////////////////
///////////////////// INTERVAL  PARAMETERS /////////////////////

// EEPROM has write cycle limit so write is done after some interval
const int writeLimitInterval = 5*60;

// Timmer to turn pump off from running state
const int pumpOffInterval = 2*60;

// Delay for the pump to be in ready state after turning off from main tank full Sensor
const int mainTankEmptyDelay = 10*60;

// Interval to check when the pump needs manual assistance (Priming Fault)
const int pumpDryRunTime = 30;

////////////////////////////////////////////////////////////////
////////////////////// COUNTER  VARIABLES //////////////////////

// Parameters to check the off time of pump
long offCountStart = -1;
long offCountEnd = -1;

// General purpose ounters
long countStart = -1;
long countEnd = -1;

//Special counter which tracks the run time of pump
long pumpRunCountStart = -1;

////////////////////////////////////////////////////////////////
//////////////////// LCD RELATED PARAMETERS ////////////////////

String blankText = "                ";

// To avoid lcd.clear() if bottom line is already blanked 
// (as lcd.clear() has some delay in it which causes the text to faint out)
bool bottomFill = false;
String currentTextOnLCD = "";

// Status Messages which are displayed on lcd
String statusCodes[] = {
/* -- *///"OOOOOOOOOOOOOOOO"
/* 00 */  "Priming fault",
/* 01 */  "Water Detected",
/* 02 */  "Dry Run ",     //"Dry Run 00:00"
/* 03 */  "Pump Ready",
/* 04 */  "Pump Running",
/* 05 */  "RunTime ",         //"RunTime 00:00:00"
/* 06 */  "Drain ",         //"Drain   00:00:00"
/* 07 */  "LastRun ",         //"LastRun 00:00:00"
/* 08 */  "No Water ",        //"No Water   00:00"
/* 09 */  "Main Tank Full",
/* 10 */  "Erasing Memory",
/* 11 */  "Memory Erased",
/* 12 */  "Reboot Device"
/* -- *///"OOOOOOOOOOOOOOOO"
};

////////////////////////////////////////////////////////////////
///////////////////////// OTHER  FLAGS /////////////////////////

// Primimg Fault
bool pumpDrained = false;

// After memory reset
bool needsReboot = false;

// False main tank signal
bool forceEmpty = false;

////////////////////////////////////////////////////////////////

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void updatePumpDrained(bool state){
  // 04-04-2020 2:30 AM UPDATE -> Dont store it on EEPROM
  pumpDrained = state;
}

// EPROM.update() - Update only single byte
// bitWrite(<num>, <bit from right to left>, bit)
void updatePumpReady(bool state){
  // Store in to EEPROM stateByte(-------x)
  pumpReady = state;
  bitWrite(stateByte, 0, state);
  EEPROM.update(addressStateByte, stateByte);
  digitalWrite(readyLED, state);
}
void updatePumpRunning(bool state){
  // Store in to EEPROM stateByte(------x-)
  pumpRunning = state;
  bitWrite(stateByte, 1, state);
  EEPROM.update(addressStateByte, stateByte);
}
void updateDrainCounter(int counter){
  drainCounter = counter;
  if (drainCounter <= 1){
    // Store in to EEPROM (condition check)  stateByte(-----x--)
    bitWrite(stateByte, 2, drainCounter);
    EEPROM.update(addressStateByte, stateByte);
  }
}

//EEPROM.put() -- Update any datatype
void updatePumpRunTime(unsigned long runTime, bool force = false){
  pumpRunTime = runTime;
  if(force){
    pumpRunCountStart = -1;
  }
  if (pumpRunTime % writeLimitInterval == 0 || force){
    // Store in to EEPROM (condition check)
    EEPROM.put(addressPumpRunTime, pumpRunTime);
  }
}

// EEPROM.read() - Read only single byte
// bitRea(<num>, <bit from right to left>)
void setPumpReady(){
  // Load from EEPROM stateByte(-------x)
  stateByte = EEPROM.read(addressStateByte);
  pumpReady = bitRead(stateByte, 0);
  digitalWrite(readyLED, pumpReady);
}
void setPumpRunning(){
  // Load from EEPROM stateByte(------x-)
  stateByte = EEPROM.read(addressStateByte);
  pumpRunning = bitRead(stateByte, 1);
}
void setDrainCounter(){
  // Load from EEPROM stateByte(-----x--)
  stateByte = EEPROM.read(addressStateByte);
  drainCounter = bitRead(stateByte, 2);
}

//EEPROM.get() -- Get any datatype
void setPumpRunTime(){
  // Load EEPROM 
  EEPROM.get(addressPumpRunTime, pumpRunTime);
}

String extraZero(byte num){
  if(num < 10)
    return "0";
  else
    return "";
}

String getFormattedTime(unsigned long temp, int limit = 3){
  int seconds = temp % 60;
  temp = temp / 60;
  int minutes = temp % 60;
  int hours = temp / 60;

  String s = "";
  if(limit > 2)
    s += (extraZero(hours) + hours) + ":";
  if(limit > 1)
    s += (extraZero(minutes) + minutes) + ":";
  if(limit > 0)
    s += (extraZero(seconds) + seconds);

  return s;
}

bool getPrimarySensor(bool force = false){
  // Put not (!) if it is active low
  if(force){
    return !digitalRead(primarySensor);
  }
  if(digitalRead(primarySensor)){
    return false;
  }
  else{
    return !getMainTankSensor();
  }
}

bool getSecondarySensor(){
  // Put not (!) if it is active low
  return !digitalRead(secondarySensor);
}

bool getMainTankSensor(){
  // Put not (!) if it is active low
  if (forceEmpty)
    return false;
  return !digitalRead(mainTankSensor);
}

unsigned long getSecondsPassed(long inSeconds = 1000){
  return(millis() / inSeconds);
}

String getN(int n, String dummy = " "){
  String text = "";
  while (text.length() < n){
    text = text + dummy;
  }
  return text;
}

/*
 * LCD directions variable:pos
 *   [tl    tm      tr]
 *   [bl    bm      br]
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
  // Serial.print(pos);
  // Serial.print(" - ");
  // Serial.println(temp);
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
  // state=true (Actual state on relay output pin) means turning pump on
  bool state = true;
  bool currentState = digitalRead(pumpControl);
  if(fromOff){
    state = !state;
    forceEmpty = false;
  }
  else{
    digitalWrite(readyLED, fromOff);
  }
  digitalWrite(pumpControl, state);
  digitalWrite(pumpRunningLED, !fromOff);
  return state != currentState;
}
bool pumpOff(){
  return pumpOn(true);
}

bool forcePumpOnWithCheck(){
  if((!pumpRunning) && digitalRead(forcePumpOn)){
    Serial.println("Turning Pump on forcefully");
    pumpOn();
    offCountStart = -1;
    offCountEnd = -1;
    countStart = -1;
    countEnd = -1;

    updateDrainCounter(0);
    updatePumpRunning(true);
    updatePumpReady(false);
    updatePumpRunTime(0, true);
    updatePumpDrained(false);

    forceEmpty = true;

    return true;
  }
  return false;
}

void setup(){
  Serial.begin(9600);
  lcd.begin(16, 2);
  
  // Initialize all pins
  pinMode(primarySensor, INPUT);
  pinMode(secondarySensor, INPUT);
  pinMode(mainTankSensor, INPUT);
  pinMode(pumpControl, OUTPUT);
  pinMode(pumpRunningLED, OUTPUT);
  pinMode(primingLED, OUTPUT);
  pinMode(readyLED, OUTPUT);
  pinMode(drainingLED, OUTPUT);
  pinMode(_throw, OUTPUT);
  pinMode(_catch, INPUT);
  pinMode(forcePumpOn, INPUT);
  

  digitalWrite(_throw, 0);
  digitalWrite(_catch, 1);
  digitalWrite(forcePumpOn, 1);

  if(digitalRead(_catch)){
    // Erase to Default
    lcdPrint(statusCodes[10], "tm");
    updatePumpReady(pumpReady); 
    updatePumpRunning(pumpRunning);
    updateDrainCounter(drainCounter);
    updatePumpRunTime(pumpRunTime, true);
    needsReboot = true;
    delay(3000);
  }
  else{
    // Load EEPROM and set values
    setPumpReady(); 
    setPumpRunning();
    setPumpRunTime();
    setDrainCounter();

    if (pumpRunning){
      if(getMainTankSensor())
        pumpOff();
      else
        pumpOn();

      pumpRunCountStart = -pumpRunTime;
    }
    else{
      pumpOff();
    }
  }
}

void loop(){
  if(!needsReboot){
    if (forcePumpOnWithCheck()) return;
    if(!pumpDrained){
      if(pumpReady){
        if(getPrimarySensor()){
          pumpOn();
          updatePumpRunning(true);
          pumpRunCountStart = getSecondsPassed();
          // updatePumpRunTime(0);
          lcdPrint(statusCodes[1], "tm");
          
          countStart = getSecondsPassed();

          // Checking pump priming fault
          while(!getSecondarySensor() && drainCounter == 0){
            if (forcePumpOnWithCheck()) return;
            countEnd = getSecondsPassed();
            int timeLeft = pumpDryRunTime - countEnd + countStart;
            lcdPrint(statusCodes[2] + getFormattedTime(timeLeft, 2), "bm");
            digitalWrite(pumpRunningLED, getSecondsPassed(300) % 2);
            if (timeLeft < 0){
              // Priming Fault
              updatePumpDrained(true);
              return;
            }
          }
          updatePumpRunTime(getSecondsPassed() - pumpRunCountStart);
          updatePumpReady(false);
          // LOOP exit means pump running proper
        }
        else{
          // Ideal state
          if (getMainTankSensor()){
            lcdPrint(statusCodes[9], "tm");
          }
          else{
            lcdPrint(statusCodes[3], "tm");
          }
          // Display time of last run
          lcdPrint(statusCodes[7] + getFormattedTime(pumpRunTime), "bm");
        }
      }
      else{
        //Either pump is running or water is draining
        updatePumpRunTime(getSecondsPassed() - pumpRunCountStart);
        lcdPrint(statusCodes[5] + getFormattedTime(pumpRunTime), "bm");
        
        if(getMainTankSensor()){
          // Main Tank is full
          pumpOff();
          updatePumpRunning(false);
          Serial.println("Pump Off");
          updatePumpRunTime(getSecondsPassed() - pumpRunCountStart, true);
          lcdPrint(statusCodes[7] + getFormattedTime(pumpRunTime), "bm");
          Serial.println("LCD Updated");
          lcdPrint(statusCodes[6], "tm");

          updateDrainCounter(0);
          while(getPrimarySensor(true) && drainCounter < mainTankEmptyDelay){
            if (forcePumpOnWithCheck()) return;
            lcdPrint(statusCodes[6] + getFormattedTime(drainCounter++), "tm");
            updateDrainCounter(drainCounter);
            delay(1000);
            digitalWrite(drainingLED, !digitalRead(drainingLED));
          }
          digitalWrite(drainingLED, 0);
          if(!getPrimarySensor(true)){
            updateDrainCounter(0);
            Serial.println("Water Drained");
          }
          updatePumpReady(true);
        }
        else if(getSecondarySensor()){
          offCountStart = -1;
          // Pump is running
          lcdPrint(statusCodes[4], "tm");
          digitalWrite(pumpRunningLED, 1);
        }
        else{
          // Wait some time to drain water and then turn pump off
          if(offCountStart == -1){
            offCountStart = getSecondsPassed();
          }
          else{
            offCountEnd = getSecondsPassed();
            int timeLeft = pumpOffInterval - offCountEnd + offCountStart;
            lcdPrint(statusCodes[8] + getFormattedTime(timeLeft, 2), "tm");
            // Condition check for bubbles
            Serial.println(offCountEnd - offCountStart);
            if(timeLeft < 0){
              pumpOff();
              updatePumpRunning(false);
              Serial.println("Pump Off");
              updatePumpRunTime(getSecondsPassed() - pumpRunCountStart, true);
              lcdPrint(statusCodes[7] + getFormattedTime(pumpRunTime), "bm");
              Serial.println("LCD Updated");
              // Contition check to empty water
              lcdPrint(statusCodes[6], "tm");

              // Should not be like this
              // Serial.print("Will wait for waterEmpty, delay ");
              updateDrainCounter(0);
              while(getPrimarySensor(true)){
                if (forcePumpOnWithCheck()) return;
                lcdPrint(statusCodes[6] + getFormattedTime(drainCounter++), "tm");
                updateDrainCounter(drainCounter);
                delay(1000);
                digitalWrite(drainingLED, !digitalRead(drainingLED));
              }
              digitalWrite(drainingLED, 0);
              updateDrainCounter(0);
              Serial.println("Water Drained");
              updatePumpReady(true);
              offCountStart = -1;
              offCountEnd = -1;
            }
            else{
              pumpOn();
              digitalWrite(pumpRunningLED, getSecondsPassed(300) % 2);
              updatePumpRunning(true);
            }
          }
        }
      }
    }
    else{
      pumpOff();
      lcdClearPrint(statusCodes[0], "bm");
      updatePumpRunning(false);
      updatePumpReady(true);
      updateDrainCounter(0);
      offCountStart = getSecondsPassed();
      
      while (getPrimarySensor(true)){
        if (forcePumpOnWithCheck()) return;
        drainCounter = getSecondsPassed() - offCountStart;
        lcdPrint(statusCodes[6] + getFormattedTime(drainCounter), "tm");
        digitalWrite(primingLED, getSecondsPassed(200) % 2);
      }
      offCountStart = -1;
      digitalWrite(primingLED, 0);
      updatePumpDrained(false);
      updatePumpReady(true);
    }
  }
  else{
    lcdPrint(statusCodes[11], "tm");
    lcdPrint(statusCodes[12], "bm");
  }
}
