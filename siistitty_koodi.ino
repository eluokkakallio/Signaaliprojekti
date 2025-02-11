// TEAM PEE KOKO KOODI 
 
#include <TimerOne.h> 
#include <LiquidCrystal.h> 
#include <Ethernet.h> 
#include <Keypad.h> 
#include <PubSubClient.h> 
#include <Wire.h> 
#include <float.h> 
 

//Näppäimistön alustus
const byte ROWS = 4;  
const byte COLS = 4;  
 
char keys[ROWS][COLS] = { 
{'1', '2', '3', 'A'},  
}; 
 
byte rowPins[ROWS] = {A5,};  
byte colPins[COLS] = {A0, A1, A2, A3}; 
 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS); 
 

//Muuttujien alustus 
int time = 0; 
volatile float puls = 0; 
volatile float frq = 0; 
float sum_brightness = 0;  
float frq_procent = 0; 
float brightness = 0;  
 
float valoisuus_to_percentage(float valoisuus); 
float valoisuus_percentage; 

//LCD-näytön alustus
const int rs = 9, en = 8, d4 = 5, d5 = 4, d6 = 7, d7 = 6; 
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); 

//Palvelimen alustus 
byte server[] = { 10, 6, 0, 21 }; // MQTT-palvelimen IP-osoite 
unsigned int Port = 1883; // MQTT-palvelimen portti 
EthernetClient ethClient; // Ethernet-kirjaston client-olio 
PubSubClient client(server, Port, ethClient); // PubSubClient-olion luominen 
 
#define outTopic "ICT4_out_2020" // Aihe, jolle viesti lähetetään 
 
static uint8_t mymac[6] = {0x44, 0x76, 0x58, 0x10, 0x00, 0x62}; // MAC-osoite Ethernet-liitäntää varten 
 
char* clientId = "a731fd44"; 
 
void pulse_interrupt(void); 
void Timer_int_routine(); 
void connect_MQTT_server(); 
int calculate_mean(float sum, float counter);
int calculate_Max(float current, float value);
int calculate_Min(float current, float value);
void print_Min_and_Max(int max_humidity, int min_humidity, int max_bright, int min_bright);
void print_connectStatus();
void print_Current_Datavalues();

 
void setup() { 
  Serial.begin(9600); 
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), pulse_interrupt, FALLING);
  Timer1.initialize(10000000);
  Timer1.attachInterrupt(Timer_int_routine);
  lcd.begin(20, 4);
  fetch_IP(); 
} 
 

void loop() { 
// Digitaalinen signaali
// z ja y on määritelty excelkaavalla	 
  float y = -0.06;
  float z = 514;
  frq_procent = (y * (frq + 520 )) + z; 
  int max_humidity = calculate_Max(max_humidity, frq_procent);
  int min_humidity = calculate_Min(min_humidity, frq_procent);
 
// Analoginen signaali
  int sensorData = analogRead(A4);
  brightness = (sensorData * (5.0 / 1023.0)); 
  int max_brightness = calculate_Max(max_brightness, brightness);
  int min_brightness = calculate_Min(min_brightness, brightness);
  
  delay(1000); 
  
  float counter = 0;
  sum_brightness += brightness;
  counter++; 
  
  if (counter == 10) { 
// lasketaan 10s keskiarvo valoisuudelle sekä lähetetään MQTT.lle keskiarvot
    int brightness_mean = calculate_mean(sum_brightness, counter);
    Serial.println(brightness_mean);
    sum_brightness = 0;
    counter = 0;
    int rounded_frq = round(frq); 
    send_MQTT_message(); 
    } 

  char key = keypad.getKey(); 
  if (key != NO_KEY) { 
    lcd.clear();
    lcd.setCursor(0, 0); 

    switch (key) {
      case '1':
        print_connectStatus();
        break;

      case '2':
        print_Current_Datavalues();
        break;

      case '3':
        print_Min_and_Max(max_humidity, min_humidity, max_brightness, min_brightness);
        break;
t
      case 'A':
        Serial.println("Key A is pressed.");
        lcd.clear();
        lcd.print("Team Pee OUT!!");
        break;

      default:
        Serial.println(key);
    }

  } 
}

int calculate_mean(float sum, float counter) {
  return round(sum / counter); 
}

int calculate_Max(float current, float value) {
  if (current > value){
	  return current;
  }
  else {
    return value;
  }
	
} 

int calculate_Min(float current, float value) {
  if (current < value){
	  return current;
    }
  else {
    return value;
    }
	
} 

void print_Min_and_Max(int max_humidity, int min_humidity, int max_bright, int min_bright) {
  lcd.print("max H ");
  lcd.print(max_humidity);
  lcd.setCursor(0, 1);
  lcd.print("min H ");
  lcd.print(min_humidity); 
  
  lcd.setCursor(0, 2); 
  lcd.print("max B ");
  lcd.print(max_bright); 
  lcd.setCursor(0, 3); 
  lcd.print("min B ");
  lcd.print(min_bright); 
}

void print_connectStatus() {
  Serial.println("Key 1 is pressed.");
  lcd.print("Connecting to MQTT");
  lcd.setCursor(0, 1);
  if (client.connect(clientId)) {
    lcd.print("Connected OK"); // Yhdistetty onnistuneesti 
  } else {
    lcd.print("Connection failed."); // Yhdistäminen epäonnistui 
  } 
}

void print_Current_Datavalues(){
  Serial.print("Key 2 is pressed.");

  lcd.print(brightness); 
  lcd.print("V");
  if (brightness > 4.49) {
      lcd.print(" It's bright");
    }; 
  if (brightness < 1.30) {
    lcd.print(" It's dark");
  } 

  lcd.setCursor(0, 2);
  lcd.print("Relative humidity is ");
  lcd.setCursor(0, 3);
  lcd.print(frq_procent);
  lcd.print("%");

 }

void pulse_interrupt(void) {
  puls++; 
} 
 
void Timer_int_routine() {
  frq = puls / 9.0;
  puls = 0; 
} 
 
void fetch_IP() { 
//yhteyden muodostaminen Ethernet-verkkoon
  Serial.println("Attempting to fetch IP...");
  bool connectionSuccess = Ethernet.begin(mymac);
  if (!connectionSuccess) {
    Serial.println("Failed to access Ethernet controller");
    } else { 
      Serial.print("Connected with IP: ");
      Serial.println(Ethernet.localIP());
  } 
} 
 
void send_MQTT_message() { 
 // jos ei yhteyttä MQTT, niin kutsutaan yhdistä funktioo
 if (!client.connected()) {
  connect_MQTT_server();
  } 

//jos yhteys, lähettää viestin MQTT-brokerille
  if (client.connected()) { // Jos yhteys on muodostettu
    char buffer[100];
    int mqtt_valoisuus = brightness;
    int mqtt_kosteus = frq_procent;
    
    sprintf(buffer, "IOTJS={\"S_name1\":\"Team_Pee_brightness\",\"S_value1\":%d,\"S_name2\":\"Team_Pee_humidity\",\"S_value2\":%d}", mqtt_valoisuus, mqtt_kosteus);
    client.publish(outTopic, buffer);
    Serial.println("Message sent to MQTT server.");
    } else {
      Serial.println("Failed to send message: not connected to MQTT server.");
      } 
} 
 
void connect_MQTT_server() { 
// yhdistää MQTT-brokeriin hyödyntäen clienID
  Serial.println("Connecting to MQTT");
  if (client.connect(clientId)) {
    Serial.println("Connected OK");
    } else {
      Serial.println("Connection failed.");
      } 
} 
 

