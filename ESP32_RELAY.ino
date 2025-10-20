#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ====== KONFIGURASI WIFI ======
const char* ssid = "Suci";
const char* password = "22222222";

// ====== KONFIGURASI MQTT ======
const char* mqtt_server = "broker.emqx.io";
const char* TOPIC_RELAY1 = "kel4/relay1";
const char* TOPIC_RELAY2 = "kel4/relay2";
const char* TOPIC_JSON   = "kel4/dht";

// ====== KONFIGURASI PIN ======
#define DHTPIN 26
#define DHTTYPE DHT11
#define RELAY1 5
#define RELAY2 18

// ====== OBJEK ======
WiFiClient espClient;
PubSubClient mqtt(espClient);
DHT dht(DHTPIN, DHTTYPE);

// ====== VARIABEL ======
float temperature;
float humidity;
bool relay1State = false;
bool relay2State = false;
unsigned long prevMillis = 0;
int intervalPengiriman = 5; // dalam detik

// ====== FUNGSI WIFI ======
void setup_wifi() {
  delay(10);
  Serial.begin(115200);
  Serial.println();
  Serial.printf("Menghubungkan ke %s\n", ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi terhubung!");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
}

// ====== CALLBACK MQTT ======
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String msg = String((char*)payload);

  Serial.print("Pesan dari [");
  Serial.print(topic);
  Serial.print("] : ");
  Serial.println(msg);

  // ===== Relay 1 =====
  if (strcmp(topic, TOPIC_RELAY1) == 0) {
    if (msg == "1") {
      digitalWrite(RELAY1, LOW);  // aktif LOW
      relay1State = true;
      Serial.println("Relay 1 ON");
    } else {
      digitalWrite(RELAY1, HIGH);
      relay1State = false;
      Serial.println("Relay 1 OFF");
    }
  }

  // ===== Relay 2 =====
  if (strcmp(topic, TOPIC_RELAY2) == 0) {
    if (msg == "1") {
      digitalWrite(RELAY2, LOW);
      relay2State = true;
      Serial.println("Relay 2 ON");
    } else {
      digitalWrite(RELAY2, HIGH);
      relay2State = false;
      Serial.println("Relay 2 OFF");
    }
  }
}

// ====== FUNGSI RECONNECT MQTT ======
void reconnect() {
  while (!mqtt.connected()) {
    Serial.print("Menghubungkan ke MQTT... ");
    String clientId = "ESP32_PubKel4_";
    clientId += String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("Berhasil!");
      mqtt.subscribe(TOPIC_RELAY1);
      mqtt.subscribe(TOPIC_RELAY2);
      Serial.println("Subscribe ke kel4/relay1 & kel4/relay2");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(mqtt.state());
      Serial.println(" coba lagi 5 detik...");
      delay(5000);
    }
  }
}

// ====== PUBLISH DATA JSON ======
void publish_json() {
  char json[256];
  JsonDocument doc;

  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["relay1"] = relay1State ? "ON" : "OFF";
  doc["relay2"] = relay2State ? "ON" : "OFF";

  serializeJson(doc, json);
  mqtt.publish(TOPIC_JSON, json);

  Serial.println("Data terkirim ke kel4/dht:");
  serializeJsonPretty(doc, Serial);
  Serial.println();
}

// ====== SETUP ======
void setup() {
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY1, HIGH); // default mati
  digitalWrite(RELAY2, HIGH); // default mati

  setup_wifi();
  dht.begin();
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback);
}

// ====== LOOP ======
void loop() {
  if (!mqtt.connected()) reconnect();
  mqtt.loop();

  if (millis() - prevMillis >= intervalPengiriman * 1000) {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    Serial.println("===========================");
    Serial.println("Temperature: " + String(temperature) + " °C");
    Serial.println("Humidity   : " + String(humidity) + " %");
    Serial.println("Relay1     : " + String(relay1State ? "ON" : "OFF"));
    Serial.println("Relay2     : " + String(relay2State ? "ON" : "OFF"));
    Serial.println("===========================\n");

    publish_json();
    prevMillis = millis();
  }
}