//import the libraries
#include <set>
#include "arduino_secrets.h"


#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>


#include <AdafruitIO_Feed.h>
#include "AdafruitIO_WiFi.h"

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, "", "");

ESP8266WiFiMulti wifiMulti;
boolean connectioWasAlive = true;

// WiFi connect timeout per AP. Increase when connecting takes longer.
const uint32_t connectTimeoutMs = 5000;

#define N_LEDS 24
#define LED_PIN 5
// #define BOT D5 // Everyone else capacitive sensor pin
#define BOT D0 // Eric capacitive sensor pin



//////////////////LAMP ID////////////////////////////////////////////////////////////
 int lampID = 1; // eric
// int lampID = 2; // katie
// int lampID = 3; // andie
// int lampID = 4; // mom
/////////////////////////////////////////////////////////////////////////////////////

// Adafruit inicialization
AdafruitIO_Feed * lamp = io.feed("lamps"); // Change to your feed

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

int sendVal {0};

const int max_intensity = 180; //  Max intensity

int selected_color = 0;  //  Index for color vector
int prev_selected_color = 0;

char msg[50]; //  Custom messages for Adafruit IO

//  Color definitions
uint32_t broncos_orange = strip.Color(251, 79, 20);
uint32_t red = strip.Color(255, 0, 0);
uint32_t r = strip.Color(208, 16, 16);
uint32_t green = strip.Color(152, 233, 19);
uint32_t blue = strip.Color(32, 97, 200);
uint32_t black = strip.Color(0, 0, 0);
uint32_t yellow = strip.Color(255, 255, 0);
uint32_t white = strip.Color(255, 255, 255);
uint32_t pink = strip.Color(255, 0, 100);
uint32_t cyan = strip.Color(0, 255, 255);
uint32_t orange = strip.Color(230, 80, 0);

uint32_t colors[] = {orange, broncos_orange, r, green, blue, yellow, white, orange, pink, cyan};

std::set<int> lampMessages {101, 102, 103, 104, 105};

int state = 0;

// Time vars
unsigned long RefMillis;
unsigned long ActMillis;
const int send_selected_color_time = 4000;
const int answer_time_out          = 900000;
const int on_time                  = 1800000;
long debounceDelay = 3000;
long lastDebounceTime = 0;

// Disconection timeout
unsigned long currentMillis;
unsigned long previousMillis = 0;
const unsigned long conection_time_out = 300000; // 5 minutos

// Long press detection
const int long_press_time = 2000;
int lastState = LOW;  // the previous state from the input pin
int currentState;     // the current reading from the input pin
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;

void setup() {
  //Start the serial monitor for debugging and status
  Serial.begin(9600);

  delay(10);
  Serial.println("starting\n");

  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);

  // Add all WiFi networks from secrets file
  for (int i = 0; i < NUM_WIFI_NETWORKS; i++) {
    wifiMulti.addAP(wifiNetworks[i].ssid, wifiNetworks[i].password);
    Serial.printf("Added network: %s\n", wifiNetworks[i].ssid);
  }

    Serial.println("Connecting ...");
    int i = 0;
    while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
      delay(1000);
      Serial.print('.');
    }
    Serial.println('\n');
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());              // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer

  
  

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer


  // Activate neopixels
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  pinMode(BOT, INPUT);

  sendVal = 100 + lampID;

  //start connecting to Adafruit IO
  Serial.printf("\nConnecting to Adafruit IO with User: %s Key: %s.\n", IO_USERNAME, IO_KEY);
  AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, "", "");
  io.connect();

  lamp -> onMessage(handle_message);

  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    spin(6);
    delay(500);
  }
  turn_off();
  Serial.println();
  Serial.println(io.statusText());
  // Animation
  spin(3);  turn_off(); delay(50);
  flash(8); turn_off(); delay(100);
  flash(8); turn_off(); delay(50);

  //get the status of our value in Adafruit IO
  lamp -> get();
  sprintf(msg, "L%d: connected", lampID);
  lamp -> save(msg);
}

	static const unsigned long REFRESH_INTERVAL = 1000; // ms
	static unsigned long lastRefreshTime = 0;
	
/* state machine
0: waiting for long press and watch timer
1: send this lamps color
2: receive another lamps color
3: turn off
*/

void loop() {
  
  currentMillis = millis();
  io.run();

  switch (state) {
  case 0:

    ActMillis = millis();
    if (selected_color > 0 && ActMillis - RefMillis > on_time) {
      state = 3;
      break;  
    }

    currentState = digitalRead(BOT);
    if(lastState == LOW && currentState == HIGH)  // Button is pressed
    {
      pressedTime = millis();
    }
    else if(currentState == HIGH) {
      releasedTime = millis();
      long pressDuration = releasedTime - pressedTime;
      if( (pressDuration > long_press_time) && (releasedTime - lastDebounceTime > debounceDelay)){
          Serial.println("debounced; changing state to 1");
          state = 1;
          lastDebounceTime = millis();
      }
    }
    lastState = currentState;
    break;
  case 1:
    sprintf(msg, "L%d: color send", lampID);
    lamp -> save(msg);
    lamp -> save(sendVal);
    Serial.println("sending: " + sendVal);
    selected_color = lampID;
    spinNewColor(selected_color);
    RefMillis = millis(); // reset timer
    state = 2; // don't get stuck
    break;
  case 2:
    Serial.println("color received");
    Serial.println(selected_color);
    if(selected_color != lampID && selected_color != prev_selected_color){
      // received someone elses new color
      spinNewColor(selected_color);
    }
    // else{
      // spinNewColor(selected_color);
    // }
    sprintf(msg, "L%d received color %s", lampID, String(selected_color));
    lamp -> save(msg);
    state = 0;
    RefMillis = millis(); // reset timer
    break;
    // Turned on
  case 3:
    Serial.println("fade_to_off inside state 4");
    fade_to_off(selected_color);
    prev_selected_color = 0;
    selected_color = 0;
    state = 0;
    break;
    // Msg received
  default:
      state = 0;
    break;
  }
  if ((currentMillis - previousMillis >= conection_time_out)) {
    if (WiFi.status() != WL_CONNECTED)
      ESP.restart();
    previousMillis = currentMillis;
  }
}


    //code that tells the ESP8266 what to do when it recieves new data from the Adafruit IO feed
    void handle_message(AdafruitIO_Data * data) {

      // 66 is reboot
      // 100 is ping pong
      // 101 is lamp 1 color
      // 102 is lamp 2 color
      // 103 is lamp 3 color
      // 104 is lamp 4 color

      //convert the recieved data to an INT
      int reading = data -> toInt();
      Serial.println("Processing new message " +  String(reading));
      if (reading == 66) {
        sprintf(msg, "L%d: rebooting", lampID);
        lamp -> save(msg);
        lamp -> save(0);
        ESP.restart();
      } else if (reading == 100) {
        sprintf(msg, "L%d: pong", lampID);
        lamp -> save(msg);
      } else if (lampMessages.count(reading) != 0) {
        prev_selected_color = selected_color;
        selected_color = reading - 100;

        sprintf(msg, "L%d: received color %d", lampID, selected_color);
        state = 2;

      }
    }

    void turn_off() {
      strip.setBrightness(max_intensity);
      for (int i = 0; i < N_LEDS; i++) {
        strip.setPixelColor(i, black);
      }
      strip.show();
    }

    // fade out
    void fade_to_off(int ind){
      for(int brightness = 255; brightness >=0; brightness--){
        strip.setBrightness(brightness);
        for (int i = 0; i < N_LEDS; i++) {
          strip.setPixelColor(i, colors[ind]);
        }
        strip.show();
        delay(2000 / 255);  // divide 10 seconds (in milliseconds) by the number or brightness steps
      }
    }

  //The code that creates the gradual color change animation in the Neopixels (thank you to Adafruit for this!!)
  void spin(int ind) {
    strip.setBrightness(max_intensity);
    for (int i = 0; i < N_LEDS; i++) {
      strip.setPixelColor(i, colors[ind]);
      strip.show();
      delay(30);
    }
    for (int i = 0; i < N_LEDS; i++) {
      strip.setPixelColor(i, black);
      strip.show();
      delay(30);
    }
  }

  void spinNewColor(int ind) {
    strip.setBrightness(max_intensity);
    for (int i = 0; i < N_LEDS; i++) {
      strip.setPixelColor(i, colors[ind]);
      strip.show();
      delay(30);
    }
  }

    //code to flash the Neopixels when a stable connection to Adafruit IO is made
    void flash(int ind) {
      strip.setBrightness(max_intensity);
      for (int i = 0; i < N_LEDS; i++) {
        strip.setPixelColor(i, colors[ind]);
      }
      strip.show();

      delay(200);

    }