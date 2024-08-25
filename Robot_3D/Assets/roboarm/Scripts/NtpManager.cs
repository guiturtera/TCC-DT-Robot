using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System;

namespace PimDeWitte.UnityMainThreadDispatcher.Assets.roboarm.Scripts
{
    public class NtpManager : MonoBehaviour
    {
        public static DateTime GetNetworkTime()
        {
            // Endereço do servidor NTP
            const string ntpServer = "a.ntp.br";
            var ntpData = new byte[48];
            ntpData[0] = 0x1B; // Indicador de solicitação NTP

            // Resolve o endereço do servidor NTP
            var addresses = Dns.GetHostEntry(ntpServer).AddressList;
            var ipEndPoint = new IPEndPoint(addresses[0], 123); // Porta padrão do NTP
            using (var socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp))
            {
                socket.Connect(ipEndPoint);

                socket.ReceiveTimeout = 3000;
                socket.Send(ntpData);
                socket.Receive(ntpData);
                socket.Close();
            }

            // Converte a resposta do servidor NTP para uma data e hora
            const byte serverReplyTime = 40;

            //Variável responsável por obter a parte dos segundos
            ulong intPart = BitConverter.ToUInt32(ntpData, serverReplyTime);

            //Variável responsável por obter os milissegundos
            ulong fractPart = BitConverter.ToUInt32(ntpData, serverReplyTime + 4);

            intPart = SwapEndianness(intPart);
            fractPart = SwapEndianness(fractPart);

            var milliseconds = (intPart * 1000) + ((fractPart * 1000) / 0x100000000L);
            var networkDateTime = (new DateTime(1900, 1, 1)).AddMilliseconds((long)milliseconds);

            return networkDateTime.ToLocalTime();
        }

        // Função para ajustar a ordem dos bytes
        private static uint SwapEndianness(ulong x)
        {
            return (uint)(((x & 0x000000ff) << 24) + ((x & 0x0000ff00) << 8) +
                          ((x & 0x00ff0000) >> 8) + ((x & 0xff000000) >> 24));
        }
    }
}