#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>

bool debug = false;
//#define SerialDebug(text)   Serial.print(text);
//#define SerialDebugln(text) Serial.println(text);
#define SerialDebug(text)
#define SerialDebugln(text)

char MyIp[16];
char MyHostname[16];
char MyRoom[32]="";

WiFiClient espMqttClient;
PubSubClient mqttClient(espMqttClient);

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String s;
  
  if (strstr(topic, "/config/ESP-") && strstr(topic, "/roomname")) {
    if (length>=32) length=31;
    strncpy(MyRoom, (char *) payload, length);
    MyRoom[length]='\0';
    SerialDebug("New Room-Name: ");
    SerialDebugln(MyRoom);

    s = "/room/"; s+=MyRoom; s+="/"; s+=MyHostname;
    mqttClient.publish(s.c_str(), MyIp, true);

    s = "/room/"; s+=MyRoom; s+="/light1";
    mqttClient.subscribe(s.c_str());
    s = "/room/"; s+=MyRoom; s+="/light2";
    mqttClient.subscribe(s.c_str());
    s = "/room/"; s+=MyRoom; s+="/light3";
    mqttClient.subscribe(s.c_str());
  } else if (strstr(topic, "/room/") && strstr(topic, "/light")) {
    int pin;
    switch (topic[strlen(topic)-1]) {
      case '1': pin=0; break;
      case '2': pin=1; break;
      case '3': pin=2; break;
      default: return;
    }
    char cmd[16];
    if (length>=16) length=15;
    strncpy(cmd, (char *) payload, length);
    cmd[length]='\0';
    if (strcmp(cmd,"0")==0 || strcmp(cmd,"off")==0) {
      digitalWrite(pin, LOW);
    } else if (strcmp(cmd,"1")==0 || strcmp(cmd,"on")==0) {
      digitalWrite(pin, HIGH);
    } else if (strcmp(cmd,"t")==0 || strcmp(cmd,"toggle")==0) {
      digitalWrite(pin, !digitalRead(pin));
    }
    s = "/room/"; s+=MyRoom; s+="/lights";
    strcpy(cmd,"0,0,0");
    if (digitalRead(0) == HIGH) cmd[0]='1';
    if (digitalRead(1) == HIGH) cmd[2]='1';
    if (digitalRead(2) == HIGH) cmd[4]='1';
    mqttClient.publish(s.c_str(), cmd);
  }
}

void mqtt_reconnect() {
  String s;
  
  while (!mqttClient.connected()) {
    s = "/config/"; s+=MyHostname; s+="/online";
    if (mqttClient.connect(MyHostname, s.c_str(), 0, true, "0")) {
      mqttClient.publish(s.c_str(), "1", true);
      
      s="/config/"; s+=MyHostname; s+="/ipaddr";
      mqttClient.publish(s.c_str(), MyIp, true);
      
      s="/config/"; s+=MyHostname; s+="/roomname";
      mqttClient.subscribe(s.c_str());
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(200);
  delay(100);

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(debug);
  wifiManager.setTimeout(3*60);
  if(!wifiManager.autoConnect()) {
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  IPAddress MyIP=WiFi.localIP();
  snprintf(MyIp, 16, "%d.%d.%d.%d", MyIP[0], MyIP[1], MyIP[2], MyIP[3]);
  snprintf(MyHostname, 15, "ESP-%08x", ESP.getChipId());
  SerialDebug("ESP-Hostname: ");
  SerialDebugln(MyHostname);
  
  MDNS.begin(MyHostname);
  int n = MDNS.queryService("mqtt", "tcp");
  SerialDebugln("mDNS query done");
  if (n == 0) {
    SerialDebugln("no services found");
  }
  else {
    SerialDebug(n);
    SerialDebugln(" service(s) found");
    for (int i = 0; i < n; ++i) {
      // Print details for each service found
      SerialDebug(i + 1);
      SerialDebug(": ");
      SerialDebug(MDNS.hostname(i));
      SerialDebug(" (");
      SerialDebug(MDNS.IP(i));
      SerialDebug(":");
      SerialDebug(MDNS.port(i));
      SerialDebugln(")");
      mqttClient.setServer(MDNS.IP(i), MDNS.port(i));
    }
  }
  SerialDebugln("");
  
  mqttClient.setCallback(mqtt_callback);

  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);   // this is TxD
  pinMode(2, OUTPUT);
}

void loop() {
  if (!mqttClient.connected()) {
    mqtt_reconnect();
  }
  mqttClient.loop();
}
