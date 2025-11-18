#include <Arduino.h>
#include <pubsubclient.h>
#include <WiFi.h>

// WiFi settings
const char *ssid = "S22 de Gabin";
const char *password = "Slender77";

// MQTT Broker settings
const char *mqtt_broker = "broker.emqx.io"; // EMQX broker endpoint
const char *mqtt_topic1 = "dataCastres/topic1";
const char *mqtt_topic2 = "dataCastres/topic2";
const char *mqtt_topic3 = "homeTrainerCastres/Group2-A/MAC";
const char *mqtt_topic4 = "homeTrainerCastres/Group2-A/OIIA";
const char *mqtt_topic5 = "homeTrainerCastres/Group2-A/Coeur";
const int mqtt_port = 1883;

String client_id = "ArduinoClient-";
String MAC_address = "";

static unsigned long lastPublishTime = 0;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// Heartbeat sensor
int ledPin = 13;
int analogPin = 0;
const int delayMsec = 60;

// Function declarations
void connectToWiFi();
void connectToMQTTBroker();
void mqttCallback(char *topic, byte *payload, unsigned int length);

// ------------------------------------------------------------
// SETUP
// ------------------------------------------------------------
void setup() {
    Serial.begin(9600);

    connectToWiFi();
    mqtt_client.setServer(mqtt_broker, mqtt_port);
    mqtt_client.setCallback(mqttCallback);
    connectToMQTTBroker();

    // Heartbeat sensor init
    pinMode(ledPin, OUTPUT);

    Serial.println("Heartbeat Detektion Beispielcode.");
}

// ------------------------------------------------------------
// PRINT MAC ADDRESS
// ------------------------------------------------------------
String printMacAddress() {
    byte mac[6];
    Serial.print("MAC Address: ");

    WiFi.macAddress(mac);

    for (int i = 0; i < 6; i++) {
        MAC_address += String(mac[i], HEX);
        if (i < 5) MAC_address += ":";
        if (mac[i] < 16) client_id += "0";
        client_id += String(mac[i], HEX);
    }

    Serial.println(MAC_address);
    return MAC_address;
}

// ------------------------------------------------------------
// WIFI CONNECTION
// ------------------------------------------------------------
void connectToWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    delay(3000);
    printMacAddress();

    Serial.println("Connected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
}

// ------------------------------------------------------------
// MQTT CONNECTION
// ------------------------------------------------------------
void connectToMQTTBroker() {
    while (!mqtt_client.connected()) {
        Serial.print("Connecting to MQTT Broker as ");
        Serial.print(client_id.c_str());
        Serial.println("...");

        if (mqtt_client.connect(client_id.c_str())) {
            Serial.println("Connected to MQTT broker");

            mqtt_client.subscribe(mqtt_topic1);
            mqtt_client.subscribe(mqtt_topic2);
            mqtt_client.subscribe(mqtt_topic3);
            mqtt_client.subscribe(mqtt_topic4);
            mqtt_client.subscribe(mqtt_topic5);

            String message = "Hello EMQX I'm " + client_id;
            mqtt_client.publish(mqtt_topic1, message.c_str());
        } else {
            Serial.print("Failed, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" retrying in 5 seconds...");
            delay(5000);
        }
    }
}

// ------------------------------------------------------------
// MQTT CALLBACK
// ------------------------------------------------------------
void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.println(topic);

    Serial.print("Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++) {
        messageTemp += (char)payload[i];
    }

    Serial.println(messageTemp);
    Serial.println("-----------------------");
}

// ------------------------------------------------------------
// HEARTBEAT SENSOR
// ------------------------------------------------------------
int rawValue;

bool heartbeatDetected(int IRSensorPin, int delay) {
    static int maxValue = 0;
    static bool isPeak = false;

    bool result = false;

    rawValue = analogRead(IRSensorPin);
    rawValue *= (1000 / delay);

    if (rawValue * 4L < maxValue)
        maxValue = rawValue * 0.8;

    if (rawValue > maxValue - (1000 / delay)) {

        if (rawValue > maxValue)
            maxValue = rawValue;

        if (!isPeak)
            result = true;

        isPeak = true;
    }
    else if (rawValue < maxValue - (3000 / delay)) {
        isPeak = false;
        maxValue -= (1000 / delay);
    }

    return result;
}

// ------------------------------------------------------------
// LOOP
// ------------------------------------------------------------
void loop() {
    if (!mqtt_client.connected())
        connectToMQTTBroker();

    mqtt_client.loop();

    unsigned long currentTime = millis();
    static int beatMsec = 0;
    int heartRateBPM = 0;

    // Publish MAC periodically
    if (currentTime - lastPublishTime >= 10000) {
        lastPublishTime = currentTime;

        String MAC = printMacAddress();
        String MACmsg = "Adresse MAC " + MAC;

        String MAC_OIIA = "OIIA ᓚᘏᗢ MAC Address : " + printMacAddress();

        // Uncomment if needed
        // mqtt_client.publish(mqtt_topic3, MACmsg.c_str());
        // mqtt_client.publish(mqtt_topic4, "Miaou");
    }

    // Heartbeat detection
    if (heartbeatDetected(analogPin, delayMsec)) {
        heartRateBPM = 60000 / beatMsec;

        digitalWrite(ledPin, HIGH);

        String msgCoeur = String(heartRateBPM);
        mqtt_client.publish(mqtt_topic5, msgCoeur.c_str());

        Serial.print("Puls erkannt: ");
        Serial.println(heartRateBPM);

        beatMsec = 0;
    }
    else {
        digitalWrite(ledPin, LOW);
    }

    delay(delayMsec);
    beatMsec += delayMsec;
}
