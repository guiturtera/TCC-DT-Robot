#include <VarSpeedServo.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ThreadController.h>
#include <SPI.h>
#include <Wire.h>

class Joystick {
  private:
    int _pinX, _pinY;

    int get(int *pin) {
      return analogRead(*pin);
    }

  public:
    Joystick(int pinX, int pinY) {
      _pinX = pinX;
      _pinY = pinY;
      pinMode(_pinX, INPUT);
      pinMode(_pinY, INPUT);

    }

    int getX(void) {
      return get(&_pinX);
    }

    int getY(void) {
      return get(&_pinY);
    }

    int isMovingRight(void) {
      if (getY() < 200) {
        return 1;
      }

      return 0;
    }

    int isMovingLeft(void) {
      if (getY() > 900) {
        return 1;
      }

      return 0;
    }

    int isMovingBottom(void) {
      if (getX() < 200) {
        return 1;
      }

      return 0;
    }

    int isMovingTop(void) {
      if (getX() > 900) {
        return 1;
      }

      return 0;
    }

};

class RoboServo : public VarSpeedServo {
  private:
    int _limits[2];
    int _pin;


  public:
    RoboServo(int pin) : VarSpeedServo() {
      _limits[0] = 0;
      _limits[1] = 180;
      _pin = pin;
    }

    uint8_t attach(int defValue) {
      VarSpeedServo::write(defValue, 30);
      uint8_t res = VarSpeedServo::attach(_pin);
      return res;
    }

    void write(int value, bool shouldWait = false) {
      if (value < _limits[0])
        value = _limits[0];
      if (value > _limits[1])
        value = _limits[1];

      VarSpeedServo::write(value, 30, shouldWait);
    }

    int getMin(void) {
      return _limits[0];
    }

    int getMax(void) {
      return _limits[1];
    }

};

#define TOPIC_SUB "/TEF/DeviceRoboArm001/cmd"
#define TOPIC_PUB_SUB_ATTRS "/TEF/DeviceRoboArm001/attrs"
#define ID_MQTT "fiware"

EthernetClient ethClient;
PubSubClient client(ethClient);

ThreadController controller = ThreadController();

enum Motor { Height, Claw, Base, Reach};
RoboServo servos[] =  { RoboServo(2) , RoboServo(7) , RoboServo(8) , RoboServo(4) };

#define SERVO_STEP 2
#define LEFT 0
#define RIGHT 1

Joystick controls[] = { Joystick(A1, A0), Joystick(A3, A2)};

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

  if (device_modified != "real") {
      moves(servoId - 1, angle);
  }
}

void mqttLoop()
{
    client.loop();
}

// Função principal de execução MQTT
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
      client.subscribe(TOPIC_SUB);
      client.subscribe(TOPIC_PUB_SUB_ATTRS);
    }
    else
    {
      Serial.println("1");
      delay(2000);
    }
  }
}

void sendAngleMQTT(int servoId, int angle) {
  //example: mt1|120|d|real
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

void processJoystick(Joystick *control, Motor mt1, Motor mt2){
  if (control->isMovingRight() == 1) {
    moves(mt1, servos[mt1].read() - SERVO_STEP);
  }

  if (control->isMovingLeft() == 1) {
    moves(mt1, servos[mt1].read() + SERVO_STEP);
  }

  if (control->isMovingBottom() == 1) {
    moves(mt2, servos[mt2].read() - SERVO_STEP);
  }

  if (control->isMovingTop() == 1) {
    moves(mt2, servos[mt2].read() + SERVO_STEP);
  }
}


void processJoystickRight() {
  processJoystick(&controls[RIGHT], Motor::Base, Motor::Height);
}

void processJoystickLeft() {
  processJoystick(&controls[LEFT], Motor::Reach, Motor::Claw);
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  attachment();
  
  byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE};
  Ethernet.begin(mac);
  delay(1500);

  IPAddress server(34, 201, 54, 193);
  
  client.setServer(server, 1883);
  client.setCallback(mqtt_callback);
  client.setKeepAlive(6);

  Thread mqttReadThread = Thread();
  Thread mqttLoopThread = Thread();
  Thread joystickControl1 = Thread();
  Thread joystickControl2 = Thread();

  // Inicialização das threads MQTT
  mqttLoopThread.onRun(mqttLoop);
  mqttLoopThread.setInterval(1005);

  mqttReadThread.onRun(reconnect);
  mqttReadThread.setInterval(3015);

  joystickControl1.onRun(processJoystickRight);
  joystickControl1.setInterval(20);

  joystickControl2.onRun(processJoystickLeft);
  joystickControl2.setInterval(20);

  controller.add(&mqttLoopThread);
  controller.add(&mqttReadThread);
  controller.add(&joystickControl1);
  controller.add(&joystickControl2);
}

void loop() {
  controller.run();
}