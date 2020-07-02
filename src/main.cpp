#include <Arduino.h>

//! STRINGS EN
// #define STR_PRESS_A_BTN "PRESS A BUTTON"
// #define STR_MA " Ma"
// #define STR_FAIL " F A I L"
// #define STR_PRESS "    P R E S S"
// #define STR_A "        A"
// #define STR_BUTTON "   B U T T O N"
// #define STR_CONTROL_OK "CONTROLE OK"
// #define STR_NEXT "PROXIMO"
// #define STR_EMPTY "                    "
// #define STR_FAIL_CONTROL "CONTOLHE COM FALHA"
// #define STR_SEQUENCE "DE SEQUENCIA"
// #define STR_RESTART " R E S T A R T"
// #define STR_ ""
//!	STRINGS PT_BR
#define STR_PRESS_A_BTN "APERTE UM BOTAO"
#define STR_MA " Ma"
#define STR_FAIL " F A L H A"
#define STR_PRESS "    A P E R T E"
#define STR_A "        U M"
#define STR_BUTTON "     B O T A O"
#define STR_CONTROL_OK "CONTROLE OK"
#define STR_NEXT "PROXIMO"
#define STR_EMPTY "                    "
#define STR_FAIL_CONTROL "CONTOLHE COM FALHA"
#define STR_SEQUENCE "DE SEQUENCIA"
#define STR_RESTART " R E C O M E Ç A R"
#define STR_ ""

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
#define BTNS_SIZE 57      // 	quantity of buttons saved on EEPROM
#define BTNS_LCOM_SEQ 41  //	quantity of buttons on lcom sequence

//	LCD
#define LCD_COL 20
#define LCD_LIN 4
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

Adafruit_INA219 ina219;

struct Button {
    unsigned long hexa;
    char name[10];
};

void clearLCDLine(int index);
Button getBTN(unsigned int index);
bool verifySequence(int code);

const int sequence[] = {0, 48, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 15, 48, 18, 24, 20, 19, 48, 21, 26, 28, 30, 32, 31, 34, 20, 21, 49, 35, 23, 50, 5, 51, 44, 45, 46, 47, 52, 41, 42};
int seq_index;
int fail_n;

void setup() {
    Serial.begin(SERIAL_PORT);

    while (!Serial) {
        delay(1);
    }

    lcd.begin(LCD_COL, LCD_LIN);
    irrecv.enableIRIn();  // Start the receiver
    pinMode(LED_PORT, OUTPUT);
    ina219.begin();

    seq_index = 0;
    fail_n = 0;

    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(STR_PRESS_A_BTN);
}

void loop() {
    float current_mA = 0;

    bool same_btn = false;

    current_mA = ina219.getCurrent_mA();

    //! IR TEST
    if (irrecv.decode(&results)) {
        if (results.value != 4294967295) {
            clearLCDLine(0);
            clearLCDLine(1);
            clearLCDLine(2);
            String name = (String)results.value;

            lcd.setCursor(0, 0);
            lcd.print(name);

            lcd.setCursor(10, 0);
            lcd.print(results.value, HEX);

            for (unsigned int i = 0; i < BTNS_SIZE; i++) {
                Button btn = getBTN(i);
                if (results.value == btn.hexa) {
                    lcd.setCursor(0, 1);
                    lcd.print(String(i) + " - " + btn.name);
                    //	VERIFICA SE A SEQUENCIA APERTADA É CORRETA
                    verifySequence(i);
                    break;
                }
            }
        }

        irrecv.resume();
    }
    //! CURRENT TEEST
    if (current_mA > 10 and !same_btn) {
        digitalWrite(LED_PORT, LOW);

        same_btn = false;

        int index = 1;
        int values[100];
        bool ok = false;

        values[0] = current_mA;

        while (current_mA > 0 and index < 100) {
            values[index] = current_mA;
            index++;
            current_mA = ina219.getCurrent_mA();
            delay(1);
        }

        for (int i = 0; i <= index; i++) {
            if (values[i] >= MIN_CURRENT_MA) {
                current_mA = values[i];
                ok = true;
                break;
            } else if (values[i] >= current_mA) {
                current_mA = values[i];
            }
        }
        String txt = String(current_mA) + STR_MA;
        if (!ok) {
            for (int i = 0; i < 20; i++) {
                digitalWrite(LED_PORT, HIGH);
                delay(LED_TIME_ERRO);
                digitalWrite(LED_PORT, LOW);
                delay(LED_TIME_ERRO);
            }
            tone(TONE_PIN, TONE_FREQ_NEG, TONE_TIME_NEG);
            txt += STR_FAIL;
            delay(TONE_TIME_NEG);
        }
        clearLCDLine(3);
        lcd.setCursor(0, 3);

        Serial.println(txt);

        lcd.print(txt);
        if (!ok) {
            delay(1000);
            clearLCDLine(0);
            lcd.setCursor(0, 0);
            lcd.print(STR_PRESS);
            clearLCDLine(1);
            lcd.setCursor(0, 1);
            lcd.print(STR_A);
            clearLCDLine(2);
            lcd.setCursor(0, 2);
            lcd.print(STR_BUTTON);
        }
    }

    //! CHECK IF THE SAME BUTTON HAS BEEN PRESSED
    if (current_mA <= 0) {
        digitalWrite(LED_PORT, HIGH);
        same_btn = false;
    }

    //! VERIFY THE END OF THE SEQUENCE
    if (seq_index >= 40) {
        seq_index = 0;
        fail_n = 0;
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
void clearLCDLine(int lcd_index) {
    lcd.setCursor(0, lcd_index);
    lcd.print(STR_EMPTY);
}

/**
 * 		Verify if the sequence tested is the correct one.
 * 		@param code Code of the button tested in sequece.
 * 		@returns {bool} true if the code is correct or false if not correct.
*/
bool verifySequence(int code) {
    if (sequence[seq_index] == code) {
        seq_index++;
        fail_n = 0;
        tone(TONE_PIN, TONE_FREQ_POS, TONE_TIME_POS);
        return true;
    } else {
        lcd.setCursor(16, 1);
        lcd.print("FAIL");
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
        }
        return false;
    }
}