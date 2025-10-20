#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "anjay";
const char* password = "mumetlek";

const char* mqtt_server = "broker.emqx.io";
const char* TOPIC_JSON = "kel4/dht";

WiFiClient espClient;
PubSubClient mqtt(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

float temperature;
String relay1Status;
String relay2Status;

void setup_wifi() {
  delay(10);
  Serial.begin(115200);
  Serial.println();
  Serial.printf("Connecting to %s\n", ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String msg = String((char*)payload);

  if (strcmp(topic, TOPIC_JSON) == 0) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (!error) {
      temperature = doc["temperature"];
      relay1Status = (const char*)doc["relay1"];
      relay2Status = (const char*)doc["relay2"];

      Serial.printf("Temp: %.1f°C | R1:%s | R2:%s\n", 
                    temperature, relay1Status.c_str(), relay2Status.c_str());

      // Baris 1: hanya suhu
      lcd.setCursor(0, 0);
      lcd.print("Suhu: ");
      lcd.print(temperature, 1);
      lcd.print((char)223);
      lcd.print("C    ");

      // Baris 2: status relay
      lcd.setCursor(0, 1);
      lcd.print("R1:");
      lcd.print(relay1Status);
      lcd.print(" R2:");
      lcd.print(relay2Status);
      lcd.print("   ");
    }
  }
}

void reconnect() {
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32_LCDKel4_";
    clientId += String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("connected!");
      mqtt.subscribe(TOPIC_JSON);
      Serial.println("Subscribed to kel4/dht");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" retry in 5s...");
      delay(5000);
    }
  }
}

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.print("LCD Subscriber");
  delay(1000);
  lcd.clear();

  setup_wifi();
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback);
}

void loop() {
  if (!mqtt.connected()) reconnect();
  mqtt.loop();
}
