
#include <time.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>

// PIN
#define PIN_LED D5
#define PIN_BTN D6
#define PIN_DHT D7

// WIFI
const char* WIFI_SSID = "SherNix";
const char* WIFI_PASS = "obsessed26";

// MQTT EMQX
const char* MQTT_HOST = "f28afda2.ala.asia-southeast1.emqxsl.com";
const int MQTT_PORT = 8883;
const char* MQTT_USER = "shernix";
const char* MQTT_PASS = "nixsilvo007";

// NIM / TOPIC
const char* NIM = "E41240991";

String TOPIC_LED = String(NIM) + "/esp8266/led";
String TOPIC_TEMP = String(NIM) + "/esp8266/temp";
String TOPIC_HUM = String(NIM) + "/esp8266/hum";
String TOPIC_BTN = String(NIM) + "/esp8266/btn";

// DHT
DHT dht(PIN_DHT, DHT22);

// MQTT
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// BUTTON
bool buttonState;
bool lastButtonState;

// TIMER
unsigned long lastSensorRead = 0;
const unsigned long sensorInterval = 5000;

// ROOT CA
static const char root_ca[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";

// SETUP
void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLUP);

  digitalWrite(PIN_LED, LOW);

  buttonState = digitalRead(PIN_BTN);
  lastButtonState = buttonState;

  dht.begin();
  delay(2000);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");

  setClock();

  espClient.setTrustAnchors(
    new BearSSL::X509List(root_ca)
  );

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  Serial.println("Setup Complete!");
}

// SET CLOCK
void setClock() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time");

  time_t now = time(nullptr);

  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }

  Serial.println("\nTime synchronized!");
}

// MQTT CALLBACK
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("MQTT Received [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (String(topic) == TOPIC_LED) {
    int ledValue = constrain(message.toInt(), 0, 1023);
    analogWrite(PIN_LED, ledValue);
  }
}

// MQTT CONNECT
void mqttConnect() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting MQTT...");

    if (mqttClient.connect(NIM, MQTT_USER, MQTT_PASS)) {
      Serial.println("Connected!");

      mqttClient.subscribe(TOPIC_LED.c_str());

      Serial.println("Subscribed LED topic!");
    } else {
      Serial.print("Failed, state = ");
      Serial.println(mqttClient.state());

      delay(5000);
    }
  }
}

// MQTT HANDLE
void mqttHandle() {
  if (!mqttClient.connected()) {
    mqttConnect();
  }

  mqttClient.loop();
}

// BUTTON HANDLE
void buttonHandle() {
  buttonState = digitalRead(PIN_BTN);

  if (buttonState != lastButtonState) {
    delay(30);

    buttonState = digitalRead(PIN_BTN);

    if (buttonState != lastButtonState) {
      lastButtonState = buttonState;

      if (buttonState == LOW) {
        Serial.println("BUTTON PRESSED");

        if (mqttClient.connected()) {
          bool sent = mqttClient.publish(TOPIC_BTN.c_str(), "1");

          Serial.print("Button publish: ");
          Serial.println(sent ? "SUCCESS" : "FAILED");
        }
      } else {
        Serial.println("BUTTON RELEASED");

        if (mqttClient.connected()) {
          bool sent = mqttClient.publish(TOPIC_BTN.c_str(), "0");

          Serial.print("Button publish: ");
          Serial.println(sent ? "SUCCESS" : "FAILED");
        }
      }
    }
  }
}

// SENSOR HANDLE
void sensorHandle() {
  if (millis() - lastSensorRead >= sensorInterval) {
    lastSensorRead = millis();

    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();

    Serial.print("Temperature: ");
    Serial.println(temp);

    Serial.print("Humidity: ");
    Serial.println(humidity);

    if (isnan(temp) || isnan(humidity)) {
      Serial.println("DHT22 READ FAILED!");
      return;
    }

    char tempstr[10];
    char humstr[10];

    dtostrf(temp, 1, 2, tempstr);
    dtostrf(humidity, 1, 2, humstr);

    if (mqttClient.connected()) {
      bool tempSent = mqttClient.publish(TOPIC_TEMP.c_str(), tempstr);
      bool humSent = mqttClient.publish(TOPIC_HUM.c_str(), humstr);

      Serial.print("Temperature publish: ");
      Serial.println(tempSent ? "SUCCESS" : "FAILED");

      Serial.print("Humidity publish: ");
      Serial.println(humSent ? "SUCCESS" : "FAILED");
    }
  }
}

// LOOP
void loop() {
  mqttHandle();

  buttonHandle();

  sensorHandle();
}
