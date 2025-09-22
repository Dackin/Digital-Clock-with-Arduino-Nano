//LIBRARY
#include <Wire.h>
#include <I2C_RTC.h>

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

//FONT
//#include <myFont.h>

//ROTARY ENCODER
#define ENCODER_CLK 4
#define ENCODER_DT 5
#define ENCODER_BTN 3

//MD Parolla MAX7219
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 10

MD_Parola layarMAX = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

//DS3231
DS3231 RTC;

//VARIABLES ENCODER
int encoder = 0;
int lastCLK = HIGH;
int currentCLK = HIGH;

unsigned long holdAwal = 0;
const long holdBatas = 1000;

int buttonTerakhir = HIGH;
bool holdButton = false;

bool onoff = false;
bool serialDebug = false; // Setting untuk enable/disable serial debug

byte seconds, hours, jamAlarm = 12, menitAlarm = 0;
String waktu = "0:00";

// ENUM untuk mode display
enum DisplayMode {
  displayJam,
  displayTgl,
  displayAlarm,
  displayAlarmJam,
  displayAlarmMenit,
  displayAlarmOn,
  displaySetting
};
DisplayMode display = displayJam;

//FUNCTIONS
void jam() {
  if (RTC.getHours() == 12 && RTC.getMeridiem() == HOUR_AM) {
    hours = 0;
  } else {
    hours = RTC.getHours();
  }
}

String titikdua() {
  if (RTC.getSeconds() % 2 == 0) {
    return ":";
  } else {
    return " ";
  }
}

String ampm() {
  if (RTC.getMeridiem() == HOUR_AM) {
    return "AM";
  } else {
    return "PM";
  }
}

void tampilkanLayar() {
  unsigned long sekarang = millis();
  static unsigned long lastUpdate = 0;

  switch (display) {
    case displayJam:
      if (sekarang - lastUpdate >= 500) {
        jam(); // update hours
        waktu = String() + (hours < 10 ? " " : "") + hours
                + titikdua()
                + (RTC.getMinutes() < 10 ? "0" : "") + RTC.getMinutes();

        layarMAX.displayClear();
        layarMAX.displayText(
          waktu.c_str(),
          PA_CENTER,
          50,
          0,
          PA_PRINT,
          PA_NO_EFFECT
        );

        if (serialDebug) {
          Serial.println("JAM: " + waktu + " " + ampm());
        }
        lastUpdate = sekarang;
      }
      break;

    case displayTgl:
      if (sekarang - lastUpdate >= 1000) {
        String tanggal = String(RTC.getDay()) + "/"
                       + String(RTC.getMonth()) + "/"
                       + String(RTC.getYear());

        layarMAX.displayClear();
        layarMAX.displayText(
          tanggal.c_str(),
          PA_CENTER,
          50,
          0,
          PA_PRINT,
          PA_NO_EFFECT
        );

        if (serialDebug) {
          Serial.println("TGL: " + tanggal);
        }
        lastUpdate = sekarang;
      }
      break;

    case displayAlarm:
      // tampilkan status alarm
      layarMAX.displayClear();
      layarMAX.displayText(
        "ALARM",
        PA_CENTER,
        50,
        0,
        PA_PRINT,
        PA_NO_EFFECT
      );
      break;

    case displayAlarmJam:
      // tampilkan jam alarm
      {
        String jamText = "J:" + String(jamAlarm);
        layarMAX.displayClear();
        layarMAX.displayText(
          jamText.c_str(),
          PA_CENTER,
          50,
          0,
          PA_PRINT,
          PA_NO_EFFECT
        );
      }
      break;

    case displayAlarmMenit:
      // tampilkan menit alarm
      {
        String menitText = "M:" + String(menitAlarm);
        layarMAX.displayClear();
        layarMAX.displayText(
          menitText.c_str(),
          PA_CENTER,
          50,
          0,
          PA_PRINT,
          PA_NO_EFFECT
        );
      }
      break;

    case displayAlarmOn:
      // tampilkan ON/OFF alarm
      layarMAX.displayClear();
      layarMAX.displayText(
        onoff ? "ON" : "OFF",
        PA_CENTER,
        50,
        0,
        PA_PRINT,
        PA_NO_EFFECT
      );
      break;

    case displaySetting:
      // tampilkan tulisan setting
      layarMAX.displayClear();
      layarMAX.displayText(
        "SET",
        PA_CENTER,
        50,
        0,
        PA_PRINT,
        PA_NO_EFFECT
      );
      break;
  }

  // WAJIB untuk refresh display
  layarMAX.displayAnimate();
}


void readButton() {
  unsigned long sekarang = millis();
  int state = digitalRead(ENCODER_BTN);

  // Deteksi tombol ditekan
  if (state == LOW && buttonTerakhir == HIGH) {
    holdAwal = sekarang;   // mulai hitung waktu tekan
    holdButton = false;    // reset status hold
  }

  // Jika tombol sedang ditekan
  if (state == LOW && !holdButton) {
    if (sekarang - holdAwal > holdBatas) {
      if (serialDebug) Serial.println(">>> HOLD: Masuk/Keluar Setting <<<");
      holdButton = true;         // tandai sudah hold

      //masuk ke display setalarm
      if (display == displayJam || display == displayTgl){
        display = displayAlarmJam;
        if (serialDebug) Serial.println("Masuk ke setting alarm - Atur JAM");

      //masuk ke setting debug dari display setting
      } else if (display == displaySetting) {
        display = displayJam;
        if (serialDebug) Serial.println("Keluar dari setting");
        
      //keluar dari display setalarm
      } else {
        // Update alarm RTC sebelum keluar
        RTC.setAlarm1(jamAlarm, menitAlarm, 0);
        if (onoff) {
          RTC.enableAlarm1();
        } else {
          RTC.disableAlarm1();
        }
        
        display = displayJam;
        if (serialDebug) Serial.println("Keluar dari setting alarm");
      }
    }
  }

  // Deteksi tombol dilepas
  if (state == HIGH && buttonTerakhir == LOW) {
    if (!holdButton) {
      if (serialDebug) Serial.println(">>> CLICK: Ganti Display <<<");

      //display normal
      if (display == displayJam) {
        display = displayTgl;
        if (serialDebug) Serial.println("Tampilkan tanggal");
      } 
      else if (display == displayTgl) {
        display = displaySetting;
        if (serialDebug) Serial.println("Tampilkan setting");
      }
      else if (display == displaySetting) {
        display = displayJam;
        if (serialDebug) Serial.println("Tampilkan jam");
      }

      //display pas setting alarm
      else if (display == displayAlarmJam){
        display = displayAlarmMenit;
        if (serialDebug) Serial.println("Setting alarm - Atur MENIT");
      }
      else if (display == displayAlarmMenit){
        display = displayAlarmOn;
        if (serialDebug) Serial.println("Setting alarm - Atur ON/OFF");
      }
      else if (display == displayAlarmOn){
        display = displayAlarmJam;
        if (serialDebug) Serial.println("Setting alarm - Atur JAM");
      }
    }
  }

  // Simpan state tombol untuk pembacaan berikutnya
  buttonTerakhir = state;
}

void readHandle() {
  // Cek apakah ada perubahan encoder
  if (encoder == 0){
    return;
  }

  int encoderChange = encoder;
  encoder = 0; // Reset counter setelah diproses

  switch (display) {
    case displayAlarmJam:
      jamAlarm += encoderChange;
      // Validasi range jam (0-23)
      if (jamAlarm > 23) jamAlarm = 0;
      if (jamAlarm < 0) jamAlarm = 23;
      
      if (serialDebug) {
        Serial.print(">>> JAM ALARM: ");
        Serial.println(jamAlarm);
      }
      break;
      
    case displayAlarmMenit:
      menitAlarm += encoderChange;
      // Validasi range menit (0-59)
      if (menitAlarm > 59) menitAlarm = 0;
      if (menitAlarm < 0) menitAlarm = 59;
      
      if (serialDebug) {
        Serial.print(">>> MENIT ALARM: ");
        Serial.println(menitAlarm);
      }
      break;
      
    case displayAlarmOn:
      // Toggle on/off dengan setiap putaran encoder
      if (encoderChange != 0) {
        onoff = !onoff;
        if (serialDebug) {
          Serial.print(">>> ALARM STATUS: ");
          Serial.println(onoff ? "ON" : "OFF");
        }
      }
      break;
      
    case displaySetting:
      // Toggle serial debug dengan setiap putaran encoder
      if (encoderChange != 0) {
        serialDebug = !serialDebug;
        // Selalu tampilkan perubahan setting ini
        Serial.print(">>> SERIAL DEBUG: ");
        Serial.println(serialDebug ? "ON" : "OFF");
      }
      break;
  }
}

// Fungsi polling encoder - Yang sudah bekerja!
void readEncoderPolling() {
  currentCLK = digitalRead(ENCODER_CLK);
  
  // Jika CLK berubah dari HIGH ke LOW (falling edge)
  if (currentCLK != lastCLK && currentCLK == LOW) {
    int dtValue = digitalRead(ENCODER_DT);
    
    if (dtValue == LOW) {
      encoder++;
      // Hanya tampilkan debug jika serial debug aktif dan sedang setting
      if (serialDebug && (display >= displayAlarmJam && display <= displaySetting)) {
        Serial.println(">>> ENCODER: CLOCKWISE <<<");
      }
    } else {
      encoder--;
      // Hanya tampilkan debug jika serial debug aktif dan sedang setting
      if (serialDebug && (display >= displayAlarmJam && display <= displaySetting)) {
        Serial.println(">>> ENCODER: COUNTER-CLOCKWISE <<<");
      }
    }
    
    delay(2); // Debounce delay
  }
  
  lastCLK = currentCLK;
}

void setup() {
  RTC.begin();
  Serial.begin(9600);
  RTC.setHourMode(CLOCK_H12);

  // Setup pins dengan pull-up internal
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_BTN, INPUT_PULLUP);

  // Baca initial state encoder
  lastCLK = digitalRead(ENCODER_CLK);

  layarMAX.begin();
  layarMAX.setIntensity(1);   // brightness
  layarMAX.displayClear();


  // Initialize alarm values
  jamAlarm = 6;
  menitAlarm = 0;
  onoff = true;

  // Set alarm di RTC
  RTC.setAlarm1(jamAlarm, menitAlarm, 0);
  RTC.enableAlarm1();
  RTC.enableAlarmPin();

  Serial.println("=== DIGITAL CLOCK WITH ALARM ===");
  Serial.println("Operasi:");
  Serial.println("- CLICK: Ganti tampilan jam/tanggal");
  Serial.println("- HOLD: Masuk/keluar setting alarm");
  Serial.println("- PUTAR: Ubah nilai saat setting alarm");
  Serial.println("===============================");
}

void loop() {
  unsigned long sekarang = millis();
  hours = RTC.getHours();
  seconds = RTC.getSeconds();

  // Update jam
  jam();

  // Cek alarm
  if (RTC.isAlarm1Tiggered()) {
    display = displayAlarm;
    if (serialDebug) Serial.println(">>> ALARM BERBUNYI! Tekan tombol untuk matikan <<<");
    
    // Matikan alarm jika tombol ditekan
    if (digitalRead(ENCODER_BTN) == LOW){
      RTC.clearAlarm1();
      display = displayJam;
      if (serialDebug) Serial.println(">>> ALARM DIMATIKAN <<<");
      delay(500); // Prevent bouncing
    }
  } 

  // Baca input
  readEncoderPolling();  // Gunakan polling method yang sudah bekerja
  readButton();
  readHandle();
  tampilkanLayar();      // Tampilkan ke MAX7219

  delay(50);
}