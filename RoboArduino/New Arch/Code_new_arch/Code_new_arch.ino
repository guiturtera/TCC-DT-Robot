#include <VarSpeedServo.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ThreadController.h>
#include <NTPClient.h>
#include <SPI.h>
#include <Wire.h>

#define TOPIC_SUB "/TEF/DeviceRoboArm001/cmd"
#define TOPIC_PUB_SUB_ATTRS "/TEF/DeviceRoboArm001/attrs"
#define TOPIC_METRICS "/TEF/DeviceRoboArm001/metrics"
#define ID_MQTT "fiware"
#define SERVO_STEP 2
#define LEFT 0
#define RIGHT 1
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

const long utcOffsetInSeconds = -3 * 3600; // Horário de Brasília (UTC-3)

EthernetUDP ntpUDP;
NTPClient timeClient(ntpUDP, "a.ntp.br", utcOffsetInSeconds);

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
  String t2 = getTimestamp();
  client.publish(TOPIC_METRICS, ("|d|real|t2|" + t2).c_str());

  // char message[length + 1];
  // memcpy(message, payload, length);
  // message[length] = '\0';

  // String msgStr = String(message);

  // int firstIndex = msgStr.indexOf('|');
  // int secondIndex = msgStr.indexOf('|', firstIndex + 1);
  // int thirdIndex = msgStr.indexOf('|', secondIndex + 1);

  // int servoId = msgStr.substring(firstIndex - 1, firstIndex).toInt();
  // int angle = msgStr.substring(firstIndex + 1, secondIndex).toInt();
  // String device_modified = msgStr.substring((thirdIndex + 1),(thirdIndex + 5));

  
  // if (device_modified == "virt") {
  //     moves(servoId - 1, angle);
  // }
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
      //client.subscribe(TOPIC_SUB);
      client.subscribe(TOPIC_METRICS);
    }
    else
    {
      Serial.println(client.state());
      delay(2000);
    }
  }
}



String getTimestamp() {
  //Obtendo Dados do NTP
  String formatDate = timeClient.getFormattedTime();
  unsigned long epochTime = timeClient.getEpochTime();
  
  //Lógica para os milisegundos
  unsigned long elapsedMillis = millis() % 1000; 
  unsigned long timestampMillis = (epochTime * 1000) + elapsedMillis;
  unsigned long millisPart = timestampMillis % 1000;

  //formatDate + milisegundos
  // hh:MM:ss.mmm
  formatDate = formatDate + "." + String(millisPart);
  return formatDate;
}

#define MQTT_MAX_PACKET_SIZE 256
void sendAngleMQTT(int servoId, int angle) {
  String t1 = getTimestamp();
  client.publish(TOPIC_METRICS, ("mt" + String(servoId + 1) + "|" + String(angle) + "|d|real|t1|" + t1).c_str());
  
}

void moveRoboArm(int i, int angle, bool shouldWait)
{
  servos[i].write(angle, shouldWait);
}

void moves(int servoId, int angle, bool shouldWait = false) {
  sendAngleMQTT(servoId, angle);
  //moveRoboArm(servoId, angle, shouldWait);
}

void attachment() {
  int defaults[] { 0, 180, 90, 150};
  for (int i = 0; i <= 3; i++) {
    servos[i].attach(defaults[i]);
  }
}

void movePredefinition(){
  moves(2, 45);
  delay(1000);
  moves(2, 135);
  delay(1000);
  moves(2, 90);
  delay(1000);
  moves(1, 180);
  delay(1000);
  moves(1, 140);
  delay(1000);
  moves(1, 180);
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  attachment();
  
  Ethernet.begin(mac);

  timeClient.begin();
  timeClient.update();
  
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
  moveThread.setInterval(6000);

  controller.add(&mqttLoopThread);
  controller.add(&mqttReadThread);
  controller.add(&moveThread);
}



void loop() {
  controller.run();
}