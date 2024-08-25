#include <VarSpeedServo.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ThreadController.h>
#include <NTPClient.h>
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
String t1 = "";

const long utcOffsetInSeconds = -3 * 3600; // Horário de Brasília (UTC-3)

EthernetUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.br", utcOffsetInSeconds, 1000);

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
  
  client.publish(TOPIC_METRICS, ("t1|" + t1 + "|t2|" + t2).c_str());
  
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
  unsigned long elapsedMillis = (millis()) % 1000; 
  unsigned long timestampMillis = (epochTime * 1000) + elapsedMillis;
  unsigned long millisPart = timestampMillis % 1000;

  //formatDate + milisegundos
  // hh:MM:ss.mmm
  formatDate = formatDate + "." + String(millisPart);
  return formatDate;
}

#define MQTT_MAX_PACKET_SIZE 256
void sendAngleMQTT(int servoId, int angle) {
  t1 = getTimestamp();
  client.publish(TOPIC_ATTRS, ("mt" + String(servoId + 1) + "|" + String(angle) + "|d|real|t1|" + t1).c_str());
  
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
  moveThread.setInterval(5000);

  controller.add(&mqttLoopThread);
  controller.add(&mqttReadThread);
  controller.add(&moveThread);
}



void loop() {
  timeClient.update();
  controller.run();
}