void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(A5, OUTPUT);
  pinMode(A0, INPUT);
  digitalWrite(A5, 0);
  digitalWrite(A0, 1);
  
  if(!digitalRead(A0)){
    Serial.println("Proceed Further");
  }
  else{
    Serial.println("Erase Memory");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if(!digitalRead(A0)){
    Serial.println("OK");
  }
  else{
    Serial.println("--");
  }
  delay(5000);
}
