#include <enet/enet.h>
#include <iostream>
#include <thread>
#include <string>

using namespace std;

ENetAddress address;
ENetHost* server = nullptr;
ENetHost* client = nullptr;
string username;
bool clientConnected = false;
bool shouldEndThread = false;


void CreateServer();
void CreateClient();
void ConnectClientToServer();
void SendPacket(ENetHost* host, string message);
void ReceivePacket(ENetPacket* packet);
void PollForServerEvents();
void PollForClientEvents();
void PollForInput(ENetHost* host);


int main(int argc, char** argv)
{
    // ENET INITIALIZATION
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        cout << "An error occurred while initializing ENet." << endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);




    // MAIN MENU
    cout << "1) Create Server " << endl;
    cout << "2) Create Client " << endl;
    int userInput;
    cin >> userInput;

    // USER CHOICE 1 VS CHOICE 2
    if (userInput == 1)
    {
        CreateServer();

        // set username
        cout << "Choose a username: ";
        cin >> username;
        
        // THREAD 1
        thread ServerEventsThread(PollForServerEvents);

        // THREAD 2
        thread InputThread(PollForInput, server);
        // dont want to not start executing this until a client is created? or does it matter?

        ServerEventsThread.join();
        InputThread.join();
    }
    else if (userInput == 2)
    {
        CreateClient();

        // set username
        cout << "Choose a username: ";
        cin >> username;

        ConnectClientToServer();

        // THREAD 1
        thread ClientEventsThread(PollForClientEvents);

        // THREAD 2
        thread InputThread(PollForInput, client);
        
        ClientEventsThread.join();
        InputThread.join();
    }
    else
    {
        cout << "Invalid Input" << endl;
    }
    
    



    // CLEAN UP
    if (server != nullptr)
    {
        enet_host_destroy(server);
    }

    if (client != nullptr)
    {
        enet_host_destroy(client);
    }
    
    return EXIT_SUCCESS;
}




void CreateServer()
{
    /* Bind the server to the default localhost.     */
    /* A specific host address can be specified by   */
    /* enet_address_set_host (& address, "x.x.x.x"); */
    address.host = ENET_HOST_ANY;
    /* Bind the server to port 1234. */
    address.port = 1234;
    server = enet_host_create(&address /* the address to bind the server host to */,
        32      /* allow up to 32 clients and/or outgoing connections */,
        2      /* allow up to 2 channels to be used, 0 and 1 */,
        0      /* assume any amount of incoming bandwidth */,
        0      /* assume any amount of outgoing bandwidth */);


    if (server == nullptr)
    {
        fprintf(stderr,
            "An error occurred while trying to create an ENet server host.\n");
        exit(EXIT_FAILURE);
    }
}

void CreateClient()
{
    client = enet_host_create(NULL /* create a client host */,
        1 /* only allow 1 outgoing connection */,
        2 /* allow up 2 channels to be used, 0 and 1 */,
        0 /* assume any amount of incoming bandwidth */,
        0 /* assume any amount of outgoing bandwidth */);

    if (client == nullptr)
    {
        fprintf(stderr,
            "An error occurred while trying to create an ENet client host.\n");
        exit(EXIT_FAILURE);
    }
}

void ConnectClientToServer()
{
    ENetAddress address;
    ENetEvent event;
    ENetPeer* peer;
    /* Connect to some.server.net:1234. */
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 1234;
    /* Initiate the connection, allocating the two channels 0 and 1. */
    peer = enet_host_connect(client, &address, 2, 0);
    if (peer == NULL)
    {
        fprintf(stderr,
            "No available peers for initiating an ENet connection.\n");
        exit(EXIT_FAILURE);
    }
    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service(client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        cout << "Connection to 127.0.0.1:1234 succeeded." << endl;
    }
    else
    {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset(peer);
        cout << "Connection to 127.0.0.1:1234 failed." << endl;
    }
}



void SendPacket(ENetHost* host, string message)
{
    const char* messageCharArray = message.c_str();

    ENetPacket* packet = enet_packet_create(messageCharArray,
        strlen(messageCharArray) + 1,
        ENET_PACKET_FLAG_RELIABLE);

    enet_host_broadcast(host, 0, packet);

    enet_host_flush(host);
}



void ReceivePacket(ENetPacket* packet)
{
    cout << (char*)packet->data << endl;

    enet_packet_destroy(packet); // Clean up the packet now that we're done receiving it.
}

void PollForServerEvents()
{
    while (!shouldEndThread)
    {
        ENetEvent event;

        // wait for a client to connect
        if (!clientConnected)
        {
            cout << "Waiting for client to connect..." << endl;
        }

        /* Wait up to 1000 milliseconds for an event. */
        while (enet_host_service(server, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT: // CLIENT CONNECTED TO SERVER
                clientConnected = true;

                cout << "A new client connected from "
                    << event.peer->address.host
                    << ":" << event.peer->address.port
                    << endl;
                /* Store any relevant client information here. */
                event.peer->data = (void*)("Client1");
                break;
            case ENET_EVENT_TYPE_RECEIVE: // PACKET RECEIVED TO SERVER
                ReceivePacket(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT: // CLIENT DISCONNECTED FROM SERVER
                cout << (char*)event.peer->data << " disconnected." << endl;
                /* Reset the peer's client information. */
                event.peer->data = NULL;
                break;
            }
        }
    }
}


void PollForClientEvents()
{
    while (!shouldEndThread)
    {
        ENetEvent event;
        /* Wait up to 1000 milliseconds for an event. */
        while (enet_host_service(client, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                ReceivePacket(event.packet);
                break;
            }
        }
    }

}


void PollForInput(ENetHost* host)
{
    string message;

    while (!shouldEndThread)
    {
        getline(std::cin >> std::ws, message); // accept all input even with spaces

        if (message == "q")
        {
            shouldEndThread = true;
            exit;
        }

        message = username + ": " + message; // concatenate username

        cout << "\x1b[1F" << "\x1b[2K"; // clear input line

        cout << message << endl; // output message to current console

        SendPacket(host, message); // send message to other console
    }
}