#include <Arduino.h>
#include <pubsubclient.h>
#include <WiFi.h>

// WiFi settings
// TODO : Replace with your WiFi credentials here
const char *ssid = "S22 de Gabin";
const char *password = "Slender77";

// MQTT Broker settings
// TODO : Update with your MQTT broker settings here if needed
const char *mqtt_broker = "broker.emqx.io";     // EMQX broker endpoint
const char *mqtt_topic1 = "dataCastres/topic1"; // MQTT topic
const char *mqtt_topic2 = "dataCastres/topic2"; // MQTT topic
const char *mqtt_topic3 = "homeTrainerCastres/Group2-A/MAC";
const char *mqtt_topic4 = "homeTrainerCastres/Group2-A/OIIA";
const char *mqtt_topic5 = "homeTrainerCastres/Group2-A/Coeur";
const int mqtt_port = 1883;                     // MQTT port (TCP)
String client_id = "ArduinoClient-";
String MAC_address = "";

// Other global variables
static unsigned long lastPublishTime = 0;
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// Coeur
int ledPin=13;
int analogPin=0;
const int delayMsec = 60; // 100msec per sample

void connectToWiFi();
void connectToMQTTBroker();
void mqttCallback(char *topic, byte *payload, unsigned int length);

void setup()
{
  Serial.begin(9600);
  connectToWiFi();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();

  /****************** Coeur ***************** */
  // Die eingebaute Arduino LED (Digital 13), wird hier zur Ausgabe genutzt
  pinMode(ledPin,OUTPUT);

  // Serielle Ausgabe Initialisierung
  Serial.begin(9600);
  Serial.println("Heartbeat Detektion Beispielcode.");
}

String printMacAddress()
{
  byte mac[6];
  Serial.print("MAC Address: ");
  WiFi.macAddress(mac);
  for (int i = 0; i < 6; i++)
  {
    MAC_address += String(mac[i], HEX);
    if (i < 5)
      MAC_address += ":";
    if (mac[i] < 16)
    {
      client_id += "0";
    }
    client_id += String(mac[i], HEX);
  }
  Serial.println(MAC_address);
  return MAC_address;
}

void connectToWiFi()
{
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  delay(3000);
  printMacAddress();
  Serial.println("Connected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void connectToMQTTBroker()
{
  while (!mqtt_client.connected())
  {
    Serial.print("Connecting to MQTT Broker as ");
    Serial.print(client_id.c_str());
    Serial.println(".....");
    if (mqtt_client.connect(client_id.c_str()))
    {
      Serial.println("Connected to MQTT broker");
      mqtt_client.subscribe(mqtt_topic1);
      mqtt_client.subscribe(mqtt_topic2);
      mqtt_client.subscribe(mqtt_topic3);
      mqtt_client.subscribe(mqtt_topic4);
      mqtt_client.subscribe(mqtt_topic5);
      // Publish message upon successful connection
      String message = "Hello EMQX I'm " + client_id;
      mqtt_client.publish(mqtt_topic1, message.c_str());
    }
    else
    {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Chaque fois qu'un message est publié par le broker, cette fonction s'exécute
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  String messageTemp;
  for (int i = 0; i < length; i++)
  {
    // Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println(messageTemp);
  Serial.println("-----------------------");
  // TODO : Add your message handling logic here
  // For example, you can check the topic and perform actions based on the message content
  // Example:
  //
  // if (String(topic) == "arduino/output")
  // {
  //   Serial.print("Changing output to ");
  //   if (messageTemp == "on")
  //   {
  //     Serial.println("LED on");
  //     digitalWrite(ledPin, HIGH);
  //   }
  //   else if (messageTemp == "off")
  //   {
  //     Serial.println("LED off");
  //     digitalWrite(ledPin, LOW);
  //   }
  // }
}

/********************************************** CAPTEUR ******************************************* */
int rawValue;

bool heartbeatDetected(int IRSensorPin, int delay)
{
  static int maxValue = 0;
  static bool isPeak = false;


  bool result = false;

  rawValue = analogRead(IRSensorPin);
  // Hier wird der aktuelle Spannungswert am Fototransistor ausgelesen und in der rawValueVariable zwischengespeichert
  rawValue *= (1000/delay);

 // Sollte der aktuelle Wert vom letzten maximalen Wert zu weit abweichen
 // (z.B. da der Finger neu aufgesetzt oder weggenommen wurde)
 // So wird der MaxValue resetiert, um eine neue Basis zu erhalten.
 if (rawValue * 4L < maxValue) { maxValue = rawValue * 0.8; } // Detect new peak
  if (rawValue > maxValue - (1000/delay)) {
  // Hier wird der eigentliche Peak detektiert. Sollte ein neuer RawValue groeßer sein
  // als der letzte maximale Wert, so wird das als Spitze der aufgezeichnten Daten erkannt.
    if (rawValue > maxValue) {
      maxValue = rawValue;
    }
    // Zum erkannten Peak soll nur ein Herzschlag zugewiesen werden
    if (isPeak == false) {
      result = true;
    }
    isPeak = true;
  } else if (rawValue < maxValue - (3000/delay)) {
    isPeak = false;
  // Hierbei wird der maximale Wert bei jeden Durchlauf
  // etwas wieder herabgesetzt. Dies hat den Grund, dass
  // nicht nur der Wert sonst immer stabil bei jedem Schlag
  // gleich oder kleiner werden wuerde, sondern auch,
  // falls der Finger sich minimal bewegen sollte und somit
  // das Signal generell schwaecher werden wuerde.
    maxValue-=(1000/delay);
  }

 return result;
}

void loop()
{
  if (!mqtt_client.connected())
  {
    connectToMQTTBroker();
  }
  mqtt_client.loop();
  // TODO: Add your main code here, to run repeatedly (e.g., sensor readings, publishing messages, etc. )
  // Example below : Publish a message every 10 seconds
  unsigned long currentTime = millis();
  static int beatMsec = 0;
  int heartRateBPM = 0;

  if (currentTime - lastPublishTime >= 10000) // 10 seconds
  {
    String message = "Hello EMQX I'm " + client_id + " at " + String(currentTime / 1000) + " seconds";
    // mqtt_client.publish(mqtt_topic1, message.c_str());
    String MAC_address = printMacAddress();
    String MACmsg = "Adresse MAC " + MAC_address;
    // mqtt_client.publish(mqtt_topic1, MACmsg.c_str());
    // Serial.println("Published message: " + message);
    lastPublishTime = currentTime;
    String MAC_address_OIIA = "OIIA ᓚᘏᗢ MAC Address : " + printMacAddress();
    // Serial.println(MAC_address_OIIA);
    // mqtt_client.publish(mqtt_topic3, MAC_address_OIIA.c_str());
    // mqtt_client.publish(mqtt_topic4, "Miaou");
  }

  /********************* Coeur **************** */
  if (heartbeatDetected(analogPin, delayMsec))
  {
    heartRateBPM = 60000 / beatMsec;
    // LED-Ausgabe bei Herzschlag
    digitalWrite(ledPin,1);
    String msgCoeur = String(heartRateBPM);
    mqtt_client.publish(mqtt_topic5, msgCoeur.c_str());

    // Serielle Datenausgabe
    Serial.print("Puls erkannt: ");
    Serial.println(heartRateBPM);

    beatMsec = 0;
  }
  else
  {
    digitalWrite(ledPin,0);
  }

  delay(delayMsec);
  beatMsec += delayMsec;
}
