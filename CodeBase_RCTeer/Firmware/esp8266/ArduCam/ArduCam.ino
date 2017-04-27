/*
   Connects ArduCam esp8266 to node js server through WiFi Internet and sends
   captured image data in the binary form

*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <Hash.h>



const char* ssid = "YOUR SSID";
const char* password = "YOUR PASSWORD";



char path[] = "/";
char host[] = "rcteer.swastibhat.com";

WebSocketsClient webSocket;
#define USE_SERIAL Serial1
WiFiClient client;
bool isConnected = false;


#if !(defined ESP8266 )
#error Please select the ArduCAM ESP8266 UNO board in the Tools/Board
#endif

//This demo can only work on OV2640_MINI_2MP or ARDUCAM_SHIELD_V2 platform.
#if !(defined (OV2640_MINI_2MP)||(defined (ARDUCAM_SHIELD_V2) && defined (OV2640_CAM)))
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif
// set GPIO16 as the slave select :
const int CS = 16;

ArduCAM myCAM(OV2640, CS);


void start_capture() {
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
}

/*
 * Important function that sends the captured 
 * image to the server
 */
void camCapture(ArduCAM myCAM) {

  size_t len = myCAM.read_fifo_length();
  if (len >= 0x07ffff) {
    Serial.println("Over size.");
    return;
  } else if (len == 0 ) {
    Serial.println("Size is 0.");
    return;
  }

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  //need to uncomment required line in memorysaver.h. It defines the arducam used
#if !(defined (ARDUCAM_SHIELD_V2) && defined (OV2640_CAM))
  SPI.transfer(0xFF);
#endif
  //send the text which will help differentiate between each image
  webSocket.sendTXT("--frameFRAMEframe\r\nContent-Type: image/jpeg\r\n\r\n");
  //data will be sent to the server in chunks of 4096 bytes
  static const size_t bufferSize = 4096;
  static uint8_t buffer[bufferSize] = {0xFF};
  int index = 0;
  //divides the image data into chunks of specified bufferSize
  while (len) {
    size_t will_copy = (len < bufferSize) ? len : bufferSize;
    SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
    Serial.println(index, will_copy);
    index++;
    //Converts data into binary format
    webSocket.sendBIN(&buffer[0], will_copy);
    len -= will_copy;
  }

  myCAM.CS_HIGH();
}


void setup() {

  Serial.begin(115200);
  delay(1000);

  uint8_t vid, pid;
  uint8_t temp;
#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif
  Serial.begin(115200);
  Serial.println("ArduCAM Start!");

  // set the CS as an output:
  pinMode(CS, OUTPUT);

  // initialize SPI:
  SPI.begin();
  SPI.setFrequency(4000000); //4MHz

  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55) {
    Serial.println("SPI1 interface Error!");
    while (1);
  }

  //Check if the camera module type is OV2640
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
    Serial.println("Can't find OV2640 module!");
  else
    Serial.println("OV2640 detected.");


  //Change to JPEG capture mode and initialize the OV2640 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.OV2640_set_JPEG_size(OV2640_160x120);
  myCAM.clear_fifo_flag();

  //Connect to the internet
  connectToWifi();
}

void serverCapture() {
  start_capture();
  Serial.println("CAM Capturing");
  int total_time = 0;
  total_time = millis();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  total_time = millis() - total_time;
  Serial.print("capture total_time used (in miliseconds):");
  Serial.println(total_time, DEC);
  total_time = 0;
  Serial.println("CAM Capture Done!");
  total_time = millis();
  camCapture(myCAM);
  total_time = millis() - total_time;
  Serial.print("send total_time used (in miliseconds):");
  Serial.println(total_time, DEC);
  Serial.println("CAM send Done!");
}


void loop() {
  webSocket.loop();
  //Capture and send data only if websocket has been established
  if (isConnected)
    serverCapture();
}
int x = 0;
void webSocketEvent(WStype_t type, uint8_t * payload, size_t lenght) {

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      isConnected = false;
      break;
    case WStype_CONNECTED:
      {
        Serial.printf("[WSc] Connected to url: %s\n",  payload);
        isConnected = true;
      }
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary lenght: %u\n", lenght);
      hexdump(payload, lenght);
      break;
  }
}


void connectToWifi() {
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
  //Websocket connection is established
  webSocket.begin(host, 8081);
  webSocket.onEvent(webSocketEvent);
}
