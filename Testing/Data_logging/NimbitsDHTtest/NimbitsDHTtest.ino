// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

// This example has been modified by Doctor Bit
// more info at: http://www.drbit.nl - http://blog.drbit.nl

#include <DHT.h>
#include <Nimbits.h>
#include <Client.h>
#include <Ethernet.h>
#include <SPI.h>
#include <PString.h>
#include <stdlib.h>
#define DHTPIN 2     // what pin we're connected to

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V (red)
// Connect pin 2 of the sensor to whatever your DHTPIN is (yellow)
// Connect pin 4 (on the right) of the sensor to GROUND (black)
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor (only in DHT11)

DHT dht(DHTPIN, DHTTYPE);
//nimbits settings, set the instance name (nimbits-02 is the public cloud on https://cloud.nimbits.com) the email of the account owner, and a read write key they have created.
String instance = "nimbits-02";
char owner[] = "betamaster50@gmail.com";
String readWriteKey = "secret_key";
byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //this ethernet shield's MAC address

Nimbits nimbits(instance, owner, readWriteKey);

void setup() {
  Serial.begin(9600); 
  Serial.println("DHTxx test!");
 
  if (Ethernet.begin(mac) == 0) {
    Serial.println("DHCP Failed!");
    while(true);
  } 
  randomSeed(analogRead(0));
  Serial.println("Online");
  digitalWrite (9,HIGH); // For debug only..
  delay(1000);
  dht.begin();
}

void loop() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT");
  } else {
    // Temperature
    Serial.println("\n*******************");
    Serial.print("Temperature: "); 
    Serial.print(t);
    Serial.println(" *C");
    Serial.println("*******************\n");
    nimbits.recordValue(t,"","betamaster50@gmail.com/Temperature"); 
    Serial.println("\n ");
    // Humidity
    Serial.println("\n*******************"); 
    Serial.print("Humidity: "); 
    Serial.print(h);
    Serial.println(" %\t");
    Serial.println("*******************\n");
    nimbits.recordValue(h,"","betamaster50@gmail.com/Humidity");
    Serial.println("\n ");
  }
  delay (60000);   // Send data every 60 seconds
}
