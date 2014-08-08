#include <SPI.h>
#include <Ethernet.h>
#include "plotly_streaming_ethernet.h"
#include "DHT.h"

// Sign up to plotly here: https://plot.ly
// View your API key and streamtokens here: https://plot.ly/settings
#define nTraces 2
// View your tokens here: https://plot.ly/settings
// Supply as many tokens as data traces
// e.g. if you want to ploty A0 and A1 vs time, supply two tokens
char *tokens[nTraces] = {"347u7napt5", "5vandj03yo"};
// arguments: username, api key, streaming token, filename
plotly graph("betamaster50", "dfp5icslmx", tokens, "Ard_TH", nTraces);

// DHT Sensor Setup
#define DHTPIN 2 // We have connected the DHT to Digital Pin 2
#define DHTTYPE DHT21 // This is the type of DHT Sensor (Change it to DHT11 if you're using that model)
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT object


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte my_ip[] = { 199, 168, 222, 18 }; // google will tell you: "public ip address"

void startEthernet(){
    Serial.println("... Initializing ethernet");
    if(Ethernet.begin(mac) == 0){
        Serial.println("... Failed to configure Ethernet using DHCP");
        // no point in carrying on, so do nothing forevermore:
        // try to congifure using IP address instead of DHCP:
        Ethernet.begin(mac, my_ip);
    }
    Serial.println("... Done initializing ethernet");
    delay(1000);
}


void setup() {
  graph.maxpoints = 500;
  graph.timezone = "Europe/Amsterdam";
  graph.fileopt = "extend"; // Remove this if you want the graph to be overwritten on initialization
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  pinMode(9, OUTPUT);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  startEthernet();

  bool success;
  success = graph.init();
  if(!success){while(true){}}
  graph.openStream();
  dht.begin();
}

float h, t;

void loop() {
  digitalWrite (9,HIGH);
  h = dht.readHumidity();
  t = dht.readTemperature();
  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT");
  } else {
    Serial.print("Humidity: "); 
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: "); 
    Serial.print(t);
    Serial.println(" *C");
  }
  graph.plot(millis(), t, tokens[0]);
  graph.plot(millis(), h, tokens[1]);
  delay (40000);
}
