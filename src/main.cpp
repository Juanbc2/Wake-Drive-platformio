#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Parametros Wifi
const char *SSID = "Flia BC";        // name of your WiFi network
const char *PASSWD = "43675702LMcc"; // password of the WiFi network
WiFiClient wClient;

/* ----- MQTT ----- */
const char *mqttBroker = "192.168.1.6"; // IP address of your MQTT
const char *ID = "pulse-bracelet";      // Name of our device, must be unique
// Topics
const char *topic1 = "shock";
const char *topic2 = "pulse";
const char *topic3 = "emergency";

// Setup MQTT client
PubSubClient mqttClient(wClient);

// put function declarations here:
void setup_wifi();
void mqttConnect();
void sendData();
void emergencyCheck();
void alarm();

// Variables del programa
#define BUTTON 36
#define SHOCK 39
#define PULSE 34

#define BUZZER 23
#define LED1 22
#define LED2 21

#define SERIAL_SPEED 115200

int belowThreshold = 1500;
bool active = false;
int lastButtonState = LOW;

int pulseCounter = 0;
int shockCounter = 0;
bool shockUndetected = false;
bool belowPulseThreshold = false;
bool emergency = false;

void setup()
{
  // Serial
  Serial.begin(SERIAL_SPEED);
  // WiFi
  setup_wifi();
  // MQTT setup
  mqttClient.setServer(mqttBroker, 1883);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(PULSE, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(SHOCK, INPUT);
}

void loop()
{
  // Check if we are still connected to the MQTT broker
  if (!mqttClient.connected())
  {
    mqttConnect();
  }
  // Let PubSubClient library do his magic
  mqttClient.loop();
  // active or deactive
  int buttonState = digitalRead(BUTTON);
  Serial.println(buttonState);
  if (buttonState != lastButtonState)
  { // Si el estado del botón ha cambiado
    if (buttonState == HIGH)
    {                   // Si el botón está presionado
      active = !active; // Cambia el estado de 'active'
    }
    lastButtonState = buttonState; // Actualiza el estado anterior del botón
  }
  // data to send
  if (active)
  {
    sendData();
    emergencyCheck();
  }
  else
  {
    emergency = false;
  }
}

void emergencyCheck()
{
  int shockState = digitalRead(SHOCK);
  if (shockState == HIGH)
  {
    shockUndetected = true;
  }
  else
  {
    shockUndetected = false;
  }

  if (shockUndetected)
  {
    if (shockCounter < 10)
    {
      shockCounter++;
    }
    else
    {
      emergency = true;
      alarm();
    }
  }
  else
  {
    mqttClient.publish(topic3, "0");
    shockCounter = 0;
    emergency = false;
  }

  int pulseState = analogRead(PULSE);
  if (pulseState < belowThreshold)
  {
    belowPulseThreshold = true;
  }
  else
  {
    belowPulseThreshold = false;
  }

  if (belowPulseThreshold)
  {
    if (pulseCounter < 10)
    {
      pulseCounter++;
    }
    else
    {
      emergency = true;
      alarm();
    }
  }
  else
  {
    pulseCounter = 0;
    shockCounter = 0;
    emergency = false;
    mqttClient.publish(topic3, "0");
  }
  delay(1000);
}

void alarm()
{
  mqttClient.publish(topic3, "1");
  if (emergency)
  {
    tone(BUZZER, 440, 200);
    delay(100);
    noTone(BUZZER);
    digitalWrite(LED1, HIGH);

    tone(BUZZER, 494, 500);
    delay(300);
    noTone(BUZZER);
    digitalWrite(LED1, LOW);

    tone(BUZZER, 523, 300);
    delay(300);
    noTone(BUZZER);
  }
}

void sendData()
{
  digitalWrite(LED2, HIGH);
  int shockState = digitalRead(SHOCK);
  mqttClient.publish(topic1, String(shockState).c_str());

  int pulseState = analogRead(PULSE);
  mqttClient.publish(topic2, String(pulseState).c_str());

  delay(1000);
  digitalWrite(LED2, LOW);
}

// Wifi conection
void setup_wifi()
{
  Serial.print("\nConnecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWD); // Connect to network

  while (WiFi.status() != WL_CONNECTED)
  { // Wait for connection
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// --- MQTT ---
void mqttConnect()
{
  // Loop until we're reconnected
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(ID))
    {
      Serial.println("OK");
      // Topic(s) subscription
      // En este ejemplo solo se publica
    }
    else
    {
      // Retry connection
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}