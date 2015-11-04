// This #include statement was automatically added by the Particle IDE.
#include "IRremote.h"
/*
Contrast UP;Contrast Down;Off;On;;
807FC837;807F28D7;807FE01F;807FB04F;;la telecommande envoie le code hexa suivi de FFFFFFFF
R;G;B;W;;
807F50AF;807FD827;807F926D;807F22DD;;
R-;;;Flash;;
807FD02F;807F7A85;807F9867;807F7887;;
R--;;;Strobe;;
807F6897;807F20DF;807FF20D;807F8877;;
R---;;;Fade;;
807F807F;807F1AE5;807FD22D;807F32CD;;
R----;;;Smooth;;
807FF00F;807F609F;807FB847;807F00FF;;
*/

// This #include statement was automatically added by the Particle IDE.
#include "Adafruit_DHT/Adafruit_DHT.h"

// DHT parameters
#define DHTPIN 5 //D5 on the particle 
#define DHTTYPE DHT11


// other parameters
#define publish_delay 60000 // 60 secondes

// Variables
IRsend irsend(D3);
int temperature;
int humidity;
int soil;
int sensorValue;

double tempC = 0;

unsigned int lastPublish = 0;

// DHT sensor
DHT dht(DHTPIN, DHTTYPE);

//Pins
int ledVert = D0;
int ledBleu = D1;
int ledRouge = D2;
int ledInfra = D3; // LED infrarouge
int PinLed = D4;   //LED témoin

int PinAnalogiqueHumidite = A0;       //Broche Analogique de mesure d'humidité
int PinNumeriqueHumidite = D6;        //Broche Numérique mesure de l'humidit

int secheresse = 1;  //0 ou 1 si seuil atteint

// Last time, we only needed to declare pins in the setup function.
// This time, we are also going to register our Spark function

void setup()
{
    Serial.begin(115200);
    // Here's the pin configuration, same as last time
    pinMode(ledRouge, OUTPUT);
    pinMode(ledVert, OUTPUT);
    pinMode(ledBleu, OUTPUT);
    pinMode(ledInfra, OUTPUT);
    
    pinMode(PinAnalogiqueHumidite, INPUT);  //pin A0 en entrée analogique
    pinMode(PinNumeriqueHumidite, INPUT);   //pin D6 en entrée numérique
    pinMode(PinLed, OUTPUT);   //LED témoin

    // We are also going to declare a Spark.function so that we can turn the LED on and off from the cloud.
    Particle.function("led",ledToggle);
    // This is saying that when we ask the cloud for the function "led", it will employ the function ledToggle() from this app.

    Particle.variable("temp", &tempC, DOUBLE);

    // For good measure, let's also make sure both LEDs are off when we start:
    digitalWrite(ledRouge, HIGH);
    digitalWrite(ledVert, HIGH);
    digitalWrite(ledBleu, HIGH);
   
    // Start DHT sensor
    dht.begin();
    
    // listen to the hook response : stock price of AAPL
    Particle.subscribe("hook-response/get_quote_AAPL", gotQuoteAAPL, MY_DEVICES);

    // Lets give ourselves 10 seconds before we actually start the program.
    // That will just give us a chance to open the serial monitor before the program sends the request
    for(int i=0;i<10;i++) {
        Serial.println("waiting " + String(9-i) + " seconds before we publish");
        delay(1000);
    }
    
    // turning LED on if you have forgotten the remote
    irsend.sendNEC(0x807FB04F, 32); 
    delay(50);

}

void loop()
{
    // Humidity measurement
    temperature = dht.getTempCelcius();
    
    // Humidity measurement
    humidity = dht.getHumidity();
    
    // soil humidity level measurement
    sensorValue = analogRead(PinAnalogiqueHumidite);
    //tempC = (((sensorValue * 3.3)/4095) - 0.5) * 100;
    tempC = dht.getTempCelcius();
    sensorValue = constrain(sensorValue, 0, 1023);
    //map the value to a percentage
    soil = map(sensorValue, 0, 1023, 100, 0);
  
    secheresse = digitalRead(PinNumeriqueHumidite); 

    if (secheresse==1) 
    {
        digitalWrite(PinLed, HIGH);
    }
    else {
        digitalWrite(PinLed, LOW);   // LED off
    }
    
    /*// Publish data
    Particle.publish("température", String(temperature) + " °C");
    delay(2000);
    Particle.publish("humidité", String(humidity) + "%");
    delay(2000);    String tempStr = tryExt
    Particle.publish("humidite du sol", String(soil) + "%");
    delay(2000);
    Particle.publish("sécheresse", String(secheresse));
    delay(2000);
    */    
    unsigned long now = millis();

    if ((now - lastPublish) > (1*publish_delay)) {
        // on publie toutes les 1 mn ...
        // on allume le wifi
        //WiFi.on();
        
        Particle.publish("librato_A0", String(temperature), 60, PRIVATE);
        Particle.publish("température", String(temperature), 60, PRIVATE);
        Particle.publish("humidité", String(humidity), 60, PRIVATE);
        Particle.publish("humidite du sol", String(soil) + "%");
        Particle.publish("sécheresse", String(secheresse));

        Serial.println("Requesting Quote!");
        Particle.publish("get_quote_AAPL");
/*
from service http://www.webservicex.net/stockquote.asmx/GetQuote?symbol=AAPL
web hook
particle webhook GET get_quote_AAPL http://www.webservicex.net/stockquote.asmx/GetQuote?symbol=AAPL
*/
        lastPublish = now;
    }
    
    // on ferme le Wifi au bout d'une minute
    if ((now - lastPublish) > publish_delay) {
        //WiFi.off();
    }
}


// This function will get called when weather data comes in
void gotQuoteAAPL(const char *name, const char *data) {
    // Important note!  -- Right now the response comes in 512 byte chunks.  
    //  This code assumes we're getting the response in large chunks, and this
    //  assumption breaks down if a line happens to be split across response chunks.

    String str = String(data);
    Serial.println(str);
    
    String SymbolStr = tryExtractString(str, "&lt;Symbol&gt;", "&lt;/Symbol&gt;");
    String LastStr = tryExtractString(str, "&lt;Last&gt;", "&lt;/Last&gt;");
    String ChangeStr = tryExtractString(str, "&lt;Change&gt;", "&lt;/Change&gt;");
    String PercentageChangeStr = tryExtractString(str, "&lt;PercentageChange&gt;", "&lt;/PercentageChange&gt;");

    if (SymbolStr != NULL) {
        Serial.println("Stock symbol: " + SymbolStr);
    }

    if (LastStr != NULL) {
        Serial.println("Last quotation: " + LastStr);
    }

    if (ChangeStr != NULL) {
        Serial.println("Stock variation: " + ChangeStr);

        if (ChangeStr.toFloat() < 0) {
            rouge();
        }

        if (ChangeStr.toFloat() > 0) {
            vert();
        }

        if (ChangeStr.toFloat() == 0) {
            bleu();
        }
    }

    if (PercentageChangeStr != NULL) {
        Serial.println("percentage change: " + PercentageChangeStr + String(" %"));
    }
}

// Returns any text found between a start and end string inside 'str'
// example: startfooend  -> returns foo
String tryExtractString(String str, const char* start, const char* end) {
    if (str == NULL) {
        return NULL;
    }

    int idx = str.indexOf(start);
    if (idx < 0) {
        return NULL;
    }

    int endIdx = str.indexOf(end);
    if (endIdx < 0) {
        return NULL;
    }

    return str.substring(idx + strlen(start), endIdx);
}

// We're going to have a super cool function now that gets called when a matching API request is sent
// This is the ledToggle function we registered to the "led" Spark.function earlier.

int ledToggle(String command) {
    /* Spark.functions always take a string as an argument and return an integer.
    Since we can pass a string, it means that we can give the program commands on how the function should be used.
    In this case, telling the function "on" will turn the LED on and telling it "off" will turn the LED off.
    Then, the function returns a value to us to let us know what happened.
    In this case, it will return 1 for the LEDs turning on, 0 for the LEDs turning off,
    and -1 if we received a totally bogus command that didn't do anything to the LEDs.
    */

    if (command == "ledon") {
        digitalWrite(ledRouge,LOW);
        digitalWrite(ledVert,LOW);
        digitalWrite(ledBleu,LOW);
    }
    if (command == "ledoff") {
        digitalWrite(ledRouge,HIGH);
        digitalWrite(ledVert,HIGH);
        digitalWrite(ledBleu,HIGH);
        return 0;
    }
    if (command == "rouge") {
        rouge();
        delay(50);
        return 1;
    }
    if (command == "vert") {
        vert();
        delay(50);
        return 2;
    }

    if (command == "bleu") {
        bleu();
        delay(50);
        return 3;
    }
    
    if (command == "blanc") {
        blanc();
        delay(50);
        return 3;
    }

    if (command == "on") {
        irsend.sendNEC(0x807FB04F, 32);
        delay(50);
        return 4;
    }
    
    if (command == "off") {
        digitalWrite(ledRouge,HIGH);
        digitalWrite(ledVert,HIGH);
        digitalWrite(ledBleu,HIGH);
        irsend.sendNEC(0x807FE01F, 32);
        delay(50);
        return 5;
    }
}

void rouge() {
    digitalWrite(ledRouge,LOW);
    digitalWrite(ledVert,HIGH);
    digitalWrite(ledBleu,HIGH);
    irsend.sendNEC(0x807F50AF, 32);
}

void vert() {
    digitalWrite(ledRouge,HIGH);
    digitalWrite(ledBleu,HIGH);
    digitalWrite(ledVert,LOW);
    irsend.sendNEC(0x807FD827, 32);
}

void bleu() {
    digitalWrite(ledRouge,HIGH);
    digitalWrite(ledVert,HIGH);
    digitalWrite(ledBleu,LOW);
    irsend.sendNEC(0x807F926D, 32);
}
    
void blanc() {
    digitalWrite(ledRouge,LOW);
    digitalWrite(ledVert,LOW);
    digitalWrite(ledBleu,LOW);
    irsend.sendNEC(0x807F22DD, 32);
    delay(50);
}
