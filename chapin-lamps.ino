//import the libraries

#include "config.h"
#include <NeoPixelBrightnessBus.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiManager.h>

#define N_LEDS 24
#define LED_PIN 5
#define BOT 16 // capacitive sensor pin

//////////////////LAMP ID////////////////////////////////////////////////////////////
int lampID = 2;
/////////////////////////////////////////////////////////////////////////////////////

// Adafruit inicialization
AdafruitIO_Feed * lamp = io.feed("lamps"); // Change to your feed

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);


int recVal  {0};
int sendVal {0};

const int max_intensity = 255; //  Max intensity

int selected_color = 0;  //  Index for color vector

int i_breath;

char msg[50]; //  Custom messages for Adafruit IO

//  Color definitions
uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 255, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t yellow = strip.Color(255, 255, 0);
uint32_t white = strip.Color(255, 255, 255);
uint32_t pink = strip.Color(255, 0, 100);
uint32_t cyan = strip.Color(0, 255, 255);
uint32_t orange = strip.Color(230, 80, 0);
uint32_t black = strip.Color(0, 0, 0);

uint32_t colors[] = {red, green, blue, yellow, white, pink, cyan, orange};

int state = 0;

// Time vars
unsigned long RefMillis;
unsigned long ActMillis;
const int send_selected_color_time = 4000;
const int answer_time_out          = 900000;
const int on_time                  = 900000;

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
  Serial.begin(115200);

  // Activate neopixels
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  wificonfig();

  pinMode(BOT, INPUT);

  //  Set ID values
  if (lampID == 1) {
    recVal = 20;
    sendVal = 10;
  }
  else if (lampID == 2) {
    recVal = 10;
    sendVal = 20;
  }

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

void loop() {
    currentMillis = millis();
    io.run();
    // State machine
    switch (state) {
      // Wait
    case 0:
      currentState = digitalRead(BOT);
      if(lastState == LOW && currentState == HIGH)  // Button is pressed
      {
        pressedTime = millis();
      }
      else if(currentState == HIGH) {
        releasedTime = millis();
        long pressDuration = releasedTime - pressedTime;
        if( pressDuration > long_press_time )
        {
            state = 1;
        }
      }
      lastState = currentState;
      break;
      // Wait for button release
    case 1:
      selected_color = 0;
      light_half_intensity(selected_color);
      state = 2;
      RefMillis = millis();
      while(digitalRead(BOT) == HIGH){}
      break;
      // Color selector
    case 2:
      if (digitalRead(BOT) == HIGH) {
        selected_color++;
        if (selected_color > 9)
          selected_color = 0;
        while (digitalRead(BOT) == HIGH) {
          delay(50);
        }
        light_half_intensity(selected_color);
        // Reset timer each time it is touched
        RefMillis = millis();
      }
      // If a color is selected more than a time, it is interpreted as the one selected
      ActMillis = millis();
      if (ActMillis - RefMillis > send_selected_color_time) {
        if (selected_color == 9) //  Cancel msg
          state = 8;
        else
          state = 3;
      }
      break;
      // Publish msg
    case 3:
      sprintf(msg, "L%d: color send", lampID);
      lamp -> save(msg);
      lamp -> save(selected_color + sendVal);
      Serial.print(selected_color + sendVal);
      state = 4;
      flash(selected_color);
      light_half_intensity(selected_color);
      delay(100);
      flash(selected_color);
      light_half_intensity(selected_color);
      break;
      // Set timer
    case 4:
      RefMillis = millis();
      state = 5;
      i_breath = 0;
      break;
      // Wait for answer
    case 5:
      for (i_breath = 0; i_breath <= 314; i_breath++) {
        breath(selected_color, i_breath);
        ActMillis = millis();
        if (ActMillis - RefMillis > answer_time_out) {
          turn_off();
          lamp -> save("L%d: Answer time out", lampID);
          lamp -> save(0);
          state = 8;
          break;
        }
      }
      break;
      // Answer received
    case 6:
      Serial.println("Answer received");
      light_full_intensity(selected_color);
      RefMillis = millis();
      sprintf(msg, "L%d: connected", lampID);
      lamp -> save(msg);
      lamp -> save(0);
      state = 7;
      break;
      // Turned on
    case 7:
      ActMillis = millis();
      //  Send pulse
      if (digitalRead(BOT) == HIGH) {
        lamp -> save(420 + sendVal);
        pulse(selected_color);
      }
      if (ActMillis - RefMillis > on_time) {
        turn_off();
        lamp -> save(0);
        state = 8;
      }
      break;
      // Reset before state 0
    case 8:
      turn_off();
      state = 0;
      break;
      // Msg received
    case 9:
      sprintf(msg, "L%d: msg received", lampID);
      lamp -> save(msg);
      RefMillis = millis();
      state = 10;
      break;
      // Send answer wait
    case 10:
      for (i_breath = 236; i_breath <= 549; i_breath++) {
        breath(selected_color, i_breath);
        if (digitalRead(BOT) == HIGH) {
          state = 11;
          break;
        }
        ActMillis = millis();
        if (ActMillis - RefMillis > answer_time_out) {
          turn_off();
          sprintf(msg, "L%d: answer time out", lampID);
          lamp -> save(msg);
          state = 8;
          break;
        }
      }
      break;
      // Send answer
    case 11:
      light_full_intensity(selected_color);
      RefMillis = millis();
      sprintf(msg, "L%d: answer sent", lampID);
      lamp -> save(msg);
      lamp -> save(1);
      state = 7;
      break;
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

      //convert the recieved data to an INT
      int reading = data -> toInt();
      if (reading == 66) {
        sprintf(msg, "L%d: rebooting", lampID);
        lamp -> save(msg);
        lamp -> save(0);
        ESP.restart();
      } else if (reading == 100) {
        sprintf(msg, "L%d: ping", lampID);
        lamp -> save(msg);
        lamp -> save(0);
      } else if (reading == 420 + recVal) {
        sprintf(msg, "L%d: pulse received", lampID);
        lamp -> save(msg);
        lamp -> save(0);
        pulse(selected_color);
      } else if (reading != 0 && reading / 10 != lampID) {
        // Is it a color msg?
        if (state == 0 && reading != 1) {
          state = 9;
          selected_color = reading - recVal;
        }
        // Is it an answer?
        if (state == 5 && reading == 1)
          state = 6;
      }
    }

    void turn_off() {
      strip.setBrightness(max_intensity);
      for (int i = 0; i < N_LEDS; i++) {
        strip.setPixelColor(i, black);
      }
      strip.show();
    }

    // 50% intensity
    void light_half_intensity(int ind) {
      strip.setBrightness(max_intensity / 2);
      for (int i = 0; i < N_LEDS; i++) {
        strip.setPixelColor(i, colors[ind]);
      }
      strip.show();
    }

    // 100% intensity
    void light_full_intensity(int ind) {
      strip.setBrightness(max_intensity);
      for (int i = 0; i < N_LEDS; i++) {
        strip.setPixelColor(i, colors[ind]);
      }
      strip.show();
    }

    void pulse(int ind) {
      int i;
      int i_step = 5;
      for (i = max_intensity; i > 80; i -= i_step) {
        strip.setBrightness(i);
        for (int i = 0; i < N_LEDS; i++) {
          strip.setPixelColor(i, colors[ind]);
          strip.show();
          delay(1);
        }
      }
      delay(20);
      for (i = 80; i < max_intensity; i += i_step) {
        strip.setBrightness(i);
        for (int i = 0; i < N_LEDS; i++) {
          strip.setPixelColor(i, colors[ind]);
          strip.show();
          delay(1);
        }
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

    // Inspired by Jason Yandell
    void breath(int ind, int i) {
      float MaximumBrightness = max_intensity / 2;
      float SpeedFactor = 0.02;
      float intensity;
      if (state == 5)
        intensity = MaximumBrightness / 2.0 * (1 + cos(SpeedFactor * i));
      else
        intensity = MaximumBrightness / 2.0 * (1 + sin(SpeedFactor * i));
      strip.setBrightness(intensity);
      for (int ledNumber = 0; ledNumber < N_LEDS; ledNumber++) {
        strip.setPixelColor(ledNumber, colors[ind]);
        strip.show();
        delay(1);
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

    void configModeCallback(WiFiManager *myWiFiManager){
      Serial.println("Entered config mode");
      strip.setBrightness(max_intensity);
      for (int i = 0; i < 24; i++)
      {
        strip.setPixelColor(i, colors[(int)i / 3]);
      }
      strip.show();
    }

    void wificonfig() {
      WiFi.mode(WIFI_STA);
      WiFiManager wifiManager;

      std::vector<const char *> menu = {"wifi","info"};
      wifiManager.setMenu(menu);
      // set dark theme
      wifiManager.setClass("invert");

      bool res;
      wifiManager.setAPCallback(configModeCallback);
      res = wifiManager.autoConnect("Lamp", "password"); // password protected ap

      if (!res) {
        spin(0);
        delay(50);
        turn_off();
      } else {
        //if you get here you have connected to the WiFi
        spin(3);
        delay(50);
        turn_off();
      }
      Serial.println("Ready");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }