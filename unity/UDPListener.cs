using System.Collections;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using UnityEngine;
using System.Text;
using System;

public class UDPListener : MonoBehaviour
{
    UdpClient clientData;
    int portData = 37020;
    public int receiveBufferSize = 120000;

    public bool showDebug = false;
    IPEndPoint ipEndPointData;
    private object obj = null;
    private System.AsyncCallback AC;
    byte[] receivedBytes;
    public static double heartRate = 0.0;
    public static int IBI = 0;
    System.String udpReceiveString = "";
    string[] udpReceiveArr;

    void Start()
    {
        InitializeUDPListener();
    }
    public void InitializeUDPListener()
    {
        ipEndPointData = new IPEndPoint(IPAddress.Any, portData);
        clientData = new UdpClient();
        clientData.Client.ReceiveBufferSize = receiveBufferSize;
        clientData.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, optionValue: true);
        clientData.ExclusiveAddressUse = false;
        clientData.EnableBroadcast = true;
        clientData.Client.Bind(ipEndPointData);
        clientData.DontFragment = true;
        if (showDebug) Debug.Log("BufSize: " + clientData.Client.ReceiveBufferSize);
        AC = new System.AsyncCallback(ReceivedUDPPacket);
        clientData.BeginReceive(AC, obj);
        Debug.Log("UDP - Start Receiving..");
    }

    void ReceivedUDPPacket(System.IAsyncResult result)
    {
        //stopwatch.Start();
        receivedBytes = clientData.EndReceive(result, ref ipEndPointData);

        ParsePacket();

        clientData.BeginReceive(AC, obj);

        //stopwatch.Stop();
        //Debug.Log(stopwatch.ElapsedTicks);
        //stopwatch.Reset();
    } // ReceiveCallBack

    void ParsePacket()
    {
        // work with receivedBytes
        Debug.Log("receivedBytes len = " + receivedBytes.Length);
        // Debug.Log("receievedBytes = " + receivedBytes);
        udpReceiveString = Encoding.ASCII.GetString(receivedBytes);
        if (udpReceiveString.Length > 1)
        {
            udpReceiveArr = udpReceiveString.Split(',');
            heartRate = Convert.ToDouble(udpReceiveArr[0]);
            IBI = Convert.ToInt32(udpReceiveArr[1]);

        }
        // heartRate = Convert.ToDouble(udpReceiveString);
        Debug.Log("str converted = " + Encoding.ASCII.GetString(receivedBytes));
        // Debug.Log("int converted = " + BitConverter.ToInt32(receivedBytes, 0));

    }

    void OnDestroy()
    {
        if (clientData != null)
        {
            clientData.Close();
        }

    }
}
