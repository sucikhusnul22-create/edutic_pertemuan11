/*
Penjelasan Alur Otomatisasi
Sensor	Parameter	Kondisi	Relay Aktif
TDS	< 500 ppm	Air terlalu encer	Pompa Nutrisi (Relay 1)
TDS	> 1200 ppm	Terlalu pekat	Pompa OFF
pH	< 6.0 atau > 7.5	Tidak ideal	Pompa Penetral (Relay 2)
pH	6.0–7.5	Ideal	Pompa OFF

⚙️ Pin ESP32
Komponen	Pin	Keterangan
DS18B20	GPIO 4	Sensor suhu air
Sensor pH	GPIO 34	Input analog
Sensor TDS	GPIO 35	Input analog
Relay 1	GPIO 25	Pompa Nutrisi
Relay 2	GPIO 26	Pompa Penetral
LCD I2C	SDA 21, SCL 22	Tampilan data
*/





#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ============================
// Konfigurasi Pin Sensor
// ============================
#define PH_PIN 34
#define TDS_PIN 35
#define ONE_WIRE_BUS 4

// ============================
// Konfigurasi Relay
// ============================
#define RELAY_TDS 25   // Relay 1 (Pompa Nutrisi)
#define RELAY_PH 26    // Relay 2 (Pompa Penetral)

// ============================
// Inisialisasi LCD & Sensor
// ============================
LiquidCrystal_I2C lcd(0x27, 16, 2);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ============================
// Variabel Global
// ============================
float suhuAir = 0.0;
float tdsValue = 0.0;
float phValue = 0.0;

// Kalibrasi sensor
float offsetPH = 0.00;

// Batas kendali otomatis
const float PH_LOW = 6.0;      // batas bawah pH
const float PH_HIGH = 7.5;     // batas atas pH
const float TDS_LOW = 500.0;   // batas bawah TDS ppm
const float TDS_HIGH = 1200.0; // batas atas TDS ppm

// ============================
// Fungsi Setup
// ============================
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Init Sensor...");

  sensors.begin();

  pinMode(RELAY_TDS, OUTPUT);
  pinMode(RELAY_PH, OUTPUT);
  digitalWrite(RELAY_TDS, HIGH);
  digitalWrite(RELAY_PH, HIGH);

  delay(1500);
  lcd.clear();
  lcd.print("Sistem Siap!");
  delay(1000);
}

// ============================
// Fungsi Baca Suhu Air
// ============================
float bacaSuhuAir() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

// ============================
// Fungsi Baca Sensor pH
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
// Fungsi Kendali Relay
// ============================
void kontrolRelay(float ph, float tds) {
  // Kendali pompa nutrisi (relay 1)
  if (tds < TDS_LOW) {
    digitalWrite(RELAY_TDS, LOW); // ON
  } else if (tds > TDS_HIGH) {
    digitalWrite(RELAY_TDS, HIGH); // OFF
  }

  // Kendali pompa penetral (relay 2)
  if (ph < PH_LOW || ph > PH_HIGH) {
    digitalWrite(RELAY_PH, LOW); // ON
  } else {
    digitalWrite(RELAY_PH, HIGH); // OFF
  }
}

// ============================
// LOOP UTAMA
// ============================
void loop() {
  suhuAir = bacaSuhuAir();
  phValue = bacaPH();
  tdsValue = bacaTDS(suhuAir);

  kontrolRelay(phValue, tdsValue);

  // Tampilkan ke Serial
  Serial.println("==============================");
  Serial.printf("🌡 Suhu Air : %.2f °C\n", suhuAir);
  Serial.printf("💧 pH Air   : %.2f\n", phValue);
  Serial.printf("🧪 TDS Air  : %.2f ppm\n", tdsValue);
  Serial.print("Relay TDS : ");
  Serial.println(digitalRead(RELAY_TDS) == LOW ? "ON" : "OFF");
  Serial.print("Relay pH  : ");
  Serial.println(digitalRead(RELAY_PH) == LOW ? "ON" : "OFF");
  Serial.println("==============================\n");

  // Tampilkan ke LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(suhuAir, 1);
  lcd.print((char)223);
  lcd.print("C ");
  lcd.print("pH:");
  lcd.print(phValue, 1);

  lcd.setCursor(0, 1);
  lcd.print("TDS:");
  lcd.print(tdsValue, 0);
  lcd.print(" ");
  if (digitalRead(RELAY_TDS) == LOW || digitalRead(RELAY_PH) == LOW)
    lcd.print("R:ON");
  else
    lcd.print("R:OFF");

  delay(2000);
}
