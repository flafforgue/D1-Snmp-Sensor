// ===============================================================
//
//  Az-Envy as Snmp Temperature , humidity sensor
//  https://www.az-delivery.de/fr/products/az-envy
//
//  Mib fully defined for Temperature
//  for Humidity only oidentPhySensorValue_2 defined
//
// ==============================================================

#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include <ESP8266mDNS.h>

#include <WiFiUdp.h>
#include <SNMP_Agent.h>

// ==============================================================
//                    Environement sensors
// ==============================================================

#include "DHTesp.h"

#define DHTPIN D4        // what pin we're connected to
#define DHTTYPE DHT11    // DHT 11

DHTesp dht;

float    temp  = 0;
float    humy  = 0;

uint32_t mtime  = 0;
uint32_t otime  = 0;
uint32_t high32 = 0;
uint64_t uptime = 0;

void UpdateSnmpValues();

void UpdateDatas() {
  mtime=millis();
  if ( mtime > otime + 1000 ) {
    
    humy = dht.getHumidity();
    temp = dht.getTemperature()-1.8; // temperature correction

    UpdateSnmpValues();
    otime=mtime;
    uptime=(uint64_t)high32 << 32 | mtime;
  } else {
    if ( otime > mtime ) {
      otime=mtime;
      high32++;
    }
  }
}

// ==============================================================
//                          Web Server
// ==============================================================

ESP8266WebServer serverWeb(80);

void handleRoot() {
  char tpstring[10];
  char hystring[10];
  
  snprintf (tpstring, sizeof(tpstring), "%.1f", temp);
  snprintf (hystring, sizeof(hystring), "%.1f", humy);

  char m[800];
  snprintf(m, 800,
  "<html>\
  <head>\
    <meta http-equiv='refresh' content='60'/>\
    <title>MySensor D1</title>\
    <style>\
      body { background-color: #dddddd; font-family: Arial, Helvetica, Sans-Serif; Color: #220088; }\
    </style>\
  </head>\
  <body>\
    <h1>MySensor D1</h1>\
  Temp : <span style=\"color: #843fa0;\">%.1f </span>&deg;C <br/><br/>\n\
  Humidity : <span style=\"color: #843fa0;\">%.1f </span>&#37;<br/><br/>\n\
  </body>\
  </html>",temp,humy);
  serverWeb.send(200, "text/html",m );
}

// --------------------------------------------------------------

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += serverWeb.uri();
  message += "\nMethod: ";
  message += (serverWeb.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += serverWeb.args();
  message += "\n";
  for (uint8_t i = 0; i < serverWeb.args(); i++) {
    message += " " + serverWeb.argName(i) + ": " + serverWeb.arg(i) + "\n";
  }
  serverWeb.send(404, "text/plain", message);
}

// ==============================================================
//                        SNMP Configuration               
// ==============================================================

const char* rocommunity = "public";  // Read only community string
const char* rwcommunity = "private"; // Read Write community string for set commands

static const unsigned long UPTIME_UPDATE_INTERVAL = 1000; // ms = 1 second
static const unsigned long SENSOR_UPDATE_INTERVAL = 5000; // ms = 5 Seconds

// RFC1213-MIB (System)
char* oidSysDescr    = ".1.3.6.1.2.1.1.1.0";   // OctetString SysDescr
char* oidSysObjectID = ".1.3.6.1.2.1.1.2.0";   // OctetString SysObjectID
char* oidSysUptime   = ".1.3.6.1.2.1.1.3.0";   // TimeTicks sysUptime (hundredths of seconds)
char* oidSysContact  = ".1.3.6.1.2.1.1.4.0";   // OctetString SysContact
char* oidSysName     = ".1.3.6.1.2.1.1.5.0";   // OctetString SysName
char* oidSysLocation = ".1.3.6.1.2.1.1.6.0";   // OctetString SysLocation
char* oidSysServices = ".1.3.6.1.2.1.1.7.0";   // Integer sysServices

std::string sysDescr        = "SNMP Agent";
std::string sysObjectID     = "";
uint32_t sysUptime          = 0;
char sysContactValue[255];
char *sysContact            = sysContactValue;
char sysNameValue[255];
char *sysName               = sysNameValue;
char sysLocationValue[255];
char *sysLocation           = sysLocationValue;
int  sysServices            = 65;       // Physical and Application

// --------------------------------------------------------------

// ENTITY-MIB .1.3.6.1.2.1.47 
// Needs to be implemented to support ENTITY-SENSOR-MIB
// An entry would be required per sensor. This is index 1.

// entityPhysicalTable
char* oidentPhysicalIndex_1            = ".1.3.6.1.2.1.47.1.1.1.1.1.1";
char* oidentPhysicalDescr_1            = ".1.3.6.1.2.1.47.1.1.1.1.2.1";
char* oidentPhysicalVendorType_1       = ".1.3.6.1.2.1.47.1.1.1.1.3.1";
char* oidentPhysicalContainedIn_1      = ".1.3.6.1.2.1.47.1.1.1.1.4.1";
char* oidentPhysicalClass_1            = ".1.3.6.1.2.1.47.1.1.1.1.5.1";
char* oidentPhysicalParentRelPos_1     = ".1.3.6.1.2.1.47.1.1.1.1.6.1";
char* oidentPhysicalName_1             = ".1.3.6.1.2.1.47.1.1.1.1.7.1";
char* oidentPhysicalHardwareRev_1      = ".1.3.6.1.2.1.47.1.1.1.1.8.1";
char* oidentPhysicalFirmwareRev_1      = ".1.3.6.1.2.1.47.1.1.1.1.9.1";
char* oidentPhysicalSoftwareRev_1      = ".1.3.6.1.2.1.47.1.1.1.1.10.1";
char* oidentPhysicalSerialNum_1        = ".1.3.6.1.2.1.47.1.1.1.1.11.1";
char* oidentPhysicalMfgName_1          = ".1.3.6.1.2.1.47.1.1.1.1.12.1";
char* oidentPhysicalModelName_1        = ".1.3.6.1.2.1.47.1.1.1.1.13.1";
char* oidentPhysicalAlias_1            = ".1.3.6.1.2.1.47.1.1.1.1.14.1";
char* oidentPhysicalAssetID_1          = ".1.3.6.1.2.1.47.1.1.1.1.15.1";
char* oidentPhysicalIsFRU_1            = ".1.3.6.1.2.1.47.1.1.1.1.16.1";
char* oidentPhysicalMfgDate_1          = ".1.3.6.1.2.1.47.1.1.1.1.17.1";
char* oidentPhysicalUris_1             = ".1.3.6.1.2.1.47.1.1.1.1.18.1";

int entPhysicalIndex_1                 = 1;
std::string entPhysicalDescr_1         = "Temperature Sensor";
std::string entPhysicalVendorType_1    = "";
int entPhysicalContainedIn_1           = 0;
int entPhysicalClass_1                 = 8; // Sensor
int entPhysicalParentRelPos_1          = -1;
std::string entPhysicalName_1          = "";
std::string entPhysicalHardwareRev_1   = "";
std::string entPhysicalFirmwareRev_1   = "";
std::string entPhysicalSoftwareRev_1   = "";
std::string entPhysicalSerialNum_1     = "";
std::string entPhysicalMfgName_1       = "";
std::string entPhysicalModelName_11    = "";
std::string entPhysicalAlias_1         = "";
std::string entPhysicalAssetID_1       = "";
int entPhysicalIsFRU_1                 = 0;
std::string entPhysicalMfgDate_1       = "'0000000000000000'H";
std::string entPhysicalUris_1          = "";

// EntityPhysicalGroup

// ENTITY-SENSOR-MIB .1.3.6.1.2.1.99
// An entry would be required per sensor. 
// This is index 1.
// Must match index in ENTITY-MIB

char* oidentPhySensorType_1            = ".1.3.6.1.2.1.99.1.1.1.1.1";
char* oidentPhySensorScale_1           = ".1.3.6.1.2.1.99.1.1.1.2.1";
char* oidentPhySensorPrecision_1       = ".1.3.6.1.2.1.99.1.1.1.3.1";
char* oidentPhySensorValue_1           = ".1.3.6.1.2.1.99.1.1.1.4.1";
char* oidentPhySensorOperStatus_1      = ".1.3.6.1.2.1.99.1.1.1.5.1";
char* oidentPhySensorUnitsDisplay_1    = ".1.3.6.1.2.1.99.1.1.1.6.1";
char* oidentPhySensorValueTimeStamp_1  = ".1.3.6.1.2.1.99.1.1.1.7.1";
char* oidentPhySensorValueUpdateRate_1 = ".1.3.6.1.2.1.99.1.1.1.8.1";

int entPhySensorType_1                 = 8;       // Celsius
int entPhySensorScale_1                = 9;       // Units
int entPhySensorPrecision_1            = 0;
int entPhySensorValue_1                = 0;       // Value to be updated
int entPhySensorOperStatus_1           = 1;       // OK
std::string entPhySensorUnitsDisplay_1 = "Celsius";
uint32_t entPhySensorValueTimeStamp_1  = 0;
int entPhySensorValueUpdateRate_1      = 0;       // Unknown at declaration, set later.

char* oidentPhySensorValue_2           = ".1.3.6.1.2.1.99.1.1.1.4.2";
char* oidentPhySensorValue_3           = ".1.3.6.1.2.1.99.1.1.1.4.3";
char* oidentPhySensorValue_4           = ".1.3.6.1.2.1.99.1.1.1.4.4";
char* oidentPhySensorValue_5           = ".1.3.6.1.2.1.99.1.1.1.4.5";

int entPhySensorValue_2                = 0;       // Value to be updated


WiFiUDP udp;
SNMPAgent snmp = SNMPAgent(rocommunity, rwcommunity); // Creates an SMMPAgent instance

void UpdateSnmpValues() {
  sysUptime = uptime /1000 ;
  entPhySensorValue_1 = temp*10;
  entPhySensorValue_2 = humy*10;
}

// --------------------------------------------------------------
//                         MIB REGISTERING
// --------------------------------------------------------------

void addRFC1213MIBHandler() {
    // Add SNMP Handlers of correct type to each OID
    snmp.addReadOnlyStaticStringHandler(oidSysDescr, sysDescr);
    snmp.addReadOnlyStaticStringHandler(oidSysObjectID, sysObjectID);
    snmp.addIntegerHandler             (oidSysServices, &sysServices);
    snmp.addTimestampHandler           (oidSysUptime,   &sysUptime);
    // Add Settable Handlers
    snmp.addReadWriteStringHandler     (oidSysContact,  &sysContact,  25, true);
    snmp.addReadWriteStringHandler     (oidSysName,     &sysName,     25, true);
    snmp.addReadWriteStringHandler     (oidSysLocation, &sysLocation, 25, true);
}

void addENTITYMIBHandler() {
    snmp.addIntegerHandler             (oidentPhysicalIndex_1,        &entPhysicalIndex_1);
    snmp.addReadOnlyStaticStringHandler(oidentPhysicalDescr_1,         entPhysicalDescr_1);
    snmp.addReadOnlyStaticStringHandler(oidentPhysicalVendorType_1,    entPhysicalVendorType_1);
    snmp.addIntegerHandler             (oidentPhysicalContainedIn_1,  &entPhysicalContainedIn_1);
    snmp.addIntegerHandler             (oidentPhysicalClass_1,        &entPhysicalClass_1);
    snmp.addIntegerHandler             (oidentPhysicalParentRelPos_1, &entPhysicalParentRelPos_1);
    snmp.addReadOnlyStaticStringHandler(oidentPhysicalName_1,          entPhysicalName_1);
    snmp.addReadOnlyStaticStringHandler(oidentPhysicalHardwareRev_1,   entPhysicalHardwareRev_1);
    snmp.addReadOnlyStaticStringHandler(oidentPhysicalFirmwareRev_1,   entPhysicalFirmwareRev_1);
    snmp.addReadOnlyStaticStringHandler(oidentPhysicalSoftwareRev_1,   entPhysicalSoftwareRev_1);
    snmp.addReadOnlyStaticStringHandler(oidentPhysicalSerialNum_1,     entPhysicalSerialNum_1);
    snmp.addReadOnlyStaticStringHandler(oidentPhysicalMfgName_1,       entPhysicalMfgName_1);
    snmp.addReadOnlyStaticStringHandler(oidentPhysicalModelName_1,     entPhysicalModelName_11);
    snmp.addReadOnlyStaticStringHandler(oidentPhysicalAlias_1,         entPhysicalAlias_1);
    snmp.addReadOnlyStaticStringHandler(oidentPhysicalAssetID_1,       entPhysicalAssetID_1);
    snmp.addIntegerHandler             (oidentPhysicalIsFRU_1,        &entPhysicalIsFRU_1);
    snmp.addReadOnlyStaticStringHandler(oidentPhysicalMfgDate_1,       entPhysicalMfgDate_1);
    snmp.addReadOnlyStaticStringHandler(oidentPhysicalUris_1,          entPhysicalUris_1);
}

void addENTITYSENSORMIBHandler() {
    entPhySensorValueUpdateRate_1 = SENSOR_UPDATE_INTERVAL;
    snmp.addIntegerHandler             (oidentPhySensorType_1,            &entPhySensorType_1);
    snmp.addIntegerHandler             (oidentPhySensorScale_1,           &entPhySensorScale_1);
    snmp.addIntegerHandler             (oidentPhySensorPrecision_1,       &entPhySensorPrecision_1);
    snmp.addIntegerHandler             (oidentPhySensorValue_1,           &entPhySensorValue_1);
    snmp.addIntegerHandler             (oidentPhySensorOperStatus_1,      &entPhySensorOperStatus_1);
    snmp.addReadOnlyStaticStringHandler(oidentPhySensorUnitsDisplay_1,     entPhySensorUnitsDisplay_1);
    snmp.addTimestampHandler           (oidentPhySensorValueTimeStamp_1,  &entPhySensorValueTimeStamp_1);
    snmp.addIntegerHandler             (oidentPhySensorValueUpdateRate_1, &entPhySensorValueUpdateRate_1);

    snmp.addIntegerHandler             (oidentPhySensorValue_2,           &entPhySensorValue_2);

}

// ==============================================================
//                       Wifi Connexion
// ==============================================================

const char * SSID     = "Home_Sensors"; // change
const char * PASSWORD = "MySensor";     // change

void onConnected(const WiFiEventStationModeConnected& event) {
  Serial.println("WiFi connecté");
  Serial.println("Adresse IP : " + WiFi.localIP().toString());
}

void onGotIP(const WiFiEventStationModeGotIP& event) {
  Serial.println("Adresse IP : " + WiFi.localIP().toString());
  Serial.println("Passerelle IP : " + WiFi.gatewayIP().toString());
  Serial.println("DNS IP : " + WiFi.dnsIP().toString());
  Serial.print("Puissance de réception : ");
  Serial.println(WiFi.RSSI());
}

// ==============================================================
//
// ==============================================================

void setup() {
  Serial.begin(115200);

  dht.setup(DHTPIN, DHTesp::DHT11);
  
  // start wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
//  static WiFiEventHandler onConnectedHandler = WiFi.onStationModeConnected(onConnected);
//  static WiFiEventHandler onGotIPHandler = WiFi.onStationModeGotIP(onGotIP);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if (MDNS.begin("MySensor D1")) {
    Serial.println("mDNS responder started");
  }
  
  // start web server
  serverWeb.on("/", handleRoot);
  serverWeb.onNotFound(handleNotFound);
  serverWeb.begin();    

  sysName    = "MySensorD1";
  sysContact = "fred";
    
  snmp.setUDP(&udp);
  snmp.begin();
  addRFC1213MIBHandler();      // RFC1213-MIB (System)
  addENTITYMIBHandler();       // ENTITY-MIB
  addENTITYSENSORMIBHandler(); // ENTITY-SENSOR-MIB
  snmp.sortHandlers();
}

// --------------------------------------------------------------
//
// --------------------------------------------------------------

void loop() {
  UpdateDatas();
  if (WiFi.isConnected()) {
    serverWeb.handleClient();
  }

  snmp.loop();
  if (snmp.setOccurred) {
    snmp.resetSetOccurred();
  }
}
