#include <VarSpeedServo.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>

#define TOPIC_ATTRS "/TEF/DeviceRoboArm001/attrs"
#define TOPIC_ACK "/TEF/DeviceRoboArm001/attrs/ack"
#define TOPIC_METRICS "/TEF/metrics/attrs"
#define ID_MQTT "fiware"
#define SERVO_STEP 2
#define MAX 180
#define MIN 0


class RoboServo : public VarSpeedServo {
  private:
    int _pin;


  public:
    RoboServo(int pin) : VarSpeedServo() {
      _pin = pin;
    }

    uint8_t attach(int defValue) {
      VarSpeedServo::write(defValue, 30);
      return VarSpeedServo::attach(_pin);
    }

    void write(int value, bool shouldWait = false) {
      value = constrain(value, MIN, MAX);
      VarSpeedServo::write(value, 30, shouldWait);
    }
};



EthernetClient ethClient;
PubSubClient client(ethClient);
IPAddress server(34,201,54,193);
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE};
unsigned long t1;

enum Motor { Height, Claw, Base, Reach};
RoboServo servos[] =  { RoboServo(2) , RoboServo(7) , RoboServo(8) , RoboServo(4) };

//-------------------------------------------------------------
//-------------------------Protótipos--------------------------
void moves(int servoId, int angle, bool shouldWait = false);
void sendAngleMQTT(int servoId, int angle);
void moveRoboArm(int i, int angle, bool shouldWait);
//-------------------------------------------------------------


void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  
    unsigned long t2 = micros();
    if(client.publish(TOPIC_METRICS, ("t1|" + String(t1) + "_" + String(t2)).c_str())){
      delay(500);
      movePredefinition();
    }
    
    
}

void reconnectMQTT()
{
  while (!client.connected())
  {
    Serial.println("-- ");
    if (client.connect(ID_MQTT))
    {
      Serial.println("0");
      client.subscribe(TOPIC_ACK);
      //client.subscribe(TOPIC_ATTRS);
    }
    else
    {
      
      Serial.println(client.state());
      delay(2000);
    }
  }
}



#define MQTT_MAX_PACKET_SIZE 256
void sendAngleMQTT(int servoId, int angle) {
  t1 = micros();
  client.publish(TOPIC_ATTRS, ("mt" + String(servoId + 1) + "|" + String(angle) + "|d|real|t1|" + String(t1)).c_str());
  
}

void moveRoboArm(int i, int angle, bool shouldWait)
{
  servos[i].write(angle, shouldWait);
}

void moves(int servoId, int angle, bool shouldWait = false) {
  sendAngleMQTT(servoId, angle);
}

void attachment() {
  int defaults[] { 0, 180, 90, 150};
  for (int i = 0; i <= 3; i++) {
    servos[i].attach(defaults[i]);
  }
}

void movePredefinition(){
  moves(random(0,4), random(0, 180));
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  //attachment();
  
  Ethernet.begin(mac);
  
  client.setServer(server, 1883);
  client.setCallback(mqtt_callback);
}

uint8_t isRun = 0;

void loop() {
  

  if (!client.connected())
  {
    reconnectMQTT();
  }
  client.loop();

  if(isRun == 0){
    this.isRun = 1;
    movePredefinition();
  }
}