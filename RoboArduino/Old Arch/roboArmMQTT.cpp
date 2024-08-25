#include <Ethernet.h>      // Inclui a biblioteca Ethernet
#include <EthernetUdp.h>   // Inclui a biblioteca Udp
#include <SPI.h>           // Inclui a biblioteca SPI
#include <PubSubClient.h>  // Inclui a biblioteca PubSubClient para MQTT
//#include <Servo.h>
#include <VarSpeedServo.h> // inclusão da biblioteca
#include <ThreadController.h>
#include <ArduinoJson.h>
#include "Wire.h"          // Inclui a biblioteca Wire para comunicação I2C

// Configuração da rede
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE};   // Endereço MAC do dispositivo
IPAddress ip(192, 168, 0, 51);                        // Endereço IP do dispositivo
unsigned int localPort = 80;                          // Porta local para comunicação
IPAddress server(52, 149, 214, 24);                    // Endereço IP do servidor MQTT

// Configurações MQTT
const char *BROKER_MQTT = "52.149.214.24";            // URL do broker MQTT
int BROKER_PORT = 1883;                               // Porta do broker MQTT
#define TOPICO_SUBSCRIBE "/TEF/DeviceRoboArm001/cmd"   // Tópico de subscrição MQTT
#define TOPICO_PUBLISH "/TEF/DeviceRoboArm001/attrs"  // Tópico de publicação MQTT
#define ID_MQTT "fiware_01"                            // ID único do cliente MQTT

EthernetClient ethClient;                              // Cliente Ethernet para conexão de rede
PubSubClient client(ethClient);                        // Cliente MQTT

ThreadController mqttThread = ThreadController();      // Controlador de threads para MQTT
ThreadController mqttThreadRead = ThreadController();  // Controlador de threads para leitura MQTT

// Declaração dos objetos do Servo Motor
VarSpeedServo servo1;  
VarSpeedServo servo2;
VarSpeedServo servo3;
VarSpeedServo servo4;

// Função de inicialização
void setup()
{
  // Inicialização dos servo motores
  servo1.attach(9);
  servo2.attach(11);
  servo3.attach(8);
  servo4.attach(10);

  // Posição inicial dos servo motores
  servo1.write(90);
  servo2.write(90);
  servo3.write(90);
  servo4.write(90);
  delay(1000);
  servo4.detach();

  // Inicialização da comunicação serial e Ethernet
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  delay(1500);

  // Configuração do cliente MQTT
  client.setServer(BROKER_MQTT, BROKER_PORT);
  client.setCallback(mqtt_callback);
  client.setKeepAlive(6);

  // Inicialização das threads MQTT
  mqttThread.onRun(mqttLoop);
  mqttThread.setInterval(1000);
  mqttThread.enabled = true;

  mqttThreadRead.onRun(readMQTT);
  mqttThread.setInterval(1000);
  mqttThread.enabled = true;

  // Menu Inicial
  Serial.println("Bem-vindo ao Menu do Controle de Servos!");
  Serial.println("Opções:");
  Serial.println("1. Mover servo individual (formato: 1:90)");
  Serial.println("2. Executar movimento pré definido (digite 'A')");
}

// Função de callback para mensagens MQTT recebidas
void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  DynamicJsonDocument jsonDoc(512);
  deserializeJson(jsonDoc, message);

  String lastDevice = jsonDoc["data"][0]["lastDevice"];
  Serial.print("lastDevice: ");
  Serial.println(lastDevice);

  if (lastDevice != "RealRoboArm")
  {
    for (int i = 1; i <= 4; ++i)
    {
      String motorKey = "motor" + String(i);
      if (jsonDoc["data"][0].containsKey(motorKey))
      {
        int valor = jsonDoc["data"][0][motorKey];
        Serial.print("Motor: ");
        Serial.println(motorKey);
        Serial.print("Valor: ");
        Serial.println(valor);
        moveRoboArm(motorKey, valor);
        break;
      }
    }
  }
}

// Função para reconectar ao broker MQTT em caso de desconexão
void reconnectMQTT()
{
  while (!client.connected())
  {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (client.connect(ID_MQTT))
    {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      client.subscribe(TOPICO_SUBSCRIBE);
    }
    else
    {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Nova tentativa de conexao em 2s");
      delay(2000);
    }
  }
}

// Função para enviar dados dos servos via MQTT
void EnviaAngloServosMQTT(int servoId)
{  
  switch (servoId) 
  {
      case 1:
        client.publish(TOPICO_PUBLISH, "ld|RealRoboArm");
        client.publish(TOPICO_PUBLISH, ("mt1|" + String(servo1.read())).c_str());
        break;
      case 2:
        client.publish(TOPICO_PUBLISH, "ld|RealRoboArm");
        client.publish(TOPICO_PUBLISH, ("mt2|" + String(servo2.read())).c_str());
        break;
      case 3:
        client.publish(TOPICO_PUBLISH, "ld|RealRoboArm");
        client.publish(TOPICO_PUBLISH, ("mt3|" + String(servo3.read())).c_str());
        break;
      case 4:
        client.publish(TOPICO_PUBLISH, "ld|RealRoboArm");
        client.publish(TOPICO_PUBLISH, ("mt4|" + String(servo4.read())).c_str());
        break;
  }  

  Serial.println("*--------//--------*");
  Serial.println("Motor1: " + String(servo1.read()));
  Serial.println("Motor2: " + String(servo2.read()));
  Serial.println("Motor3: " + String(servo3.read()));
  Serial.println("Motor4: " + String(servo4.read()));
  Serial.println("*--------//--------*");

  delay(100);
}

// Função para ler mensagens MQTT
void readMQTT()
{
  if (client.connect(ID_MQTT))
  {
    client.loop();
    delay(1000);
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

// Função para mover o braço do robô baseado no servo e ângulo recebidos
void moveRoboArm(String servo, int angle)
{
  if (servo.equals("motor1"))
  {
    servo1.slowmove(angle, 30);
    delay(2000);
  }
  else if (servo.equals("motor2"))
  {
    servo2.slowmove(angle, 30);
    delay(2000);
  }
  else if (servo.equals("motor3"))
  {
    servo3.slowmove(angle, 30);
    delay(2000);
  }
  else if (servo.equals("motor4"))
  {
    servo4.attach(10);
    delay(500);
    servo4.slowmove(angle, 30);
    delay(1500);
    servo4.detach();
  }
  else
  {
    Serial.println("Servo incorreto!");
  }
}

void executarMovimentoPredefinido()
{
  // Mova o braço para pegar o objeto
  moveRoboArm("motor1", 45);

  EnviaAngloServosMQTT(1);

  // Feche a garra
  moveRoboArm("motor2", 135);

  EnviaAngloServosMQTT(2);

  // Levante o objeto
  moveRoboArm("motor4", 135);

  EnviaAngloServosMQTT(4);

  // Vire para a esquerda
  moveRoboArm("motor3", 45);

  EnviaAngloServosMQTT(3);

  // Abaixe o objeto
  moveRoboArm("motor4", 50);

  EnviaAngloServosMQTT(4);

  // Abra a garra para soltar o objeto
  moveRoboArm("motor2", 90);

  EnviaAngloServosMQTT(2);

  // Feche a garra
  moveRoboArm("motor2", 135);

  EnviaAngloServosMQTT(2);

  // Levante o objeto novamente
  moveRoboArm("motor4", 135);

  EnviaAngloServosMQTT(4);

  // Vire para a direita (ajuste o ângulo conforme necessário)
  moveRoboArm("motor3", 135);

  EnviaAngloServosMQTT(3);

  // Abaixe o objeto
  moveRoboArm("motor4", 50);

  EnviaAngloServosMQTT(4);

  // Feche a garra para pegar o objeto novamente
  moveRoboArm("motor2", 135);

  EnviaAngloServosMQTT(2);
}

void processarComando(String comando) 
{
  if (comando.startsWith("1:") || comando.startsWith("2:") || comando.startsWith("3:") || comando.startsWith("4:")) 
  {
    String servoId = comando.substring(0, 1);
    int angulo = comando.substring(2).toInt();
    
    moveRoboArm("motor"+servoId, angulo);
    EnviaAngloServosMQTT(servoId.toInt());
  } 
  else if (comando.equalsIgnoreCase("A"))
  {
    executarMovimentoPredefinido();
  } 
  else 
  {
    Serial.println("Comando inválido. Use o formato: servo_id:ângulo (por exemplo, 1:90) ou A para movimento pré definido");
  }
}


// Função principal de execução
void loop()
{
  // Executa as threads MQTT
  mqttThread.run();  
  mqttThreadRead.run();

  // Lê dados da porta serial e controla os servos correspondentes
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    processarComando(input);
  }       
}
