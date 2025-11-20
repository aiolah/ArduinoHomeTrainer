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

static unsigned long lastPublishTime = 0;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// Heartbeat sensor
int ledPin = 13;
int analogPin = 0;
const int delayMsec = 60;

// Buzzer
#define D0 -1
#define D1 262
#define D2 293
#define D3 329
#define D4 349
#define D5 392
#define D6 440
#define D7 494
#define M1 523
#define M2 586
#define M3 658
#define M4 697
#define M5 783
#define M6 879
#define M7 987
#define H1 1045
#define H2 1171
#define H3 1316
#define H4 1393
#define H5 1563
#define H6 1755
#define H7 1971
#define WHOLE 1
#define HALF 0.5
#define QUARTER 0.25
#define EIGHTH 0.25
#define SIXTEENTH 0.625

// L'hymne à la joie
int tune[] =
{
  M3,M3,M4,M5, 
  M5,M4,M3,M2, 
  M1,M1,M2,M3, 
  M3,M2,M2, 
  M3,M3,M4,M5, 
  M5,M4,M3,M2, 
  M1,M1,M2,M3, 
  M2,M1,M1, 
  M2,M2,M3,M1, 
  M2,M3,M4,M3,M1, 
  M2,M3,M4,M3,M2, 
  M1,M2,D5,D0, 
  M3,M3,M4,M5, 
  M5,M4,M3,M4,M2, 
  M1,M1,M2,M3, 
  M2,M1,M1
};

float durt[]=
{
  1,1,1,1,
  1,1,1,1,
  1,1,1,1,
  1+0.5,0.5,1+1,
  1,1,1,1,
  1,1,1,1,
  1,1,1,1,
  1+0.5,0.5,1+1,
  1,1,1,1,
  1,0.5,0.5,1,1,
  1,0.5,0.5,1,1,
  1,1,1,1,
  1,1,1,1,
  1,1,1,0.5,0.5,
  1,1,1,1,
  1+0.5,0.5,1+1,
};

// Au clair de la Lune
int lune[] =
{
  M1,M1,M1,M2,M3,M2,
  M1,M3,M2,M2,M1
};

float rythmLune[] =
{
  1,1,1,1,1+1,1+1,
  1,1,1,1,1,
};

// int length;
int tonepin = 7;

// Zelda chest opening sound
/* This code is derived from:
 * http://www.arduino.cc/en/Tutorial/Melody
 * This plays the chest noise from the Legend of Zelda on a piezo buzzer connected to pin 9 and ground. It has been tuned to a buzzer I had on hand, but you may want to adjust the values, testing against a tuner.
 */
  
int speakerPin = 7;
const int switchPin = 1;

char notes[] = "gabygabyxzCDxzCDabywabywzCDEzCDEbywFCDEqywFGDEqi        azbC"; // a space represents a rest
int length = sizeof(notes); // the number of notes
int beats[] = { 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 2,3,3,16,};
int tempo = 75;

// Function declarations
void connectToWiFi();
void connectToMQTTBroker();
void mqttCallback(char *topic, byte *payload, unsigned int length);

// ------------------------------------------------------------
// SETUP
// ------------------------------------------------------------
void setup()
{
  Serial.begin(9600);

  connectToWiFi();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();

  // Heartbeat sensor init
  pinMode(ledPin, OUTPUT);

  Serial.println("Heartbeat Detektion Beispielcode.");

  // Buzzer L'hymne à la joie
  // pinMode(tonepin, OUTPUT);
  // length = sizeof(tune)/sizeof(tune[0]);

  // Buzzer Zelda music
  pinMode(switchPin, INPUT);
  digitalWrite(switchPin, HIGH);
}

// ------------------------------------------------------------
// PRINT MAC ADDRESS
// ------------------------------------------------------------
String printMacAddress()
{
  String MAC_address = "";
  
  byte mac[6];
  Serial.print("MAC Address: ");

  WiFi.macAddress(mac);

  for (int i = 0; i < 6; i++)
  {
    MAC_address += String(mac[i], HEX);
    if (i < 5)
      MAC_address += ":";
    if (mac[i] < 16)
      client_id += "0";
    client_id += String(mac[i], HEX);
  }

  Serial.println(MAC_address);
  return MAC_address;
}

// ------------------------------------------------------------
// WIFI CONNECTION
// ------------------------------------------------------------
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

// ------------------------------------------------------------
// MQTT CONNECTION
// ------------------------------------------------------------
void connectToMQTTBroker()
{
  while (!mqtt_client.connected())
  {
    Serial.print("Connecting to MQTT Broker as ");
    Serial.print(client_id.c_str());
    Serial.println("...");

    if (mqtt_client.connect(client_id.c_str()))
    {
      Serial.println("Connected to MQTT broker");

      mqtt_client.subscribe(mqtt_topic1);
      mqtt_client.subscribe(mqtt_topic2);
      mqtt_client.subscribe(mqtt_topic3);
      mqtt_client.subscribe(mqtt_topic4);
      mqtt_client.subscribe(mqtt_topic5);

      String message = "Hello EMQX I'm " + client_id;
      mqtt_client.publish(mqtt_topic1, message.c_str());
    }
    else
    {
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
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  Serial.print("Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    messageTemp += (char)payload[i];
  }

  Serial.println(messageTemp);
  Serial.println("-----------------------");
}

// ------------------------------------------------------------
// HEARTBEAT SENSOR
// ------------------------------------------------------------
int rawValue;

bool heartbeatDetected(int IRSensorPin, int delay)
{
  static int maxValue = 0;
  static bool isPeak = false;

  bool result = false;

  rawValue = analogRead(IRSensorPin);
  rawValue *= (1000 / delay);

  if (rawValue * 4L < maxValue)
    maxValue = rawValue * 0.8;

  if (rawValue > maxValue - (1000 / delay))
  {

    if (rawValue > maxValue)
      maxValue = rawValue;

    if (!isPeak)
      result = true;

    isPeak = true;
  }
  else if (rawValue < maxValue - (3000 / delay))
  {
    isPeak = false;
    maxValue -= (1000 / delay);
  }

  return result;
}

// ------------------------------------------------------------
// ZELDA MUSIC
// ------------------------------------------------------------
void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}

void playNote(char note, int duration) {
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'x', 'a', 'z', 'b', 'C', 'y', 'D', 'w', 'E', 'F', 'q', 'G', 'i' };
  // c=C4, C = C5. These values have been tuned.
  int tones[] = { 1898, 1690, 1500, 1420, 1265, 1194, 1126, 1063, 1001, 947, 893, 843, 795, 749, 710, 668, 630, 594 };
   
  // play the tone corresponding to the note name
  for (int i = 0; i < 18; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}

// ------------------------------------------------------------
// LOOP
// ------------------------------------------------------------
void loop()
{
  if (!mqtt_client.connected())
    connectToMQTTBroker();

  mqtt_client.loop();

  unsigned long currentTime = millis();
  static int beatMsec = 0;
  int heartRateBPM = 0;

  // Publish MAC periodically
  if (currentTime - lastPublishTime >= 10000)
  {
    lastPublishTime = currentTime;

    String MAC = printMacAddress();
    String MACmsg = "Adresse MAC " + MAC;

    String MAC_OIIA = "OIIA ᓚᘏᗢ MAC Address : " + printMacAddress();

    // Uncomment if needed
    // mqtt_client.publish(mqtt_topic3, MACmsg.c_str());
    // mqtt_client.publish(mqtt_topic4, "Miaou");
  }

  // Au clair de la Lune
  // Change lune and rythmLune variables with tune and durt variables to play L'hymne à la joie
  // for(int x=0; x<length; x++)
  // {
  //   tone(tonepin, lune[x]);
  //   delay(500 * rythmLune[x]); noTone(tonepin);
  // }

  // delay(2000);

    pinMode(speakerPin, OUTPUT);
    // if (digitalRead(switchPin) == 1)
    // {
      for (int i = 0; i < length; i++)
      {
        if (notes[i] == ' ')
        {
          delay(beats[i] * tempo); // rest
        }
        else
        {
          playNote(notes[i], beats[i] * tempo);
        }
      
      // pause between notes
      delay(tempo / 2); 
    // }
  }

  delay(100);

  // Heartbeat detection
  if (heartbeatDetected(analogPin, delayMsec))
  {
    heartRateBPM = 60000 / beatMsec;

    // Light the LED when a heartbeat is detected
    digitalWrite(ledPin, HIGH);

    String msgCoeur = String(heartRateBPM);
    mqtt_client.publish(mqtt_topic5, msgCoeur.c_str());

    Serial.print("Puls erkannt: ");
    Serial.println(heartRateBPM);

    beatMsec = 0;
  }
  else
  {
    digitalWrite(ledPin, LOW);
  }

  delay(delayMsec);
  beatMsec += delayMsec;
}
