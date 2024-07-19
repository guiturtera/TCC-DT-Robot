#include <VarSpeedServo.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Thread.h>
#include <ThreadController.h>

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

#define TOPIC_SUB "/ROB/DeviceRoboArm001/cmd"
#define TOPIC_PUB_ATTRS "/ROB/DeviceRoboArm001/attrs"
#define ID_MQTT "fiware"

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE};
IPAddress server(34, 201, 54, 193);

EthernetClient ethClient;
PubSubClient client(ethClient);

Thread mqttReadThread = Thread();
Thread mqttLoopThread = Thread();
Thread mqttStatusThread = Thread();
Thread joystickControl1 = Thread();
Thread joystickControl2 = Thread();
ThreadController controller = ThreadController();

enum Motor { Height, Claw, Base, Reach};
RoboServo servos[] =  { RoboServo(2) , RoboServo(7) , RoboServo(8) , RoboServo(4) };
int defaults[] { 0, 180, 90, 150};

int current_motor = 1;
int current_angle = 1;

#define SERVO_STEP 2
enum control { Left, Right};
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

  Serial.println("Servo ID: " + String(servoId));
  Serial.println("Angle: " + String(angle));
  Serial.println("Device Modified: " + device_modified);

  if (device_modified != "real") {
      moves(servoId - 1, angle);
  }
}

void readMQTT()
{
  if (client.connect(ID_MQTT))
  {
    client.loop();
  }
}

// Função principal de execução MQTT
void mqttLoop()
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
    Serial.println("* Tentando se conectar ao Broker MQTT: ");
    if (client.connect(ID_MQTT))
    {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      client.subscribe(TOPIC_SUB);
    }
    else
    {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Nova tentativa de conexao em 2s");
      delay(2000);
    }
  }
}

void sendAngleMQTT(int servoId, int angle) {
  //example: mt1|120|d|real
  client.publish(TOPIC_PUB_ATTRS, ("mt" + String(servoId + 1) + "|" + String(angle) + "|d|real").c_str());
}

void moveRoboArm(int i, int angle, bool shouldWait)
{
  current_motor = i;
  current_angle = angle;
  servos[i].write(angle, shouldWait);
}

void moves(int servoId, int angle, bool shouldWait = false) {
  sendAngleMQTT(servoId, angle);
  moveRoboArm(servoId, angle, shouldWait);
}


void movesPredefinition() {
  moves(0, 20, true); // extensao  pra 20
  moves(2, 45, true); // base para 45
  moves(3, 130, true); // altura pra 130
  moves(1, 150, true); // garra para 150
  moves(1, 180, true); // garra para 180
  moves(3, 150, true); // altura para 150
  moves(2, 90, true); // base para 90
  moves(0, 0, true); // extensao para 0
}

void attachment() {
  for (int i = 0; i <= 3; i++) {
    servos[i].attach(defaults[i]);
  }
}


void processJoystickRight() {
  if (controls[control::Right].isMovingRight() == 1) {
    moves(Motor::Base, servos[Motor::Base].read() - SERVO_STEP);
  }

  if (controls[control::Right].isMovingLeft() == 1) {
    moves(Motor::Base, servos[Motor::Base].read() + SERVO_STEP);
  }

  if (controls[control::Right].isMovingBottom() == 1) {
    moves(Motor::Height, servos[Motor::Height].read() - SERVO_STEP);
  }

  if (controls[control::Right].isMovingTop() == 1) {
    moves(Motor::Height, servos[Motor::Height].read() + SERVO_STEP);
  }
}

void processJoystickLeft() {
  if (controls[control::Left].isMovingRight() == 1) {
    moves(Motor::Reach, servos[Motor::Reach].read() - SERVO_STEP);
  }

  if (controls[control::Left].isMovingLeft() == 1) {
    moves(Motor::Reach, servos[Motor::Reach].read() + SERVO_STEP);
  }

  if (controls[control::Left].isMovingBottom() == 1) {
    moves(Motor::Claw, servos[Motor::Claw].read() - SERVO_STEP);
  }

  if (controls[control::Left].isMovingTop() == 1) {
    moves(Motor::Claw, servos[Motor::Claw].read() + SERVO_STEP);
  }
}

void command(String c) {
  if (c.startsWith("1:") || c.startsWith("2:") || c.startsWith("3:") || c.startsWith("4:"))
  {
    int servoId = c.substring(0, 1).toInt();
    int angle = c.substring(2).toInt();

    moves((servoId - 1), angle, true);
  }else if(c.equalsIgnoreCase("a")){
    movesPredefinition();
  } else {
    Serial.print("Valores do Joystick(1):");
    Serial.println(controls[control::Right].getX());
    Serial.println(controls[control::Right].getY());

    Serial.print("Valores do Joystick(2):");
    Serial.println(controls[control::Left].getX());
    Serial.println(controls[control::Left].getY());

  }
}


void setup() {
  Serial.begin(9600);

  attachment();

  Ethernet.begin(mac);
  delay(1500);

  client.setServer(server, 1883);
  client.setCallback(mqtt_callback);
  client.setKeepAlive(6);

  // Inicialização das threads MQTT
  mqttLoopThread.onRun(mqttLoop);
  mqttLoopThread.setInterval(1000);

  mqttReadThread.onRun(readMQTT);
  mqttReadThread.setInterval(1000);

  joystickControl1.onRun(processJoystickRight);
  joystickControl1.setInterval(50);

  joystickControl2.onRun(processJoystickLeft);
  joystickControl2.setInterval(50);

  controller.add(&mqttLoopThread);
  controller.add(&mqttReadThread);
  controller.add(&joystickControl1);
  controller.add(&joystickControl2);
}

void loop() {
  controller.run();

  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    command(input);
  }

}