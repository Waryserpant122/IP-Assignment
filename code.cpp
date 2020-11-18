#include <bits/stdc++.h>
#include <thread>
#include <unistd.h>

using namespace std;

int sId = 1, cId = 1;

struct options
{
    string messageType;                    //DHCPDISCOVER , DHCPOFFER etc
    string clientId = "0", serverId = "0"; // clientId will be assigned as its mac address here to identify it.
    string resquestedIpAddress;
    int ipLeaseTime = 0, renewalTime = 0, rebindingTime = 0; //renewaltime = T1 , rebindingtime = T2;
};

class DHCPMessage
{
public:
    //opcode = 1 for reques and 2 for reply
    // Ip address coloumn present in the message
    // Here we assume there is no gateway/relay present and we have our own subnet;
    int opcode, transcationId, secs;
    string clientIpAddr = "0.0.0.0", yourIpAddr = "0.0.0.0", serverIpAddr = "192.168.0.1", clientMacAddress;
    options *opt = new options;

    DHCPMessage(int opcode, int transId, string messageType)
    {
        this->opcode = opcode;
        this->transcationId = transId;
        this->opt->messageType = messageType;
    }

    void printDHCPMessage()
    {
        cout << "Opcode :- " << opcode << "\tTransaction Id = " << transcationId;
        cout << "\nMessage Type: " << opt->messageType << endl;
    }
};

class serverNode
{
public:
    int serverId;
    string nodeName;
    string macAddress, ipAddress;
    vector<DHCPMessage *> sendMessageBuffer;
    vector<DHCPMessage *> recieveMessageBuffer;
    //A vector to store all thr Ip addresses available with the server
    vector<string> addressPool;

    //Initializing the server node class
    serverNode(string name, string macAddress, string ipAddress)
    {
        this->serverId = sId++;
        this->nodeName = name;
        this->macAddress = macAddress;
        this->ipAddress = ipAddress;
    }
    //Populate the availabe Ip addresses initially
    void populateAvailableIpAddresses(vector<string> addresses)
    {
        this->addressPool = addresses;
    }
};

class clientNode
{
public:
    int clientId;
    string nodeName;
    list<DHCPMessage *> sendMessageBuffer;
    list<DHCPMessage *> recieveMessageBuffer;
    string macAddress, ipAddress = "";
    int leaseTimeLeft; //in hours

    // boolean value to determine whether the client node is new or needs
    // a new Ipaddress after the lease time is expired
    bool needsIpAddress;

    //Initalizing the client Node which is already existing in the subnet
    // and has a Ip address assigned to it.
    clientNode(string name, string macAddress, string ipAddress, int leaseTimeLeft)
    {
        this->needsIpAddress = 0;
        this->clientId = cId++;
        this->nodeName = name;
        this->macAddress = macAddress;
        this->ipAddress = ipAddress;
        this->leaseTimeLeft = leaseTimeLeft;
    }

    //Initialization function for new nodes with only known MAC Address
    clientNode(string name, string macAddress)
    {
        this->needsIpAddress = 1;
        this->clientId = cId++;
        this->nodeName = name;
        this->macAddress = macAddress;
    }
};

vector<serverNode *> serversList;
vector<clientNode *> clientsList;
int lck = 0;
void *serverFunc(void *args)
{
    pthread_detach(pthread_self());
    serverNode *server = (serverNode *)args;
    cout << server->nodeName << endl;
    pthread_exit(NULL);
}

void *clientFunc(void *args)
{
    clientNode *client = (clientNode *)args;
    while (1)
    {
        if (client->recieveMessageBuffer.size() > 0)
        {
            DHCPMessage *mess = client->sendMessageBuffer.front();
            if (mess->opcode == 1)
                client->sendMessageBuffer.pop_front();
        }
        if (client->sendMessageBuffer.size() > 0)
        {
            DHCPMessage *sendMess = client->sendMessageBuffer.front();
            client->sendMessageBuffer.pop_front();
            if (sendMess->opt->messageType == "DHCPOFFER")
            {
                for (int i = 0; i < serversList.size(); i++)
                    serversList[i]->recieveMessageBuffer.push_back(sendMess);
                for (int i = 0; i < clientsList.size(); i++)
                {
                    if (clientsList[i]->clientId != client->clientId)
                        clientsList[i]->recieveMessageBuffer.push_back(sendMess);
                }
                this_thread::sleep_for(chrono::milliseconds(100));
                cout << "Broadcasted a DHCPOFFER messages to all members in the network\n";
                //printDHCPMessage()
            }
        }
    }
    pthread_exit(NULL);
}

int main()
{
    serverNode *server1 = new serverNode("server1", "0a:2C:11:23:cf:03", "193.197.21.4");
    serverNode *server2 = new serverNode("server2", "3a:1f:23:18:cc:01", "193.197.21.5");

    //client 1 2 and 3 are already exsiting
    // client 4 and 5 are newly inducted in the system
    clientNode *client1 = new clientNode("client1", "1b:43:21:65:77:fa", "193.194.22.7", 24);
    clientNode *client2 = new clientNode("client2", "65:77:fa:1b:43:21", "193.194.25.41", 1224);
    clientNode *client3 = new clientNode("client3", "1b:77:fa:43:21:65", "193.194.23.42", 144);
    clientNode *client4 = new clientNode("client4", "3a:1f:01:23:18:cc");
    clientNode *client5 = new clientNode("client5", "23:18:4d:22:1d:cc");

    DHCPMessage *mess = new DHCPMessage(1, 100, "DHCPDISCOVER");
    mess->clientMacAddress = client4->macAddress;
    mess->opt->clientId = client4->clientId;
    client4->sendMessageBuffer.push_back(mess);

    serversList.push_back(server1);
    serversList.push_back(server2);

    clientsList.push_back(client1);
    clientsList.push_back(client2);
    clientsList.push_back(client3);
    clientsList.push_back(client4);
    clientsList.push_back(client5);

    vector<pthread_t> serverThreads(2), clientThreads(5);

    for (int i = 0; i < serversList.size(); i++)
        pthread_create(&serverThreads[i], NULL, serverFunc, serversList[i]);

    for (int i = 0; i < clientsList.size(); i++)
        pthread_create(&clientThreads[i], NULL, clientFunc, clientsList[i]);

    for (int i = 0; i < serversList.size(); i++)
        pthread_join(serverThreads[i], NULL);

    for (int i = 0; i < clientThreads.size(); i++)
        pthread_join(clientThreads[i], NULL);
}