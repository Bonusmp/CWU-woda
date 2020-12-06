
#define BLYNK_PRINT Serial        // Uncomment for debugging 

#include "settings_sz.h"           
#include "secret_s.h"               
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>


#include <ESP8266mDNS.h>  // For OTA with ESP8266
#include <WiFiUdp.h>  // For OTA
#include <ArduinoOTA.h>  // For OTA
#include <WidgetRTC.h>
WidgetTerminal terminal(V32);
WidgetRTC rtc;


int volume; //zmienna do obliczania objętości zbiornika
int percent; //zmienna do obliczanie procentu zapełnienia
float waterFillLevel;  //zmienna do obliczania poziomu zapełnienia 

long duration, distance;     // zmienne do obliczania odległości, pomiaru itp

int wifiStrength; //zmienna do okreslania siły sygnału wifi

// Recurring cm level alert variable

float topWaterLevelAlertValue; // wartość dla dla alarmu odległosci od czujnika
float topWaterLevelAlertOnOffState; // status on off alarmu odległosci od czujnika

int OTA_ON_OFF; // zmienna do wł i wył aktualizacji oprogramowania

int percentageGreaterThanAlertOnOffState95; // zmienne do alertów
int percentageGreaterThanAlertOnOffState90;
int percentageGreaterThanAlertOnOffState75;
int percentageGreaterThanAlertOnOffState50;
int percentageGreaterThanAlertValue95;
int percentageGreaterThanAlertValue90;
int percentageGreaterThanAlertValue75;
int percentageGreaterThanAlertValue50;

int ALERT_FLAG_OnOffState;
int long ALERT_FLAG_AlertValue;

int  raw_v= 0;//voltage value from ADC
int c;
int cval = 0;
int sleep_time = 60;  // max 60 min

int wifi_connect_count = 0;          // New variable to keep track of how manty times we've tried to connect to the Wi-Fi
int wifi_connect_max_retries = 10;   // New variable to specify how many attempts we will have at connecting to the Wi-Fi

float v = 0;//counted value of volts



BLYNK_CONNECTED() { // runs once at device startup, once connected to server.

  Blynk.syncVirtual(VPIN_BUTTON_CM_IS_LESS_THAN);
  Blynk.syncVirtual(VPIN_NUMERIC_CM_IS_LESS_THAN);
  Blynk.syncVirtual(VPIN_BUTTON_PERCENTAGE_GREATER_THAN95);
  Blynk.syncVirtual(VPIN_NUMERIC_PERCENTAGE_GREATER_THAN95);
  Blynk.syncVirtual(VPIN_BUTTON_PERCENTAGE_GREATER_THAN90);
  Blynk.syncVirtual(VPIN_NUMERIC_PERCENTAGE_GREATER_THAN90);
  Blynk.syncVirtual(VPIN_BUTTON_PERCENTAGE_GREATER_THAN75);
  Blynk.syncVirtual(VPIN_NUMERIC_PERCENTAGE_GREATER_THAN75);
  Blynk.syncVirtual(VPIN_BUTTON_PERCENTAGE_GREATER_THAN50);
  Blynk.syncVirtual(VPIN_NUMERIC_PERCENTAGE_GREATER_THAN50);
  Blynk.syncVirtual(VPIN_BUTTON_OTA_ON_OFF);
 Blynk.syncVirtual(VPIN_cval);
Blynk.syncVirtual(VPIN_BUTTON_ALERT_FLAG);          
Blynk.syncVirtual(VPIN_NUMERIC_ALERT_FLAG);
  rtc.begin();
}


void MeasureCm() {
  //  The following trigPin/echoPin cycle is used to determine the
  // distance of the nearest object by bouncing soundwaves off of it.

//  digitalWrite(SENSORPOWER, HIGH);
 // terminal.println("Sensor power ON");
 delay(1000);
   digitalWrite(TRIGPIN, LOW);
   delayMicroseconds(10);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(15);
  digitalWrite(TRIGPIN, LOW);

  duration = pulseIn(ECHOPIN, HIGH);

  /*originalDistanceinCm*/
  
  distance  = duration / 58.2; //We get water level distance from top in cm. Calculate the distance (in cm) based on the speed of sound. Distance in centimeters = Time / 58

 terminal.print("Distance: ");
    terminal.print(distance);
    terminal.println(" cm");


  waterFillLevel = (WATER_TANK_HEIGHT_IN_CM - distance); //total water tank hight - fill water level hight
 {
  if (distance >= MINIMUMRANGE && distance <= WATER_TANK_HEIGHT_IN_CM )  {  // here we are finding tank volume

      volume = ((WATER_TANK_LENGTH_IN_CM * waterFillLevel * WATER_TANK_WIDTH_IN_CM) / 1000); // Filled Volume = Length * Width * Fill Height (Liquid Height) and divide by 1000 because we are passing vlaues in cm so to convert to liters we need to divide
      percent = ((float)volume / FULLTANK) * 100;
    }

if (distance >= WATER_TANK_HEIGHT_IN_CM) { //we don't want to display negative values
      volume = 0;
      percent = 0;
    }

    if (distance < MINIMUMRANGE && distance >0) { //we don't want to display wrong data
      volume = FULLTANK;
      percent = 100;
    }

    if (distance ==0) {
      MeasureCm();
    }
  }
//digitalWrite(SENSORPOWER, LOW);
  //terminal.println("Sensor power OFF");
  terminal.flush();
}



void wifiSignalStrength() { //function to calculate wifi signal strength

  wifiStrength = WiFi.RSSI(); //here we are getting wifi signal strength

  Blynk.virtualWrite(VPIN_WIFI_SIGNAL_STRENGTH, wifiStrength); // assigning value to V3
  terminal.print("WiFi Strength: "); // to print data on terminal monitor
  terminal.println(wifiStrength);

  

  if (wifiStrength > -50)
  {
    Blynk.virtualWrite(VPIN_WIFI_STRENGTH_IN_WORDS, "Excellent");
    terminal.println("WiFi Signal is Excellent");
  }

  if (wifiStrength <= -50 && wifiStrength >= -60)
  {
    Blynk.virtualWrite(VPIN_WIFI_STRENGTH_IN_WORDS, "Good");
    terminal.println("WiFi Signal is Good");
  }

  if (wifiStrength <= -60 && wifiStrength >= -70)
  {
    Blynk.virtualWrite(VPIN_WIFI_STRENGTH_IN_WORDS, "Fair");
    terminal.println("WiFi Signal is Fair");
  }

  if (wifiStrength < -70)
  {
    Blynk.virtualWrite(VPIN_WIFI_STRENGTH_IN_WORDS, "Weak");
    terminal.println("WiFi Signal is Weak");
  }
  terminal.flush();
}


//***************************************************************************
//*** TOP Water Level Alert - Continous Alert till we Turn OFF Alert  *******
//***************************************************************************

BLYNK_WRITE(VPIN_BUTTON_CM_IS_LESS_THAN) // button widget is writing value to pin v6. Sending data from app (V6) to hardware(nodemcu)
{
  topWaterLevelAlertOnOffState = param.asInt();
}
BLYNK_WRITE(VPIN_NUMERIC_CM_IS_LESS_THAN) // get numeric widget value from app to know top water level alert value
{
  topWaterLevelAlertValue = param.asFloat();
}

void topWaterLevelAlert() {


  terminal.print("Alert Value: ");
  terminal.print(topWaterLevelAlertValue);
  terminal.println (" cm");


  if (topWaterLevelAlertValue >= distance && topWaterLevelAlertOnOffState == 1) // if top water level alert value is greater than current water level distance; means it triggers when water level cross specified water level 
  {

    Blynk.notify(String("Poziom jest wyższy niż ") + topWaterLevelAlertValue + " cm teraz (od góry)" ); //here we are sending push notification if the above condition satisfies
    
  }
  terminal.flush();
}



BLYNK_WRITE(VPIN_BUTTON_PERCENTAGE_GREATER_THAN95 ){
  
percentageGreaterThanAlertOnOffState95=  param.asInt();
}

BLYNK_WRITE(VPIN_BUTTON_PERCENTAGE_GREATER_THAN90 ){
  
percentageGreaterThanAlertOnOffState90=  param.asInt();
}

BLYNK_WRITE(VPIN_BUTTON_PERCENTAGE_GREATER_THAN75 ){
  
percentageGreaterThanAlertOnOffState75=  param.asInt();
}

BLYNK_WRITE(VPIN_BUTTON_PERCENTAGE_GREATER_THAN50 ){
  
percentageGreaterThanAlertOnOffState50=  param.asInt();
}

BLYNK_WRITE(VPIN_NUMERIC_PERCENTAGE_GREATER_THAN95 ){
  
percentageGreaterThanAlertValue95=  param.asFloat();
}

BLYNK_WRITE(VPIN_NUMERIC_PERCENTAGE_GREATER_THAN90 ){
  
percentageGreaterThanAlertValue90=  param.asFloat();
}

BLYNK_WRITE(VPIN_NUMERIC_PERCENTAGE_GREATER_THAN75 ){
  
percentageGreaterThanAlertValue75=  param.asFloat();
}

BLYNK_WRITE(VPIN_NUMERIC_PERCENTAGE_GREATER_THAN50 ){
  
percentageGreaterThanAlertValue50=  param.asFloat();
}


BLYNK_WRITE(VPIN_BUTTON_ALERT_FLAG ){
  
ALERT_FLAG_OnOffState=  param.asInt();
}

BLYNK_WRITE(VPIN_NUMERIC_ALERT_FLAG ){
  
ALERT_FLAG_AlertValue=  param.asFloat();
}

void cval_sleeptime(){

  /*uwaga max czas spania to 71 min dla mnie 60min
*/
  if( percentageGreaterThanAlertValue50 >percent  ){
  cval=144;
  Blynk.virtualWrite(VPIN_cval, cval);
 }
 
 if( percentageGreaterThanAlertValue75 >percent && percent >=percentageGreaterThanAlertValue50 ){
  cval=72;
  Blynk.virtualWrite(VPIN_cval, cval);
 }

  if(percentageGreaterThanAlertValue90> percent && percent >=percentageGreaterThanAlertValue75){
  cval=48;
  Blynk.virtualWrite(VPIN_cval, cval);
}

  if(percentageGreaterThanAlertValue95 > percent && percent >= percentageGreaterThanAlertValue90){
  cval=8;
  Blynk.virtualWrite(VPIN_cval, cval);
}

  if(percent>= percentageGreaterThanAlertValue95 ){
  cval=4;
 
  Blynk.virtualWrite(VPIN_cval, cval);
}
if(percent >98){
  cval=1;
  Blynk.virtualWrite(VPIN_cval, cval);
}

}

void alert50(){
  if(percentageGreaterThanAlertOnOffState50 == 1 && percentageGreaterThanAlertValue75 >percent && percent >=percentageGreaterThanAlertValue50 ){
  Blynk.notify(String("Poziom jest wyższy niż: ") + percentageGreaterThanAlertValue50 + "% teraz." );
terminal.println(" **** Percentage  Water Level Alert 50 **** ");
if(percentageGreaterThanAlertOnOffState50 == 0){
  terminal.println(" **** Percentage  Water Level Alert 50 is OFF **** ");
}
}
terminal.flush();
}

void alert75(){
  if(percentageGreaterThanAlertOnOffState75 == 1 && percentageGreaterThanAlertValue90> percent && percent >=percentageGreaterThanAlertValue75){
Blynk.notify(String("Poziom jest wyższy niż: ") + percentageGreaterThanAlertValue75 + "% teraz." );
terminal.println(" **** Percentage  Water Level Alert 75 **** ");
if(percentageGreaterThanAlertOnOffState75 == 0){
  terminal.println(" **** Percentage  Water Level Alert 75 is OFF **** ");
}
}
terminal.flush();
}

void alert90(){
  if(percentageGreaterThanAlertOnOffState90 == 1 && percentageGreaterThanAlertValue95 > percent && percent >= percentageGreaterThanAlertValue90){
Blynk.notify(String("Poziom jest wyższy niż: ") + percentageGreaterThanAlertValue90 + "% teraz." );
terminal.println(" **** Percentage  Water Level Alert 90 **** ");
if(percentageGreaterThanAlertOnOffState90 == 0){
  terminal.println(" **** Percentage  Water Level Alert90 is OFF **** ");
}
}
terminal.flush();
}

void alert95(){
  if( percentageGreaterThanAlertOnOffState95 == 1 && percent>= percentageGreaterThanAlertValue95 ){
Blynk.notify(String("Poziom jest wyższy niż: ") + percentageGreaterThanAlertValue95 + "% teraz." );
terminal.println(" **** Percentage  Water Level Alert 95 **** ");
if(percentageGreaterThanAlertOnOffState95 == 0){
  terminal.println(" **** Percentage  Water Level Alert 95 is OFF **** ");
}
}
terminal.flush();
}


void alert(){
c = (ALERT_FLAG_AlertValue + 1);
Blynk.virtualWrite(VPIN_NUMERIC_ALERT_FLAG, c);



if(ALERT_FLAG_OnOffState == 1 && c >= cval){ //wpisz co ile włączeń wysłać powiadomienia

alert50();
alert75();
alert90();
alert95();

}


if (ALERT_FLAG_OnOffState == 0){
  
    alert50();
    alert75();
    alert90();
    alert95();
}


if (c>=cval){
  Blynk.virtualWrite(VPIN_NUMERIC_ALERT_FLAG, 0);
  }
}

void bat(){ //function to calculate battery volt

  raw_v = analogRead(A0); //Odczytujemy wartość napięcia

  v = raw_v * 0.0041015625; //Przeliczenie wartości na napięcie 4.2/1024

  Blynk.virtualWrite(VPIN_BAT_VOLT, v); // assigning value to

  terminal.print("Batery volts: "); // to print data on terminal monitor

  terminal.println(v);
  
  if (v > 4.15)  {
    Blynk.virtualWrite(VPIN_BAT_VOLT_IN_WORDS, "Full");
    terminal.println("Battery is full");
  }
  if (v <= 4.15 && v >= 3.85)
  {
    Blynk.virtualWrite(VPIN_BAT_VOLT_IN_WORDS, "Good");
    terminal.println("Battery is good");
  }

  if (v<= 3.85 && v >= 3.75)
  {
    Blynk.virtualWrite(VPIN_BAT_VOLT_IN_WORDS, "Fair");
    terminal.println("Battery is Fair");
  }

  if (v< 3.75) //uwaga wartość też w alertach
  {
    Blynk.virtualWrite(VPIN_BAT_VOLT_IN_WORDS, "Weak");
    terminal.println("Battery is Weak");
      }
      
if (v<3.75 && c==cval && ALERT_FLAG_OnOffState == 1){
Blynk.notify(String("Battery is Weak") );
}
if (v<3.75 && ALERT_FLAG_OnOffState == 0){
Blynk.notify(String("Battery is Weak") );
}
terminal.flush();
}

void sending(){
  terminal.println(" ");

  Blynk.virtualWrite(VPIN_WATER_PERCENTAGE, percent); // virtual pin
  terminal.print("Percent of Water in Tank: ");
  terminal.print(percent);
  terminal.println("%");

  Blynk.virtualWrite(VPIN_LITERS_OF_WATER, volume); // virtual pin
  terminal.print("Liters of Water in Sump: ");
  terminal.print(volume);
  terminal.println(" Liters");

  Blynk.virtualWrite(VPIN_DISTANCE_IN_CM, distance);// virtual pin
  terminal.print("Distance in cm to Reach Water Level: ");
  terminal.print(distance);
  terminal.println(" cm");
  terminal.print("Wifi conection try:");
  terminal.println(wifi_connect_count);
  terminal.print("Alarm value:");
  terminal.println(cval);

terminal.flush();
}

BLYNK_WRITE(VPIN_BUTTON_OTA_ON_OFF ){
  
OTA_ON_OFF =  param.asInt();
}

void sleep(){

if (OTA_ON_OFF == 0)
  {
  
  terminal.println("Go to sleep");
   terminal.print(sleep_time);
   terminal.print("min");
   terminal.flush();
   delay(100);
   ESP.deepSleep(sleep_time * 60000000); // w milisekundach czas x 1000000 x 60 = minuty
    delay(200);
 }
if (OTA_ON_OFF == 1){
 Blynk.notify("ready for update");

}

}




BLYNK_WRITE(VPIN_terminal)
{

  if (String("sleep") == param.asStr()) {
    terminal.println("Go to sleep") ;
    delay(100);
    terminal.flush();
   ESP.deepSleep(1*60000000); // w milisekundach czas x 1000000 x 60 = minuty
  delay(200);
  }
   
    
  if (String("clear") == param.asStr()) {
    
   terminal.clear();
    } 
   

      if (String("wifi") == param.asStr()) {
    
     wifiSignalStrength();
    } 
    
   if (String("cm") == param.asStr()) {
    
   MeasureCm();
    } 
   
   if (String("bat") == param.asStr()) {
    
   bat();
    } 
    
     if (String("c") == param.asStr()) {
    
     cval_sleeptime();
    }  
    
     if (String("sending") == param.asStr()) {
    
    sending();
    } 
    else{
    terminal.println("sleep/clear/wifi/cm/bat/sending") ;
    } 


  
  // Ensure everything is sent
  terminal.flush();
}
void WiFi_Connect() // New functon to handle the connectuon to the Wi-Fi network
{
  if (WiFi.status() != WL_CONNECTED)
  {
      WiFi.begin(WIFI_SSID, WIFI_PASS); // connect to the network
  }
  while (WiFi.status() != WL_CONNECTED  && wifi_connect_count <wifi_connect_max_retries) // Loop until we've connected, or reached the maximum number of attemps allowed
  {
    delay(10000);
    wifi_connect_count++;

  }

  if (WiFi.status() == WL_CONNECTED)
  {
    WiFi.mode(WIFI_STA);

  }
} // End of void WiFi_Connect
void setup() {

  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(SENSORPOWER, OUTPUT);
  
  //Serial.begin(9600);
//delay(5);
/*New section of code - stop using Blynk.begin, which is a blocking
function, and instead do the following:
  //
  // 1) Attempt to connect to Wi-Fi a few times (how many times we try
is specified by the 'wifi_connect_max_retries' variable)
  // 2) If we successfully connected to Wi-Fi then attempt to connect
to Blynk in a non-blocking way. If we aren't connected to Wi-Fi then
go to sleep
  // 3) If we connected to Blynk then run the rest of the code as
normal. If we aren't connected to Blynk then go to sleep*/


  WiFi_Connect(); // Attempt to connect to Wi-Fi

  if (WiFi.status() == WL_CONNECTED)               // If we managed to connect to Wi-Fi then try to connect to Blynk, else go to sleep
  {
    Blynk.config(AUTH);  // Initialise the Blynk connection settings
    Blynk.connect();                               // Attempt to connect to Blynk
  }
  else
  {
    ESP.deepSleep(15 * 60000000); // w milisekundach czas x 1000000 x 60 = minuty
    delay(200);
  }

  if (Blynk.connected())                          // If we manages to connect to Blynk then carry-on as normal, else go to sleep
  {
    //polaczono
  }
  else
  {

    ESP.deepSleep(15 * 60000000); // w milisekundach czas x 1000000 x 60 = minuty
    delay(200);
  }


  
  Serial.println ( WiFi.localIP() );
  



    terminal.clear();
    terminal.println ( WiFi.localIP() );
    
   String currentTime = String(hour()) + ":" + minute() + ":" + second();
   
  String currentDate = String(day()) + " " + month() + " " + year();
  terminal.print(currentTime);
    terminal.print(" ");
  terminal.print(currentDate);
    terminal.println();
      terminal.flush();  //sends everything
 ArduinoOTA.setHostname(OTA_HOSTNAME);  // For OTA - Use your own device identifying name
ArduinoOTA.begin();  // For OTA


delay(5);
MeasureCm();
delay(50);
cval_sleeptime();
delay(100); 
topWaterLevelAlert();
  delay(100);
alert();
delay(100);
sending();
delay(100);
 wifiSignalStrength();
 delay(100);
bat();
delay(100);
 sleep();


}
void loop() {



Blynk.run(); // Initiates Blynk
//ArduinoOTA.handle();  // For OTA

}
