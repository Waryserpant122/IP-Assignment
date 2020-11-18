//TODO
// 1. Make / add print of dhcp message format
// 2. figure of sending of messages to clients while broadcast
// 3. add feature of lease extension
// 4. add feature of Ip address decline
// 5. add feature of server nack

#include <bits/stdc++.h>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <string>
using namespace std;

int sId = 1, cId = 1;

struct hostIp
{
   int leaseTime, clientId;
   string ipAddress;
};
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
   bool dhcpOffer;
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
   usleep(10);
   serverNode *server = (serverNode *)args;
   DHCPMessage *recvMess, *message1, *message2, *ackMessage;
   while (1)
   {
      if (server->recieveMessageBuffer.size() > 0)
      {
         recvMess = server->recieveMessageBuffer.front();
         server->recieveMessageBuffer.pop_front();
         if (recvMess->opt->messageType == "DHCPOFFER")
            continue;
         string req = recvMess->opt->messageType;

         if (req == "DHCPDISCOVER")
         {
            if (server->serverId == 2)
               usleep(10);
            cout << "\nAt server " << server->serverId << " recieved a DHCP " << req << " request\n\n";
            this_thread::sleep_for(chrono::milliseconds(2));
            string toAssignIp = server->addressPool.front();
            DHCPMessage *replyOffer = recvMess;
            replyOffer->opcode = 2;
            replyOffer->yourIpAddr = toAssignIp;
            replyOffer->serverIpAddr = server->ipAddress;
            replyOffer->opt->messageType = "DHCPOFFER";
            replyOffer->opt->resquestedIpAddress = toAssignIp;
            replyOffer->opt->serverId = server->serverId;
            replyOffer->opt->ipLeaseTime = 72;

            //print dhcp message
            cout << "DHCPOFFER sent by " << server->nodeName << endl;
            if (server->serverId == 1)
               clientsList[recvMess->opt->clientId - 1]->dhcpOffer = 1;
            for (int i = 0; i < clientsList.size(); i++)
               clientsList[i]->recieveMessageBuffer.push_back(replyOffer);
            while (server->recieveMessageBuffer.size() == 0)
               ;
            if (server->recieveMessageBuffer.size() > 0)
            {
               message1 = server->recieveMessageBuffer.front();
               server->recieveMessageBuffer.pop_front();

               if (message1->opt->messageType == "DHCPREQUEST" && message1->opt->serverId == server->serverId)
               {
                  cout << "DHCP Request given to " << server->nodeName;
                  cout << " by Client " << message1->opt->clientId;
                  cout << "\nSending ACK to the client\n";

                  ackMessage = message1;
                  ackMessage->opcode = 2;
                  ackMessage->opt->messageType = "DHCPACK";
                  ackMessage->clientIpAddr = toAssignIp;
                  ackMessage->yourIpAddr = toAssignIp;

                  for (int i = 0; i < clientsList.size(); i++)
                     clientsList[i]->recieveMessageBuffer.push_back(ackMessage);
                  usleep(2000);
                  //Grant by ack
               }
               else if (message1->opt->messageType == "DHCPDECLINE")
               {
                  // do the work of sending again
               }
            }
         }
      }
   }
   pthread_exit(NULL);
}
void *clientFunc(void *args)
{
   pthread_detach(pthread_self());
   this_thread::sleep_for(chrono::milliseconds(100));
   clientNode *client = (clientNode *)args;
   while (1)
   {
      while (client->recieveMessageBuffer.size() > 0)
      {
         // DHCPMessage *mess = client->recieveMessageBuffer.front();
         // if (mess->opcode == 1)
         client->recieveMessageBuffer.pop_front();
         // if (mess->opt->clientId != client->clientId)
         //    client->recieveMessageBuffer.pop_front();
      }
      if (client->sendMessageBuffer.size() > 0)
      {
         DHCPMessage *sendMess = client->sendMessageBuffer.front(), *message1, *message2;
         client->sendMessageBuffer.pop_front();
         if (sendMess->opt->messageType == "DHCPDISCOVER")
         {

            cout << "Broadcasted a DHCPDISCOVER messages to all members in the network\n";

            for (int i = 0; i < serversList.size(); i++)
               serversList[i]->recieveMessageBuffer.push_back(sendMess);
            // for (int i = 0; i < clientsList.size(); i++)
            // {
            //    if (clientsList[i]->clientId != client->clientId)
            //       clientsList[i]->recieveMessageBuffer.push_back(sendMess);
            // }
            usleep(100);
            //printDHCPMessage()
            // TODO
            // 1. Print Message function
            // 2. write server DHCP get code and write client code
            // to get reply from server and then send dhcp request to server
            //if(client)
            while (client->recieveMessageBuffer.size() == 0)
               ;
            while (client->recieveMessageBuffer.size() > 0)
            {
               message1 = client->recieveMessageBuffer.front();
               client->recieveMessageBuffer.pop_front();
               if (message1->opt->clientId == client->clientId)
               {
                  if (message1->opt->messageType == "DHCPOFFER" && client->dhcpOffer == 1)
                  {
                     client->dhcpOffer = 0;
                     string recvIp = message1->opt->resquestedIpAddress;
                     cout << "DHCPOFFER recieved from " << message1->opt->serverId;
                     cout << " With Ip Address " << recvIp;
                     cout << "\n Lease time as " << message1->opt->ipLeaseTime << " hours \n";

                     //Decline all other offers from servers.
                     while (client->recieveMessageBuffer.size() > 0)
                     {
                        DHCPMessage *temp = client->recieveMessageBuffer.front();
                        client->recieveMessageBuffer.pop_front();
                        cout << "DHCP offer recieved from " << temp->opt->serverId << " has to be discarded\n";
                     }
                     // usleep(200);
                     // usleep(20);
                     //Check wheteher Ip is valid or not (ARP used)
                     bool isValidAddress = 1;
                     for (int i = 0; i < clientsList.size(); i++)
                     {
                        if (clientsList[i]->clientId != client->clientId)
                        {
                           if (clientsList[i]->ipAddress == recvIp)
                           {
                              isValidAddress = 0;
                              break;
                           }
                        }
                     }
                     if (isValidAddress)
                     {
                        //Sending DHCP Request
                        DHCPMessage *sendRequest = message1;
                        sendRequest->opcode = 1;
                        sendRequest->opt->messageType = "DHCPREQUEST";
                        // print DHCP message
                        cout << " \n DHCP MESSAGE OF REQUEST sent by " << client->nodeName << endl;
                        for (int i = 0; i < serversList.size(); i++)
                           serversList[i]->recieveMessageBuffer.push_back(sendRequest);
                        // for (int i = 0; i < clientsList.size(); i++)
                        // {
                        //    if (clientsList[i]->clientId != client->clientId)
                        //       clientsList[i]->recieveMessageBuffer.push_back(sendRequest);
                        // }
                        while (client->recieveMessageBuffer.size() == 0)
                           ;
                        while (client->recieveMessageBuffer.size() > 0)
                        {
                           message1 = client->recieveMessageBuffer.front();
                           client->recieveMessageBuffer.pop_front();
                           if (message1->opt->messageType == "DHCPACK")
                           {
                              cout << "\nACK RECIEVED\n";
                              cout << "Client " << client->clientId << " has been assigned the IP ";
                              cout << message1->opt->resquestedIpAddress << " for the time of ";
                              cout << message1->opt->ipLeaseTime << " hours\n\n";
                              client->ipAddress = message1->opt->resquestedIpAddress;
                              client->leaseTimeLeft = message1->opt->ipLeaseTime;
                              client->needsIpAddress = 0;
                           }
                           else
                           {
                              int i = 1;
                              //do for server NACK
                           }
                        }
                     }
                     else
                     {
                        int i = 1;
                        //TODO send message to server no accepted the Address
                     }
                  }
               }
               else
               {
                  int i = 1;
                  usleep(100);
               }
            }
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

   vector<string> populateIp1, populateIp2;

   populateIp1.push_back("193.197.21.123");
   populateIp1.push_back("193.197.21.124");
   populateIp1.push_back("193.197.56.45");
   populateIp1.push_back("193.197.56.46");
   populateIp1.push_back("193.197.56.47");

   populateIp2.push_back("193.197.22.34");
   populateIp2.push_back("193.197.22.35");
   populateIp2.push_back("193.197.102.124");
   populateIp2.push_back("193.197.102.44");

   server1->addressPool = populateIp1;
   server2->addressPool = populateIp2;

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