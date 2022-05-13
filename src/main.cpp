#include <Arduino.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <TimeLib.h>

#define RELAY_PIN D1

// Network SSID
const char* ssid = "Starlink";
const char* password = "HawaiiLite";

unsigned long epochTime = 0;
unsigned long startTime = 0;
int currentDay;
String currentDate;


//Your Domain name with URL path or IP address with path
String hockeyApi = "https://v1.hockey.api-sports.io/games";
String apiKey = "5111097bc93c2838ad5396df52b59707";
const int httpsPort = 443;

String league = "57";
String season = "2021";
int team = 698;
int currentGoals = 0;

bool startup = true;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup() {
  // begin serial
  Serial.begin(9600);
  delay(10);

  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  // Connect WiFi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname("RedLight");
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Print the IP address
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Start Time Client
  timeClient.begin();
}

void startupTest() {
  Serial.print("Startup");
  digitalWrite(RELAY_PIN, HIGH);
  delay(2000);
  digitalWrite(RELAY_PIN, LOW);
  delay(2000);
}

void scored() { 
  Serial.print("GOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOAAAAAAAAALLLLLLLLLLLLLLLLL");
  currentGoals += 1;
  digitalWrite(RELAY_PIN, HIGH);
  delay(15000);
  digitalWrite(RELAY_PIN, LOW);
  delay(2000);
  digitalWrite(RELAY_PIN, HIGH);
  delay(10000);
  digitalWrite(RELAY_PIN, LOW);
  delay(2000);
  digitalWrite(RELAY_PIN, HIGH);
  delay(5000);
  digitalWrite(RELAY_PIN, LOW);

  // Wait a set delay for game restart
  delay(120000);
}

unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

void gameFinished() {

}

void refreshScore(String date)
{
	static const unsigned long REFRESH_INTERVAL = 30000; // ms
	static unsigned long lastRefreshTime = 0;

	if((millis() - lastRefreshTime >= REFRESH_INTERVAL) || startup)
	{
    startup = false;
		lastRefreshTime += REFRESH_INTERVAL;
    Serial.println("Update");

    if (WiFi.status() == WL_CONNECTED) 
    {
      WiFiClientSecure client;
      client.connect(hockeyApi, httpsPort);
      client.setInsecure();

      HTTPClient http; //Object of class HTTPClient
      String path = hockeyApi + "?season=" + season + "&league=" + league + "&team=" + String(team) + "&date=" + date;
      Serial.println("calling: " + path);

      http.useHTTP10(true);
      http.begin(client, path);
      http.addHeader("x-apisports-key", apiKey);
      // http.addHeader("X-RapidAPI-Host", "v1.hockey.api-sports.io");

      int httpCode = http.GET();

      if (httpCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpCode);
        // Parse response
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, http.getStream());

        startTime = doc["response"][0]["timestamp"].as<long>();
        if (startTime > epochTime) { return; }

        String status = doc["response"][0]["status"]["short"].as<String>();
        if (status == "NS") { return; }
        if (status == "AOT" || status == "AP" || status == "FT") { 
          gameFinished();
          return; 
        }

        const int homeId = doc["response"][0]["teams"]["home"]["id"].as<int>();
        String key;
        if (homeId == team) {
          key = "home";
        } else {
          key = "away";
        }

        const int gameScore = doc["response"][0]["scores"][key].as<int>();
        if (gameScore > currentGoals) {
          currentGoals = gameScore;
          scored();
        }
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpCode);
      }
      // Free resources
      http.end();
    }
	}
}

void loop() {
  if (startup) { 
    startupTest();
    startup = false;
  }

  epochTime = getTime();
  int d = day(epochTime);
  int m = month(epochTime);
  int y = year(epochTime);
  currentDate = String(y) + "-" + String(m) + "-" + String(d);
  Serial.print(currentDate);

  if (epochTime > startTime) { 
    refreshScore(currentDate);
  }

  // No action we delay a minute and check again
  delay(60000);
}