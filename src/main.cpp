#include <Arduino.h>

//  Includes for IR
#include <EEPROM.h>
#include <IRremote.h>

//  Includes for Current Test
#include <Wire.h>
#include <Adafruit_INA219.h>

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

unsigned int lcom_map[54] = {0, 1, 54, 2, 28, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 15, 53, 18, 19, 22, 23, 24, 20, 21, 50, 26, 51, 35, 30, 36, 32, 33, 34, 4, 31, 55, 52, 56, 39, 40, 41, 42, 43, 44, 45, 46, 47};
int index;


void setup(){
  Serial.begin(SERIAL_PORT);

  
  while (!Serial) {
        delay(1);
  }

  startControl();

  irrecv.enableIRIn(); // Start the receiver

  pinMode(LED_PORT,OUTPUT);
  
  ina219.begin();
  index = 0;


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

          if(lcom_map[i] != i){
            Serial.print(" - ");
            Serial.print(" WRONG ");

          }else{
            index++;
          }

          break;
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
    for (int i = 0; i < BTNS_SIZE; i++){
      btns[i] = getBTN(i);
    }
}

Botao getBTN(unsigned int index){
  Botao btn;
  Serial.print( "index ");Serial.print( index + 1);
  
  index = index * sizeof(Botao) + sizeof(unsigned int);
  EEPROM.get(index, btn);
  Serial.print( "  - ") ;Serial.println( btn.name);
  delay(20);
  return btn;
}