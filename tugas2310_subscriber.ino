#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>

// Konfigurasi WiFi
const char* ssid = "REDMI 15C";
const char* password = "firman82#";

// Konfigurasi MQTT
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;  // default port TCP
const char* topic_ph = "esp32/ph";
const char* topic_tds = "esp32/tds";

// Variabel data
float phValue = 0.0;
float tdsValue = 0.0;

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);  // ubah alamat jika tidak tampil (0x3F kadang dipakai)

// WiFi dan MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// ====== FUNGSI KONEKSI WIFI ======
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Menyambung ke WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi tersambung ✅");
  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());
}

// ====== CALLBACK SAAT TERIMA PESAN MQTT ======
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("Pesan diterima [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  if (String(topic) == topic_ph) {
    phValue = msg.toFloat();
  } else if (String(topic) == topic_tds) {
    tdsValue = msg.toFloat();
  }

  // Update tampilan LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("pH : ");
  lcd.print(phValue, 2);
  lcd.setCursor(0, 1);
  lcd.print("TDS: ");
  lcd.print(tdsValue, 0);
  lcd.print(" ppm");
}

// ====== FUNGSI RECONNECT MQTT ======
void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    String clientId = "ESP32Sub_" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("✅ Tersambung!");
      client.subscribe(topic_ph);
      client.subscribe(topic_tds);
      Serial.println("Berlangganan ke topik:");
      Serial.println(" - esp32/ph");
      Serial.println(" - esp32/tds");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 5 detik...");
      delay(5000);
    }
  }
}

// ====== SETUP ======
void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ESP32 Subscriber");
  lcd.setCursor(0, 1);
  lcd.print("Init WiFi...");

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  lcd.clear();
  lcd.print("Init MQTT...");
}

// ====== LOOP ======
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Tampilkan ke Serial
  Serial.print("pH: ");
  Serial.print(phValue, 2);
  Serial.print(" | TDS: ");
  Serial.print(tdsValue, 0);
  Serial.println(" ppm");

  delay(2000);
}
