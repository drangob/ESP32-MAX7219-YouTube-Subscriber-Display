struct WiFiCredentials {
  const char* ssid;
  const char* password;
};

WiFiCredentials networks[] = {
  {"SSID", "PASSWORD"}
};

const int numNetworks = sizeof(networks) / sizeof(networks[0]);