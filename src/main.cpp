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
// void verifica_sequencia(int code);

IRrecv irrecv(RECV_PIN);

decode_results results;


// const int sequencia[] = {0, 48, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 15, 48, 18, 24, 20, 19, 48, 21, 26, 28, 30, 32, 31, 34, 20, 21, 49, 35, 23, 50, 5, 51, 44, 45, 46, 47, 52, 41, 42};
// int index;
// int falhas;

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup(){
  Serial.begin(SERIAL_PORT);
  

  while (!Serial) {
    delay(1);
  }

  lcd.begin(20, 4);
  irrecv.enableIRIn(); // Start the receiver
  pinMode(LED_PORT,OUTPUT);
  ina219.begin();

  // index = 0;
  // falhas = 0;

  lcd.clear();  
  lcd.setCursor(0,1);
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
      
      lcd.setCursor(0,1);
      lcd.print(results.value, HEX);

      for (unsigned int i = 0; i < BTNS_SIZE; i++){
        Botao btn = getBTN(i);
        if(results.value == btn.hexa){
          lcd.setCursor(0,2);
          lcd.print(String(i) + " - " + btn.name);
          // verifica_sequencia(i);
          break;
        }
      }
    }

    irrecv.resume(); 
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
          delay(1);
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

Botao getBTN(unsigned int index){

  Botao btn;
  
  index = index * sizeof(Botao) + sizeof(unsigned int);
  EEPROM.get(index, btn);

  return btn;
}

void limpaLCD(int index){
  lcd.setCursor(0,index);
  lcd.print("                    ");
}

// void verifica_sequencia(int code){
//   Serial.println(String(code));
//   if (sequencia[index] == code){
//     index++;
//     falhas = 0;
//   }else{
//     lcd.setCursor(13,2);
//     lcd.print("FAIL");
//     falhas++;
//     if(falhas >= 3){
//       lcd.clear();
//       lcd.setCursor(0,0);
//       lcd.print("CONTOLHE COM FALHA");
//       lcd.setCursor(0,1);
//       lcd.print("DE SEQUENCIA");
//       delay(3000);
//       lcd.setCursor(2,2);
//       lcd.print(" R E S T A R T");
//     }
//   }

// }