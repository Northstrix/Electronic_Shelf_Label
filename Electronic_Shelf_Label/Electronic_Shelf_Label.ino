/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-esp8266-input-data-html-form/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <SPIFFS.h>
#else // libraries for ESP8266
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <Hash.h>
  #include <FS.h>
#endif
#include <ESPAsyncWebServer.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 24, 2); // (address, columns, rows)
AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "Blackberry 1291";   // Your SSID (Network's name)
const char* password = "seasidehillsESL"; // Password from your network

const char* PARAM_STRING = "label_st";
const char* PARAM_FLOAT = "price_st";

// HTML web page to handle 2 input fields (label_st, price_st)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Electronic shelf label</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>


body {
  padding:0px;
  margin-top: 5vh;
  font-family:$font;
  background:#65CCB8;
  font-family: 'Poppins', 'Helvetica', sans-serif;
  color: #F2F2F2;
  text-align: center;
}

  </style>
  <script>
    function submitMessage() {
      alert("The value was successfully sent. Press ok to continue");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
    <h2>Label N67</h2>
  <form action="/get" target="hidden-form">
    Enter the label: (current label: %label_st%)
    </br>
    <input type="text" name="label_st">
    <input type="submit" value="Send" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Enter the price: (current price: %price_st% USD)
    </br>
    <input type="number " name="price_st">
        <input type="submit" value="Send" onclick="submitMessage()">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Replaces placeholder with stored values
String processor(const String& var){
  //Serial.println(var);
  if(var == "label_st"){
    return readFile(SPIFFS, "/label_st.txt");
  }
  else if(var == "price_st"){
    return readFile(SPIFFS, "/price_st.txt");
  }
  return String();
}
void dprint(){
  lcd.clear();
  delay(100);
  String Label = readFile(SPIFFS, "/label_st.txt");
  lcd.setCursor(0, 0);
  lcd.print(Label); // Print the label
  String Price = readFile(SPIFFS, "/price_st.txt");
  int str_len = Price.length() + 1;
  char pricelen[str_len];
  Price.toCharArray(pricelen, str_len);
  int prlength = strlen(pricelen);
  lcd.setCursor(0, 1);      
  lcd.print("Price:");
  lcd.setCursor(6, 1);      
  lcd.print(Price); // Print the price
  lcd.setCursor(6+prlength, 1);
  lcd.print("$"); // Print the dollar sign
  server.onNotFound(notFound);
  server.begin(); 
}
void setup() {
  Serial.begin(115200);
  lcd.begin(24,2);  // Initialize 24x2 LCD
  lcd.clear();  // Clear the LCD
  lcd.init(); // Initialise display
  lcd.backlight(); // Turn on the backlight (if you have it).
  // Initialize SPIFFS
  #ifdef ESP32
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #else
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?label_st=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET label_st value on <ESP_IP>/get?label_st=<inputMessage>
    if (request->hasParam(PARAM_STRING)) {
      inputMessage = request->getParam(PARAM_STRING)->value();
      writeFile(SPIFFS, "/label_st.txt", inputMessage.c_str());
    }
    // GET price_st value on <ESP_IP>/get?price_st=<inputMessage>
    else if (request->hasParam(PARAM_FLOAT)) {
      inputMessage = request->getParam(PARAM_FLOAT)->value();
      writeFile(SPIFFS, "/price_st.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No data sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
    dprint();
  });
  dprint();
}

void loop() {
}
