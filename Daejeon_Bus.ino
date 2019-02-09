#include "ssd1306_i2c.h"
#include "icon_xbm.h"
#include <ESP8266WiFi.h>

#define I2C_address 0x3c
#define SDA D6
#define SCL D7
#define BTN_pin D5
#define BTN_pin_2 D1

SSD1306 display(I2C_address, SDA, SCL);

const char *ssid = "Home";
const char *pass = "";

String host = "openapitraffic.daejeon.go.kr";
String url = "/api/rest/arrive/getArrInfoByUid?serviceKey=";
//Your API KEY
String API_key = "RN6UQVhSlQmNrcl4uUlq8Qeq2%2B0n2rEnZsDzIlIp9JdYCPbnYM8Qv15aLj9t8gkS6MjOq8asXprprgTbqfe1DQ%3D%3D";
//Bus stop ID
String Busstop_id = "33060";
//HTTP port = 80
int HttpPort = 80;

String buff = "";
int i = 0;


bool readyForUpdate = true;
bool Fin_MIN = false;
bool Fin_NO = false;
bool Fin_TP = false;

int BTN_status_last = 1;
int BTN_status_last_2 = 1;
int scroll = 0;

long last_millis;

struct BUS_info {
	String BUS_EXTIME = "";
	String BUS_NO = "";
	String BUS_TP = "";
};

struct BUS_info BUS_info[4];

void setup() {

  Serial.begin(115200);
  pinMode(BTN_pin, INPUT_PULLUP);
  pinMode(BTN_pin_2, INPUT_PULLUP);
  
  display.init();
  display.clear();
  display.flipScreenVertically();
  display.display();

}

void loop() {

  if (readyForUpdate) {
    if (WiFi.status() != WL_CONNECTED) {
      wificonnect();
    }
    if (WiFi.status() == WL_CONNECTED) {

      getUpdate();
	    delay(500);

    }
  }
  BTN_action();
  BTN_action_2();
  display.clear();
  for(int j = 1; j<4 ; j++){
    
    int y = ((j-1)*32) - 1;
    if(y<0) {
      y = 0;
    }
    if(scroll == 1 ) y -= 32;
    
    Frame(&display,0,y,j);
  }
  display.display();

  if ((millis() - last_millis) >= 40000) {
	  readyForUpdate = true;
  }

}

void getUpdate() {

  //initial
  WiFiClient client;
	  buff = "";
	  i = 0;
    for(int j=0;j<4;j++) {
      BUS_info[j].BUS_EXTIME = "";
      BUS_info[j].BUS_NO = "";
      BUS_info[j].BUS_TP = "";
      
    }

	display.clear();
	display.drawXbm(51, 13, 26, 26, icon_bus_bits);
	display.display();
    
	  client.connect(host, HttpPort);
	  while (!client.connected()) {
		  Serial.print('.');
	  }
	  Serial.println("HTTP completed");
  //request http
  client.print(String("GET ") +
               url + API_key + String("&arsId=") + Busstop_id +
               String(" HTTP/1.1\r\n") +
               String("Host: ") +
               host +
               String("\r\n") +
               String("Connection: close\r\n\r\n"));
  delay(1000);
  int retry_count = 0;

  while (client.available()) {
	  char line = client.read();

	  if (line != NULL && line != ' ') { //NULL = \0
		  buff += line;

		  if (buff.endsWith("<EXTIME_MIN>")) {
			  Fin_MIN = true;
			  i++;
		  }

		  if (buff.endsWith("<ROUTE_NO>")) {
			  Fin_NO = true;

		  }
		  if (buff.endsWith("<ROUTE_TP>")) {
			  Fin_TP = true;
		  }

		  if (Fin_MIN) {
			  dataparaser(line, BUS_info[i].BUS_EXTIME, Fin_MIN);
		  }
		  if (Fin_NO) {
			  dataparaser(line, BUS_info[i].BUS_NO, Fin_NO);
		  }
		  if (Fin_TP) {
			  dataparaser(line, BUS_info[i].BUS_TP, Fin_TP);
		  }
	  }
  }
    
  
  for(int j=1; j<4; j++){
      if(BUS_info[j].BUS_NO.length() == 1) {
          BUS_info[j].BUS_NO = String("00") + BUS_info[j].BUS_NO;
        } else if(BUS_info[j].BUS_NO.length() == 2) {
			BUS_info[j].BUS_NO = String("0") + BUS_info[j].BUS_NO;
        }

       if(BUS_info[j].BUS_EXTIME.length() == 1) {
		   BUS_info[j].BUS_EXTIME = String("0") + BUS_info[j].BUS_EXTIME;
       }

      
  }
  
  readyForUpdate = false;

  last_millis = millis();

}

void Frame(SSD1306 *display, int x,int y,int count) {

		display->drawXbm(x + 1, y + 3, 26, 26, icon_bus_bits);
		display->drawXbm(x + 32, y + 3, 26, 26, getBusType(BUS_info[count].BUS_TP));
		display->drawString(x + 59, y + 13, BUS_info[count].BUS_NO);
		display->drawString(x + 90, y + 13, BUS_info[count].BUS_EXTIME + "m"); 
    
  }

const char* getBusType(String icon) {
	if (icon == "1") {
		return geubhaeng_bits;
	}else if (icon == "2") {
	  return gansun_bits;
	}else if (icon == "3") {
	  return icon_bus_bits;
	}

	return icon_bus_bits;
}


void wificonnect() {

  WiFi.begin(ssid, pass);

  display.clear();
  display.drawString(20, 20, "connecting...");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {

    Serial.print('.');
    delay(100);


  }
  Serial.println("connected");

  delay(500);
}

void dataparaser(char c, String &a, bool &TR) {
  if (c != '<') {
    if (c != '>') {
      a += c;
    }
  }
  else {
	  TR = false;
  }
}

void BTN_action() {
  int BTN_status = digitalRead(BTN_pin);

  if(BTN_status != BTN_status_last) {
    if(BTN_status == 0){
      scroll++;
	  if (scroll > 1) {
		  scroll = 0;
	  }
    }
  }
  

  BTN_status_last = BTN_status;
}

void BTN_action_2() {
  int BTN_status = digitalRead(BTN_pin_2);

  if(BTN_status!= BTN_status_last_2) {
    if(BTN_status == 0){
    readyForUpdate = true;
    }
  }

  BTN_status_last_2 = BTN_status;
}
