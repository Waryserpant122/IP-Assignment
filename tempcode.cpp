#include <bits/stdc++.h>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <string>
using namespace std;

int sId = 1, cId = 1;

struct options
{
    string messageType;             //DHCPDISCOVER , DHCPOFFER etc
    int clientId = 0, serverId = 0; // clientId will be assigned as its mac address here to identify it.
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
    string clientIpAddr = "0.0.0.0", yourIpAddr = "0.0.0.0", serverIpAddr = "255.255.255.255", clientMacAddress;
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
class hostIp
{
public:
    int leaseTime, clientId;
    string ipAddress;

    hostIp(DHCPMessage *ackMessage)
    {
        this->clientId = ackMessage->opt->clientId;
        this->ipAddress = ackMessage->yourIpAddr;
        this->leaseTime = ackMessage->opt->ipLeaseTime;
    }
};

class serverNode
{
public:
    int serverId;
    string nodeName;
    string macAddress, ipAddress;
    list<DHCPMessage *> sendMessageBuffer;
    list<DHCPMessage *> recieveMessageBuffer;
    vector<hostIp *> hostIpList;
    //A vector to store all thr Ip addresses available with the server
    vector<string> addressPool;

    //Initializing the server node class
    serverNode(int sid, string name, string macAddress, string ipAddress)
    {
        this->serverId = sid;
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
    int leaseTimeProvided, leaseTimeLeft; //in hours
    // boolean value to determine whether the client node is new or needs
    // a new Ipaddress after the lease time is expired
    bool needsIpAddress, needsRenewal = 0;

    //Initalizing the client Node which is already existing in the subnet
    // and has a Ip address assigned to it.
    clientNode(string name, string macAddress, string ipAddress, int leaseTimeLeft, int leaseTimeProvided)
    {
        this->needsIpAddress = 0;
        this->clientId = cId++;
        this->nodeName = name;
        this->macAddress = macAddress;
        this->ipAddress = ipAddress;
        this->leaseTimeLeft = leaseTimeLeft;
        this->leaseTimeProvided = leaseTimeProvided;
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

DHCPMessage *copyMessage(DHCPMessage *m)
{
    DHCPMessage *temp = new DHCPMessage(m->opcode, m->transcationId, m->opt->messageType);
    temp->yourIpAddr = m->yourIpAddr;
    temp->clientIpAddr = m->clientIpAddr;
    temp->clientMacAddress = m->clientMacAddress;
    temp->opt->clientId = m->opt->clientId;
    temp->opt->serverId = m->opt->serverId;
    temp->opt->resquestedIpAddress = m->opt->resquestedIpAddress;
    temp->opt->ipLeaseTime = m->opt->ipLeaseTime;

    return temp;
}

vector<serverNode *> serversList;
vector<clientNode *> clientsList;
int lck = 0;

// helper functions

void printDHCPMessage(DHCPMessage *mess)
{
    cout << "\n---------------------------------------------------------------------";
    cout << "\n\t\t\tDHCP MESSAGE\n--------------------------------------------------\n";
    cout << "| opcode     |  transcationId  | messageType      |\n";
    cout << "| ----------------------------------------------- |\n";
    printf("|    %d       |  %d            |  ", mess->opcode, mess->transcationId);
    cout << mess->opt->messageType << "    |\n";
    cout << "| ----------------------------------------------- |\n";
    cout << "| client Ip Addr       | server Ip addr           |\n";
    cout << "| ----------------------------------------------- |\n";
    cout << "|      " << mess->clientIpAddr << "        |        " << mess->serverIpAddr << " |\n";
    cout << "| ----------------------------------------------- |\n";
    cout << "|     client id           |       server id       |\n";
    cout << "| ----------------------------------------------- |\n";
    printf("|           %d             |            %d          |\n", mess->opt->clientId, mess->opt->serverId);
    cout << "| ----------------------------------------------- |\n";
    if (mess->opt->ipLeaseTime != 0)
    {
        cout << "|      Leased Ip Address  |  Lease time in hrs    |\n";
        cout << "| ----------------------------------------------- |\n";
        cout << "|      " << mess->opt->resquestedIpAddress << "      |        " << mess->opt->ipLeaseTime << "            |\n";
        cout << "| ----------------------------------------------- |\n";
    }
    cout << "\n";
    cout << "\n---------------------------------------------------------------------\n";
}

bool isthere(serverNode *server, int clid)
{
    for (auto i : server->hostIpList)
    {
        if (i->clientId == clid)
            return 1;
    }
    return 0;
}

void broadcastMessage(DHCPMessage *sendMess, clientNode *client)
{
    for (int i = 0; i < serversList.size(); i++)
    {
        serversList[i]->recieveMessageBuffer.push_back(copyMessage(sendMess));
    }
    for (int i = 0; i < clientsList.size(); i++)
    {
        if (clientsList[i]->clientId != client->clientId)
            clientsList[i]->recieveMessageBuffer.push_back(copyMessage(sendMess));
    }
}
void dhcpRequestSequence(DHCPMessage *message1, clientNode *client)
{
    DHCPMessage *sendRequest = copyMessage(message1);
    sendRequest->opcode = 1;
    sendRequest->transcationId++;
    sendRequest->opt->serverId = message1->opt->serverId;
    sendRequest->opt->messageType = "DHCPREQUEST";
    // print DHCP message
    cout << " \nDHCP MESSAGE OF REQUEST sent by Client " << client->clientId;
    cout << "\nIntended for Server " << sendRequest->opt->serverId << endl;
    printDHCPMessage(sendRequest);
    broadcastMessage(sendRequest, client);
}
void dhcpCliAckSequence(clientNode *client, int recvServer)
{

    DHCPMessage *message1 = copyMessage(client->recieveMessageBuffer.front());
    client->recieveMessageBuffer.pop_front();
    while (message1->opt->serverId != recvServer && client->recieveMessageBuffer.size() > 0)
    {
        message1 = client->recieveMessageBuffer.front();
        client->recieveMessageBuffer.pop_front();
    }
    if (message1->opt->messageType == "DHCPACK" && message1->opt->serverId == recvServer)
    {
        cout << "\nACK RECIEVED from server " << message1->opt->serverId << "\n";
        cout << "Client " << client->clientId << " has been assigned the IP ";
        cout << message1->opt->resquestedIpAddress << " for the time of ";
        cout << message1->opt->ipLeaseTime << " hours\n\n";
        client->ipAddress = message1->opt->resquestedIpAddress;
        client->leaseTimeLeft = message1->opt->ipLeaseTime;
        client->leaseTimeProvided = message1->opt->ipLeaseTime;
        client->needsIpAddress = 0;

        this_thread::sleep_for(chrono::milliseconds(100));
        cout << endl;
        if (clientsList[4]->needsIpAddress == 1)
        {
            clientsList[4]->needsIpAddress = 0;
            clientNode *client4 = clientsList[4];
            DHCPMessage *mess = new DHCPMessage(1, 200, "DHCPDISCOVER");
            mess->clientMacAddress = client4->macAddress;
            mess->opt->clientId = client4->clientId;
            client4->sendMessageBuffer.push_back(mess);
        }
    }
    // else
    // {
    //    int i = 1;
    //    //do for server NACK
    // }
}

void *serverFunc(void *args)
{
    pthread_detach(pthread_self());
    //this_thread::sleep_for(chrono::microseconds(100));
    serverNode *server = (serverNode *)args;
    if (server->serverId == 2)
        this_thread::sleep_for(chrono::microseconds(700));
    while (1)
    {
        DHCPMessage *recvMess, *message1, *message2, *ackMessage;
        if (server->recieveMessageBuffer.size() > 0)
        {
            DHCPMessage *recvMesstemp;
            recvMesstemp = server->recieveMessageBuffer.front();
            server->recieveMessageBuffer.pop_front();
            string req = recvMesstemp->opt->messageType;
            if (req == "DHCPDISCOVER")
            {
                string toAssignIp = server->addressPool.back();
                DHCPMessage *replyOffer = copyMessage(recvMesstemp);
                replyOffer->opcode = 2;
                replyOffer->transcationId++;
                replyOffer->yourIpAddr = toAssignIp;
                replyOffer->serverIpAddr = server->ipAddress;
                replyOffer->opt->resquestedIpAddress = toAssignIp;
                replyOffer->opt->serverId = server->serverId;
                replyOffer->opt->messageType = "DHCPOFFER";
                replyOffer->opt->ipLeaseTime = rand() % 100;
                free(recvMesstemp);
                this_thread::sleep_for(chrono::microseconds(30));
                cout << "\nAt server " << server->serverId << " recieved a DHCP " << req << " request\n\n";
                cout << "DHCPOFFER sent by server " << server->serverId << " \nmessage type " << replyOffer->opt->messageType << "\n\n";
                int sleepTime = 20 * server->serverId;
                this_thread::sleep_for(chrono::microseconds(sleepTime));
                printDHCPMessage(replyOffer);
                this_thread::sleep_for(chrono::microseconds(10));
                for (int i = 0; i < clientsList.size(); i++)
                    clientsList[i]->recieveMessageBuffer.push_back(copyMessage(replyOffer));
                free(replyOffer);
                this_thread::sleep_for(chrono::microseconds(20));
                while (server->recieveMessageBuffer.size() == 0)
                    this_thread::sleep_for(chrono::microseconds(20));

                if (server->recieveMessageBuffer.size() > 0)
                {
                    message1 = server->recieveMessageBuffer.front();
                    server->recieveMessageBuffer.pop_front();
                    string messType = message1->opt->messageType;
                    if (message1->opt->serverId == server->serverId)
                    {
                        if (message1->opt->messageType == "DHCPREQUEST")
                        {
                            string toSendIp = server->addressPool.back();
                            cout << "DHCP Request given to " << server->nodeName;
                            cout << " by Client " << message1->opt->clientId;
                            cout << "\nSending ACK from server " << server->serverId << " to the client ";
                            cout << message1->opt->clientId << " \n";
                            cout << "WIth Ip-address of " << toSendIp << endl;
                            ackMessage = copyMessage(message1);
                            ackMessage->opt->resquestedIpAddress = toSendIp;
                            ackMessage->transcationId++;
                            ackMessage->opcode = 2;
                            ackMessage->opt->messageType = "DHCPACK";
                            ackMessage->clientIpAddr = ackMessage->opt->resquestedIpAddress;
                            ackMessage->serverIpAddr = server->ipAddress;
                            ackMessage->yourIpAddr = ackMessage->opt->resquestedIpAddress;
                            printDHCPMessage(ackMessage);

                            for (int i = 0; i < clientsList.size(); i++)
                                clientsList[i]->recieveMessageBuffer.push_back(copyMessage(ackMessage));

                            //Remove the Ip from the address pool
                            server->addressPool.pop_back();
                            hostIp *addTemp = new hostIp(ackMessage);
                            server->hostIpList.push_back(addTemp);

                            cout << "\nClient " << ackMessage->opt->clientId << " Added to the address book of ";
                            cout << "Server " << server->serverId << " with Ip addresss " << addTemp->ipAddress;
                            cout << " and Lease Time " << addTemp->leaseTime << " hours\n\n";
                            free(addTemp);
                            this_thread::sleep_for(chrono::microseconds(200));
                        }
                        else if (message1->opt->messageType == "DHCPDECLINE")
                        {
                            // do the work of sending again
                        }
                    }
                }
            }
            // add function to search client in host ip list
            if (req == "DHCPREQUEST")
            {
                if (isthere(server, recvMesstemp->opt->clientId))
                {
                    //do renewal of ip address
                    this_thread::sleep_for(chrono::microseconds(30));
                    DHCPMessage *message1 = copyMessage(recvMesstemp);
                    cout << "\nDHCP Request given to " << server->nodeName;
                    cout << " by Client " << message1->opt->clientId;
                    cout << " for renewal \nSending ACK from Server " << server->serverId << " to the client ";
                    cout << message1->opt->clientId << " \n";
                    cout << "WIth Ip-address of " << message1->opt->resquestedIpAddress << endl;
                    ackMessage = copyMessage(message1);
                    ackMessage->opt->resquestedIpAddress = message1->opt->resquestedIpAddress;
                    ackMessage->transcationId++;
                    ackMessage->opcode = 2;
                    ackMessage->opt->serverId = server->serverId;
                    ackMessage->opt->messageType = "DHCPACK";
                    ackMessage->clientIpAddr = ackMessage->opt->resquestedIpAddress;
                    ackMessage->serverIpAddr = server->ipAddress;
                    ackMessage->yourIpAddr = ackMessage->opt->resquestedIpAddress;
                    srand(time(NULL));
                    int leaseExtendedTime = rand() % 100;
                    ackMessage->opt->ipLeaseTime = leaseExtendedTime;
                    this_thread::sleep_for(chrono::microseconds(20));
                    printDHCPMessage(ackMessage);
                    this_thread::sleep_for(chrono::microseconds(10));
                    for (int i = 0; i < clientsList.size(); i++)
                        clientsList[i]->recieveMessageBuffer.push_back(copyMessage(ackMessage));
                }
                else
                {
                    // Client not present in server book
                    //print that
                    if (server->serverId == 3)
                        this_thread::sleep_for(chrono::microseconds(5));
                    cout << "\nMessage sent to Server " << server->serverId;
                    cout << " By client " << recvMesstemp->opt->clientId << " is discarded as ";
                    cout << " the client's IP address is not there in the Address table of ";
                    cout << "Server " << server->serverId;
                    cout << "\n\n";
                    //this_thread::sleep_for(chrono::microseconds10));
                }
            }
        }
    }
    pthread_exit(NULL);
}
void *clientFunc(void *args)
{
    pthread_detach(pthread_self());
    this_thread::sleep_for(chrono::microseconds(100));
    clientNode *client = (clientNode *)args;
    while (1)
    {
        if (client->recieveMessageBuffer.size() > 0)
        {
            DHCPMessage *t = client->recieveMessageBuffer.front();
            if (t->opt->clientId != client->clientId)
                client->recieveMessageBuffer.pop_front();
        }
        if (client->sendMessageBuffer.size() > 0)
        {
            DHCPMessage *sendMess = client->sendMessageBuffer.front(), *message1, *message2;
            client->sendMessageBuffer.pop_front();
            if (sendMess->opt->messageType == "DHCPDISCOVER")
            {
                cout << "\nClient " << client->clientId << " needs a new IP address\n";
                cout << "Broadcasted a DHCPDISCOVER messages to all members in the network\n";
                printDHCPMessage(sendMess);
                this_thread::sleep_for(chrono::microseconds(10));
                broadcastMessage(sendMess, client);
                this_thread::sleep_for(chrono::microseconds(150));

                while (client->recieveMessageBuffer.size() != 2)
                    this_thread::sleep_for(chrono::microseconds(50));

                message1 = copyMessage(client->recieveMessageBuffer.front());
                client->recieveMessageBuffer.pop_front();
                if (message1->opt->clientId == client->clientId && message1->opt->messageType == "DHCPOFFER")
                {
                    int recvServer = message1->opt->serverId;
                    string recvIp = message1->opt->resquestedIpAddress;
                    cout << "DHCPOFFER recieved from " << message1->opt->serverId;
                    cout << " With Ip Address " << recvIp;
                    cout << "\nLease time as " << message1->opt->ipLeaseTime << " hours \n";
                    this_thread::sleep_for(chrono::microseconds(10));

                    //Decline all other offers from servers.
                    while (client->recieveMessageBuffer.size() == 0)
                        ;
                    while (client->recieveMessageBuffer.size() > 0)
                    {
                        DHCPMessage *temp = client->recieveMessageBuffer.front();
                        client->recieveMessageBuffer.pop_front();
                        cout << "DHCP offer recieved from " << temp->opt->serverId << " is late and has been discarded\n";
                    }
                    client->recieveMessageBuffer.clear();
                    this_thread::sleep_for(chrono::microseconds(20));

                    //Check wheteher Ip is valid or not (ARP used in bg)
                    bool isValidAddress = 1;
                    for (int i = 0; i < clientsList.size(); i++)
                        if (clientsList[i]->clientId != client->clientId)
                            if (clientsList[i]->ipAddress == recvIp)

                                isValidAddress = 0;
                    if (isValidAddress == 1)
                    {
                        dhcpRequestSequence(message1, client);
                        this_thread::sleep_for(chrono::microseconds(10));
                        while (client->recieveMessageBuffer.size() == 0)
                            this_thread::sleep_for(chrono::microseconds(10));
                        while (client->recieveMessageBuffer.size() > 0)
                            dhcpCliAckSequence(client, recvServer);
                    }
                    // else
                    // {
                    //   int i = 1;
                    //   //TODO send message to server no accepted the Address
                    // }
                }
                else
                {
                    int i = 1;
                    usleep(100);
                }
            }
            if (sendMess->opt->messageType == "DHCPREQUEST")
            {

                DHCPMessage *sendRequest = copyMessage(sendMess);
                // print DHCP message
                cout << "\nClient " << client->clientId << " needs renewal of ip address as ";
                cout << "lease time left:- " << client->leaseTimeLeft;
                cout << " is 1/2 times as against lease time provided by server " << client->leaseTimeProvided;
                cout << " \nDHCP MESSAGE OF REQUEST sent by Client " << client->clientId << endl;
                printDHCPMessage(sendRequest);
                this_thread::sleep_for(chrono::microseconds(30));
                broadcastMessage(sendRequest, client);
                this_thread::sleep_for(chrono::microseconds(30));
                while (client->recieveMessageBuffer.size() == 0)
                    this_thread::sleep_for(chrono::microseconds(20));
                DHCPMessage *messageAck = copyMessage(client->recieveMessageBuffer.front());
                client->recieveMessageBuffer.pop_front();
                // if (client->recieveMessageBuffer.size() > 0)
                // {
                //    messageAck = client->recieveMessageBuffer.front();
                //    client->recieveMessageBuffer.pop_front();
                // }
                message1 = copyMessage(messageAck);
                if (message1->opt->messageType == "DHCPACK" && message1->opt->clientId == client->clientId)
                {
                    cout << "Message recieved from server is \n";
                    printDHCPMessage(message1);
                    cout << "\nACK RECIEVED from server " << message1->opt->serverId << "\n";
                    cout << "Client " << client->clientId << " has been given access to renew IP ";
                    cout << message1->opt->resquestedIpAddress << " for the time of ";
                    cout << message1->opt->ipLeaseTime << " hours by the server ";
                    cout << message1->opt->serverId << "\n\n";
                    client->ipAddress = message1->opt->resquestedIpAddress;
                    client->leaseTimeLeft = message1->opt->ipLeaseTime;
                    client->leaseTimeProvided = message1->opt->ipLeaseTime;
                    client->needsRenewal = 0;
                    this_thread::sleep_for(chrono::milliseconds(100));
                    cout << endl;
                }
                //Todo for renewal
            }
        }
    }
    pthread_exit(NULL);
}

void *waitTill(void *args)
{
    pthread_detach(pthread_self());
    clientNode *cli = clientsList[0];
    while (clientsList[4]->needsIpAddress == 1)
    {
        this_thread::sleep_for(chrono::milliseconds(1));
    }
    this_thread::sleep_for(chrono::milliseconds(100));
    DHCPMessage *message = new DHCPMessage(1, 400, "DHCPREQUEST");
    message->clientMacAddress = cli->macAddress;
    message->clientIpAddr = cli->ipAddress;
    message->opt->clientId = cli->clientId;
    message->opt->resquestedIpAddress = cli->ipAddress;
    cli->needsRenewal = 1;
    cli->sendMessageBuffer.push_back(message);
    pthread_exit(NULL);
}

int main()
{
    serverNode *server1 = new serverNode(1, "server1", "0a:2C:11:23:cf:03", "193.197.21.4");
    serverNode *server2 = new serverNode(2, "server2", "3a:1f:23:18:cc:01", "193.197.21.5");
    serverNode *server3 = new serverNode(3, "server3", "1a:3f:2b:ff:1c:01", "193.197.21.6");
    //client 1 2 and 3 are already exsiting
    // client 4 and 5 are newly inducted in the system
    clientNode *client1 = new clientNode("client1", "1b:43:21:65:77:fa", "193.194.22.7", 24, 47);
    clientNode *client2 = new clientNode("client2", "65:77:fa:1b:43:21", "193.194.25.41", 1224, 2455);
    clientNode *client3 = new clientNode("client3", "1b:77:fa:43:21:65", "193.194.23.42", 144, 302);
    clientNode *client4 = new clientNode("client4", "3a:1f:01:23:18:cc");
    clientNode *client5 = new clientNode("client5", "23:18:4d:22:1d:cc");

    //assume client 1 and client 2 are initially registered in server 1
    // assume client 3 registred in server 2
    //populate for client
    DHCPMessage *t = new DHCPMessage(1, 100, "DHCPDISCOVER");
    hostIp *temp = new hostIp(t);
    temp->clientId = client1->clientId;
    temp->ipAddress = client1->ipAddress;
    temp->leaseTime = client1->leaseTimeProvided;
    server1->hostIpList.push_back(temp);
    temp = new hostIp(t);
    temp->clientId = client2->clientId;
    temp->ipAddress = client2->ipAddress;
    temp->leaseTime = client2->leaseTimeProvided;
    server1->hostIpList.push_back(temp);
    temp = new hostIp(t);
    temp->clientId = client3->clientId;
    temp->ipAddress = client3->ipAddress;
    temp->leaseTime = client3->leaseTimeProvided;
    server2->hostIpList.push_back(temp);
    // temp = new hostIp();
    // server1->hostIpList.push_back(temp);
    // temp = new hostIp();
    // server2->hostIpList.push_back(temp);
    // free(temp);

    DHCPMessage *mess = new DHCPMessage(1, 100, "DHCPDISCOVER");
    mess->clientMacAddress = client4->macAddress;
    mess->opt->clientId = client4->clientId;
    client4->sendMessageBuffer.push_back(mess);

    vector<string> populateIp1, populateIp2, populateIp3;

    populateIp1.push_back("193.197.21.123");
    populateIp1.push_back("193.197.21.124");
    populateIp1.push_back("193.197.56.45");
    populateIp1.push_back("193.197.56.46");
    populateIp1.push_back("193.197.56.47");

    populateIp2.push_back("193.197.22.34");
    populateIp2.push_back("193.197.22.35");
    populateIp2.push_back("193.197.102.124");
    populateIp2.push_back("193.197.102.44");

    populateIp3.push_back("193.197.212.34");
    populateIp3.push_back("193.197.122.35");
    populateIp3.push_back("193.197.12.124");
    populateIp3.push_back("193.197.152.44");

    server1->addressPool = populateIp1;
    server2->addressPool = populateIp2;
    server3->addressPool = populateIp3;

    serversList.push_back(server1);
    serversList.push_back(server2);
    serversList.push_back(server3);

    clientsList.push_back(client1);
    clientsList.push_back(client2);
    clientsList.push_back(client3);
    clientsList.push_back(client4);
    clientsList.push_back(client5);

    vector<pthread_t> serverThreads(serversList.size()), clientThreads(clientsList.size());
    pthread_t renewForclient1;
    pthread_create(&renewForclient1, NULL, waitTill, NULL);

    for (int i = 0; i < serversList.size(); i++)
        pthread_create(&serverThreads[i], NULL, serverFunc, serversList[i]);

    for (int i = 0; i < clientsList.size(); i++)
        pthread_create(&clientThreads[i], NULL, clientFunc, clientsList[i]);

    for (int i = 0; i < serversList.size(); i++)
        pthread_join(serverThreads[i], NULL);

    for (int i = 0; i < clientThreads.size(); i++)
        pthread_join(clientThreads[i], NULL);

    // for (int i = 0; i < 100; i++)
    // {
    //     DHCPMessage *mess = new DHCPMessage(1, 100, "DHCPOffer");
    //     mess->secs = 100;
    //     mess->printDHCPMessage();
    //     mess->opcode = 1;
    //     mess->transcationId = 100;
    // }
}