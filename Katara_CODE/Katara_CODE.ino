/***************************************************
  Includes
 ***************************************************/
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

#include <SDI12.h>
#include <FastLED.h>

/***************************************************
  Wi-Fi Credentials
 ***************************************************/
const char* ssid     = "living";
const char* password = "deranke68";

/***************************************************
  Firebase (Firestore) Configuration
 ***************************************************/
// Firestore configuration
const char* firestoreHost      = "firestore.googleapis.com";
const char* firestoreProjectID = "project-katara";  
const char* apiKey            = "AIzaSyCWSBStmBWQA8IzZtEyxeb2tco0xXVhpsg";             

// This is the ID for this specific board in Firestore.
// We'll send data to the document named "test_plant"
String plantDocumentID = "test_plant";

/***************************************************
  Time and Timing
 ***************************************************/
// NTP servers for time syncing
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";

// Interval to read sensor & update database (in milliseconds)
// Now set to 1 minute
unsigned long sensorInterval = 10UL * 1000UL; // 10 seconds
unsigned long previousSensorMillis = 0;

/***************************************************
  12-hour LED 'auto' mode
 ***************************************************/
// We track how long the LED has been ON or OFF
static unsigned long ledAutoCycleStartTime = 0; 
bool ledAutoState = false; 
unsigned long halfDaySeconds = 12UL * 60UL * 60UL; // 12 hours in seconds

/***************************************************
  Pins and Constants for the TEROS 12 Sensor
 ***************************************************/
#define SDI12_DATA_PIN 5       // Pin connected to the SDI-12 data line of the sensor
#define MOSFET_CONTROL_PIN 26  // Pin connected to the MOSFET gate powering the sensor

SDI12 mySDI12(SDI12_DATA_PIN);

/***************************************************
  Pins and Constants for the Water Motor
 ***************************************************/
// Motor on pin 25, reverse logic (LOW = ON, HIGH = OFF)
#define MOTOR_PIN 25
int motorOnTime = 3000; // 3 seconds ON time

/***************************************************
  Pins and Constants for NeoPixel (WS2811)
 ***************************************************/
#define NUM_LEDS 30
#define DATA_PIN 18
#define LED_TYPE WS2811
#define COLOR_ORDER BRG
CRGB leds[NUM_LEDS];
#define BRIGHTNESS 230

// We'll track the last LED mode so we only update the strip if it changes
String lastLedMode = "";
bool   lastLedAutoState = false;  // only for "auto" mode on/off toggling

/***************************************************
  Variables to store settings fetched from database
 ***************************************************/
// Default values if Firestore doesn't have them
int desiredMoisture = 1000;     // Water if below 1000
String ledMode      = "off";    // LED strip default OFF
bool isActive       = true;     // If inactive, skip watering, sensor read, etc.

/***************************************************
  Forward Declarations
 ***************************************************/
void setupWiFi();
void setupTime();
String getCurrentTimeISO8601();
bool fetchSettingsFromFirestore();
void performMeasurement();
void handleLEDMode();
void handleInactiveState();
void controlMotorIfNeeded(float moistureValue);

// Firestore send function (partial update)
void sendSensorDataToFirestore(float moisture, float temperature, float conductivity);

// SDI-12 read/parse
String readFullResponse();
void parseSensorData(const String &data, float &moisture, float &temperature, float &conductivity);

/***************************************************
  Setup
 ***************************************************/
void setup() {
  Serial.begin(115200);

  // === Wi-Fi ===
  setupWiFi();

  // === Time (NTP) ===
  setupTime();

  // === Pin Modes ===
  pinMode(MOSFET_CONTROL_PIN, OUTPUT);
  digitalWrite(MOSFET_CONTROL_PIN, HIGH); // sensor power OFF/ON (active HIGH)
  
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, HIGH); // Motor OFF (reverse logic)

  // === SDI-12 Begin ===
  mySDI12.begin();
  Serial.println("SDI-12 sensor communication initialized");

  // === NeoPixel Setup ===
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  // Initialize LED strip to "off" color
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // === Attempt to fetch initial settings from Firestore ===
  bool gotSettings = fetchSettingsFromFirestore();
  if (gotSettings) {
    Serial.println("Fetched initial settings from Firestore.");
  } else {
    Serial.println("Failed to fetch initial settings from Firestore.");
  }
}

/***************************************************
  Main Loop
 ***************************************************/
void loop() {
  // Check Wi-Fi. If not connected, skip actions (but keep looping).
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping loop actions.");
    delay(2000);
    return;
  }

  // Periodically fetch the latest settings from Firestore
  bool gotSettings = fetchSettingsFromFirestore();
  if (!gotSettings) {
    Serial.println("Failed to update settings from Firestore, using old settings.");
  }

  // If device is inactive, do minimal tasks
  if (!isActive) {
    handleInactiveState();
    return; // skip the rest
  }

  // It's active => proceed with normal operation
  unsigned long currentMillis = millis();
  if (currentMillis - previousSensorMillis >= sensorInterval) {
    previousSensorMillis = currentMillis;
    performMeasurement(); // read sensor, send data, possibly water
  }

  // Handle LED logic
  handleLEDMode();

  // Small delay to avoid spamming Firestore too fast
  delay(500);
}

/***************************************************
  Wi-Fi Setup
 ***************************************************/
void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/***************************************************
  Time Setup (NTP)
 ***************************************************/
void setupTime() {
  configTime(0, 0, ntpServer1, ntpServer2);
  Serial.print("Syncing time");
  while (time(nullptr) < 100000) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
  Serial.println("Time synchronized!");
}

/***************************************************
  getCurrentTimeISO8601
 ***************************************************/
String getCurrentTimeISO8601() {
  time_t now = time(nullptr);
  struct tm* timeinfo = gmtime(&now);
  char buf[sizeof("2023-05-18T20:52:52Z")];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
  return String(buf);
}

/***************************************************
  fetchSettingsFromFirestore()
  - updates desiredMoisture, ledMode, isActive
 ***************************************************/
bool fetchSettingsFromFirestore() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, skipping Firestore GET.");
    return false;
  }

  HTTPClient http;
  // Example path: GET /v1/projects/<PROJECT_ID>/databases/(default)/documents/planters/test_plant?key=<API_KEY>
  String url = String("https://") + firestoreHost + "/v1/projects/" + firestoreProjectID +
               "/databases/(default)/documents/planters/" + plantDocumentID +
               "?key=" + apiKey;

  http.begin(url);
  int httpResponseCode = http.GET();
  if (httpResponseCode <= 0) {
    Serial.print("Error on GET: ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }

  if (httpResponseCode == 200) {
    String response = http.getString();
    http.end();

    // Parse JSON
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return false;
    }

    // If fields exist, parse them
    if (doc["fields"].isNull()) {
      Serial.println("No fields found in Firestore document.");
      return false;
    }

    JsonObject fields = doc["fields"];

    // desiredMoisture
    if (!fields["desiredMoisture"].isNull()) {
      // Firestore might store as integerValue or doubleValue
      int newDesiredMoisture = desiredMoisture; // default
      if (!fields["desiredMoisture"]["integerValue"].isNull()) {
        newDesiredMoisture = fields["desiredMoisture"]["integerValue"];
      } else if (!fields["desiredMoisture"]["doubleValue"].isNull()) {
        newDesiredMoisture = (int)fields["desiredMoisture"]["doubleValue"].as<float>();
      }
      if (newDesiredMoisture != desiredMoisture) {
        desiredMoisture = newDesiredMoisture;
        Serial.print("desiredMoisture updated to: ");
        Serial.println(desiredMoisture);
      }
    }

    // ledMode
    if (!fields["ledMode"].isNull()) {
      String newLedMode = fields["ledMode"]["stringValue"] | ledMode;
      if (newLedMode != ledMode) {
        ledMode = newLedMode;
        Serial.print("ledMode updated to: ");
        Serial.println(ledMode);
      }
    }

    // active
    if (!fields["active"].isNull()) {
      String activeStr = fields["active"]["stringValue"] | String(isActive ? "active" : "inactive");
      bool newIsActive = (activeStr.equalsIgnoreCase("active"));
      if (newIsActive != isActive) {
        isActive = newIsActive;
        Serial.print("isActive updated to: ");
        Serial.println(isActive ? "true" : "false");
      }
    }

    return true;
  } else {
    Serial.print("Firestore GET response code: ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }
}

/***************************************************
  performMeasurement()
  Reads from Teros 12, prints result, sends to Firestore
 ***************************************************/
void performMeasurement() {
  Serial.println("Measuring TEROS 12...");

  // Power on the sensor
  digitalWrite(MOSFET_CONTROL_PIN, HIGH);
  delay(100); 

  // Send measurement command
  String measurementCommand = "0M!";
  mySDI12.sendCommand(measurementCommand);
  delay(1500);

  // Request data
  String dataCommand = "0D0!";
  mySDI12.sendCommand(dataCommand);
  delay(300);

  // Read full response
  String response = readFullResponse();
  if (response.length() > 0) {
    float moistureVal = 0, temperatureVal = 0, conductivityVal = 0;
    parseSensorData(response, moistureVal, temperatureVal, conductivityVal);

    Serial.print("Moisture: ");
    Serial.println(moistureVal);
    Serial.print("Temperature: ");
    Serial.println(temperatureVal);
    Serial.print("Conductivity: ");
    Serial.println(conductivityVal);

    // Send data to Firestore (partial update)
    sendSensorDataToFirestore(moistureVal, temperatureVal, conductivityVal);

    // Check if we need to water
    controlMotorIfNeeded(moistureVal);
  } else {
    Serial.println("No data received. Check sensor and command syntax.");
  }

  // Power off the sensor if you want to save power
  digitalWrite(MOSFET_CONTROL_PIN, LOW);
}

/***************************************************
  readFullResponse()
 ***************************************************/
String readFullResponse() {
  String data;
  while (mySDI12.available()) {
    data += (char)mySDI12.read();
  }
  return data;
}

/***************************************************
  parseSensorData()
  - modifies the passed-in float references
 ***************************************************/
void parseSensorData(const String &data, float &moisture, float &temperature, float &conductivity) {
  // Example data: "0+12.34+23.45+1234.56"
  int firstPlus  = data.indexOf('+', 1);
  int secondPlus = data.indexOf('+', firstPlus + 1);
  int thirdPlus  = data.indexOf('+', secondPlus + 1);

  if (firstPlus < 0 || secondPlus < 0 || thirdPlus < 0) {
    Serial.println("Parse error: not enough '+' symbols in sensor response.");
    return;
  }

  String sMoisture     = data.substring(firstPlus + 1, secondPlus);
  String sTemperature  = data.substring(secondPlus + 1, thirdPlus);
  String sConductivity = data.substring(thirdPlus + 1);

  moisture     = sMoisture.toFloat();
  temperature  = sTemperature.toFloat();
  conductivity = sConductivity.toFloat();
}

/***************************************************
  sendSensorDataToFirestore()
  - PARTIAL UPDATE so we don't overwrite other fields
 ***************************************************/
void sendSensorDataToFirestore(float moisture, float temperature, float conductivity) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, skipping Firestore send.");
    return;
  }

  HTTPClient http;
  String url = String("https://") + firestoreHost + "/v1/projects/" + firestoreProjectID +
               "/databases/(default)/documents/planters/" + plantDocumentID +
               "?key=" + apiKey;

  // We'll do a partial update (PATCH) with updateMask on just the sensor fields
  // so we don't overwrite "ledMode", "active", "desiredMoisture", etc.
  // e.g. ?updateMask.fieldPaths=moisture&updateMask.fieldPaths=temperature&...
  url += "&currentDocument.exists=true"; 
  url += "&updateMask.fieldPaths=moisture";
  url += "&updateMask.fieldPaths=temperature";
  url += "&updateMask.fieldPaths=conductivity";
  url += "&updateMask.fieldPaths=timestamp";

  String timeISO = getCurrentTimeISO8601();

  // JSON body
  // {
  //   "fields": {
  //     "moisture": {"doubleValue": ... },
  //     "temperature": {"doubleValue": ... },
  //     "conductivity": {"doubleValue": ... },
  //     "timestamp": {"timestampValue": "2023-01-01T12:34:56Z"}
  //   }
  // }
  StaticJsonDocument<400> doc;
  JsonObject fields = doc.createNestedObject("fields");

  JsonObject moistureObj         = fields.createNestedObject("moisture");
  moistureObj["doubleValue"]     = moisture;

  JsonObject temperatureObj      = fields.createNestedObject("temperature");
  temperatureObj["doubleValue"]  = temperature;

  JsonObject conductivityObj     = fields.createNestedObject("conductivity");
  conductivityObj["doubleValue"] = conductivity;

  JsonObject timestampObj        = fields.createNestedObject("timestamp");
  timestampObj["timestampValue"] = timeISO;

  String payload;
  serializeJson(doc, payload);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.sendRequest("PATCH", payload);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Firestore response code: ");
    Serial.println(httpResponseCode);
    Serial.println(response);
  } else {
    Serial.print("Error on sending PATCH: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

/***************************************************
  controlMotorIfNeeded()
  - Turn motor on for 3 seconds if moisture < desiredMoisture
 ***************************************************/
void controlMotorIfNeeded(float moistureValue) {
  if (moistureValue < desiredMoisture) {
    Serial.print("Current moisture (");
    Serial.print(moistureValue);
    Serial.print(") < desired (");
    Serial.print(desiredMoisture);
    Serial.println("). Watering for 3 seconds...");

    digitalWrite(MOTOR_PIN, LOW); // Motor ON (reverse logic)
    delay(motorOnTime);
    // In case there's a glitch, yield to the OS
    yield();
    digitalWrite(MOTOR_PIN, HIGH); // Motor OFF
    Serial.println("Watering done.");
  } else {
    Serial.println("Moisture above threshold, no watering needed.");
  }
}

/***************************************************
  handleLEDMode()
  - "on", "off", or "auto" (12 on, 12 off)
  - Only update if the state changes (avoid flicker)
 ***************************************************/
void handleLEDMode() {
  // If mode is unchanged AND auto-state is unchanged, do nothing
  if (ledMode == lastLedMode) {
    // If in auto mode, we might still need to check if 12 hours passed
    // But only update the strip if the auto ON/OFF state changes
    if (ledMode == "auto") {
      // We want 12 hours ON, 12 hours OFF
      if (ledAutoCycleStartTime == 0) {
        // Begin a cycle
        ledAutoCycleStartTime = time(nullptr);
        ledAutoState = true; // start ON
        lastLedAutoState = true;
        setLedStripColor(CRGB::Purple);
      }

      time_t now = time(nullptr);
      unsigned long elapsed = now - ledAutoCycleStartTime;

      if (ledAutoState && elapsed >= halfDaySeconds) {
        // Switch from ON to OFF
        ledAutoCycleStartTime = now;
        ledAutoState = false;
      } else if (!ledAutoState && elapsed >= halfDaySeconds) {
        // Switch from OFF to ON
        ledAutoCycleStartTime = now;
        ledAutoState = true;
      }

      // If auto-state changed from the last time, update the strip
      if (ledAutoState != lastLedAutoState) {
        if (ledAutoState) {
          setLedStripColor(CRGB::Purple);
        } else {
          setLedStripColor(CRGB::Black);
        }
        lastLedAutoState = ledAutoState;
      }
    }
    // If mode != auto, do nothing since mode hasn't changed
    return;
  }

  // If code reaches here, the mode changed from lastLedMode => we update
  lastLedMode = ledMode;  // store new mode

  if (ledMode == "off") {
    setLedStripColor(CRGB::Black);
    Serial.println("LED strip set to OFF.");
  }
  else if (ledMode == "on") {
    setLedStripColor(CRGB::Purple);
    Serial.println("LED strip set to ON (Purple).");
  }
  else if (ledMode == "auto") {
    Serial.println("LED strip set to AUTO mode (12h ON, 12h OFF).");
    ledAutoCycleStartTime = time(nullptr);
    ledAutoState = true; // start ON for the next 12h
    lastLedAutoState = true;
    setLedStripColor(CRGB::Purple);
  }
}

/***************************************************
  setLedStripColor(CRGB color)
  Helper: sets entire strip, shows once
 ***************************************************/
void setLedStripColor(CRGB color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
}

/***************************************************
  handleInactiveState()
  - If device is inactive, skip normal logic
  - Could do deep sleep here; for now we just wait
 ***************************************************/
void handleInactiveState() {
  Serial.println("Device is INACTIVE. Skipping measurements and watering.");

  // Turn LED strip off (black)
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // Power off the sensor
  digitalWrite(MOSFET_CONTROL_PIN, LOW);

  // Wait some time before checking again
  delay(10000);
}
