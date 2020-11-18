#include <bits/stdc++.h>
#include <thread>
#include <unistd.h>

using namespace std;

class node;

int nodeId = 1, switchId = 1;

class ARP
{
public:
    string senderIp, senderMac, destIP, destMac, operationCode;
    ARP(string senderIp, string senderMac, string destIP, string destMac, string operationCode)
    {
        this->senderIp = senderIp;
        this->senderMac = senderMac;
        this->destIP = destIP;
        this->destMac = destMac;
        this->operationCode = operationCode;
    }

    void printArp()
    {
        cout << "\t\t" << operationCode << endl;
        cout << "\t\t-------------------------------------------------------------------------------------------------------------------------------------------------\n";
        cout << "\t\t|\tSource Ip address\t|\tSource Mac Address\t|\tDest IP Address\t|\tDest Mac Address\t|\tOperation Code\t|\n";
        cout << "\t\t-------------------------------------------------------------------------------------------------------------------------------------------------\n";
        cout << "\t\t|\t" << senderIp << "      \t|\t" << senderMac << "\t|\t" << destIP << "\t|\t" << destMac << "\t|\t" << operationCode << "\t|" << endl;
        cout << "\t\t-------------------------------------------------------------------------------------------------------------------------------------------------\n";
    }
};

class Switch
{
public:
    int id;
    string switch_name;
    vector<node *> neighbours;

    Switch(string name)
    {
        this->id = switchId;
        switchId++;
        switch_name = name;
    }
    vector<ARP> redirect;
};

class node
{
public:
    int id;
    string mac, ip;

    int numberOfArpRequests;
    vector<string> requests;

    vector<ARP> responses;

    Switch *server;

    unordered_map<string, string> arp_cache;

    node(string mac, string ip, Switch *server)
    {
        this->id = nodeId;
        nodeId++;
        this->mac = mac;
        this->ip = ip;
        this->numberOfArpRequests = 0;
        this->server = server;
        server->neighbours.push_back(this);
    }

    int storeRequest(string senderIpAddress)
    {
        numberOfArpRequests++;
        requests.push_back(senderIpAddress);
    }

    void printCache()
    {
        cout << "\t\tARP CACHE OF client" << id << endl;
        cout << "\t\t-------------------------------------------------\n";
        cout << "\t\t|\tIp address\t|\tMac Address\t|\n";
        cout << "\t\t-------------------------------------------------\n";
        for (auto i = arp_cache.begin(); i != arp_cache.end(); i++)
        {
            cout << "\t\t|\t" << (*i).first << "\t|   " << (*i).second << "   |" << endl;
        }
        cout << "\t\t-------------------------------------------------\n";
    }
};

unordered_map<int, node *> idToNode;
unordered_map<int, Switch *> idToSwitch;

void *switchFunc(void *arg)
{
    pthread_detach(pthread_self());

    int *id = (int *)arg;
    Switch *server = idToSwitch[*id];

    while (1)
    {
        if (server->redirect.size() > 0)
        {
            ARP back = server->redirect.back();
            server->redirect.pop_back();

            cout << "\n* ARP packet arrives at Switch *\n";
            back.printArp();

            if (back.destMac == "ff:ff:ff:ff:ff:ff")
            {
                cout << "\nSwitch: As the destination mac address is ff:ff:ff:ff:ff:ff, so it is broadcast ARP_REQUEST\n";
                cout << "\nSwitch:  Broadcasting the ARP PAcket from switch\n\n";
                for (int i = 0; i < server->neighbours.size(); i++)
                {
                    if (server->neighbours[i]->ip != back.senderIp)
                        server->neighbours[i]->responses.push_back(back);
                }
            }

            else
            {
                cout << "\nSwitch:  As the destination mac address is " << back.destMac << ", so it is Unicast ARP_REPLY\n";
                cout << "\nSwitch:  SENDING the ARP_REPLY to client with mac = " << back.destMac << "\n\n";
                for (int i = 0; i < server->neighbours.size(); i++)
                {
                    if (server->neighbours[i]->mac == back.destMac)
                        server->neighbours[i]->responses.push_back(back);
                }
            }
        }
    }

    pthread_exit(NULL);
}

void *nodeFunc(void *arg)
{
    pthread_detach(pthread_self());
    int *id = (int *)arg;
    node *client = idToNode[*id];

    while (1)
    {

        if (client->requests.size() > 0)
        {
            string senderIpAddress = client->requests.back();
            client->requests.pop_back();

            cout << endl
                 << "** Client " << client->id << " request mac Address for the given " + senderIpAddress + " Ip address ***\n\n";
            cout << "Present: \n";
            client->printCache();

            if (client->arp_cache.find(senderIpAddress) != client->arp_cache.end())
            {
                cout << endl;
                cout << "MAC Address for the sender Ip Address ( " << senderIpAddress << " ) is availble in the cache of Client " << client->id << ". So no need to send Arp request\n";
            }
            else
            {
                cout << endl;
                cout << "MAC Address for the sender Ip Address ( " << senderIpAddress << " ) is not availble in the cache of Client " << client->id << ". So there's need to send Arp request\n";

                //  senderIp, senderMac,  destIP, destMac,  operationCode
                ARP req(client->ip, client->mac, senderIpAddress, "ff:ff:ff:ff:ff:ff", "ARP_REQUEST");
                req.printArp();

                client->server->redirect.push_back(req);
            }
        }

        if (client->responses.size() > 0)
        {

            ARP back = client->responses.back();
            client->responses.pop_back();

            cout << endl
                 << "** Client " << client->id << " receives ARP packet from switch ***\n\n";
            back.printArp();

            client->arp_cache[back.senderIp] = back.senderMac;

            cout << "Updated Arp cache of client " << client->id << endl;
            client->printCache();

            if (back.operationCode == "ARP_RESPONSE")
            {
                cout << "Client " << client->id << " get back the mac address ( " << back.senderMac << " ) for the IP address " << back.senderIp << " ) ";
                cout << endl
                     << endl;
            }

            else
            {
                cout << endl;

                if (client->ip != back.destIP)
                {
                    cout << "Client " << client->id << " DROPS THE PACKET AS THE destination IP address ( " << back.destIP << " ) doesn't match with the current client IP Address ( " << client->ip << " ) \n\n";
                }

                else
                {
                    cout << "Client " << client->id << " IP address ( " << client->ip << " ) matches with the ARP request destination IP Address ( " << back.destIP << " ) \n\n";
                    cout << "Client " << client->id << ": Sending back ARP response to the source IP address of the ARP Request  \n\n";
                    ARP res(client->ip, client->mac, back.senderIp, back.senderMac, "ARP_RESPONSE");
                    res.printArp();
                    client->server->redirect.push_back(res);
                }
            }
        }
    }

    pthread_exit(NULL);
}

int main()
{

    Switch *server = new Switch("Main Server");

    node *client1 = new node("ef:ds:fd:ef:ds:fd", "23:23:23:32", server);
    node *client2 = new node("ed:ds:wd:re:dd:gd", "23:53:33:23", server);
    node *client3 = new node("ed:sd:w3:r0:2d:1d", "22:23:23:32", server);

    client1->storeRequest("23:53:33:23");

    // client1->arp_cache["ef:df:sd:ds"] =  "23:23:32:23:24:43";

    pthread_t ptid1, ptid2, ptid3, ptid4;

    int id = server->id;
    pthread_create(&ptid1, NULL, switchFunc, &id);
    idToSwitch[id] = server;

    int id1 = client1->id;
    pthread_create(&ptid2, NULL, nodeFunc, &id1);
    idToNode[id1] = client1;

    int id2 = client2->id;
    pthread_create(&ptid3, NULL, nodeFunc, &id2);
    idToNode[id2] = client2;

    int id3 = client3->id;
    pthread_create(&ptid4, NULL, nodeFunc, &id3);
    idToNode[id3] = client3;

    pthread_join(ptid1, NULL);
    pthread_join(ptid2, NULL);
    pthread_join(ptid3, NULL);
    pthread_join(ptid4, NULL);
    while (1)
        ;
    return 0;
}