/*
📘 Sistem Otomatisasi Smartfarm Hidroponik (Versi 1 Relay)
Sensor    Parameter       Kondisi            Relay Aktif
TDS       < 500 ppm        Air terlalu encer  Pompa Nutrisi (Relay 1)
TDS       > 1200 ppm       Terlalu pekat      Pompa OFF

⚙️ Pin ESP32
Komponen        Pin     Keterangan
DS18B20         GPIO 4  Sensor suhu air
DHT22 / DHT11   GPIO 5  Sensor suhu & kelembapan udara
Sensor pH       GPIO 34 Input analog
Sensor TDS      GPIO 35 Input analog
Relay Nutrisi   GPIO 25 Pompa Nutrisi (Pompa Grothen 1)
LCD I2C         SDA 21, SCL 22 Tampilan data

⚡ Wiring Pompa Grothen ke Relay
----------------------------------------------------------
Pompa Grothen (Pompa Nutrisi):
- ESP32 GPIO25 → IN1 modul relay
- COM relay → ke kabel fase 220V AC
- NO relay  → ke kabel fase masuk ke Pompa Grothen
- Netral listrik langsung ke pompa

⚠️ Catatan Keamanan:
- Gunakan modul relay dengan optocoupler dan isolasi yang baik.
- Jangan pasang pompa Grothen langsung ke ESP32.
- Gunakan terminal box atau stop kontak berpengaman untuk sambungan AC.
*/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

// ============================
// Konfigurasi Pin Sensor
// ============================
#define PH_PIN 35
#define TDS_PIN 34
#define ONE_WIRE_BUS 26
#define DHTPIN 32
#define DHTTYPE DHT11   // Ganti ke DHT11 jika pakai DHT11

// ============================
// Konfigurasi Relay
// ============================
// Relay 1 untuk Pompa Nutrisi (Pompa Grothen 1)
#define RELAY_TDS 23

// ============================
// Inisialisasi Sensor
// ============================
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensorAir(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ============================
// Variabel Global
// ============================
float suhuAir = 0.0;
float tdsValue = 0.0;
float phValue = 0.0;
float suhuUdara = 0.0;
float kelembapanUdara = 0.0;

// Kalibrasi sensor
float offsetPH = 0.00;

// Batas kendali otomatis
const float TDS_LOW = 500.0;
const float TDS_HIGH = 1200.0;

// ============================
// Fungsi Setup
// ============================
void setup() {
  Serial.begin(115200);
  Serial.println("🔌 Inisialisasi Sistem Smartfarm Hidroponik...");

  // Inisialisasi sensor
  sensorAir.begin();
  dht.begin();
  lcd.init();
  lcd.backlight();

  // Inisialisasi relay untuk pompa Grothen
  pinMode(RELAY_TDS, OUTPUT);
  digitalWrite(RELAY_TDS, HIGH);  // relay off

  lcd.setCursor(0, 0);
  lcd.print("Smartfarm Ready");
  lcd.setCursor(0, 1);
  lcd.print("Inisialisasi...");
  delay(1500);
  lcd.clear();

  Serial.println("✅ Sistem siap digunakan!");
  Serial.println("==============================");
}

// ============================
// Fungsi Baca Suhu Air
// ============================
float bacaSuhuAir() {
  sensorAir.requestTemperatures();
  return sensorAir.getTempCByIndex(0);
}

// ============================
// Fungsi Baca Sensor DHT (Udara)
// ============================
void bacaDHT() {
  suhuUdara = dht.readTemperature();
  kelembapanUdara = dht.readHumidity();

  if (isnan(suhuUdara) || isnan(kelembapanUdara)) {
    Serial.println("⚠️ Gagal baca sensor DHT!");
    suhuUdara = 0;
    kelembapanUdara = 0;
  }
}

// ============================
// Fungsi Baca Sensor pH (untuk monitoring saja)
// ============================
float bacaPH() {
  int buffer_arr[10];
  for (int i = 0; i < 10; i++) {
    buffer_arr[i] = analogRead(PH_PIN);
    delay(10);
  }

  // Urutkan nilai untuk filter noise
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buffer_arr[i] > buffer_arr[j]) {
        int temp = buffer_arr[i];
        buffer_arr[i] = buffer_arr[j];
        buffer_arr[j] = temp;
      }
    }
  }

  long avgValue = 0;
  for (int i = 2; i < 8; i++) avgValue += buffer_arr[i];
  float volt = (float)avgValue * 3.3 / 4096 / 6.0;
  float ph = 7 + ((2.5 - volt) / 0.18) + offsetPH;
  return ph;
}

// ============================
// Fungsi Baca Sensor TDS
// ============================
float bacaTDS(float suhuAir) {
  int analogValue = analogRead(TDS_PIN);
  float volt = analogValue * 3.3 / 4096.0;
  float suhuKompensasi = 1.0 + 0.02 * (suhuAir - 25.0);
  float voltKompensasi = volt / suhuKompensasi;
  float tds = (133.42 * voltKompensasi * voltKompensasi * voltKompensasi
              - 255.86 * voltKompensasi * voltKompensasi
              + 857.39 * voltKompensasi) * 0.5;
  return tds;
}

// ============================
// Fungsi Kendali Relay Pompa Nutrisi
// ============================
void kontrolPompaNutrisi(float tds) {
  if (tds < TDS_LOW) {
    digitalWrite(RELAY_TDS, LOW); // Pompa ON
  } else if (tds > TDS_HIGH) {
    digitalWrite(RELAY_TDS, HIGH); // Pompa OFF
  }
}

// ============================
// LOOP UTAMA
// ============================
void loop() {
  suhuAir = bacaSuhuAir();
  bacaDHT();
  phValue = bacaPH(); // hanya untuk ditampilkan
  tdsValue = bacaTDS(suhuAir);

  kontrolPompaNutrisi(tdsValue);

  // === Serial Monitor ===
  Serial.println("==============================");
  Serial.printf("🌡 Suhu Udara : %.2f °C\n", suhuUdara);
  Serial.printf("💧 Kelembapan : %.2f %%\n", kelembapanUdara);
  Serial.printf("🌡 Suhu Air   : %.2f °C\n", suhuAir);
  Serial.printf("💧 pH Air     : %.2f\n", phValue);
  Serial.printf("🧪 TDS Air    : %.2f ppm\n", tdsValue);
  Serial.print("Relay Nutrisi : ");
  Serial.println(digitalRead(RELAY_TDS) == LOW ? "ON (Pompa Aktif)" : "OFF");
  Serial.println("==============================\n");

  // === LCD I2C ===
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(suhuAir, 1);
  lcd.print((char)223);
  lcd.print("C pH:");
  lcd.print(phValue, 1);

  lcd.setCursor(0, 1);
  lcd.print("TDS:");
  lcd.print(tdsValue, 0);
  lcd.print(" ");
  if (digitalRead(RELAY_TDS) == LOW)
    lcd.print("R:ON");
  else
    lcd.print("R:OFF");

  delay(2000);
}
