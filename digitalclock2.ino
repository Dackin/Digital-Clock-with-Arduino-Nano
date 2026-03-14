//LIBRARY
#include <Wire.h>
#include <I2C_RTC.h>

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// PIN DEFINITIONS
#define ENCODER_CLK 4
#define ENCODER_DT 5
#define ENCODER_BTN 3
#define BUZZER_PIN 6 // Pin Buzzer diaktifkan kembali

//MD Parolla MAX7219
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 10

MD_Parola layarMAX = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

//DS3231
DS3231 RTC;

//VARIABLES ENCODER & BUTTON
int encoder = 0;
int lastCLK = HIGH;
int currentCLK = HIGH;

unsigned long holdAwal = 0;
const long holdBatas = 1500; 

int buttonTerakhir = HIGH;
bool holdButton = false;

byte seconds, hours;

// VARIABLE SETTING WAKTU
int tempJam = 0;
int tempMenit = 0;
bool confirmSave = false; 

char textBuffer[15];
unsigned long lastEncoderReadTime = 0;

int sensitivitasEncoder = 15; 

// ENUM untuk mode display
enum DisplayMode {
  displayJam,
  displaySetJam,    
  displaySetMenit,  
  displaySetConfirm 
};

DisplayMode display = displayJam;

//FUNCTIONS
void jam() {
  hours = RTC.getHours(); // Format 24 Jam
}

String titikdua() {
  if (RTC.getSeconds() % 2 == 0) return ":";
  else return " ";
}

void tampilkanLayar() {
  unsigned long sekarang = millis();
  static unsigned long lastUpdate = 0;

  static int lastJam = -1, lastMenit = -1;
  static bool lastConfirm = !confirmSave; 
  static DisplayMode lastMode = displayJam;

  bool modeBerubah = (display != lastMode);
  if (modeBerubah) {
    layarMAX.displayClear(); 
    lastMode = display;
    lastUpdate = 0;          
  }

  switch (display) {
    case displayJam:
      if (sekarang - lastUpdate >= 500) {
        jam();
        String waktuBaru = String() + (hours < 10 ? " " : "") + hours
                + titikdua()
                + (RTC.getMinutes() < 10 ? "0" : "") + RTC.getMinutes();
                
        waktuBaru.toCharArray(textBuffer, 15); 
        layarMAX.displayText(textBuffer, PA_CENTER, 50, 0, PA_PRINT, PA_NO_EFFECT);
        lastUpdate = sekarang;
      }
      break;

    case displaySetJam:
      if (modeBerubah || tempJam != lastJam) { 
        sprintf(textBuffer, "H:%02d", tempJam); // Diubah menjadi H (Hour)
        layarMAX.displayText(textBuffer, PA_CENTER, 50, 0, PA_PRINT, PA_NO_EFFECT);
        lastJam = tempJam;
      }
      break;

    case displaySetMenit:
      if (modeBerubah || tempMenit != lastMenit) {
        sprintf(textBuffer, "M:%02d", tempMenit); // Tetap M (Minute)
        layarMAX.displayText(textBuffer, PA_CENTER, 50, 0, PA_PRINT, PA_NO_EFFECT);
        lastMenit = tempMenit;
      }
      break;

    case displaySetConfirm:
      if (modeBerubah || confirmSave != lastConfirm) {
        strcpy(textBuffer, confirmSave ? "SAVE Y" : "SAVE N");
        layarMAX.displayText(textBuffer, PA_CENTER, 50, 0, PA_PRINT, PA_NO_EFFECT);
        lastConfirm = confirmSave;
      }
      break;
  }
  
  layarMAX.displayAnimate();
}

void readButton() {
  unsigned long sekarang = millis();
  int stateRaw = digitalRead(ENCODER_BTN); 

  static int state = HIGH;
  static unsigned long lastDebounceTime = 0;
  static int lastRawState = HIGH;

  if (stateRaw != lastRawState) {
    lastDebounceTime = sekarang;
  }

  if ((sekarang - lastDebounceTime) > 50) {
    if (stateRaw != state) {
      state = stateRaw;

      if (state == LOW) {
        holdAwal = sekarang;
        holdButton = false;
      }

      if (state == HIGH && !holdButton) {
        // --- LOGIKA CLICK (Siklus Setting) ---
        if (display == displaySetJam){
            display = displaySetMenit;
        }
        else if (display == displaySetMenit){
            display = displaySetConfirm;
        }
        else if (display == displaySetConfirm){
            display = displaySetJam;
        }
      }
    }
  }
  
  lastRawState = stateRaw;

  // 2. LOGIKA HOLD (Masuk & Keluar Mode Setting)
  if (state == LOW && !holdButton) {
    if (sekarang - holdAwal > holdBatas) {
      holdButton = true; 

      if (display == displayJam){
        display = displaySetJam;
        tempJam = 0;   
        tempMenit = 0; 
        confirmSave = false; 
      } 
      else if (display >= displaySetJam && display <= displaySetConfirm) {
        if (confirmSave) {
            RTC.setHours(tempJam);
            RTC.setMinutes(tempMenit);
            RTC.setSeconds(0); 
            
            // Bunyikan buzzer sebagai notifikasi sukses tersimpan (1500 Hz, 300 ms)
            tone(BUZZER_PIN, 1500, 300); 
        }
        display = displayJam;
      }
    }
  }
}

void readHandle() {
  if (encoder == 0) return;
  
  int encoderChange = encoder;
  encoder = 0; 

  switch (display) {
    case displaySetJam:
      tempJam += encoderChange;
      if (tempJam > 23) tempJam = 0; 
      if (tempJam < 0) tempJam = 23;
      break;
      
    case displaySetMenit:
      tempMenit += encoderChange;
      if (tempMenit > 59) tempMenit = 0;
      if (tempMenit < 0) tempMenit = 59;
      break;
      
    case displaySetConfirm:
      if (encoderChange != 0) confirmSave = !confirmSave;
      break;
      
    default:
      break;
  }
}

void readEncoderPolling() {
  currentCLK = digitalRead(ENCODER_CLK);
  if (currentCLK != lastCLK && currentCLK == LOW) {
    unsigned long sekarang = millis();
    if (sekarang - lastEncoderReadTime > sensitivitasEncoder) {
      delay(1);
      if (digitalRead(ENCODER_CLK) == LOW) { 
        
        int dtValue = digitalRead(ENCODER_DT);
        if (dtValue == LOW) encoder++;
        else encoder--;
        
        lastEncoderReadTime = sekarang;
      }
    }
  }
  lastCLK = currentCLK;
}

void setup() {
  RTC.begin();
  RTC.setHourMode(CLOCK_H24); 
  
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_BTN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT); // Inisialisasi pin Buzzer
  
  lastCLK = digitalRead(ENCODER_CLK);

  layarMAX.begin();
  layarMAX.setIntensity(1);  
  layarMAX.displayClear();
}

void loop() {
  readEncoderPolling();
  readButton();
  readHandle();
  tampilkanLayar();
}
