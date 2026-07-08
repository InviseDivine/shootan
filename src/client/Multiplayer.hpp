#pragma once
#include <string>

// fkcn windows...
struct _ENetPacket;
struct _ENetHost;
struct _ENetPeer;

typedef struct _ENetPacket ENetPacket;
typedef struct _ENetHost ENetHost;
typedef struct _ENetPeer ENetPeer;

class Multiplayer {
public:
    static inline Multiplayer& get() {
        static Multiplayer inst;
        return inst;
    }

    void init(std::string nickname, std::string ip, int port = 6829);

    void handlePacket(ENetPacket* packet);

    void sendPacket(char* data, int size, bool reliable);

    std::string getStringReason();
private:
    ENetHost* m_client;
    ENetPeer* m_peer;

    bool m_connected;
    
    int m_reason;

    void update();
};