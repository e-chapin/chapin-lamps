#ifndef ARDUINO_SECRETS_H
#define ARDUINO_SECRETS_H

struct WiFiCredentials {
  const char* ssid;
  const char* password;
};

const WiFiCredentials wifiNetworks[] = {
  {"YourNetwork1", "password1"},
  {"YourNetwork2", "password2"},
  {"YourNetwork3", "password3"},
};

const int NUM_WIFI_NETWORKS = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

#define IO_USERNAME "your_adafruit_username"
#define IO_KEY "your_adafruit_io_key"

#endif