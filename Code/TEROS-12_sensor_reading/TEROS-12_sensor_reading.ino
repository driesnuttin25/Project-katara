#include <SDI12.h>

#define SDI12_DATA_PIN 5  // Pin connected to the SDI-12 data line of the sensor
#define MOSFET_CONTROL_PIN 26  // Pin connected to the MOSFET gate

SDI12 mySDI12(SDI12_DATA_PIN);

void setup() {
  Serial.begin(9600);  // Start serial communication at 9600 baud rate

  // Set up MOSFET control pin
  pinMode(MOSFET_CONTROL_PIN, OUTPUT);  // Set GPIO26 as output
  digitalWrite(MOSFET_CONTROL_PIN, HIGH);  // Set GPIO26 HIGH to power the sensor

  mySDI12.begin();  // Begin SDI-12 communication
  Serial.println("SDI-12 sensor communication initialized");
}

void loop() {
  // Initiate a measurement
  String measurementCommand = "0M!";  // Change address '0' if your sensor has a different address
  mySDI12.sendCommand(measurementCommand);
  delay(1500);  // Wait for the sensor to process the command

  // Request the measured data
  String dataCommand = "0D0!";  // Request data from the sensor
  mySDI12.sendCommand(dataCommand);
  delay(300);  // Wait for the data to be ready

  // Read the full response from the sensor and parse the data
  String response = readFullResponse();
  if (response.length() > 0) {
    parseSensorData(response);
  } else {
    Serial.println("No data received. Check sensor and command syntax.");
  }

  delay(2000);  // Wait 5 seconds before the next loop iteration
}

String readFullResponse() {
  String data = "";
  while (mySDI12.available()) {
    char c = mySDI12.read();
    data += c;  // Append each character into the response string
  }
  return data;
}

void parseSensorData(String data) {
  int firstPlus = data.indexOf('+', 1);  // Find the first '+' after the address '0'
  int secondPlus = data.indexOf('+', firstPlus + 1);  // Find the second '+'
  int endOfData = data.indexOf('+', secondPlus + 1);  // Find the end of data segment

  String moisture = data.substring(firstPlus + 1, secondPlus);
  String temperature = data.substring(secondPlus + 1, endOfData);
  String conductivity = data.substring(endOfData + 1);

  Serial.print("Moisture: ");
  Serial.println(moisture);
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Conductivity: ");
  Serial.println(conductivity);
}
