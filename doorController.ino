#include <SD.h>
#include "Wire.h"
#include <SoftwareSerial.h>
#include <Keypad.h>

#define MAX_PASSWORDS 12
#define DS3231_I2C_ADDRESS 0x68
#define PASSWORD_LENGTH 12

// Created by Kushagra Mansingh and Dawson Foushee

/* d13 is SCK
  d12 is MISO
  d11 is MOSI
  d10 is CS/SS
  A4 is SDA
  A5 is SCL
  */
const int chipSelect = 10;
File myFile;

//rfid stuff
SoftwareSerial id20(9,2);

//keypad stuff
const byte rows = 4; //four rows
const byte cols = 3; //three columns
const char keys[rows][cols] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'},
};
byte rowPins[rows] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[cols] = {6, 7, 8, }; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );

//string buffers
const char password[PASSWORD_LENGTH+1] = "111222333444";
char* passwords[MAX_PASSWORDS];
char testTag[PASSWORD_LENGTH+1];
char keyTag[PASSWORD_LENGTH+1];
int keyTagIndex = 0;
int nextPass = 1;

//time stuff
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

void setup() {
  for (int j = 0; j < MAX_PASSWORDS; j++) {
    passwords[j] = "0400215C354C";
  }
  
  Serial.begin(9600);
  id20.begin(9600);
  Wire.begin();
  pinMode(chipSelect, OUTPUT);
  pinMode(A3, OUTPUT);
  
  if(!SD.begin(chipSelect)) {
    Serial.println("SD card didn't init");
    return;
  }
}

void loop() {
  if (id20.available()) {
    readTag();
    if (checkPasswords(testTag)){
      match(testTag);
    }
  }
  //digitalWrite(A3,HIGH);

  char key = keypad.getKey();
  if (key != NO_KEY) {
    Serial.print(key);
    Serial.print("\n");
    if (keyTagIndex < 12) {
      keyTag[keyTagIndex++] = key;
    }
    if (keyTagIndex == 12) {
      Serial.print("Checking keypad input: ");
      Serial.print(keyTag);
      Serial.print("\n");
      
      if (strcmp(keyTag, password)==0) {
        match(keyTag);
      }
      else {
        Serial.println("Failed Attempt");
      }
      keyTagIndex = 0;
    }
  }
  delay(200);
}

boolean checkPasswords(char s[]) {
  Serial.print("CHECKING PASSWORD ");
  Serial.print(s);
  Serial.print(":\n");
  for(int j=0; j<MAX_PASSWORDS;j++){
    Serial.print("   ");
    Serial.print(passwords[j]);
    Serial.print("\n");
    if(strcmp(s, passwords[j])==0){
      return true;
    }
  }
  return false;
}

void add(char s[], boolean added) {
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);
  Serial.print("Match!\n");
  myFile = SD.open("data.log", FILE_WRITE);

  if(myFile) {
    Serial.println("Writing to file");
    myFile.print(month, DEC);
    myFile.print("/");
    myFile.print(dayOfMonth, DEC);
    myFile.print("/");
    myFile.print(year, DEC);
    myFile.print(" ");
    if(hour<10) {
      myFile.print("0");
    }
    myFile.print(hour, DEC);
    myFile.print(":");
    if(minute<10) {
      myFile.print("0");
    }
    myFile.print(minute, DEC);
    myFile.print(":");
    if(second<10) {
      myFile.print("0");
    }
    myFile.print(second, DEC);
    if (added) {
      myFile.print(" : Added new key: ");
    }
    else {
      myFile.print(" : Unlocked using key: ");
    }
    myFile.print(s);
    myFile.println(" ");
    myFile.close();
  }
  else {
    Serial.print("File couldn't be opened\n");
  }
}
 
void match(char s[]) {
  add(s, false);
  digitalWrite(A3,HIGH);
  //delay(2000);
  long time = millis();
  while (time + 2000 > millis()) {
    delay(100);
    if (id20.available()) {
      readTag();
      if (nextPass < MAX_PASSWORDS && !checkPasswords(testTag)) {
        char* temp = (char*)malloc(PASSWORD_LENGTH+1);
        if(temp){
          strcpy(temp, testTag);
          passwords[nextPass++] = temp;
          Serial.print("Added ");
          Serial.print(passwords[nextPass-1]);
          Serial.print("\n");
          add(passwords[nextPass-1], true);
        }
        else {
          Serial.print("Couldn't malloc \n");
        }
        delay(1000);
        digitalWrite(A3,LOW);
        return;
      }
      else if (nextPass == MAX_PASSWORDS){
        Serial.print("Max number of passcodes reached\n");
      }
    }
  }
  
  digitalWrite(A3,LOW);
}

void readTag() {
  
  for (int j = 0; j < 13; j++) {
    char i = id20.read();
    if (j == 1 && i != '0') {
      //Serial.print("Bad read\n");
      return;
    }
    else if (j == 1){
      Serial.print("Reading Tag: ");
    }
    Serial.print(i);
    //Serial.print(" ");
    if (j != 0 && i != (char) -1) {
      testTag[j-1] = i;
    }
  }
  Serial.print("\n");
}

void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year) {
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

byte bcdToDec(byte val) {
  return( (val/16*10) + (val%16) );
}
