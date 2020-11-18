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
    int ipLeaseTime, renewalTime, rebindingTime; //renewaltime = T1 , rebindingtime = T2;
};

class DHCPMessage
{
public:
    //opcode = 1 for reques and 2 for reply
    // Ip address coloumn present in the message
    // Here we assume there is no gateway/relay present and we have our own subnet;
    int opcode, transcationId, secs;
    string clientIpAddr, yourIpAddr, serverIpAddr, clientMacAddress;
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
    vector<DHCPMessage *> messagesBuffer;
    string macAddress, ipAddress;

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
    vector<DHCPMessage *> messagesBuffer;
    string macAddress, ipAddress;
    int leaseTimeLeft;

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

int main()
{
}