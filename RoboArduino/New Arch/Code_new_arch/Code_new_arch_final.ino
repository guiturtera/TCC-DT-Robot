#include <VarSpeedServo.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>
#include <ThreadController.h>

#define TOPIC_PUB_SUB_ATTRS "/TEF/DeviceRoboArm001/attrs"
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



ThreadController controller = ThreadController();
Thread joystickControl1 = Thread();
Thread joystickControl2 = Thread();

EthernetClient ethClient;
PubSubClient client(ethClient);
IPAddress server(44,201,194,139);
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
  
   char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  String msgStr = String(message);

  int firstIndex = msgStr.indexOf('|');
  int secondIndex = msgStr.indexOf('|', firstIndex + 1);
  int thirdIndex = msgStr.indexOf('|', secondIndex + 1);

  int servoId = msgStr.substring(firstIndex - 1, firstIndex).toInt();
  int angle = msgStr.substring(firstIndex + 1, secondIndex).toInt();
  String device_modified = msgStr.substring(thirdIndex + 1);

  Serial.println("Servo ID: " + String(servoId));
  Serial.println("Angle: " + String(angle));
  Serial.println("Device Modified: " + device_modified);

  if (device_modified != "real") {
      moves(servoId - 1, angle);
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
      client.subscribe(TOPIC_PUB_SUB_ATTRS);
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
  client.publish(TOPIC_PUB_SUB_ATTRS, ("mt" + String(servoId + 1) + "|" + String(angle) + "|d|real").c_str());
}

void moveRoboArm(int i, int angle, bool shouldWait)
{
  servos[i].write(angle, shouldWait);
}

void moves(int servoId, int angle, bool shouldWait = false) {
  sendAngleMQTT(servoId, angle);
  moveRoboArm(servoId, angle, shouldWait);
}

void attachment() {
  int defaults[] { 0, 180, 90, 150};
  for (int i = 0; i <= 3; i++) {
    servos[i].attach(defaults[i]);
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  attachment();
  
  Ethernet.begin(mac);
  
  client.setServer(server, 1883);
  client.setCallback(mqtt_callback);

  joystickControl1.onRun(readPins);
  joystickControl1.setInterval(50);

  controller.add(&joystickControl1);
}

void readPins(){
  int x1 = analogRead(A0);
  int y1 = analogRead(A1);
  int x2 = analogRead(A2);
  int y2 = analogRead(A3);
  if(y1 > 900){
    moves(Motor::Base, servos[Motor::Base].read() + SERVO_STEP); 
  }

  if(y1 < 200){
    moves(Motor::Base, servos[Motor::Base].read() - SERVO_STEP); 
  }

  if(x1 > 900){
    moves(Motor::Height, servos[Motor::Height].read() - SERVO_STEP); 
  }

  if(x1 < 200){
    moves(Motor::Height, servos[Motor::Height].read() + SERVO_STEP); 
  }

  if(y2 > 900){
    moves(Motor::Reach, servos[Motor::Reach].read() + SERVO_STEP); 
  }

  if(y2 < 200){
    moves(Motor::Reach, servos[Motor::Reach].read() - SERVO_STEP); 
  }

  if(x2 > 900){
    moves(Motor::Claw, servos[Motor::Claw].read() - SERVO_STEP); 
  }

  if(x2 < 200){
    moves(Motor::Claw, servos[Motor::Claw].read() + SERVO_STEP); 
  }
}

void loop() {
  controller.run();

  if (!client.connected())
  {
    reconnectMQTT();
  }
  client.loop();
  
}