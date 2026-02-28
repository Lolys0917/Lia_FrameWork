//============================================================
// net.cpp 完全版
// 機能維持 + 接続確認 + IP常時描画 + WinSock初期化
// Consoleテスト main 付き
//============================================================

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <conio.h>

#include "Manager.h"

#pragma comment(lib,"ws2_32.lib")

//============================================================
// 定数
//============================================================
constexpr int PORT = 5000;
constexpr int MAX_CLIENT = 16;

//============================================================
// Network Core
//============================================================
SOCKET g_Sock = INVALID_SOCKET;
SOCKET g_ListenSock = INVALID_SOCKET;

bool g_IsHost = false;
bool g_Connected = false;

SOCKET g_UDPSock = INVALID_SOCKET;
sockaddr_in g_UDPAddr{};
bool g_UDPReady = false;


//============================================================
// WinSock
//============================================================
bool Net_Init()
{
    WSADATA wsa;
    int r = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (r != 0)
    {
        std::cout << "WSAStartup Failed : " << r << std::endl;
        return false;
    }
    return true;
}

void Net_Shutdown()
{
    WSACleanup();
}

void NetUDP_Init(const char* ip)
{
    g_UDPSock = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(PORT);
    local.sin_addr.s_addr = INADDR_ANY;

    bind(g_UDPSock, (sockaddr*)&local, sizeof(local));

    g_UDPAddr.sin_family = AF_INET;
    g_UDPAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, ip, &g_UDPAddr.sin_addr);

    u_long nb = 1;
    ioctlsocket(g_UDPSock, FIONBIO, &nb);

    g_UDPReady = true;
}


//============================================================
// Struct Sync
//============================================================

void Net_AddSendStruct(const char* name, Vec4 init)
{
    KeyMap_Add(&GetObjectDataPool()->g_StructKeys, name);
    Vec4_PushBack(&GetObjectDataPool()->g_SendValues, init);
    Vec4_PushBack(&GetObjectDataPool()->g_RecvValues, { 0,0,0,0 });
}
void Net_SetSendStruct(const char* name, Vec4 value)
{
    int index = KeyMap_GetIndex(&GetObjectDataPool()->g_StructKeys, name);
    if (index >= 0 && index < (int)GetObjectDataPool()->g_SendValues.size)
    {
        GetObjectDataPool()->g_SendValues.data[index] = value;
    }
}
Vec4 Net_GetRecvStruct(const char* name)
{
    int index = KeyMap_GetIndex(&GetObjectDataPool()->g_StructKeys, name);
    if (index >= 0 && index < (int)GetObjectDataPool()->g_RecvValues.size)
    {
        return GetObjectDataPool()->g_RecvValues.data[index];
    }
    return { 0,0,0,0 };
}


//============================================================
// ローカルIP取得（常時描画）
//============================================================
std::string GetLocalIP()
{
    char hostname[256];
    gethostname(hostname, sizeof(hostname));

    addrinfo hints{};
    hints.ai_family = AF_INET;

    addrinfo* res = nullptr;
    getaddrinfo(hostname, nullptr, &hints, &res);

    sockaddr_in* addr = (sockaddr_in*)res->ai_addr;
    std::string ip = inet_ntoa(addr->sin_addr);

    freeaddrinfo(res);
    return ip;
}

//============================================================
// Host開始
//============================================================
void Net_StartHost()
{
    g_IsHost = true;

    g_ListenSock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(g_ListenSock, (sockaddr*)&addr, sizeof(addr));
    listen(g_ListenSock, MAX_CLIENT);

    // 非ブロック
    u_long nb = 1;
    ioctlsocket(g_ListenSock, FIONBIO, &nb);
}

//============================================================
// Client接続
//============================================================
bool Net_Connect(const char* ip)
{
    g_Sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int r = connect(g_Sock, (sockaddr*)&addr, sizeof(addr));
    if (r != 0)
    {
        std::cout << "Connect Failed\n";
        return false;
    }

    g_Connected = true;
    return true;
}

//============================================================
// Update
//============================================================
void Net_Update()
{
    // Host Accept
    if (g_IsHost && !g_Connected)
    {
        SOCKET s = accept(g_ListenSock, nullptr, nullptr);
        if (s != INVALID_SOCKET)
        {
            g_Sock = s;
            g_Connected = true;
        }
    }

    if (!g_Connected) return;

    // Send
    send(g_Sock,
        (char*)GetObjectDataPool()->g_SendValues.data,
        (int)(GetObjectDataPool()->g_SendValues.size * sizeof(Vec4)),
        0);

    // Recv
    recv(g_Sock,
        (char*)GetObjectDataPool()->g_RecvValues.data,
        (int)(GetObjectDataPool()->g_RecvValues.size * sizeof(Vec4)),
        0);
}

void NetTCP_Update()
{
    // Host Accept
    if (g_IsHost && !g_Connected)
    {
        SOCKET s = accept(g_ListenSock, nullptr, nullptr);
        if (s != INVALID_SOCKET)
        {
            g_Sock = s;
            g_Connected = true;
            std::cout << "TCP接続完了\n";
        }
    }

    if (!g_Connected) return;

    // TCP Struct Sync
    send(g_Sock,
        (char*)GetObjectDataPool()->g_SendValues.data,
        (int)(GetObjectDataPool()->g_SendValues.size * sizeof(Vec4)),
        0);

    recv(g_Sock,
        (char*)GetObjectDataPool()->g_RecvValues.data,
        (int)(GetObjectDataPool()->g_RecvValues.size * sizeof(Vec4)),
        0);
}
void NetUDP_Update()
{

    //--------------------------------
    // Send
    //--------------------------------
    sendto(g_UDPSock,
        (char*)GetObjectDataPool()->g_SendValues.data,
        (int)(GetObjectDataPool()->g_SendValues.size * sizeof(Vec4)),
        0,
        (sockaddr*)&g_UDPAddr,
        sizeof(g_UDPAddr));

    //--------------------------------
    // Recv
    //--------------------------------
    sockaddr_in from{};
    int fromlen = sizeof(from);

    recvfrom(g_UDPSock,
        (char*)GetObjectDataPool()->g_RecvValues.data,
        (int)(GetObjectDataPool()->g_RecvValues.size * sizeof(Vec4)),
        0,
        (sockaddr*)&from,
        &fromlen);
}




//============================================================
// Ping
//============================================================
int g_Ping = 0;

void Net_UpdatePing()
{
    static DWORD last = GetTickCount();
    DWORD now = GetTickCount();
    g_Ping = (int)(now - last);
    last = now;
}

//============================================================
// Console描画
//============================================================
void DrawConsole()
{
    system("cls");

    std::cout << "==== Network ====\n";

    if (g_IsHost)
    {
        std::cout << "Mode : HOST\n";
        std::cout << "IP   : " << GetLocalIP() << "\n";
        std::cout << "Port : " << PORT << "\n";
    }
    else
    {
        std::cout << "Mode : CLIENT\n";
    }

    std::cout << "State : "
        << (g_Connected ? "接続完了" : "未接続")
        << "\n";

    std::cout << "Ping : " << g_Ping << " ms\n";

    for (size_t i = 0; i < GetObjectDataPool()->g_SendValues.size; i++)
    {
        Vec4 s = GetObjectDataPool()->g_SendValues.data[i];
        Vec4 r = GetObjectDataPool()->g_RecvValues.data[i];

        std::cout << "\n[" << i << "] SEND : "
            << s.X << "," << s.Y << ","
            << s.Z << "," << s.W;

        std::cout << "\n    RECV : "
            << r.X << "," << r.Y << ","
            << r.Z << "," << r.W << "\n";
    }
}

char ip[64];
void InitNet(int SetSC)
{
    if (!Net_Init()) return;

    KeyMap_Init(&GetObjectDataPool()->g_StructKeys);
    Vec4_Init(&GetObjectDataPool()->g_SendValues);
    Vec4_Init(&GetObjectDataPool()->g_RecvValues);

    /*int mode;
    std::cout << "1:Host 2:Client\n";
    std::cin >> mode;*/

    if (SetSC == 1)
    {
        Net_StartHost();
    }
    else
    {
        //std::cin >> ip;
        Net_Connect(ip);
    }
}
void SetIP(const char* SetIP)
{
    strcpy_s(ip, SetIP);
}
void UpdateNet()
{
    Net_Update();
    Net_UpdatePing();
    //DrawConsole();
}

// --------------------------------------------------
// UDP Init
// mode : 1 = Host / 2 = Client
//--------------------------------------------------
void UDPInit(int SetSC)
{
    // 二重初期化防止

    //--------------------------------
    // WinSock Init
    //--------------------------------
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        MessageBoxA(NULL, "WSAStartup Failed", "UDPInit", MB_OK);
    }

    //--------------------------------
    // Socket 作成
    //--------------------------------
    g_UDPSock = socket(AF_INET, SOCK_DGRAM, 0);

    if (g_UDPSock == INVALID_SOCKET)
    {
        MessageBoxA(NULL, "Socket Create Failed", "UDPInit", MB_OK);

    }

    //--------------------------------
    // 非ブロック
    //--------------------------------
    u_long nb = 1;
    ioctlsocket(g_UDPSock, FIONBIO, &nb);

    //--------------------------------
    // Host
    //--------------------------------
    if (SetSC == 1)
    {
        g_UDPAddr.sin_family = AF_INET;
        g_UDPAddr.sin_port = htons(5000);
        g_UDPAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(g_UDPSock,
            (sockaddr*)&g_UDPAddr,
            sizeof(g_UDPAddr)) == SOCKET_ERROR)
        {
            MessageBoxA(NULL, "Bind Failed", "UDPInit", MB_OK);
        }
    }
    //--------------------------------
    // Client
    //--------------------------------
    else
    {
        g_UDPAddr.sin_family = AF_INET;
        g_UDPAddr.sin_port = htons(5000);
        inet_pton(AF_INET, ip, &g_UDPAddr.sin_addr);
    }
}
void UDPUpdate()
{
    NetUDP_Update();
    //Net_UpdatePing();
}


//============================================================
// Console main
//============================================================
//int amain()
//{
//    if (!Net_Init()) return -1;
//
//    KeyMap_Init(&g_StructKeys);
//    Vec4_Init(&g_SendValues);
//    Vec4_Init(&g_RecvValues);
//
//    Net_AddSendStruct("Player", { 0,0,0,0 });
//
//    int mode;
//    std::cout << "1:Host 2:Client\n";
//    std::cin >> mode;
//
//    if (mode == 1)
//    {
//        Net_StartHost();
//    }
//    else
//    {
//        char ip[64];
//        std::cout << "IP入力 : ";
//        std::cin >> ip;
//        Net_Connect(ip);
//    }
//
//    //================ ループ =================
//    while (true)
//    {
//        if (GetAsyncKeyState(VK_UP) & 0x8000)
//            g_SendValues.data[0].Y += 0.1f;
//        if (GetAsyncKeyState(VK_DOWN) & 0x8000)
//            g_SendValues.data[0].Y -= 0.1f;
//        if (GetAsyncKeyState(VK_LEFT) & 0x8000)
//            g_SendValues.data[0].X -= 0.1f;
//        if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
//            g_SendValues.data[0].X += 0.1f;
//
//        Net_Update();
//        Net_UpdatePing();
//        DrawConsole();
//
//        Sleep(16);
//    }
//
//    Net_Shutdown();
//}
