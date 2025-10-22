#include <WiFi.h>
#include <ModbusIP_ESP8266.h>
#include "DHT.h"

// ========== KONFIGURASI PIN ==========
#define DHTPIN 14
#define DHTTYPE DHT11
#define RELAY1_PIN 25
#define RELAY2_PIN 26
#define TOUCH1_PIN 4
#define TOUCH2_PIN 15

// ========== ALAMAT REGISTER ==========
#define TEMPERATURE_ADDRESS 100
#define HUMIDITY_ADDRESS 101
#define RELAY1_COIL 1
#define RELAY2_COIL 2
#define TOUCH1_COIL 3
#define TOUCH2_COIL 4

// ========== KONFIGURASI WIFI ==========
const char* ssid = "REDMI 15C";
const char* pass = "firman82#";

IPAddress local_IP(192, 168, 221, 123);
IPAddress gateway(192, 168, 221, 72);
IPAddress subnet(255, 255, 255, 0);

// ========== OBJEK ==========
DHT dht(DHTPIN, DHTTYPE);
ModbusIP mb;

// ===== Variabel Touch =====
bool lastTouch1 = false;
bool lastTouch2 = false;
unsigned long lastToggle1 = 0;
unsigned long lastToggle2 = 0;
const unsigned long debounceDelay = 200;  // ms

void setup() {
  Serial.begin(115200);
  Serial.println(F("MODBUS TCP OVER WIFI ESP32 (FAST TOUCH STATUS)"));

  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Inisialisasi sensor & relay
  dht.begin();
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);

  // Inisialisasi Modbus server
  mb.server();

  // Tambah Holding Register
  mb.addHreg(TEMPERATURE_ADDRESS);
  mb.addHreg(HUMIDITY_ADDRESS);

  // Tambah Coil
  mb.addCoil(RELAY1_COIL);
  mb.addCoil(RELAY2_COIL);
  mb.addCoil(TOUCH1_COIL);
  mb.addCoil(TOUCH2_COIL);

  // Nilai awal
  mb.Coil(RELAY1_COIL, false);
  mb.Coil(RELAY2_COIL, false);
  mb.Coil(TOUCH1_COIL, false);
  mb.Coil(TOUCH2_COIL, false);
}

void loop() {
  mb.task();  // Jalankan server Modbus

  // ==== Pembacaan DHT tiap 2 detik (non-blok) ====
  static unsigned long lastDHT = 0;
  if (millis() - lastDHT >= 2000) {
    lastDHT = millis();
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    if (!isnan(humidity) && !isnan(temperature)) {
      mb.Hreg(TEMPERATURE_ADDRESS, temperature * 10);
      mb.Hreg(HUMIDITY_ADDRESS, humidity * 10);
      Serial.printf("T: %.1f°C | H: %.1f%%\n", temperature, humidity);
    }
  }

  // ==== Baca Touch Sensor (real-time) ====
  int val1 = touchRead(TOUCH1_PIN);
  int val2 = touchRead(TOUCH2_PIN);
  bool touched1 = (val1 < 500);
  bool touched2 = (val2 < 500);

  // Update status sentuhan langsung ke Modbus coil (bisa dibaca di Modbus Poll)
  mb.Coil(TOUCH1_COIL, touched1);
  mb.Coil(TOUCH2_COIL, touched2);

  // ==== Toggle Relay kalau disentuh ====
  if (touched1 && !lastTouch1 && millis() - lastToggle1 > debounceDelay) {
    lastToggle1 = millis();
    bool newState = !mb.Coil(RELAY1_COIL);
    mb.Coil(RELAY1_COIL, newState);
    Serial.printf("Touch1 pressed → Relay1 %s\n", newState ? "ON" : "OFF");
  }

  if (touched2 && !lastTouch2 && millis() - lastToggle2 > debounceDelay) {
    lastToggle2 = millis();
    bool newState = !mb.Coil(RELAY2_COIL);
    mb.Coil(RELAY2_COIL, newState);
    Serial.printf("Touch2 pressed → Relay2 %s\n", newState ? "ON" : "OFF");
  }

  lastTouch1 = touched1;
  lastTouch2 = touched2;

  // ==== Update Relay Output ====
  bool relay1_state = mb.Coil(RELAY1_COIL);
  bool relay2_state = mb.Coil(RELAY2_COIL);
  digitalWrite(RELAY1_PIN, relay1_state ? LOW : HIGH);
  digitalWrite(RELAY2_PIN, relay2_state ? LOW : HIGH);
}
