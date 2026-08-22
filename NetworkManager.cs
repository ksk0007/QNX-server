using System;
using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using UnityEngine;

/// <summary>
/// General-purpose UDP NetworkManager for Unity <-> QNX server connectivity.
///
/// Design goals:
///  - Works with any QNX (or other) UDP server that speaks simple text packets
///    of the form "TYPE" or "TYPE payload...".
///  - Non-blocking from Unity's perspective: all socket I/O happens on a background
///    thread; Unity's Update() only drains a thread-safe queue, so the game loop
///    never stalls on network calls.
///  - Automatic HELLO handshake + periodic keep-alive PING, with reconnect if the
///    server stops responding.
///  - Simple pub/sub: other scripts subscribe to OnMessage / OnConnected /
///    OnDisconnected instead of editing this file for every new packet type.
///
/// Usage:
///   NetworkManager.Instance.Send("MOVE", "12.5,0,3.2");
///   NetworkManager.Instance.OnMessage += (type, payload) => { ... };
/// </summary>
public class NetworkManager : MonoBehaviour
{
    public static NetworkManager Instance { get; private set; }

    [Header("Connection")]
    public string serverIP = "xxx.xxx.xx.x";// enter your ip address inside the quates and replace the xxx with yours
    public int serverPort = xxxx; // enter your requided port and replace the xxx of your wish

    [Header("Handshake / Keep-Alive")]
    [Tooltip("Seconds between PING packets sent while connected.")]
    public float pingInterval = 5f;
    [Tooltip("Seconds without any server message before we consider the connection dead.")]
    public float connectionTimeout = 15f;
    [Tooltip("Seconds to wait before retrying HELLO if no WELCOME is received.")]
    public float helloRetryInterval = 3f;

    /// <summary>Raised on Unity's main thread for every message: (type, payload).</summary>
    public event Action<string, string> OnMessage;
    /// <summary>Raised on Unity's main thread once WELCOME is received.</summary>
    public event Action OnConnected;
    /// <summary>Raised on Unity's main thread when the connection is lost or closed.</summary>
    public event Action OnDisconnected;

    public bool IsConnected { get; private set; }

    private UdpClient client;
    private Thread receiveThread;
    private volatile bool running;

    private readonly ConcurrentQueue<string> incomingQueue = new ConcurrentQueue<string>();
    private readonly object sendLock = new object();

    private float lastServerMessageTime;
    private float lastPingTime;
    private float lastHelloTime;

    void Awake()
    {
        if (Instance != null && Instance != this)
        {
            Destroy(gameObject);
            return;
        }

        Instance = this;
        DontDestroyOnLoad(gameObject);
    }

    void Start()
    {
        Connect();
    }

    /// <summary>
    /// Opens the UDP socket and starts the background receive thread.
    /// Safe to call again after Disconnect() to reconnect.
    /// </summary>
    public void Connect()
    {
        if (running)
            return;

        try
        {
            client = new UdpClient();
            client.Client.ReceiveTimeout = 1000;

            running = true;
            IsConnected = false;

            receiveThread = new Thread(ReceiveLoop);
            receiveThread.IsBackground = true;
            receiveThread.Start();

            lastServerMessageTime = Time.time;
            lastHelloTime = -999f; // force immediate HELLO on next Update
            lastPingTime = Time.time;

            Debug.Log("[NetworkManager] Connecting to " + serverIP + ":" + serverPort);
        }
        catch (Exception ex)
        {
            Debug.LogError("[NetworkManager] Connect failed: " + ex.Message);
            Disconnect();
        }
    }

    void Update()
    {
        if (!running)
            return;

        // Drain messages received on the background thread.
        while (incomingQueue.TryDequeue(out string raw))
        {
            HandleRawMessage(raw);
        }

        if (client == null)
            return;

        float now = Time.time;

        if (!IsConnected)
        {
            // Keep retrying HELLO until the server responds with WELCOME.
            if (now - lastHelloTime >= helloRetryInterval)
            {
                lastHelloTime = now;
                Send("HELLO");
            }
            return;
        }

        // Connected: send periodic keep-alive PINGs.
        if (now - lastPingTime >= pingInterval)
        {
            lastPingTime = now;
            Send("PING");
        }

        // Detect a dead connection (server stopped responding).
        if (now - lastServerMessageTime >= connectionTimeout)
        {
            Debug.LogWarning("[NetworkManager] Connection timed out, reconnecting...");
            Disconnect();
            Connect();
        }
    }

    /// <summary>
    /// Sends a packet to the server. Packet format: "TYPE" or "TYPE payload".
    /// </summary>
    public void Send(string type, string payload = null)
    {
        if (client == null)
            return;

        string packet = string.IsNullOrEmpty(payload) ? type : type + " " + payload;

        try
        {
            byte[] data = Encoding.UTF8.GetBytes(packet);

            lock (sendLock)
            {
                client.Send(data, data.Length, serverIP, serverPort);
            }
        }
        catch (Exception ex)
        {
            Debug.LogError("[NetworkManager] Send failed: " + ex.Message);
        }
    }

    // Runs on a background thread. Never touch Unity API here directly;
    // only push raw strings into the thread-safe queue for Update() to process.
    private void ReceiveLoop()
    {
        IPEndPoint remoteEP = new IPEndPoint(IPAddress.Any, 0);

        while (running)
        {
            try
            {
                if (client == null)
                    break;

                byte[] data = client.Receive(ref remoteEP);
                string message = Encoding.UTF8.GetString(data);

                incomingQueue.Enqueue(message);
            }
            catch (SocketException ex)
            {
                // Timeout is expected (used so the loop can check `running` regularly).
                if (ex.SocketErrorCode != SocketError.TimedOut &&
                    ex.SocketErrorCode != SocketError.Interrupted)
                {
                    if (running)
                        Debug.LogWarning("[NetworkManager] Socket error: " + ex.Message);
                }
            }
            catch (ObjectDisposedException)
            {
                // Socket was closed from Disconnect(); exit quietly.
                break;
            }
            catch (Exception ex)
            {
                if (running)
                    Debug.LogError("[NetworkManager] Receive error: " + ex.Message);
            }
        }
    }

    // Runs on Unity's main thread (called from Update via the queue).
    private void HandleRawMessage(string raw)
    {
        if (string.IsNullOrEmpty(raw))
            return;

        lastServerMessageTime = Time.time;

        string type;
        string payload;

        int spaceIndex = raw.IndexOf(' ');
        if (spaceIndex >= 0)
        {
            type = raw.Substring(0, spaceIndex);
            payload = raw.Substring(spaceIndex + 1);
        }
        else
        {
            type = raw;
            payload = string.Empty;
        }

        switch (type)
        {
            case "WELCOME":
                if (!IsConnected)
                {
                    IsConnected = true;
                    Debug.Log("[NetworkManager] Connected to QNX server.");
                    OnConnected?.Invoke();
                }
                break;

            case "KICK":
                Debug.LogWarning("[NetworkManager] Kicked by server: " + payload);
                Disconnect();
                break;

            case "PONG":
                // Keep-alive acknowledged; lastServerMessageTime already updated above.
                break;

            default:
                // Any other packet type (BROADCAST, MOVE, custom game packets, etc.)
                // is forwarded to subscribers to handle.
                break;
        }

        OnMessage?.Invoke(type, payload);
    }

    /// <summary>
    /// Closes the socket and stops the background thread. Safe to call multiple times.
    /// </summary>
    public void Disconnect()
    {
        bool wasConnected = IsConnected;

        running = false;
        IsConnected = false;

        if (client != null)
        {
            try { client.Close(); }
            catch (Exception) { /* already closed */ }
            client = null;
        }

        if (receiveThread != null)
        {
            receiveThread.Join(200);
            receiveThread = null;
        }

        // Drop any messages that arrived after we decided to disconnect.
        while (incomingQueue.TryDequeue(out _)) { }

        if (wasConnected)
        {
            Debug.Log("[NetworkManager] Disconnected from QNX server.");
            OnDisconnected?.Invoke();
        }
    }

    void OnApplicationQuit()
    {
        Disconnect();
    }

    void OnDestroy()
    {
        Disconnect();
    }
}
