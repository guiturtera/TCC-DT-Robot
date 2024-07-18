using UnityEngine;
using System.Text;
using System.Collections;
using System.Collections.Generic;
using uPLibrary.Networking.M2Mqtt;
using uPLibrary.Networking.M2Mqtt.Messages;
using uPLibrary.Networking.M2Mqtt.Exceptions;
using PimDeWitte.UnityMainThreadDispatcher;

public class MqttManager : MonoBehaviour
{
    public static MqttManager instancia;

    private void Awake() {
        instancia = this;
    }

    private MqttClient client;

    public string brokerIpAddress = "34.201.54.193";
    public int brokerPort = 1883;
    public string clientId = "UnityClient";
    public string subscribeTopic = "/TEF/DeviceRoboArm001/cmd";
    public string publishTopic = "/TEF/DeviceRoboArm001/attrs";

    private void Start()
    {
        client = new MqttClient(brokerIpAddress);
        client.MqttMsgPublishReceived += Client_MqttMsgPublishReceived;
        string clientId = "UnityClient_" + System.Guid.NewGuid().ToString();
        client.Connect(clientId);

        if (client.IsConnected)
        {
            client.Subscribe(new string[] { subscribeTopic }, new byte[] { MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE });
            Debug.Log("Connect to MQTT broker.");
        }
        else
        {
            Debug.LogError("Failed to connect to MQTT broker.");
        }
    }

    private void Client_MqttMsgPublishReceived(object sender, MqttMsgPublishEventArgs e)
    {
        string jsonString = Encoding.UTF8.GetString(e.Message);

        Debug.Log("jsonString => "+jsonString);

        RootObject obj = JsonUtility.FromJson<RootObject>(jsonString);
        
        // A partir deste ponto, você pode acessar os dados desserializados usando obj.subscriptionId, obj.data, etc.
        
        // Determinando qual motor foi enviado e seu valor
        if (obj.data != null && obj.data.Length > 0)
        {
            Data motorData = obj.data[0];

            if (motorData.lastDevice == "VirtualRoboArm") return;

            UnityMainThreadDispatcher.Instance().Enqueue(() => {
                // Operações que na thread principal
                if (!float.IsNaN(motorData.motor1))
                {
                    Debug.Log("Motor 1 foi enviado com valor: " + motorData.motor1);
                    Roboarm.instancia.SetAngleRotationServo1(motorData.motor1);
                }
                else if (!float.IsNaN(motorData.motor2))
                {
                    Debug.Log("Motor 2 foi enviado com valor: " + motorData.motor2);
                    Roboarm.instancia.SetAngleRotationServo2(motorData.motor2);
                }
                else if (!float.IsNaN(motorData.motor3))
                {
                    Debug.Log("Motor 3 foi enviado com valor: " + motorData.motor3);
                    Roboarm.instancia.SetAngleRotationServo3(motorData.motor3);
                }
                else if (!float.IsNaN(motorData.motor4))
                {
                    Debug.Log("Motor 4 foi enviado com valor: " + motorData.motor4);
                    Roboarm.instancia.SetAngleRotationServo4(motorData.motor4);
                }
                else
                {
                    Debug.Log("Nenhum motor foi enviado no JSON.");
                }
            });
        }
        else
        {
            Debug.Log("Nenhum dado foi recebido no JSON.");
        }       
    }

    public void MoveRoboArmServo1(float motor1Angle)
    {     
        client.Publish(publishTopic, Encoding.UTF8.GetBytes("ld|VirtualRoboArm"), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true);
        client.Publish(publishTopic, Encoding.UTF8.GetBytes($"mt1|{(int)motor1Angle}"), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true);
    }

    public void MoveRoboArmServo2(float motor2Angle)
    {     
        client.Publish(publishTopic, Encoding.UTF8.GetBytes("ld|VirtualRoboArm"), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true);
        client.Publish(publishTopic, Encoding.UTF8.GetBytes($"mt2|{(int)motor2Angle}"), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true);
    }

    public void MoveRoboArmServo3(float motor3Angle)
    {     
        client.Publish(publishTopic, Encoding.UTF8.GetBytes("ld|VirtualRoboArm"), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true);
        client.Publish(publishTopic, Encoding.UTF8.GetBytes($"mt3|{(int)motor3Angle}"), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true);
    }

    public void MoveRoboArmServo4(float motor4Angle)
    {     
        client.Publish(publishTopic, Encoding.UTF8.GetBytes("ld|VirtualRoboArm"), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true);
        client.Publish(publishTopic, Encoding.UTF8.GetBytes($"mt4|{(int)motor4Angle}"), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true);
    }
    
    private void OnDestroy()
    {
        if (client != null && client.IsConnected)
        {
            client.Disconnect();
        }
    }

    [System.Serializable]
    public class Data
    {
        public string id;
        public string type;
        public float motor1 = float.NaN;
        public float motor2 = float.NaN;
        public float motor3 = float.NaN;
        public float motor4 = float.NaN;
        public string lastDevice;
    }

    [System.Serializable]
    public class RootObject
    {
        public string subscriptionId;
        public Data[] data;
    }
}
