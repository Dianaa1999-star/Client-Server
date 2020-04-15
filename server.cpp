#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "generals.h"
#include <cstring>
#include <math.h>
#include <unordered_set>
#include <unordered_map>
#include <iomanip> 
#include <set>
#define INT_MAX 2147483647
#define DIV 100


void sendMessages (client& newClient, unordered_map<string, int>& topicIndexes,
                         int socket, set<string>& online,vector<vector<msg_to_tcp>>& allMessages,
                         unordered_map<string, unordered_map<int, topic_struct>>& topicsToStore) {

        // pentru fiecare topic al clientului
    for (auto& crtTopic : topicsToStore[newClient.id]) {
        // daca avea SF = 1
        if (crtTopic.second.SF == 1) {
            auto& crtBuffer = allMessages[crtTopic.first];

            // trimit toate mesajele de la lastMsgRecv pana in curent
            int i = crtTopic.second.lastMsgRecv;
            while(i < crtBuffer.size()) {
                DIE(send(socket, (char*) &crtBuffer[i], sizeof(msg_to_tcp), 0) < 0 ,"Could not send message to TCP client");
                i++;
            }

            // updatez indicele ultimului mesaj primit
            crtTopic.second.lastMsgRecv = crtBuffer.size();
        }
    }
}

bool decode_my_code(struct msg_to_tcp &tcpMsg,struct msg_from_udp &udpMsg) {

    DIE(udpMsg.type > 3, "Incorrect publisher message type.\n");
    uint32_t integer;
    double real_num;
    float real;
    strncpy(tcpMsg.topic, udpMsg.topic, 50);
    tcpMsg.topic[50] = '\0';
    if (!udpMsg.type || udpMsg.type == 2) {
        DIE((udpMsg.info[0] > 1),"Message has incorrect sign byte\n");
    }
     if (!udpMsg.type) {
        char m[] = "INT";
        memcpy(tcpMsg.type, m, strlen(m) + 1);
        integer = ntohl(*(uint32_t*) (udpMsg.info + 1));

        if (!udpMsg.info[0]) {
            sprintf(tcpMsg.info, "%u", integer);
        } else {
            sprintf(tcpMsg.info, "-%u", integer);
        }
    } else if (udpMsg.type == 1) {
        char m[] = "SHORT_REAL";
        memcpy(tcpMsg.type, m, strlen(m) + 1);
        real = ntohs(*(uint16_t*) (udpMsg.info));
        real /= DIV;
        setprecision(2);
        sprintf(tcpMsg.info, "%.2f", real);
    } else if (udpMsg.type == 2) {
        char m[] = "FLOAT";
        memcpy(tcpMsg.type, m, strlen(m) + 1);
        real_num= ntohl(*(uint32_t*) (udpMsg.info + 1));
        real_num = real_num / pow(10, udpMsg.info[5]);

        if (!udpMsg.info[0]) {
            sprintf(tcpMsg.info, "%lf", real_num);
        } else {
            sprintf(tcpMsg.info, "-%lf",real_num);
        }
    } else {
        char m[] = "STRING";
        memcpy(tcpMsg.type, m, strlen(m) + 1);
        sprintf(tcpMsg.info, "%s", udpMsg.info);
    }
    return true;
}


int main(int argc, char *argv[]) {
    // daca nu am primit argumentele corecte
    DIE(argc != 2, "Usage: ./server <PORT>\n");

    bool rec_exit = false;
    fd_set read_fds, tmp_fds;
    socklen_t tcpLen,udpLen;
    int fav_val = 1;
    sockaddr_in udp_addr, tcp_addr;
    sockaddr_in new_tcp_addr;
    unsigned int crtMaxSize = 100;
    unsigned int nrTopics = 0;

    // setez file descriptorii socketilor creati
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
                   
    int port = atoi(argv[1]);
    char buff[BUFLEN];
     // clientii mei
    vector<client> clients;
    clients.reserve(100);

    // clienti online
    set<string> online;
    // se creeaza socketul UDP
    int udp_socket = socket(PF_INET, SOCK_DGRAM, 0);
    DIE(udp_socket < 0, "Could not create UDP socket.\n");
    FD_SET(udp_socket, &read_fds);

    // se creeaza socketul TCP
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_socket < 0, "Could not create TCP socket.\n");
    FD_SET(tcp_socket, &read_fds);

     // initializari informatii socketi
    udpLen = sizeof(sockaddr);
    tcpLen = sizeof(sockaddr);
    udp_addr.sin_port = htons(port);
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_family = AF_INET;
    

    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = udp_addr.sin_port;


    // retine totalitatea topicelor la care este abonat fiecare client
    unordered_map<string, unordered_map<int, topic_struct>> topicsToStore;

    unordered_map<string, int> topicIndexes;

    // retine totalitatea mesajelor primite de la UDP
    vector<vector<msg_to_tcp>> allMessages; // mesajele bufferate
    allMessages.resize(100);

    // retine pe ce file descriptor ruleaza fiecare client
    unordered_map<string, int> clientSocket;

    int siz = sizeof(sockaddr_in);
    DIE(bind (tcp_socket, (sockaddr *) &tcp_addr, siz) < 0 ,"Could not bind TCP socket\n" ); 
    DIE(bind (udp_socket, (sockaddr *) &udp_addr, siz) < 0, "Could not bind UDP socket\n");
    DIE(listen(tcp_socket, INT_MAX) < 0,  "Cannot listen to TCP socket\n") ;

    
    //Adaugam citirea de la tastatura
    FD_SET(0, &read_fds);
   
    // retine indicele maxim al unui socket
    int maxim = tcp_socket;

    while(!rec_exit) {
          tmp_fds = read_fds;
          int current = 0;
          memset(buff, 0, BUFLEN);
          DIE(select(maxim + 1, &tmp_fds, nullptr, nullptr, nullptr) < 0, "Could not execute select\n");
          while (current <= maxim) {
             if (FD_ISSET(current, &tmp_fds) == 1) {
                // daca se primeste o comanda de la stdin
                // verific ca aceasta sa fie 'exit'
                if (!current) {
                    scanf("%s",buff);
                      if (strncmp(buff, "exit", 4)) {
                        printf("Server only accepts command 'exit'\n");
                    } else {
                        rec_exit = true;
                    }
                } else if(current == tcp_socket) {
                    // daca e client tcp
                    sockaddr_in newTcpAddress;
                    int new_socket = accept(current, (sockaddr*) &new_tcp_addr, &tcpLen);
                    DIE(new_socket < 0, "Cannot accept a new client.\n");

                    // s-a alocat un socket mai mare decat maximul de pana acum
                    maxim = max(new_socket, maxim);
                
                    // opresc algoritmul lui Nagle
                    setsockopt(new_socket, IPPROTO_TCP, TCP_NODELAY, (char*) &fav_val, sizeof(int));
                    FD_SET(new_socket, &read_fds);

                    DIE(recv(new_socket, buff, BUFLEN - 1, 0) < 0, "Did not receive client ID\n");
                    client newClient;
                    sendMessages (newClient, topicIndexes,
                             new_socket, online,allMessages, topicsToStore);
                    memcpy(newClient.id,buff,sizeof(buff));
                    online.insert(newClient.id);
                    unsigned clientsLen = clients.size();
                    if ((unsigned) new_socket > clientsLen) {
                        clients.resize(2 * clientsLen);
                    }
                    
                    clientSocket.insert(make_pair(newClient.id, new_socket));
                    printf("New client %s connected from %s:%d.\n", newClient.id,
                           inet_ntoa(new_tcp_addr.sin_addr), ntohs(new_tcp_addr.sin_port));
                    clients[new_socket] = newClient;
                } else if(current == udp_socket) {
                    DIE(recvfrom(udp_socket, buff, BUFLEN, 0, (sockaddr*) &udp_addr, &udpLen) < 0,"Nothing received from UDP socket.\n");
                    msg_from_udp my_rec_udp = *(msg_from_udp*) buff;
                    msg_to_tcp send_msg;
                    send_msg.udpSender = ntohs(udp_addr.sin_port);
                    int msize = sizeof(inet_ntoa(udp_addr.sin_addr));
                    memcpy(send_msg.IP, inet_ntoa(udp_addr.sin_addr), msize);
                    bool succeded = decode_my_code(send_msg,my_rec_udp);
                    if(succeded){
                        std::unordered_map<std::string,int>::const_iterator topicIt =
                         topicIndexes.find(send_msg.topic);
                        if (topicIt == topicIndexes.end()) {
                            allMessages[nrTopics].emplace_back(send_msg);
                            topicIndexes.insert(make_pair(send_msg.topic, nrTopics));
                            nrTopics++;
                            crtMaxSize = max(crtMaxSize,nrTopics);
                         if (nrTopics == crtMaxSize) {
                                allMessages.resize(crtMaxSize * 2);
                                crtMaxSize = crtMaxSize * 2;
                            }
                            allMessages[nrTopics - 1].emplace_back(send_msg);
                        } else {
                             allMessages[topicIt->second].emplace_back(send_msg);
                              for (auto& crtClient : online) {
                                auto find_topic = topicsToStore.find(crtClient);
                                if ( find_topic != topicsToStore.end()) {
                                    int topicPos = topicIndexes.find(send_msg.topic)->second;

                                    if (topicsToStore.find(crtClient)->second.find(topicPos) !=
                                        topicsToStore.find(crtClient)->second.end()) {
                                        // actualizez indexul ultimului mesaj trimis
                                        int sizeAll = allMessages[topicIt->second].size();
                                        topicsToStore.find(crtClient)->second.find(topicPos)->second.lastMsgRecv = sizeAll;
                                        int fd ;
                                        fd = clientSocket.find(crtClient)->second;
                                        DIE(send(fd, (char *) &send_msg, sizeof(msg_to_tcp), 0) < 0, "Could not send message to TCP client\n");
                                    }
                                }
                            }
                        }
                    }
                } else {
                    int recv1;
                    recv1 = recv(current, buff, BUFLEN - 1, 0);
                    DIE(recv1 < 0 ,"Did not receive anything from the TCP client\n");
                    // daca a clientul s-a deconectat
                    if (!recv1) {
                        // resetez file descriptorul si actualizez maxFd
                        FD_CLR(current, &read_fds);
                        for (int i = maxim ; i >= 3; i--) {
                            if (FD_ISSET(maxim, &read_fds)) {
                                break;
                            }
                        }

                        // il deconectez
                        auto currentiID = clients[current].id;
                        // elimin clientul din set-ul de clienti online
                        online.erase(currentiID);
                        printf("Client %s disconnected\n", clients[current].id);
                        close(current);
                    } else {
                        msg_rec_from_tcp fromTcp = *(msg_rec_from_tcp*) buff;
                        char* topicName = (char*)(calloc((strlen(fromTcp.topic) + 1) , sizeof(char)));
                        strcpy(topicName,fromTcp.topic);
                        // fac subscribe/unsubscribe la topic
                        auto it = topicIndexes.find(topicName);
                        if (it == topicIndexes.end()) {
                            printf("Requested topic does not exist\n");
                        } else if (strstr(fromTcp.type, "unsubscribe")) {
                             auto currentiID = clients[current].id;
                             int topicPos = topicIndexes.find(fromTcp.topic)->second;
                             topicsToStore[currentiID].erase(topicsToStore[clients[current].id].find(topicPos));
                        } else {
                            // adaug topicul in map-ul clientului
                            int allsize =  allMessages[topicIndexes.find(fromTcp.topic)->second].size();
                            auto currId = clients[current].id;
                            topic_struct newStruct(fromTcp.topic, fromTcp.SF,allsize);
                            topicsToStore[currId].insert(make_pair(topicIndexes.find(fromTcp.topic)->second, newStruct));
                        }
                    }
                }
            }
            current++;   
            }

        }
    int current = 2;
    while(current <= maxim){
        if (FD_ISSET(current, &read_fds)) {
            close(current);
        }
        current++;
    }
    return 0;
}