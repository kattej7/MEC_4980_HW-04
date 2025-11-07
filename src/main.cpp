#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

const char *ssid = "NSA Security Van HQ";
const char *password = "windowstothehallway";
const int wifi_channel = 6;

#define step_pin_hr 11
#define dir_pin_hr 12
#define step_pin_min 9
#define dir_pin_min 10
#define hourLimit A0
#define minuteLimit A1

#define steps_per_rev 200
#define steps_per_deg (steps_per_rev / 360.0)

int initial = 0;
float prev_hour_angle = 0;
float prev_minute_angle = 0;

const long GMT_OFFSET_SEC = -6 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0;

void stepMotor(int stepPin, int dirPin, float degrees, bool reverse = false)
{
  int steps = abs(degrees * steps_per_deg);

  bool direction = degrees >= 0 ? HIGH : LOW;
  if (reverse)
    direction = !direction;

  digitalWrite(dirPin, direction);

  for (int i = 0; i < steps; i++)
  {
    digitalWrite(stepPin, HIGH);
    delay(10);
    digitalWrite(stepPin, LOW);
    delay(10);
  }
}

void homeMotors()
{
  Serial.println("Homing motors...");

  pinMode(hourLimit, INPUT_PULLDOWN);
  pinMode(minuteLimit, INPUT_PULLDOWN);

  Serial.println("Homing hour motor...");
  digitalWrite(dir_pin_hr, LOW);
  while (digitalRead(hourLimit) == HIGH)
  {
    digitalWrite(step_pin_hr, HIGH);
    delay(15);
    digitalWrite(step_pin_hr, LOW);
    delay(15);
  }
  Serial.println("Hour motor homed.");

  Serial.println("Homing minute motor...");
  digitalWrite(dir_pin_min, HIGH);
  while (digitalRead(minuteLimit) == HIGH)
  {
    digitalWrite(step_pin_min, HIGH);
    delay(3);
    digitalWrite(step_pin_min, LOW);
    delay(3);
  }
  Serial.println("Minute motor homed.");

  Serial.println("Motors homed to 12:00 position.");
}

void setup()
{
  Serial.begin(9600);

  pinMode(step_pin_hr, OUTPUT);
  pinMode(dir_pin_hr, OUTPUT);
  pinMode(step_pin_min, OUTPUT);
  pinMode(dir_pin_min, OUTPUT);

  pinMode(hourLimit, INPUT_PULLUP);
  pinMode(minuteLimit, INPUT_PULLUP);

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org");
  delay(1000);

  homeMotors();
}

void loop()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time, retrying...");
    delay(2000);
    return;
  }

  int hour = timeinfo.tm_hour % 12;
  int minute = timeinfo.tm_min;
  int second = timeinfo.tm_sec;

  float hour_angle = (hour * 30.0);
  float minute_angle = (minute * 6.0);

  Serial.printf("Time: %02d:%02d:%02d | Hour: %.1f°, Minute: %.1f°\n",
                hour, minute, second, hour_angle, minute_angle);

  if (initial == 1)
  {
    float newHour = hour_angle - prev_hour_angle;
    float newMinute = minute_angle - prev_minute_angle;

    stepMotor(step_pin_hr, dir_pin_hr, newHour);
    stepMotor(step_pin_min, dir_pin_min, newMinute, true);
  }
  else
  {
    stepMotor(step_pin_hr, dir_pin_hr, hour_angle);
    stepMotor(step_pin_min, dir_pin_min, minute_angle, true);
    initial = 1;
  }

  prev_hour_angle = hour_angle;
  prev_minute_angle = minute_angle;
  delay(60000);
}
