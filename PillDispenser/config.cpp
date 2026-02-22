#include "config.h"

// WIFI
const char* ssid     = "LCK";
const char* password = "123456790";

// MQTT
const char* mqtt_server  = "34.158.48.218";
const int   mqtt_port    = 1883;

const char* mqtt_topic_sub    = "pill/command/schedule";
const char* mqtt_topic_config = "pill/command/config";
const char* mqtt_topic_status = "pill/command/status";
const char* mqtt_topic_measurement = "pill/data/measurement";
const char* mqtt_topic_refill = "pill/command/refill";
const char* USER_ID = "69133fba40de254edf366794";
