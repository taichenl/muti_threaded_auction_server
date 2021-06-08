#ifndef SERVER_H
#define SERVER_H

#include "server.h"
#include "protocol.h"
#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "linkedList.h"
#include <semaphore.h>
#include <csapp.h>
#include <string.h>
#define BUFFER_SIZE 1024
#define SA struct sockaddr
//define all the data structures here

//each user is stored with its name, password, and client_fd
//also a list of won auctions to store
struct User{
    char username[256];
    char password[BUFFER_SIZE];
    int client_fd;
    int won_auction[BUFFER_SIZE];
    int number_of_auction; 
    int active;
};

//each auction consists of its id number, name, and other values
//all these values are considered useful for job thread to process
struct Auction{
    int id_number;
    char action_name[256];
    int creater_fd;
    int price;
    int current_leader_fd;
    int current_bid;
    int remaining_ticks;
    int watchlist[256];
    int number_of_watch;
};

//data type that is to be inserted into the buffer
struct job{
    petr_header* job_info;
    int client_fd;
    char message[256];
};

//this is the sbuf I learned from the lecture
typedef struct {
    struct job **buf; 
    int n; 
    int front; 
    int rear; 
    sem_t mutex; 
    sem_t slots; 
    sem_t items; 
} sbuf_t;

//shared variables
List_t User_db; 
int auctionID; 
List_t AuctionList; 
int listen_fd;
char buffer[BUFFER_SIZE];
int* client_info;
pthread_t tick_tid;
int time_interval;
sbuf_t *job_buffer;

//mutex and semifors
pthread_mutex_t action_mutex;
pthread_mutex_t write_user;
pthread_mutex_t read_user;
pthread_mutex_t read_auction;
pthread_mutex_t write_auction;
pthread_mutex_t auction_number_lock;
int reader_number; 
int auction_reader;

//helper functions
void print_actionlist();
void free_everything();
void sigint_handler(int sig);
void print_userlist();
int cmpfunc (const void * a, const void * b);
struct User* find_username(char* username);
struct User* find_username_byfd(int fd);
struct Auction* find_auction_byid(int id);
void process_ticks();

//mutex functions
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, struct job*);
struct job* sbuf_remove(sbuf_t *sp);
void start_reading_usertable();
void finish_reading_usertable();
void start_writing_table();
void finish_writing_table();
void start_reading_auction();
void finish_read_auction();
void start_writing_auction();
void finish_writing_auction();

//process signals functions
void process_USRLIST(struct job* temp);
void process_ANLIST(struct job* temp);
void process_ANCREATE(struct job* temp);
void process_ANWATCH(struct job* temp);
void process_ANLEAVE(struct job* temp);
void process_ANBID(struct job* temp);
void process_USRWINS(struct job* temp);
void process_USRSALES(struct job* temp);
void process_USRBLNC(struct job* temp);

//server function
int server_init(int server_port);
void run_server(int server_port);
void run_client(char *server_addr_str, int server_port);

#endif
