using UnityEngine;
using System;
using System.Text;
using System.Collections;
using System.Collections.Generic;
using uPLibrary.Networking.M2Mqtt;
using uPLibrary.Networking.M2Mqtt.Messages;
using uPLibrary.Networking.M2Mqtt.Exceptions;
using PimDeWitte.UnityMainThreadDispatcher;
using PimDeWitte.UnityMainThreadDispatcher.Assets.roboarm.Scripts;

public class MqttManager : MonoBehaviour
{
    DateTime networkTime = NtpManager.GetNetworkTime();
    public static MqttManager instancia;

    private void Awake()
    {
        instancia = this;
    }

    private MqttClient client;

    public string brokerIpAddress = "34.201.54.193";
    public int brokerPort = 1883;
    public string clientId = "UnityClient";
    public string subscribeTopic = "/TEF/DeviceRoboArm001/attrs";
    public string publishTopic = "/TEF/DeviceRoboArm001/attrs";
    public string ackTopic = "/TEF/DeviceRoboArm001/ack";
    public string metricsTopic = "/TEF/metrics/attrs";

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
        DateTime receiveTime = NtpManager.GetNetworkTime();
        string ultralightString = Encoding.UTF8.GetString(e.Message);
        /*DateTime receiveTime = DateTime.Now;*/

        /*Debug.Log("ultralightString => " + ultralightString);*/

        // ultralightString += $"|t2|{receiveTime.ToString("HH:mm:ss.fff")}";
        // Debug.Log("ultralightString => " + ultralightString);

        string[] data = ultralightString.Split("|");
        string motor = data[0], device = data[3], t1 = data[5];

        Debug.Log("t1 => " + t1);
        if (device == "virtual")
            return;
        else
            StartCoroutine(WaitForAck(this.networkTime.ToString("HH:mm:ss.fff")));

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

        /*ultralightString += $"|t2|{receiveTime.ToString("yyyy-MM-dd HH:mm:ss")}"*/
        client.Publish(metricsTopic, Encoding.UTF8.GetBytes($"t1|{t1}|t2|{receiveTime.ToString("HH:mm:ss.fff")}"), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true);
    }

    // Método para fins de teste, com a ideia de repetir os movimentos consecutivas vezes
    private void LoopMovement()
    {
        for (int i = 0; i <= 30; i++)
        {
            NotifyMovement("mt1", 45);
            NotifyMovement("mt3", 45);
        }
    }

    public void NotifyMovement(string motorId, float angle)
    {
        Debug.Log($"Notificando movimento do {motorId} para o ângulo {(int)angle} às {this.networkTime.ToString("HH:mm:ss.fff")}.");

        // Publica a mensagem inicial com o t1
        client.Publish(publishTopic, Encoding.UTF8.GetBytes($"{motorId}|{(int)angle}|d|virtual|t1|{this.networkTime.ToString("HH:mm:ss.fff")}"), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true);
    }

    private IEnumerator WaitForAck(string t1)
    {
        bool ackReceived = false;
        DateTime ackTime = DateTime.MinValue;

        client.MqttMsgPublishReceived += (sender, e) =>
        {
            string ackMessage = Encoding.UTF8.GetString(e.Message);
            if (ackMessage == "ack|d|virtual")
            {
                ackReceived = true;
                ackTime = NtpManager.GetNetworkTime();
            }
        };

        while (!ackReceived)
        {
            yield return null;
        }

        if (ackReceived)
        {
            /*client.Publish(metricsTopic, Encoding.UTF8.GetBytes($"t1|{t1}|t2|{ackTime.ToString("HH:mm:ss.fff")}"), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true);*/
            Debug.Log($"ACK recebido. Tempo registrado: {ackTime.ToString("HH:mm:ss.fff")}");
        }
        else
        {
            Debug.LogWarning("Timeout ao aguardar ACK.");
        }
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