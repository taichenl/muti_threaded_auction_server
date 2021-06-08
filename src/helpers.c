#include "server.h"

void print_actionlist(){
    node_t* current = AuctionList.head;
    while (current != NULL){
        struct Auction* temp = (struct Auction*) current->value;
        printf("Auction name: %s\n", temp->action_name);
        printf("Auction price: %d\n", temp->price);
        printf("Auction tick: %d\n", temp->remaining_ticks);
        printf("Auction id: %d\n", temp->id_number);
        current= current->next;
    }
};

int cmpfunc (const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}

void free_everything(){
    sbuf_deinit(job_buffer);
    free(job_buffer);
    node_t* current = User_db.head;
    while (current != NULL){
        struct User* temp = (struct User*) current->value;
        free(temp);
        current = current->next;
    }
    deleteList(&User_db);
    current = AuctionList.head;
    while (current != NULL){
        struct Auction* temp = (struct Auction*) current->value;
        free(temp);
        current = current->next;
    }
    deleteList(&AuctionList);
    if (client_info != NULL) free(client_info);
}

void sigint_handler(int sig)
{
    printf("shutting down server\n");
    free_everything();
    close(listen_fd);
    exit(0);
}

void print_userlist(){
    node_t* current = User_db.head;
    while (current != NULL){
        struct User* temp = (struct User*) current->value;
        printf("username: %s\n", temp->username);
        printf("password: %s\n", temp->password);
        printf("number of auction: %d\n", temp->number_of_auction);

        current= current->next;
    }
};

struct User* find_username(char* username){
    node_t* temp = User_db.head;
    while(temp != NULL){
        struct User * current = (struct User *) temp->value;
        if (strcmp(username, current->username) == 0){
            return current;
        }
        temp = temp->next;
    }
    return NULL;
};

struct User* find_username_byfd(int fd){
    node_t* temp = User_db.head;
    while(temp != NULL){
        struct User * current = (struct User *) temp->value;
        if (current->client_fd == fd){
            return current;
        }
        temp = temp->next;
    }
    return NULL;
};

struct Auction* find_auction_byid(int id){
    node_t* temp = AuctionList.head;
    while(temp != NULL){
        struct Auction * current = (struct Auction *) temp->value;
        if (current->id_number == id){
            return current;
        }
        temp = temp->next;
    }
    return NULL;
};

void process_ticks(){
    node_t* current = AuctionList.head;
    while (current != NULL){
        struct Auction* temp = (struct Auction*) current->value;
        temp->remaining_ticks--;
        if (temp->remaining_ticks == 0){
            struct job* update = malloc(sizeof(struct job));
            update->job_info = malloc(sizeof(petr_header));
            if (temp->current_leader_fd != -1){
                struct User* this_winner = find_username_byfd(temp->current_leader_fd);
                this_winner->won_auction[this_winner->number_of_auction] = temp->id_number;
                this_winner->number_of_auction++;

                int count = temp->number_of_watch;
                int start = 0;
                while (count >0){
                    if (temp->watchlist[start] == -1){
                        ;
                    }
                    else{
                        update->client_fd = temp->watchlist[start];
                        char num[BUFFER_SIZE];
                        sprintf(num, "%d\r\n%s\r\n%d", temp->id_number, this_winner->username, temp->current_bid);
                        update->job_info->msg_len = strlen(num)+1;
                        update->job_info->msg_type = ANCLOSED;
                        wr_msg(temp->watchlist[start], update->job_info, num);
                        count--;
                    }
                    start++;
                }
                
            }
            else{
                int count = temp->number_of_watch;
                int start = 0;
                while (count >0){
                    if (temp->watchlist[start] == -1){
                        ;
                    }
                    else{
                        update->client_fd = temp->watchlist[start];
                        char num[BUFFER_SIZE];
                        sprintf(num, "%d\r\n\r\n", temp->id_number);
                        update->job_info->msg_len = strlen(num)+1;
                        update->job_info->msg_type = ANCLOSED;
                        wr_msg(temp->watchlist[start], update->job_info, num);
                        count--;
                    }
                    start++;
                }
            }
            free(update->job_info);
            free(update);
        }
        current = current->next;
    }
}