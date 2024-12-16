#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define BL
#define BLYNK_TEMPLATE_NAME "Irrigation System"

//Authentication code
char auth[] = "";
char ssid[] = "******"; //Wifi SSID
char pass[] = "*****";  //Wifi Password

BlynkTimer timer;


bool Relay = false;
bool requestThresholdFlag = false;
bool automaticMode = false;
bool waterNowTriggered = false;
bool pumpActive = false; // Flag to indicate pump state

const int moisturePin = A0;
const int controlPin = D5;

int moistureThreshold = 0;
int moistureValue = 0;
int waterAmount = 0;
float flowRate = 28.4; // ml/s

unsigned long previousDispenseTime = 0;
unsigned long dispenseInterval = 100; // Interval between water dispensing in milliseconds

void setup()
{
  Serial.begin(9600);

  Blynk.begin(auth, ssid, pass);
  pinMode(controlPin, OUTPUT);

  // Set initial values to zero
  Blynk.virtualWrite(V1, 0);
  Blynk.virtualWrite(V2, 0);
  Blynk.virtualWrite(V3, 0);
  Blynk.virtualWrite(V4, 0);

  timer.setInterval(400L, checkMoisture);
  timer.setInterval(1000L, requestMoistureThreshold);
}

void loop()
{
  Blynk.run();
  timer.run();
  dispenseWater(); // Check if it's time to dispense water
}

BLYNK_WRITE(V3)
{
  automaticMode = param.asInt();

  if (!automaticMode)
  {
    waterNowTriggered = false; // Reset water now trigger when switching to manual mode
  }
}

BLYNK_WRITE(V1)
{
  if (!automaticMode && param.asInt() == 1)
  {
    if (!pumpActive)
    {
      waterNowTriggered = true;
      pumpActive = true;
      digitalWrite(controlPin, HIGH);
    }
  }
  else if (automaticMode && param.asInt() == 1)
  {
    Serial.println("Automatic Mode is still on! Switch it off first to access Manual mode features");
  }
}

BLYNK_WRITE(V2)
{
  if (!automaticMode && !waterNowTriggered)
  {
    waterAmount = param.asInt();
  }
}

void checkMoisture()
{
  if (automaticMode && !waterNowTriggered)
  {
    moistureValue = analogRead(moisturePin);
    int mappedMoistureValue = map(moistureValue, 1024, 590, 0, 100);
    Blynk.virtualWrite(V0, mappedMoistureValue );
    if (mappedMoistureValue < moistureThreshold)
    {
      if (!pumpActive) // Switch on the pump only if it's not already active
      {
        pumpActive = true;
        digitalWrite(controlPin, HIGH);
      }
    }
    else
    {
      if (pumpActive) // Switch off the pump if it was active
      {
        pumpActive = false;
        digitalWrite(controlPin, LOW);
      }
    }
  }
}

void requestMoistureThreshold()
{
  if (automaticMode && !waterNowTriggered)
  {
    requestThresholdFlag = true;
  }
}

BLYNK_WRITE(V4)
{
  if (requestThresholdFlag && automaticMode)
  {
    moistureThreshold = param.asInt();
    requestThresholdFlag = false;
  }
}

void dispenseWater()
{
  if (!automaticMode && waterNowTriggered)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousDispenseTime >= dispenseInterval)
    {
      previousDispenseTime = currentMillis;

      int duration = waterAmount / flowRate * 1000; // Calculate duration in milliseconds
      digitalWrite(controlPin, LOW);
      delay(500); // Adjust this delay if needed
      digitalWrite(controlPin, HIGH);
      delay(duration);
      digitalWrite(controlPin, LOW);
      waterNowTriggered = false;
      pumpActive = false;
      Blynk.virtualWrite(V1, 0); // Reset Water Now button to off position
    }
  }
}
