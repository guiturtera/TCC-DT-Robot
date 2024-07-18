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
        string ultralightString = Encoding.UTF8.GetString(e.Message);

        Debug.Log("ultralightString => " + ultralightString);

        string[] data = ultralightString.Split("|");
        string motor = data[0], device = data[3];

        if (device != "virtual")
            return;

        float angle = float.Parse(data[1]);

        UnityMainThreadDispatcher.Instance().Enqueue(() =>
        {
            switch (motor) 
            {
                case "mt1":
                    Debug.Log("Motor 1 foi enviado com valor: " + angle);
                    Roboarm.instancia.SetAngleRotationServo1(angle);
                    return;
                case "mt2":
                    Debug.Log("Motor 2 foi enviado com valor: " + angle);
                    Roboarm.instancia.SetAngleRotationServo2(angle);
                    return;
                case "mt3":
                    Debug.Log("Motor 3 foi enviado com valor: " + angle);
                    Roboarm.instancia.SetAngleRotationServo3(angle);
                    return;
                case "mt4":
                    Debug.Log("Motor 4 foi enviado com valor: " + angle);
                    Roboarm.instancia.SetAngleRotationServo4(angle);
                    return;
                default:
                    Debug.Log("Nenhum movimento foi enviado.");
                    return;
            }    
        });
    }

    public void NotifyMovement(string motorId, float angle)
    {
        Debug.Log($"Notificando movimento do {motorId} para o ângulo {(int)angle}.");
        client.Publish(publishTopic, Encoding.UTF8.GetBytes($"{motorId}|{(int)angle}|d|real"), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true);
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
