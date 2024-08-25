#include <VarSpeedServo.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ThreadController.h>
#include <SPI.h>
#include <Wire.h>

#define TOPIC_ATTRS "/TEF/DeviceRoboArm001/attrs"
#define TOPIC_ACK "/TEF/DeviceRoboArm001/ack"
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
IPAddress server(34, 201, 54, 193);
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE};
unsigned long t1;

ThreadController controller = ThreadController();

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
  if(strcmp(topic, TOPIC_ATTRS) == 0){
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
         client.publish(TOPIC_ACK, ("ack|d|real"));
      }
    
  }else{
    unsigned long t2 = micros();
  
    client.publish(TOPIC_METRICS, ("t1|" + String(t1) + "|t2|" + String(t2)).c_str());
  }
  
  
}

void mqttLoop()
{
    client.loop();
}

void reconnect()
{
  if (!client.connected())
  {
    reconnectMQTT();
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
      client.subscribe(TOPIC_ATTRS);
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
  client.setKeepAlive(6);

  Thread mqttReadThread = Thread();
  Thread mqttLoopThread = Thread();
  Thread moveThread = Thread();

  // Inicialização das threads MQTT
  mqttLoopThread.onRun(mqttLoop);
  mqttLoopThread.setInterval(1005);

  mqttReadThread.onRun(reconnect);
  mqttReadThread.setInterval(3015);

  moveThread.onRun(movePredefinition);
  moveThread.setInterval(5000);

  controller.add(&mqttLoopThread);
  controller.add(&mqttReadThread);
  controller.add(&moveThread);
}



void loop() {
  controller.run();
}