#include "server.h"

void process_USRLIST(struct job* temp){
    start_reading_usertable();
    node_t* current = User_db.head;
    char result[] = "";
    while (current != NULL){
        struct User* here = (struct User*) current->value;
        if (here->active == 1 && here->client_fd != temp->client_fd){
            if (current == User_db.head){
                strcpy(result, here->username);
            }
            else{
                strcat(result, "\n");
                strcat(result, here->username);
            }
        }
        current = current->next;
    }
    strcat(result, "\n\x00");
    temp->job_info->msg_len = strlen(result)+1;
    temp->job_info->msg_type = USRLIST;
    wr_msg(temp->client_fd, temp->job_info, result);
    finish_reading_usertable();
}

void process_ANLIST(struct job* temp){
    start_reading_auction();
    node_t* current = AuctionList.head;
    char result[] = "";
    char num[BUFFER_SIZE];
    char num1[BUFFER_SIZE];
    int count = 0;
    while (current != NULL){
        struct Auction* here = (struct Auction*) current->value;
        if (here->remaining_ticks>0){
            count++;
            if (current == AuctionList.head){
                sprintf(num, "%d;%s;%d;%d;%d;%d", here->id_number, here->action_name, here->price, here->number_of_watch, here->current_bid, here->remaining_ticks);
                strcat(result, num);
            }
            else{
                sprintf(num1, "\n%d;%s;%d;%d;%d;%d", here->id_number, here->action_name, here->price, here->number_of_watch, here->current_bid, here->remaining_ticks);
                strcat(result, num1);
            }
        }
        current = current->next;
    }
    if (count != 0) {
        strcat(result, "\n\x00");
        temp->job_info->msg_len = strlen(result)+1;
    }
    else{
        temp->job_info->msg_len = 0;
    }
    temp->job_info->msg_type = ANLIST;
    wr_msg(temp->client_fd, temp->job_info, result);
    finish_read_auction();
}

void process_ANCREATE(struct job* temp){
    start_writing_auction();
    start_reading_usertable();
    char * name = malloc(sizeof(char)*BUFFER_SIZE);
    strcpy(name, strtok(temp->message, "\n"));
    int duration = atoi(strtok(NULL, "\n"));
    int bin_price = atoi(strtok(NULL, "\n"));
    name[strcspn(name, "\r")] = 0;
    if (duration<1 || bin_price<0 || strlen(name) == 0){
        temp->job_info->msg_type = EINVALIDARG;
        temp->job_info->msg_len = 0;
        wr_msg(temp->client_fd, temp->job_info, "");
    }
    else{
        struct Auction* new_auc = malloc(sizeof(struct Auction));
        pthread_mutex_lock(&auction_number_lock);
        new_auc->id_number = auctionID;
        auctionID++;
        pthread_mutex_unlock(&auction_number_lock);
        strcpy(new_auc->action_name, name);
        struct User* nobody = find_username_byfd(temp->client_fd);
        new_auc->creater_fd = temp->client_fd;
        new_auc->current_bid = 0;
        new_auc->number_of_watch = 0;
        new_auc->price = bin_price;
        new_auc->remaining_ticks = duration;
        new_auc->current_leader_fd = -1;
        insertRear(&AuctionList, (void*) new_auc);
        char num[BUFFER_SIZE];
        sprintf(num, "%d", new_auc->id_number);
        temp->job_info->msg_len = strlen(num)+1;
        temp->job_info->msg_type = ANCREATE;
        wr_msg(temp->client_fd, temp->job_info, num);
    }  
    free(name);
    finish_writing_auction();
    finish_reading_usertable();
}


void process_ANWATCH(struct job* temp){
    start_writing_auction();
    struct Auction* watch_auc = find_auction_byid(atoi(temp->message));
    if (watch_auc == NULL){
        temp->job_info->msg_type = EANNOTFOUND;
        temp->job_info->msg_len = 0;
        wr_msg(temp->client_fd, temp->job_info, "");
    }
    else{
        if (watch_auc->number_of_watch == 5){
            temp->job_info->msg_type = EANFULL;
            temp->job_info->msg_len = 0;
            wr_msg(temp->client_fd, temp->job_info, "");
        }
        else{
            watch_auc->watchlist[watch_auc->number_of_watch] = temp->client_fd;
            watch_auc->number_of_watch++;
            temp->job_info->msg_type = ANWATCH;
            char num[BUFFER_SIZE];
            sprintf(num, "%s\r\n%d", watch_auc->action_name, watch_auc->price);
            temp->job_info->msg_len = strlen(num)+1;
            wr_msg(temp->client_fd, temp->job_info, num);
        }
    }
    finish_writing_auction();
}

void process_ANLEAVE(struct job* temp){
    start_writing_auction();
    struct Auction* watch_auc = find_auction_byid(atoi(temp->message));
    if (watch_auc == NULL){
        temp->job_info->msg_type = EANNOTFOUND;
        temp->job_info->msg_len = 0;
        wr_msg(temp->client_fd, temp->job_info, "");
    }
    else{ 
        for (int i = 0; i < watch_auc->number_of_watch; i++){
            if (watch_auc->watchlist[i] == temp->client_fd){
                watch_auc->watchlist[i] = -1;
            }
        }
        watch_auc->number_of_watch--;
        temp->job_info->msg_type = OK;
        temp->job_info->msg_len = 0;
        wr_msg(temp->client_fd, temp->job_info, "");
    }
    finish_writing_auction();
}

void process_ANBID(struct job* temp){
    start_writing_auction();
    start_writing_table();
    int auc_id = atoi(strtok(temp->message, "\n"));
    int bid_price = atoi(strtok(NULL, "\n"));
    struct Auction* watch_auc = find_auction_byid(auc_id);
    if (watch_auc == NULL){
        temp->job_info->msg_type = EANNOTFOUND;
        temp->job_info->msg_len = 0;
        wr_msg(temp->client_fd, temp->job_info, "");
    }
    else{
        struct User* check_creator = find_username_byfd(temp->client_fd);
        if (watch_auc->creater_fd == temp->client_fd){
            temp->job_info->msg_type = EANDENIED;
            temp->job_info->msg_len = 0;
            wr_msg(temp->client_fd, temp->job_info, "");
        }
        else{
            if (bid_price < watch_auc->current_bid){
                temp->job_info->msg_type = EBIDLOW;
                temp->job_info->msg_len = 0;
                wr_msg(temp->client_fd, temp->job_info, "");
            }
            else{
                if (bid_price >= watch_auc->price && watch_auc->price != 0){
                    watch_auc->current_bid = bid_price;
                    watch_auc->current_leader_fd = temp->client_fd;
                    watch_auc->remaining_ticks = 0;
                    temp->job_info->msg_type = OK;
                    temp->job_info->msg_len = 0;
                    wr_msg(temp->client_fd, temp->job_info, "");
                    check_creator->won_auction[check_creator->number_of_auction] = watch_auc->id_number;
                    check_creator->number_of_auction++;
                    int count = watch_auc->number_of_watch;
                    int start = 0;
                    while (count>0){
                        if (watch_auc->watchlist[start] == -1){
                            ;
                        }
                        else{
                            char num[BUFFER_SIZE];
                            sprintf(num, "%d\r\n%s\r\n%d", watch_auc->id_number, check_creator->username, bid_price);
                            temp->job_info->msg_len = strlen(num)+1;
                            temp->job_info->msg_type = ANCLOSED;
                            wr_msg(watch_auc->watchlist[start], temp->job_info, num);
                            
                            count--;
                        }
                        start++;
                    }
                }
                else{
                    watch_auc->current_bid = bid_price;
                    watch_auc->current_leader_fd = temp->client_fd;
                    temp->job_info->msg_type = OK;
                    temp->job_info->msg_len = 0;
                    wr_msg(temp->client_fd, temp->job_info, "");
                    int count = watch_auc->number_of_watch;
                    int start = 0;
                    while (count>0){
                        if (watch_auc->watchlist[start] == -1){
                            ;
                        }
                        else{
                            char num[BUFFER_SIZE];
                            sprintf(num, "%d\r\n%s\r\n%s\r\n%d", watch_auc->id_number, watch_auc->action_name, check_creator->username, bid_price);
                            temp->job_info->msg_len = strlen(num)+1;
                            temp->job_info->msg_type = ANUPDATE;
                            wr_msg(watch_auc->watchlist[start], temp->job_info, num);
                            count--;
                        }
                        start++;
                    }
                }

            }
        }
    }
    finish_writing_table();
    finish_writing_auction();
}

void process_USRWINS(struct job* temp){
    start_reading_usertable();
    struct User* check_creator = find_username_byfd(temp->client_fd);
    char result[] = "";
    if (check_creator->number_of_auction == 0){
        temp->job_info->msg_type = USRWINS;
        temp->job_info->msg_len = 0;
        wr_msg(temp->client_fd, temp->job_info, result);
    }
    else{
        int sorted[50];
        for (int i = 0; i < check_creator->number_of_auction; i++){
            sorted[i] = check_creator->won_auction[i];
        }
        qsort(sorted, check_creator->number_of_auction, sizeof(int), cmpfunc);
        for(int i = 0; i < check_creator->number_of_auction; i++){
            struct Auction* here = find_auction_byid(sorted[i]);
            char num[BUFFER_SIZE];
            sprintf(num, "%d;%s;%d\n", here->id_number, here->action_name, here->current_bid);
            strcat(result, num);
        }
        strcat(result, "\0");
        temp->job_info->msg_len = strlen(result)+1;
        temp->job_info->msg_type = USRWINS;
        wr_msg(temp->client_fd, temp->job_info, result);
    }
    finish_reading_usertable();
}

void process_USRSALES(struct job* temp){
    start_reading_usertable();
    start_reading_auction();
    node_t* current = AuctionList.head;
    char result[] = "";
    int sorted[50];
    int count = 0;
    while (current != NULL){
        struct Auction* this_auc = (struct Auction*) current->value;
        if (this_auc->remaining_ticks <=0){
            if (this_auc->current_leader_fd != -1){
                if (this_auc->creater_fd == temp->client_fd){
                    sorted[count] = this_auc->id_number;
                    count++;
                }
            }
        }
        current= current->next;
    }
    qsort(sorted, count, sizeof(int), cmpfunc);
    for (int i = 0; i < count; i++){
        struct Auction* sorted_auction = find_auction_byid(sorted[i]);
        struct User* abc = find_username_byfd(sorted_auction->current_leader_fd);
        char num[BUFFER_SIZE];
        sprintf(num, "%d;%s;%s;%d\n", sorted_auction->id_number,sorted_auction->action_name, abc->username, sorted_auction->current_bid);
        strcat(result, num);
    }
    if (strlen(result) == 0){
        temp->job_info->msg_len = 0;
        temp->job_info->msg_type = USRSALES;
        wr_msg(temp->client_fd, temp->job_info, "");
    } 
    else{
        strcat(result, "\0");
        temp->job_info->msg_len = strlen(result)+1;
        temp->job_info->msg_type = USRSALES;
        wr_msg(temp->client_fd, temp->job_info, result);
    }
    finish_reading_usertable();
    finish_read_auction();
}

void process_USRBLNC(struct job* temp){
    start_reading_usertable();
    start_reading_auction();
    int balance = 0;
    node_t* current = AuctionList.head;
    while (current != NULL){
        struct Auction* this_auc = (struct Auction*) current->value;
        if (this_auc->remaining_ticks <=0){
            if (this_auc->current_leader_fd != -1){
                if (this_auc->creater_fd == temp->client_fd){
                    balance = balance+ this_auc->current_bid;
                }
            }
        }
        current= current->next;
    }
    struct User* check_creator = find_username_byfd(temp->client_fd);
    if (check_creator->number_of_auction != 0){
        for(int i = 0; i < check_creator->number_of_auction; i++){
            struct Auction* here = find_auction_byid(check_creator->won_auction[i]);
            balance = balance-here->current_bid;
        }
    }
    char num[BUFFER_SIZE];
    sprintf(num, "%d", balance);
    temp->job_info->msg_len = strlen(num)+1;
    temp->job_info->msg_type = USRBLNC;
    wr_msg(temp->client_fd, temp->job_info, num);
}