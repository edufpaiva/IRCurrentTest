#include <Arduino.h>

//! STRINGS EN
// #define STR_PRESS_A_BTN "PRESS A BUTTON"
// #define STR_MA " Ma"
// #define STR_FAIL_SPLIT " F A I L"
// #define STR_PRESS "    P R E S S"
// #define STR_A "        A"
// #define STR_BUTTON "   B U T T O N"
// #define STR_CONTROL_OK "CONTROLE OK"
// #define STR_NEXT "PROXIMO"
// #define STR_EMPTY "                    "
// #define STR_FAIL_CONTROL "CONTOLHE COM FALHA"
// #define STR_SEQUENCE "DE SEQUENCIA"
// #define STR_RESTART " R E S T A R T"
// #define STR_NAME "Name : "
// #define STR_ ""

//!	STRINGS PT_BR
#define STR_ ""
#define STR_A "        U M"
#define STR_MA " Ma"
#define STR_NAME "Nome : "
#define STR_FAIL "FALHA"
#define STR_NEXT "PROXIMO"
#define STR_PRESS "    A P E R T E"
#define STR_EMPTY "                    "
#define STR_BUTTON "     B O T A O"
#define STR_RESTART " R E C O M E Ç A R"
#define STR_SEQUENCE "DE SEQUENCIA"
#define STR_CONTROL_OK "CONTROLE OK"
#define STR_SEQUENCE_OK "SEQUENCIA OK"
#define STR_FAIL_SPLIT " FALHA"
#define STR_PRESS_A_BTN "APERTE UM BOTAO"
#define STR_FAIL_CONTROL "CONTOLHE COM FALHA"

//  Includes for IR
#include <EEPROM.h>
#include <IRremote.h>

//  Includes for Current Test
#include <Adafruit_INA219.h>
#include <Wire.h>

//  Include LCD
#include <LiquidCrystal.h>

//  Constants for IR
#define IR_PIN 9
#define IR_SEQUENCE 41
IRrecv irrecv(IR_PIN);
decode_results results;

//  Current
#define MIN_CURRENT_MA 100

//	LED
#define LED_PORT 13
#define LED_TIME_ERRO 20

//	TONE
#define TONE_PIN 6
#define TONE_TIME_POS 200
#define TONE_FREQ_POS 200  // between 150 and 200
#define TONE_FREQ_NEG 50
#define TONE_TIME_NEG 1000

#define SERIAL_PORT 115200
#define BTNS_SIZE 58      // 	quantity of buttons saved on EEPROM
#define BTNS_LCOM_SEQ 44  //	quantity of buttons on lcom sequence

//	LCD
#define LCD_COL 20
#define LCD_LIN 4
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

Adafruit_INA219 ina219;

/**
 *      @param hexa The code of the button.
 *      @param name The name of the button.
 * 
 * */
struct Button {
    unsigned long hexa;
    char name[10];
};

void clearLCDLine(unsigned int index);
bool verifySequence(int code);
Button getBTN(unsigned int index);
void clearLCDLine(unsigned int index, unsigned int lcd_index_fin);

const int sequence[] = {0, 48, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 15, 48, 18, 24, 20, 19, 48, 21, 26, 28, 30, 32, 31, 34, 20, 21, 49, 35, 23, 50, 5, 51, 44, 45, 46, 47, 52, 41, 42, 48, 49, 0};
int seq_index;
int fail_n;
bool sequence_ok = false;

float vCurrent[10];
int index_current = 0;

void setup() {
    Serial.begin(SERIAL_PORT);

    while (!Serial) {
        delay(1);
    }

    //  Start the lcd with num of cols and the num of lines
    lcd.begin(LCD_COL, LCD_LIN);

    //  Start the IR receiver
    irrecv.enableIRIn();

    //  Start a led
    pinMode(LED_PORT, OUTPUT);

    //  Start evasive current sensor
    ina219.begin();

    seq_index = 0;
    fail_n = 0;

    //  Clear the lcd and prints default message
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(STR_PRESS_A_BTN);
}

void loop() {
    float current_mA = 0;

    current_mA = ina219.getCurrent_mA();

    vCurrent[index_current] = current_mA;
    index_current++;
    if (index_current >= 10) index_current = 0;
    Serial.println(current_mA);
    //! IR TEST
    if (irrecv.decode(&results)) {
        if (results.value != 4294967295) {
            clearLCDLine(0, 2);
            String name = (String)results.value;

            lcd.setCursor(0, 0);
            lcd.print(name);

            lcd.setCursor(10, 0);
            lcd.print(results.value, HEX);

            Serial.println(name + " " + String(results.value, HEX));

            for (unsigned int i = 0; i < BTNS_SIZE; i++) {
                Button btn = getBTN(i);
                if (results.value == btn.hexa) {
                    lcd.setCursor(0, 1);
                    lcd.print(STR_NAME + String(btn.name));
                    //	VERIFICA SE A SEQUENCIA APERTADA É CORRETA
                    if (verifySequence(i)) {
                        seq_index++;
                        fail_n = 0;
                        tone(TONE_PIN, TONE_FREQ_POS, TONE_TIME_POS);
                        lcd.setCursor(0, 2);
                        lcd.print("Next : " + String(getBTN(sequence[seq_index]).name));
                    } else {
                        lcd.setCursor(16, 1);
                        lcd.print(STR_FAIL_SPLIT);
                        lcd.setCursor(0, 2);
                        lcd.print("Expec: " + String(getBTN(sequence[seq_index]).name));
                        fail_n++;
                        if (fail_n >= 3) {
                            tone(TONE_PIN, TONE_FREQ_NEG, TONE_TIME_NEG);
                            lcd.clear();
                            lcd.setCursor(0, 0);
                            lcd.print(STR_FAIL_CONTROL);
                            lcd.setCursor(0, 1);
                            lcd.print(STR_SEQUENCE);
                            delay(3000);
                            lcd.setCursor(2, 2);
                            lcd.print(STR_RESTART);
                            seq_index = 0;
                            fail_n = 0;
                            delay(1);
                            lcd.clear();
                            lcd.print(STR_PRESS_A_BTN);
                        } else {
                            tone(TONE_PIN, TONE_FREQ_NEG, TONE_TIME_POS);
                        }
                    }
                    break;
                }
            }
        }

        irrecv.resume();
    }

    //! CURRENT TEEST
    if (current_mA > 2) {
        digitalWrite(LED_PORT, LOW);
        clearLCDLine(3);
        lcd.setCursor(0, 3);

        int qtd = 0;
        float ctemp = 0;
        for (size_t i = 0; i < 10; i++) {
            if (vCurrent[i] > 2) {
                qtd++;
                ctemp += vCurrent[i];
            }
        }

        lcd.print(String(ctemp / qtd) + STR_MA);
        if (seq_index >= IR_SEQUENCE)
            delay(400);
        else
            delay(100);
    }

    //! VERIFY THE END OF THE SEQUENCE
    if (seq_index == IR_SEQUENCE && !sequence_ok) {
        tone(TONE_PIN, TONE_FREQ_POS, TONE_TIME_POS);
        delay(500);
        clearLCDLine(0, 2);
        lcd.setCursor(0, 0);
        lcd.print(STR_SEQUENCE_OK);
        lcd.setCursor(0, 1);
        lcd.print("PAREAR CONTROLE");
        lcd.setCursor(0, 2);
        lcd.print("Next : Microfone");

        sequence_ok = true;
    }

    //! VERIFY THE END OF THE SEQUENCE
    if (seq_index >= BTNS_LCOM_SEQ) {
        tone(TONE_PIN, TONE_FREQ_POS, TONE_TIME_POS);
        delay(TONE_TIME_POS);
        seq_index = 0;
        fail_n = 0;
        sequence_ok = false;
        delay(1000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(STR_CONTROL_OK);
        lcd.setCursor(0, 2);
        delay(1000);
        lcd.print(STR_NEXT);
    }
};

/**
 *  	Get a Button from EEPROM.
 * 		@param index The index of btn in EEPROM.
 * 		@returns {Botao} Returns a Button object.
*/
Button getBTN(unsigned int btn_index) {
    Button btn;

    btn_index = btn_index * sizeof(Button) + sizeof(unsigned int);
    EEPROM.get(btn_index, btn);

    return btn;
}

/**
 * 		Clear a single line from liquid cristal display.
 * 		@param lcd_index The index of lcd to clear.
*/
void clearLCDLine(unsigned int lcd_index) {
    lcd.setCursor(0, lcd_index);
    lcd.print(STR_EMPTY);
}

/**
 * 		Clear lines from liquid cristal display from a start index to a final index.
 *      0 <= lcd_index <= lcd_index_fin < lcd total lines.
 * 		@param lcd_index     The index of lcd to clear.
 * 		@param lcd_index_fin The final index of lcd to clear.
*/
void clearLCDLine(unsigned int lcd_index, unsigned int lcd_index_fin) {
    for (uint8_t i = 0; i <= lcd_index_fin; i++) {
        lcd.setCursor(0, i);
        lcd.print(STR_EMPTY);
    }
}

/**
 * 		Verify if the sequence tested is the correct one.
 * 		@param code Code of the button tested in sequece.
 * 		@returns {bool} true if the code is correct or false if not correct.
*/
bool verifySequence(int code) {
    if (sequence[seq_index] == code) {
        return true;
    } else {
        return false;
    }
}
