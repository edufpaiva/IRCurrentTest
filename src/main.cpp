#include <Arduino.h>

//  Includes for IR
#include <EEPROM.h>
#include <IRremote.h>

//  Includes for Current Test
#include <Wire.h>
#include <Adafruit_INA219.h>

//  Include LCD
#include <LiquidCrystal.h>

//  Constants for IR
#define BTNS_SIZE 57
#define LED_PORT 13
#define SERIAL_PORT 115200
#define RECV_PIN  9

//  Constants for Current
#define TONE_PIN 6
#define TONE_DURATION 1000
#define TONE_FREQ 500
#define LED_TIME_ERRO 20
#define MIN_CURRENT_MA 100

Adafruit_INA219 ina219;

struct Botao{
  unsigned long hexa;
  char name[10];
};


void startControl();
Botao getBTN(unsigned int index);


IRrecv irrecv(RECV_PIN);

decode_results results;

Botao btns[BTNS_SIZE];

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup(){
  Serial.begin(SERIAL_PORT);

  while (!Serial) {
    delay(1);
  }

  lcd.begin(20, 4);
  delay(100);

  startControl();
  delay(100);

  irrecv.enableIRIn(); // Start the receiver


  pinMode(LED_PORT,OUTPUT);
  
  ina219.begin();

  Serial.println("\n\n\n\tPressione alguma tecla do controle");

};

void loop() {
  if (irrecv.decode(&results)) {
    if(results.value != 4294967295){
      String name = (String) results.value;
      Serial.print(name);

      for (unsigned int i = 0; i < BTNS_SIZE; i++){
        if(results.value == btns[i].hexa){
          Serial.print(" - ");
          Serial.print(results.value, HEX);
          Serial.print(" - ");
          Serial.print(i);
          Serial.print(" - ");
          Serial.print(btns[i].name);
        }
      }
      
      Serial.println("");
    }
    
    irrecv.resume(); // Receive the next value
  }

  float current_mA = 0;
    
  bool same_btn = false;

  current_mA = ina219.getCurrent_mA();
  
  if (current_mA > 10 and !same_btn){
      Serial.println("      .                              .");
      digitalWrite(LED_PORT,LOW);

      same_btn = false;

      int index = 1;
      int values[100];
      bool ok = false;

      values[0] = current_mA;

      while( current_mA > 0 and index < 100){
          values[index] = current_mA;
          index++;
          // Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
          current_mA = ina219.getCurrent_mA();
          delay(20);
      }

      for(int i = 0; i <= index; i++){
          if(values[i] >= MIN_CURRENT_MA) {
              current_mA = values[i];
              ok = true;
              break;
          }else if(values[i] >= current_mA) {
              current_mA = values[i];
          }
      }
      if(!ok){
          // tone(TONE_PIN, TONE_FREQ, TONE_DURATION); 
          
          for(int i = 0; i < 20; i++){
              digitalWrite(LED_PORT,HIGH);
              delay(LED_TIME_ERRO);
              digitalWrite(LED_PORT,LOW);
              delay(LED_TIME_ERRO);
          }
          delay(TONE_DURATION);
          Serial.print("Current: _ _ _ _ F A I L _ _ _ _"); Serial.print(current_mA); Serial.println(" mA");
      }else{
          Serial.print("Current: _ _ _ _ _ _ _ _ _ _ _ _"); Serial.print(current_mA); Serial.println(" mA");
      }
  }
  if (current_mA <= 0 ) {
      digitalWrite(LED_PORT, HIGH);
      same_btn = false;
  }
};

void startControl(){
    lcd.print("LOADING CONTROL");
    delay(500);
    for (int i = 0; i < BTNS_SIZE; i++){
      btns[i] = getBTN(i);
    }
    delay(500);
}

Botao getBTN(unsigned int index){
  Botao btn;
  String txt = "";
  Serial.print( "index ");Serial.print( index + 1);
  
  
  txt += "index ";
  txt += index + 1;

  index = index * sizeof(Botao) + sizeof(unsigned int);
  EEPROM.get(index, btn);
  Serial.print( "  - ") ;Serial.println( btn.name);

  txt += "  - ";
  txt += btn.name;
  
  lcd.print(txt);

  delay(30);
  return btn;
}