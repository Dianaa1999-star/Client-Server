#include <cstdlib>
#include <cstdio>
#include <stdio.h>
#include<iostream>
#include<vector>
#include<string>
#include<sstream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
using namespace std;
#define BUFLEN 1552
// analizeaza o conditie si, daca aceasta este adevarata, inchide programul afisand o eroare data
#define DIE(condition, message) \
    do { \
        if ((condition)) { \
            fprintf(stderr, "[%d]: %s\n", __LINE__, (message)); \
            perror(""); \
            exit(1); \
        } \
    } while (0)
#define PRINT(condition, message) \
    if ((condition)) {					\
        fprintf(stderr, "[%d]: %s\n", __LINE__, (message)); \
   		exit(0); \
    }
// retine un client
struct client {
    char id[11];
};
// mesaj primit de la TCP
struct __attribute__((packed)) msg_rec_from_tcp {
    char type[12];
    char topic[51];
    bool SF;
};

// mesaj primit de la UDP
struct __attribute__((packed)) msg_from_udp {
    char topic[50];
    uint8_t type;
    char info[1501];
};

struct __attribute__((packed)) msg_to_tcp {
    char IP[16];
    char topic[51];
    char info[1501];
    char type[11];
    int udpSender;
};
struct topic_struct {
    string name;
    bool SF;
    int lastMsgRecv;
    topic_struct() {
        name = "";
        SF = false;
        lastMsgRecv = 0;
    }
    topic_struct(string n, bool s, int i) {
        name = n;
        SF = s;
        lastMsgRecv = i;
    }
};