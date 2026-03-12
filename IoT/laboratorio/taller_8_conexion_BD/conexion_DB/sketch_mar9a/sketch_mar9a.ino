/**
UABC
erik garcia chavez 01275863 
ingenieria en computacion 
internet de las cosas 
taller-lab 8 conexion a base de datos con arduino/ESP-IDF

*/

//libreria para conectarse a WIFI
#include "WiFi.h"
#include <HTTPClient.h>
#include<string.h>
#include <ESP32_MySQL.h>

//macros
#define pinLed 18
#define pinButtom 19
#define pin_ADC 32
#define ESP32_MYSQL_DEBUG_PORT Serial
#define _ESP32_MYSQL_LOGLEVEL_ 1

#define USE_TLS false


const char* ssid = "INFINITUMF4AF";
const char* password = "nFukH34MPW";

//database information
char server[] = "srv871.hstgr.io";
uint16_t server_port = 3306;
char user[]         = "u598733893_iotusr";              // MySQL user login username
char password_DB[]     = "iotdb@UabcTj252";          // MySQL user login password


char database[] = "u598733893_iot_db";
char table[] = "data"; 

//columnas de la tabla 
//nombre de las columnas dentro de la tabal 
char device_column[]="DEVICE";
char RSSI_column[] = "RSSI";
char IP_column[] = "IP";
char LED_column[] = "LED";
char ADC_column[] = "ADC";

//toggle

unsigned long lastSendTime = 0;
bool ledState = false;           // estado actual del LED
bool lastButtonState = HIGH;     // estado anterior del botón
const unsigned long SEND_INTERVAL = 300000ul;
// const unsigned long SEND_INTERVAL = 60000ul;

//recopilar datos 
char IP_PUBLIC[16];
//guardar el RSSI de la senial wifi 
long rssi;
char device_value[]="1275863";

//LED / ADC

int LED_VALUE = 0;
int ADC_VALUE = 0;


ESP32_MySQL_Connection conn((Client *)&client);
ESP32_MySQL_Query *query_mem;

void initWiFi();
void runInsert();
void getIP_Public();

//funciones que llen led y ADC



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //inicia WIFI
  initWiFi();

  pinMode(pinLed, OUTPUT); // -> este es para el PIN  
  pinMode(pinButtom, INPUT); // -> PIN de entrada del boton 
  
  //obtenemos la IP publica del interent
  getIP_Public();

  ESP32_MYSQL_DISPLAY3("Connecting to SQL Server @", server, ", Port =", server_port);
  ESP32_MYSQL_DISPLAY5("User =", user, ", PW =", password_DB, ", DB =", database);
}


void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  Serial.println(WiFi.localIP());

}

void getIP_Public(){

  if(WiFi.status()== WL_CONNECTED){
    HTTPClient http;
    http.begin("http://api.ipify.org"); // Servicio para obtener IP
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("IP Pública: " + payload);
      strncpy(IP_PUBLIC, payload.c_str(), 15);
      IP_PUBLIC[15] = '\0'; // asegurar terminación null
    }
    http.end();
  }

}



void runInsert()
{
  // Initiate the query class instance
  ESP32_MySQL_Query query_mem = ESP32_MySQL_Query(&conn);

  if (conn.connected())
  {
    ESP32_MYSQL_DISPLAY("Database connected. Inserting data...");
    String INSERT_SQL = String("INSERT INTO ") + database + "." + table 
         + " (" + device_column + "," + RSSI_column + "," + IP_column + "," + LED_column + "," + ADC_column + ") VALUES ('" 
         + device_value + "', " + rssi + ",'" + IP_PUBLIC + "'," + LED_VALUE + "," + ADC_VALUE + ");";
    ESP32_MYSQL_DISPLAY(INSERT_SQL);
    
    // Execute the query
    if ( !query_mem.execute(INSERT_SQL.c_str()) )
    {
      ESP32_MYSQL_DISPLAY("Insert error");
    }
    else
    {
      ESP32_MYSQL_DISPLAY("Data Inserted.");
    }
  }
  else
  {
    ESP32_MYSQL_DISPLAY("Error connecting to Database. Can't insert.");
  }
}



void loop() {
  bool currentButtonState = digitalRead(pinButtom);

  if (currentButtonState == HIGH && lastButtonState == LOW) {
    ledState = !ledState;                 
    digitalWrite(pinLed, ledState);       
    Serial.println(ledState ? "LED ON" : "LED OFF");
    delay(50); 
  }

  lastButtonState = currentButtonState;
  unsigned long now = millis();

  if (now - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = now;

    rssi = WiFi.RSSI();
    LED_VALUE = ledState ? 1 : 0;
    ADC_VALUE = analogRead(pin_ADC);

    ESP32_MYSQL_DISPLAY("comunicacion con la DB...");
    if (conn.connectNonBlocking(server, server_port, user, password_DB) != RESULT_FAIL)
    {
      delay(50);
      runInsert();
      conn.close();                     // close the connection
    } 
    else 
    {
      ESP32_MYSQL_DISPLAY("\nConnect failed. Trying again on next iteration.");
    }

    ESP32_MYSQL_DISPLAY("\nSleeping...");
    ESP32_MYSQL_DISPLAY("================================================");

  }



}



