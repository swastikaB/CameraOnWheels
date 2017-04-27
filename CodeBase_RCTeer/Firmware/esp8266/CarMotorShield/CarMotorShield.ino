/*
    This sketch sends data via HTTP GET requests to rcteer.swastibhat.com service.

*/

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

const char* ssid     = "YOUR SSID";
const char* password = "YOUR PASSWORD";

const char* host = "rcteer.swastibhat.com";

void setup() {
  Serial.begin(115200);
  delay(10);

  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, 0);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);

  digitalWrite(5, 0);
  digitalWrite(4, 0);

  digitalWrite(0, 1);
  digitalWrite(2, 1);
}

void loop() {
  //delay(200);

  Serial.print("connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    analogWrite(5, 0);
    analogWrite(4, 0);
    digitalWrite(0, 1);
    digitalWrite(2, 1);
    return;
  }

  // We now create a URI for the request
  String url = "/getValues";

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  // Read all the lines of the reply from server and print them to Serial
  String line;
  while (client.available()) {
    line = client.readStringUntil('\r');

  }

  //Part where engine value is readed from client
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(line);
  String leftText = root["left"];
  String rightText = root["right"];

  Serial.println("[" + leftText + "][" + rightText + "]");
  int left = leftText.toInt();
  int right = rightText.toInt();
  move(left, right);

  client.flush();
  Serial.println();
  Serial.println("closing connection");
}

void move(int left, int right) {
  int motorASpeed = 1023;
  int motorBSpeed = 1023;
  int motorAForward = 1;
  int motorBForward = 1;
  if (left < 0) {
    motorAForward = 0;
  } else {
    motorAForward = 1;
  }
  if (right < 0) {
    motorBForward = 0;
  } else {
    motorBForward = 1;
  }

  //Part where we set engine with given value and correct direction
  analogWrite(5, abs(left));
  analogWrite(4, abs(right));
  digitalWrite(2, motorAForward);
  digitalWrite(0, motorBForward);

  

}


