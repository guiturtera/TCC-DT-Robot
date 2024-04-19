using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine.Networking;
using UnityEngine;
using UnityEditor;

namespace PimDeWitte.UnityMainThreadDispatcher.Assets.roboarm.Scripts
{
    public class HttpManager
    {
        private string _url = "http://34.201.54.193:1026/v2/entities/urn:ngsi-ld:RoboArm:001/attrs?type=RoboArm";
        private static HttpManager _instance;

        public static HttpManager GetInstance()
        {
            if (_instance == null)
                return new HttpManager();

            return _instance;
        }
        
        public void PostMotorCommand(string motorName, float angle)
        {
            MotorCommand command = new MotorCommand(angle);

            string jsonData = JsonUtility.ToJson(command);
            jsonData = $"{{ \"{motorName}\": {jsonData} }}";

            Debug.Log($"Sending {jsonData}");

            PlayerSettings.insecureHttpOption = InsecureHttpOption.AlwaysAllowed;
            UnityWebRequest webRequest = UnityWebRequest.Put(_url, jsonData);
            webRequest.method = "PATCH";
            webRequest.SetRequestHeader("Content-Type", "application/json");
            webRequest.SetRequestHeader("fiware-service", "smart");
            webRequest.SetRequestHeader("fiware-servicepath", "/");
            webRequest.SendWebRequest();

            if (webRequest.error != null)
            {
                Debug.LogError(webRequest.error);
            }
            else
            {
                Debug.Log("Response: " + webRequest.downloadHandler.text);
            }
        }

        private HttpManager() { }
    }
}
