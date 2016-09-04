#include <SPI.h>
#include <Ethernet2.h>

/************************Server configuration*********|***************************/
const char server[] = "https://hydroponics.eu-gb.mybluemix.net";
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x77, 0xC8};
const int port = 6001;

EthernetClient client;

/************************Hardware Related Macros*********|***************************/
#define         MG_PIN                       0     //define which analog input channel you are going to use
#define         BOOL_PIN                     2     //Arduino D2-CO2 sensor digital pinout, labled with "D" on PCB  
#define         DC_GAIN                      8.5   //define the DC gain of amplifier
 
/***********************Software Related Macros************************************/
#define         READ_SAMPLE_TIMES            10     //define how many samples you are going to take in normal operation
#define         READ_SAMPLE_INTERVAL         50    //define the time interval(in milisecond) between each samples in
//normal operation
 
/**********************Application Related Macros**********************************/
//These values differ from sensor to sensor. User should derermine this value.
#define         ZERO_POINT_X                 2.602 //lg400=2.602, the start point_on X_axis of the curve
#define         ZERO_POINT_VOLTAGE           0.324 //define the output of the sensor in volts when the concentration of CO2 is 400PPM
#define         MAX_POINT_VOLTAGE            0.265 //define the output of the sensor in volts when the concentration of CO2 is 10,000PPM
#define         REACTION_VOLTGAE             0.059 //define the voltage drop of the sensor when move the sensor from air into 1000ppm CO2
 
/*****************************Globals***********************************************/
float           CO2Curve[3]  =  {ZERO_POINT_X, ZERO_POINT_VOLTAGE, (REACTION_VOLTGAE / (2.602 - 4))};

/*****************************Hum const*********************************************/
#define          HUM_PIN                      2
#define DHT_OK                0
#define DHT_ERROR_CHECKSUM   -1
#define DHT_ERROR_TIMEOUT    -2

unsigned int _humidity;
unsigned int _temperatureC;


void setup() {
  

  Serial.begin(9600);
  while (!Serial) {
    ;
  }
  Serial.println("Started");
  delay(1000);
  //setup analog input
  pinMode(BOOL_PIN, INPUT);                       
  digitalWrite(BOOL_PIN, HIGH);   
 
  //setup ethernet
  //Ethernet.begin(mac);
  Serial.println("Setup done");
}

void loop() {
  String data;
  // get data from sensors
  addSensorInfoToData(&data, 1, 1.0);
  addSensorInfoToData(&data, 2, 2.0);
  //send data to server
  sendDataToServer(&data);
  
  readHum(HUM_PIN);
  Serial.print("temperature ");
  Serial.println(_temperatureC);
  Serial.print("humidity");
  Serial.println(_humidity);
  
  int percentage;
  float volts;
 
  volts = MGRead(MG_PIN);
  Serial.print( "SEN0159:" );
  Serial.print(volts);
  Serial.print( "V           " );
 
  percentage = MGGetPercentage(volts, CO2Curve);
  Serial.print("CO2:");
  if (percentage == -1) {
    Serial.print("Under heating/beyond range(400~10,000)");
  } else {
    Serial.print(percentage);
  }
  Serial.print( "ppm" );
 
  Serial.print( "       Time point:" );
  Serial.print(millis());
  Serial.print("\n");
 
  if (digitalRead(BOOL_PIN) ) {
    Serial.print( "=====BOOL is HIGH======" );
  } else {
    Serial.print( "=====BOOL is LOW======" );
  }
  Serial.print("\n");
  
  delay(1000);
}

void addSensorInfoToData(String* data, unsigned int sensorId, float value) {
  *data = *data + String(sensorId) + ',' + String(value) + "\r\n";
}

void sendDataToServer(String* data) {
  // start the Ethernet connection:
  Ethernet.maintain();
  //renew connection
  client.stop();
  if (client.connect(server, port)) {
    client.println("POST /hydro HTTP/1.1");
    client.println("Host: 192.168.0.21:6001");
    client.println("Content-Type: text/csv");
    client.print("Content-Length: ");
    client.println((*data).length());
    client.println();
    client.println((*data));
  }
  client.stop();
}

/*****************************  MGRead *********************************************
Input:   mg_pin - analog channel
Output:  output of SEN-000007
Remarks: This function reads the output of SEN-000007
************************************************************************************/
float MGRead(int mg_pin) {
  int i;
  float v = 0;
 
  for (i = 0; i < READ_SAMPLE_TIMES; i++) {
    v += analogRead(mg_pin);
    delay(READ_SAMPLE_INTERVAL);
  }
  v = (v / READ_SAMPLE_TIMES) * 5 / 1024 ;
  return v;
}
 
/*****************************  MQGetPercentage **********************************
Input:   volts   - SEN-000007 output measured in volts
         pcurve  - pointer to the curve of the target gas
Output:  ppm of the target gas
Remarks: By using the slope and a point of the line. The x(logarithmic value of ppm)
         of the line could be derived if y(MG-811 output) is provided. As it is a
         logarithmic coordinate, power of 10 is used to convert the result to non-logarithmic
         value.
************************************************************************************/
int  MGGetPercentage(float volts, float *pcurve) {
  volts = volts / DC_GAIN;
  if (volts > ZERO_POINT_VOLTAGE || volts < MAX_POINT_VOLTAGE ) {
    return -1;
  } else {
    return pow(10, (volts - pcurve[1]) / pcurve[2] + pcurve[0]);
    volts = 0;
  }
}

unsigned int readHum(int _pin) {
    // буффер данных
    uint8_t bits[5];
    uint8_t cnt = 7;
    uint8_t idx = 0;

    // очистка буффера
    for (int i = 0; i < 5; i++) bits[i] = 0;

    // согласование с датчиком
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    delay(18);
    digitalWrite(_pin, HIGH);
    delayMicroseconds(40);
    pinMode(_pin, INPUT);

    // проверка отвечает ли датчик
    unsigned int loopCnt = 10000;
    while (digitalRead(_pin) == LOW)
        if (loopCnt-- == 0) return DHT_ERROR_TIMEOUT;

    loopCnt = 10000;
    while (digitalRead(_pin) == HIGH)
        if (loopCnt-- == 0) return DHT_ERROR_TIMEOUT;

    // Считываем 40 бит
    for (int i = 0; i < 40; i++) {
        loopCnt = 10000;
        while (digitalRead(_pin) == LOW)
            if (loopCnt-- == 0) return DHT_ERROR_TIMEOUT;

        unsigned long t = micros();
        loopCnt = 10000;
        while (digitalRead(_pin) == HIGH)
            if (loopCnt-- == 0) return DHT_ERROR_TIMEOUT;

        if ((micros() - t) > 40) bits[idx] |= (1 << cnt);
        // следующий байт?
        if (cnt == 0) {
            cnt = 7;
            idx++;
        } else {
            cnt--;
        }
    }

    // запись данных
    _humidity    = bits[0];
    _temperatureC = bits[2];
    // проверка контрольной суммы
    uint8_t sum = bits[0] + bits[2];

    if (bits[4] != sum) return DHT_ERROR_CHECKSUM;
    return DHT_OK;
}
