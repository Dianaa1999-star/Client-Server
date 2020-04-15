#include <bits/stdc++.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <netinet/tcp.h>
#include "generals.h"
#define MAXSIZE (len + 1)

using namespace std;
#define STDIN 0

void wrong_use()
{
  fprintf(stderr, "Usage: server_address server_port\n");
}
void printcommand(msg_rec_from_tcp msg) {
    if (!strcmp(msg.type, "subscribe ")) {
           printf("subscribed %s\n", msg.topic);
    } else {
          printf("unsubscribed %s\n", msg.topic);
    }
}
void printmsg(msg_to_tcp rec_msg) {
   printf("%s:%d - %s - %s - %s\n", rec_msg.IP, rec_msg.udpSender,
                    rec_msg.topic, rec_msg.type, rec_msg.info);
}
void check_subscribe(msg_rec_from_tcp msg, bool &correct) {
  char *word;
  if (!strcmp(msg.type, "subscribe ")) {
      word = strtok(nullptr, " ");    
      if (word == nullptr) {
          wrong_use();
          correct = false;
      } else if (word[0] != '0') {
          if(word[0] != '1') {
          fprintf(stderr, "SF is not 0 or 1\n");
          correct = false;
        }
      } else {
          msg.SF = word[0] - '0';
      }
  }

}
int verify(char *buff) {
  if (strstr(buff, "exit") != nullptr) {
            return 1;
  }
    return 0;
}
int main(int argc, char *argv[]) {

   int sockfd , ret, len, id_len;
   char* id_client;
   struct sockaddr_in serv_addr;
   len = sizeof(msg_to_tcp);
   // verific ca rulez corespunzator
   DIE(argc != 4, "Usage: ./subscriber <ID> <IP_SERVER> <PORT_SERVER>.\n");

   // iau un buffer
   char *buff = new char[MAXSIZE];
   // verific sa fie un ID bun
   DIE(strlen(argv[1]) > 10, "Subscriber ID should be at most 10 characters long.\n");
   id_len = strlen(argv[1]);

   // copiez in id_client id-ul clientului meu
   id_client = (char*)(calloc((id_len + 1) , sizeof(char)));
   memcpy(id_client, argv[1], id_len + 1);


  // se creeaza socketul dedicat conexiunii la server
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "Unable to open server socket.\n");

  // populez campurile corespunzator
  serv_addr.sin_family = AF_INET;
  ret = inet_aton(argv[2], &serv_addr.sin_addr);
  serv_addr.sin_port = htons(atoi(argv[3]));
  DIE(ret == 0, "Incorrect <IP_SERVER>. Conversion failed.\n");

  // ma conectez la server
  ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
  DIE(ret < 0, "Unable to connect to server.\n");

  //trimit serverului id ul clientului
  int n = send(sockfd, id_client, strlen(id_client)+1, 0);
  DIE(n < 0,  "Could not send <ID> to the server\n");
  
  // opresc algoritmul lui Nagle
  int optval = 1;
  setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &optval, sizeof(int));
  fd_set read_fds;  // multimea de citire folosita in select()
  fd_set tmp_fds;   // multime folosita temporar

  // initializeaza cu 0
  FD_ZERO(&read_fds);
  FD_ZERO(&tmp_fds);

  //Adaugam socketul serverului
  FD_SET(sockfd, &read_fds);

  //Adaugam citirea de la tastatura
  FD_SET(0, &read_fds);

  while(1) {
    tmp_fds = read_fds;
    // golim bufferul
    memset(buff, 0, MAXSIZE);
    ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
    DIE(ret < 0, "Select\n");

    // citesc de la tastatura 
    if (FD_ISSET(STDIN, &tmp_fds)) {
        fgets(buff, MAXSIZE- 1, stdin);
        msg_rec_from_tcp msg;
        bool correct = true;
        int len_buf = strlen(buff);
        buff[len_buf - 1] = '\0';
           int my_exit = verify(buff);
           if (my_exit) {
                break;
            }
        char *word = strtok(buff, " ");
            if(!word) {
                wrong_use();
                correct = false;
            } else if (!strcmp(word, "subscribe")) {
                  char m[] = "subscribe ";
                  memcpy(msg.type, m, strlen(m) + 1);
             } else if (!strcmp(word,"unsubscribe")){
                  char m1[] = "unsubscribe ";
                  memcpy(msg.type, m1, strlen(m1) + 1);
             } else {
                fprintf(stderr, "Invalid command\n");
                correct = false;
            }

            if(correct == true) {
              int word_len;
              word = strtok(nullptr, " ");
              word_len = strlen(word); 
              if (!word || word_len > 50) {
                 correct = false;
                 printf("Over 50 characters\n");
              } else {
                    memcpy(msg.topic , word , strlen(word) + 1);
                    check_subscribe(msg,correct);
              }
            }
      int msg_len = sizeof(msg);
      if(correct) {
        DIE(send(sockfd, (char*) &msg, msg_len, 0) < 0, "Unable to send message to server.\n");
        printcommand(msg);
      }
   }
    if (FD_ISSET(sockfd, &tmp_fds) == 1) {
            int sz = sizeof(msg_to_tcp);
            auto bytesReceived = recv(sockfd, buff, sz, 0);
            DIE(bytesReceived < 0,"Could not receive message from server\n");
            if (!bytesReceived) {
                break;
            }
            // altfel afisez mesajul primit
            msg_to_tcp rec_msg = *(msg_to_tcp*) buff;
            printmsg(rec_msg);
        }
      }

return 0;
}