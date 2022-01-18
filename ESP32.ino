#include <WiFi.h>              // Built-in
#include <WiFiMulti.h>         // Built-in
#include <ESP32WebServer.h>    // https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder
#include <ESPmDNS.h>
#include "FS.h"

#include "Network.h"
#include "Sys_Variables.h"
#include "CSS.h"
//===========================sdcardModule
//#include "FS.h"
#include "SD.h"
#include "SPI.h"
//==========================ds3231
#include <SPI.h>
#include <Wire.h>
//#include <Adafruit_GFX.h>
#include "RTClib.h"

//=================
//#include <WiFi.h>
//#include <PubSubClient.h>
/*
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32
 * GND      GND
 * VCC      VCC 5V
 * MOSO     G19
 * MOSI     G23
 * SCK      G18
 * CS       G5
 */

WiFiMulti wifiMulti;
ESP32WebServer server(80);

//============================ds3231
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//vcc => 3.3v
//SCL => G22
//SDA => G21


// Update these with values suitable for your network.
const char* ssid = "PowerOfPhysics";
const char* password = "siJonasBakling";
const char* mqtt_server = "broker.mqtt-dashboard.com";
#define mqtt_port 1883
#define MQTT_USER "mqtt username"
#define MQTT_PASSWORD "mqtt password"
#define MQTT_SERIAL_PUBLISH_CH "/ic/esp32/serialdata/uno/"

// adding serial 
#define RXD2 16
#define TXD2 17

//WiFiClient wifiClient;

//PubSubClient client(wifiClient);

//===================SD Card Module

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }


    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

void setup() {
  //initializing serial 2
  // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
  //Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  Serial.begin(115200); //Serial must always be 115200 baud
  Serial.setTimeout(500);// Set time out for 

  if (!WiFi.config(local_IP, gateway, subnet, dns)) { //WiFi.config(ip, gateway, subnet, dns1, dns2);
    Serial.println("WiFi STATION Failed to configure Correctly"); 
  } 
  wifiMulti.addAP(ssid_1, password_1);  // add Wi-Fi networks you want to connect to, it connects strongest to weakest
  wifiMulti.addAP(ssid_2, password_2);  // Adjust the values in the Network tab
  wifiMulti.addAP(ssid_3, password_3);
  wifiMulti.addAP(ssid_4, password_4);  // You don't need 4 entries, this is for example!
  
  Serial.println("Connecting ...");
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(250); Serial.print('.');
  }
  Serial.println("\nConnected to "+WiFi.SSID()+" Use IP address: "+WiFi.localIP().toString()); // Report which SSID and IP is in use
  // The logical name http://fileserver.local will also access the device if you have 'Bonjour' running or your system supports multicast dns
  if (!MDNS.begin(servername)) {          // Set your preferred server name, if you use "myserver" the address would be http://myserver.local/
    Serial.println(F("Error setting up MDNS responder!")); 
    ESP.restart(); 
  } 

  // setup sd card module
  if (! rtc.begin()) {
  Serial.println("Couldn't find RTC");
  while (1);
  }
  rtc.adjust(DateTime(__DATE__, __TIME__));

  //Setup SD Card
  if(!SD.begin()){
        Serial.println("Card Mount Failed");
        SD_present = false; 
        return;
    }
    else{
      Serial.println(F("Card initialised... file access enabled..."));
      SD_present = true; 
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    server.on("/",         HomePage);
    server.on("/download", File_Download);
    server.on("/delete", File_Delete);
    ///////////////////////////// End of Request commands
    server.begin();
    Serial.println("HTTP server started");
}


void loop() {
   server.handleClient(); // Listen for client connections
   
   DateTime now = rtc.now();
   String Time = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
   String Date = String(now.month()) + "-" + String(now.day()) + "-" + String(now.year());
   String StrDay = daysOfTheWeek[now.dayOfTheWeek()];
   String strSoilMoistureReading = "";
   
   while (Serial2.available()) {
    strSoilMoistureReading = Serial2.readStringUntil('\n');
    String strToPrint = strSoilMoistureReading.substring(0, strSoilMoistureReading.length() - 1) + " " + Time + " " + Date + " " + StrDay + "\n";
    Serial.println(strToPrint);
    appendFile(SD, "/SoilMoistureLog.txt", strToPrint.c_str());
   }
 }

 void HomePage(){
  SendHTML_Header();
  webpage += F("<a href='/download'><button>Download</button></a>");
  webpage += F("<a href='/delete'><button>Delete</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Download(){ // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
  //if (server.args() > 0 ) { // Arguments were received
  //  if (server.hasArg("download")) 
  SD_file_download("SoilMoistureLog.txt");
  //}
  //else SelectInput("File Download","Enter filename to download","download","download");
}
void File_Delete(){
  deleteFile(SD, "/SoilMoistureLog.txt");
  SendHTML_Header();
  webpage += F("<a href='/download'><button>Download</button></a>");
  webpage += F("<a href='/delete'><button>Delete</button></a>");
  append_page_footer();
  deletedMessage("Soil Moisture Log");
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_download(String filename){
  if (SD_present) { 
    File download = SD.open("/"+filename);
    if (download) {
      server.sendHeader("Content-Type", "text/text");
      server.sendHeader("Content-Disposition", "attachment; filename="+filename);
      server.sendHeader("Connection", "close");
      server.streamFile(download, "application/octet-stream");
      download.close();
    } else ReportFileNotPresent("download"); 
  } else ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Header(){
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
  server.sendHeader("Pragma", "no-cache"); 
  server.sendHeader("Expires", "-1"); 
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves. 
  append_page_header();
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Content(){
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Stop(){
  server.sendContent("");
  server.client().stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SelectInput(String heading1, String heading2, String command, String arg_calling_name){
  SendHTML_Header();
  webpage += F("<h3 class='rcorners_m'>");
  webpage += heading1+"</h3><br>";
  webpage += F("<h3>"); 
  webpage += heading2 + "</h3>"; 
  webpage += F("<FORM action='/"); 
  webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
  webpage += F("<input type='text' name='"); 
  webpage += arg_calling_name; 
  webpage += F("' value=''><br>");
  webpage += F("<type='submit' name='"); 
  webpage += arg_calling_name; 
  webpage += F("' value=''><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportSDNotPresent(){
  SendHTML_Header();
  webpage += F("<h3>No SD Card present</h3>"); 
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportFileNotPresent(String target){
  SendHTML_Header();
  webpage += F("<h3>File does not exist</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
