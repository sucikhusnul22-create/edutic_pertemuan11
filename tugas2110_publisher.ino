#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <ModbusMaster.h>

// ======== KONFIGURASI WIFI & MQTT =========
const char* ssid = "REDMI 15C";
const char* password = "firman82#";
const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ======== KONFIGURASI MODBUS =========
#define RX_PIN 16
#define TX_PIN 17
#define SLAVE_ID 1
ModbusMaster node;

// ======== KONFIGURASI RELAY =========
#define RELAY1_PIN 18
#define RELAY2_PIN 19
bool relay1State = false;
bool relay2State = false;

// ======== TOPIC MQTT =========
const char* topic_publish  = "kelompok4/data";     // Publisher kirim data ke sini
const char* topic_subscribe = "kelompok4/control"; // Menerima perintah dari subscriber

// ======== VARIABEL =========
unsigned long lastMsg = 0;
const long interval = 5000;  // kirim data tiap 5 detik

// ======== WIFI =========
void setup_wifi() {
  Serial.print("Menghubungkan ke WiFi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi terhubung!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ======== CALLBACK MQTT =========
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  Serial.print("Pesan diterima [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print("Error parsing JSON: ");
    Serial.println(error.c_str());
    return;
  }

  if (doc.containsKey("command")) {
    String cmd = doc["command"].as<String>();

    if (cmd == "relay1_toggle") {
      relay1State = !relay1State;
      digitalWrite(RELAY1_PIN, relay1State);
      Serial.println("Relay 1 di-Toggle!");
    }
    else if (cmd == "relay2_toggle") {
      relay2State = !relay2State;
      digitalWrite(RELAY2_PIN, relay2State);
      Serial.println("Relay 2 di-Toggle!");
    }

    // update LCD setelah kontrol
    lcd.setCursor(0, 1);
    lcd.print("R1:");
    lcd.print(relay1State ? "ON " : "OFF");
    lcd.print(" R2:");
    lcd.print(relay2State ? "ON" : "OFF");
  }
}

// ======== RECONNECT MQTT =========
void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect("ESP32Publisher")) {
      Serial.println("Terhubung!");
      client.subscribe(topic_subscribe);
      Serial.print("Subscribe ke: ");
      Serial.println(topic_subscribe);
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" mencoba lagi 5 detik...");
      delay(5000);
    }
  }
}

// ======== BACA SUHU DARI MODBUS =========
float readTemperatureFromModbus() {
  uint8_t result;
  uint16_t data[2];

  result = node.readHoldingRegisters(0x0001, 1);  // contoh register suhu
  if (result == node.ku8MBSuccess) {
    float temperature = node.getResponseBuffer(0) / 10.0;
    return temperature;
  } else {
    Serial.println("Gagal membaca Modbus!");
    return NAN;
  }
}

// ======== KIRIM DATA KE MQTT =========
void publishData() {
  float t = readTemperatureFromModbus();
  if (isnan(t)) {
    Serial.println("Sensor gagal dibaca");
    return;
  }

  StaticJsonDocument<200> doc;
  doc["temperature"] = t;
  doc["relay1"] = relay1State ? "ON" : "OFF";
  doc["relay2"] = relay2State ? "ON" : "OFF";

  char buffer[200];
  serializeJson(doc, buffer);
  client.publish(topic_publish, buffer);

  Serial.print("Data dikirim ke MQTT: ");
  Serial.println(buffer);

  // Update LCD
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(t, 1);
  lcd.print((char)223);
  lcd.print("C       ");

  lcd.setCursor(0, 1);
  lcd.print("R1:");
  lcd.print(relay1State ? "ON " : "OFF");
  lcd.print(" R2:");
  lcd.print(relay2State ? "ON" : "OFF");
}

// ======== SETUP =========
void setup() {
  Serial.begin(115200);

  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  node.begin(SLAVE_ID, Serial2);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.print("Init Publisher...");
  delay(1000);
  lcd.clear();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  lcd.print("MQTT Publisher");
  delay(1500);
  lcd.clear();
}

// ======== LOOP =========
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    publishData();
  }
}
