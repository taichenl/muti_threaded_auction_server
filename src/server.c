#include "server.h"

// the following are thread functions

// the tick thread counts down the auction each N seconds
// n is taken from the user. By default, it's trigger by enter
// non determining threads
void * tick_thread(){
    pthread_detach(pthread_self());
    while(1){
        if (time_interval == -1){
            while( getchar() != '\n' );
        }
        else{
            sleep(time_interval);
        }
        start_writing_table();
        start_reading_auction();
        process_ticks();
        finish_read_auction();
        finish_writing_table();
    }
    return NULL;
};

// clients thread are created one per person
// terminated by a LOGOUT signal
// short lived thread
void* client_thread(void* clientfd_ptr){
    pthread_detach(pthread_self());
    int client_fd = *(int *)clientfd_ptr;
    free(clientfd_ptr);
    int received_size;
    petr_header* message = malloc(sizeof(petr_header));
    rd_msgheader(client_fd, message);
    received_size = read(client_fd, buffer, message->msg_len);
    char* username = malloc(sizeof(char)*50);
    char* password = malloc(sizeof(char)*50);
    strcpy(username, strtok(buffer, "\n"));
    username[strcspn(username, "\r")] = 0;
    char* ok = strtok(NULL, "\n");
    strcpy(password, ok);
    start_writing_table();
    struct User* here = find_username(username);
    if (here != NULL){
        if (here->active == 1){
            message->msg_type = EUSRLGDIN;
            message->msg_len = 0;
            wr_msg(client_fd, message, "");
            free(username);
            free(password);
            free(message);
            finish_writing_table();
            return NULL;
        }
        else{
            if (strcmp(here->password, password) == 0){
                here->active = 1;
            }
            else{
                message->msg_type = EWRNGPWD;
                message->msg_len = 0;
                wr_msg(client_fd, message, "");
                free(username);
                free(password);
                free(message);
                finish_writing_table();
                return NULL;
            }
        }
    }
    else{
        struct User* user = malloc(sizeof(struct User));
        user->client_fd = client_fd;
        user->number_of_auction = 0;
        user->active = 1;
        strcpy(user->username, username);
        strcpy(user->password, password);
        insertRear(&User_db, (void*) user);
    }
    finish_writing_table();
    message->msg_type = OK;
    message->msg_len = 0;
    wr_msg(client_fd, message, "");
    free(message);
    while(1){
        bzero(buffer, BUFFER_SIZE);
        petr_header* current = malloc(sizeof(petr_header));
        rd_msgheader(client_fd, current);
        received_size = read(client_fd, buffer, current->msg_len);
        if (current->msg_type == LOGOUT){
            current->msg_len = 0;
            current->msg_type = OK;
            wr_msg(client_fd, current, "");
            start_reading_usertable();
            struct User* user = find_username(username);
            finish_reading_usertable();
            user->active = 0;
            free(current);
            break;
        }
        struct job* thisjob = malloc(sizeof(struct job));
        thisjob->client_fd = client_fd;
        thisjob->job_info = current;
        strcpy(thisjob->message, buffer);
        sbuf_insert(job_buffer, thisjob);
    }
    free(username);
    free(password);
    return NULL;
}

// job thread starts as the server starts
// perform a producer and consumer model
// long lived thread
void* job_thread(){
    pthread_detach(pthread_self());
    while(1){ 
        struct job* temp = sbuf_remove(job_buffer);
        if (temp->job_info->msg_type == USRLIST){
            process_USRLIST(temp);
        }
        if (temp->job_info->msg_type ==ANLIST){
            process_ANLIST(temp);
        }
        if (temp->job_info->msg_type == ANCREATE){
            process_ANCREATE(temp);
        }
        if (temp->job_info->msg_type == ANWATCH){
            process_ANWATCH(temp);
        }
        if (temp->job_info->msg_type == ANLEAVE){
            process_ANLEAVE(temp);
        }
        if (temp->job_info->msg_type == ANBID){
            process_ANBID(temp);
        }
        if (temp->job_info->msg_type == USRWINS){
            process_USRWINS(temp);
        }
        if (temp->job_info->msg_type == USRSALES){
            process_USRSALES(temp);
        }
        if (temp->job_info->msg_type == USRBLNC){
            process_USRBLNC(temp);
        }
        free(temp->job_info);
        free(temp);
        finish_reading_usertable();
        finish_read_auction();
    }
    return NULL;
}

// initilize the server to be connected
int server_init(int server_port){
    int sockfd;
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        exit(EXIT_FAILURE);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(server_port);
    int opt = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt))<0)
    {
	perror("setsockopt");exit(EXIT_FAILURE);
    }
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed\n");
        exit(EXIT_FAILURE);
    }
    if ((listen(sockfd, 1)) != 0) {
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

// run the server
void run_server(int server_port){
    listen_fd = server_init(server_port);
    printf("Currently listening on port %d\n", server_port);
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_create(&tick_tid, NULL, tick_thread, NULL);
    pthread_create(&tick_tid, NULL, job_thread, NULL);

    pthread_t client;
    while(1){
        client_info = malloc(sizeof(int));
        *client_info = accept(listen_fd, (SA*)&client_addr, &client_addr_len);
        if (*client_info < 0) {
            exit(EXIT_FAILURE);
        }
        else{
            pthread_create(&client, NULL, client_thread, (void *)client_info); 
        }
    }
    free_everything();
}


int main(int argc, char* argv[])
{
    //initilize all the global variables
    int opt;
    signal(SIGINT, sigint_handler);
    unsigned int port = 0;
    User_db.length = 0;
    User_db.head = NULL;
    User_db.comparator = NULL;
    auctionID = 1; 
    AuctionList.head = NULL; 
    AuctionList.length = 0;
    int listen_fd;
    reader_number = 0;
    time_interval = -1;
    auction_reader = 0;
    int job_buffer_size =2;

    //read auction from input file
    while ((opt = getopt(argc, argv, "h:t:j:")) >= 0) {
        switch (opt) {
        case 'j':
            printf("this is J. Meassge is: %s\n", optarg);
            job_buffer_size = atoi(optarg);
            break;
        case 'h':
            printf("this is help menu.\n");
            return EXIT_SUCCESS;
        case 't':
            time_interval = atoi(optarg);
            printf("this is time interval %d\n", time_interval);
            break;
        default: /* '?' */
            fprintf(stderr, "Server Application Usage: %s -h/j/t <port_number>\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }


    //initilize job buffer by sbuf function
    job_buffer = malloc(sizeof(sbuf_t));
    sbuf_init(job_buffer, job_buffer_size);
    

    //construct the port value
    port = atoi(argv[argc-2]);
    FILE * fp = fopen(argv[argc-1], "r");
    int read;
    size_t len =0;
    char* line = NULL;
    int count = 0;
    struct Auction* read_auction;
    while ((read = getline(&line, &len, fp)) != -1){
        if (count%4 == 0){
            line[strcspn(line, "\n")] = 0;
            line[strcspn(line, "\r")] = 0;
            read_auction = malloc(sizeof(struct Auction));
            strcpy(read_auction->action_name, line);
        }
        else if (count%4 == 1){
            read_auction->remaining_ticks = atoi(line);
        }
        else if (count%4 == 2){
            read_auction->price = atoi(line);
        }
        else if (count%4 == 3){
            read_auction->id_number = auctionID;
            auctionID++;
            read_auction->number_of_watch = 0;
            read_auction->current_bid = 0;
            read_auction->current_leader_fd = -1;
            insertRear(&AuctionList, (void*) read_auction);
        }
        count++;
    }

    // clean up after finishing. Although it will not end
    free(line);
    fclose(fp);  
    run_server(port);
    free_everything();
    return 0;
}
