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
void limpaLCD(int index);
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
    
  lcd.clear();  
  lcd.setCursor(0,3);
  lcd.print("PRESS A BUTTON");

}

void loop() {

  float current_mA = 0;

  bool same_btn = false;

  current_mA = ina219.getCurrent_mA();

  //! TESTE IR
  if (irrecv.decode(&results)) {
    if(results.value != 4294967295){
      limpaLCD(0);
      limpaLCD(1);
      limpaLCD(2);
      String name = (String) results.value;
      
      lcd.setCursor(0, 0);
      lcd.print(name);

      for (unsigned int i = 0; i < BTNS_SIZE; i++){
        if(results.value == btns[i].hexa){
          
          lcd.setCursor(0,1);
          lcd.print(results.value, HEX);

          lcd.setCursor(0,2);
          lcd.print(btns[i].name);

          break;
        }
      }
    }

    irrecv.resume(); // Receive the next value
  }

  //! TESTE DE CORRENTE
  if (current_mA > 10 and !same_btn){
      
      digitalWrite(LED_PORT,LOW);

      same_btn = false;

      int index = 1;
      int values[100];
      bool ok = false;

      values[0] = current_mA;

      while( current_mA > 0 and index < 100){
          values[index] = current_mA;
          index++;
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
      String txt = String(current_mA) + " mA";
      if(!ok){
          
          for(int i = 0; i < 20; i++){
              digitalWrite(LED_PORT,HIGH);
              delay(LED_TIME_ERRO);
              digitalWrite(LED_PORT,LOW);
              delay(LED_TIME_ERRO);
          }
          delay(TONE_DURATION);
          txt += "  F A I L";   
      }
      limpaLCD(3);
      lcd.setCursor(0,3);

      Serial.println(txt);
      
      lcd.print(txt);
      

      if(!ok){
        delay(1000);
        limpaLCD(0);
        lcd.setCursor(0,0);
        lcd.print("    P R E S S");
        limpaLCD(1);
        lcd.setCursor(0,1);
        lcd.print("        A");
        limpaLCD(2);
        lcd.setCursor(0,2);
        lcd.print("   B U T T O N");
      }
  }

  //! VERIFICA SE O MESMO BOTAO FOI APERTADO
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
    lcd.setCursor(0,0);
    lcd.clear();
    lcd.print("COMPLETE");

}

Botao getBTN(unsigned int index){
  lcd.clear();
  Botao btn;
  
  lcd.setCursor(0,0);
  lcd.print("index " + String(index + 1));

  index = index * sizeof(Botao) + sizeof(unsigned int);
  EEPROM.get(index, btn);
  
  lcd.setCursor(0,1);
  lcd.print(btn.name);

  delay(30);
  return btn;
}

void limpaLCD(int index){
  lcd.setCursor(0,index);
  lcd.print("                    ");
}