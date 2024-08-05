#include <VarSpeedServo.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ThreadController.h>
#include <RTClib.h>
#include <NTPClient.h>
#include <SPI.h>
#include <Wire.h>

#define TOPIC_SUB "/TEF/DeviceRoboArm001/cmd"
#define TOPIC_PUB_SUB_ATTRS "/TEF/DeviceRoboArm001/attrs"
#define ID_MQTT "fiware"
#define SERVO_STEP 2
#define LEFT 0
#define RIGHT 1
#define MAX 180
#define MIN 0
#define MQTT_MAX_PACKET_SIZE 256

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
RTC_DS3231 rtc;

ThreadController controller = ThreadController();

enum Motor { Height, Claw, Base, Reach};
RoboServo servos[] =  { RoboServo(2) , RoboServo(7) , RoboServo(8) , RoboServo(4) };
String t1;

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
  String device_modified = msgStr.substring((thirdIndex + 1),(thirdIndex + 5));

  
  if (device_modified == "virt") {
      t1 = getRTCTimestamp();
      moves(servoId - 1, angle);
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
      client.subscribe(TOPIC_SUB);
      client.subscribe(TOPIC_PUB_SUB_ATTRS);
    }
    else
    {
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setupRTC() {
  if (!rtc.begin()) {
    Serial.println(F("Não foi possível encontrar o RTC"));
    while (true);
  }

  if (rtc.lostPower()) {
    Serial.println(F("RTC perdeu energia, ajustando a hora!"));
    timeClient.update();
    rtc.adjust(DateTime(timeClient.getEpochTime()));
  }
}

String getRTCTimestamp() {
  DateTime now = rtc.now();
  char timestamp[24];
  sprintf(timestamp, "%02d-%02d-%02d %02d:%02d:%02d.%03d", 
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second(), 
          (millis() % 1000));
  return String(timestamp);
}

void sendAngleMQTT(int servoId, int angle) {
  String t2 = getRTCTimestamp();
  client.publish(TOPIC_PUB_SUB_ATTRS, ("mt" + String(servoId + 1) + "|" + String(angle) + "|d|real|t1|" + t1 + "|t2|" + t2).c_str());
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

  timeClient.begin();
  setupRTC();
  
  client.setServer(server, 1883);
  client.setCallback(mqtt_callback);
  client.setKeepAlive(6);

  Thread mqttReadThread = Thread();
  Thread mqttLoopThread = Thread();

  // Inicialização das threads MQTT
  mqttLoopThread.onRun(mqttLoop);
  mqttLoopThread.setInterval(1005);

  mqttReadThread.onRun(reconnect);
  mqttReadThread.setInterval(3015);

  controller.add(&mqttLoopThread);
  controller.add(&mqttReadThread);
}

void loop() {
  controller.run();
}