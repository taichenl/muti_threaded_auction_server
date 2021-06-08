#include "server.h"

void sbuf_init(sbuf_t *sp, int n){
    sp->buf = calloc(n, sizeof(struct job));
    sp->n = n;
    sp->front = 0;
    sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1); 
    Sem_init(&sp->slots, 0, n); 
    Sem_init(&sp->items, 0, 0);
    return;
};

void sbuf_deinit(sbuf_t *sp){
    for (int i = 0; i < sp->n; ++i){
        if (sp->buf[i] != NULL){
            free(sp->buf[i]->job_info);
            free(sp->buf[i]);
        }
    }
    free(sp->buf);
    return;
};

void sbuf_insert(sbuf_t *sp, struct job* temp){
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear)%(sp->n)] = temp;
    V(&sp->mutex);
    V(&sp->items);
    return;
};

struct job* sbuf_remove(sbuf_t *sp){
    struct job* result;
    P(&sp->items); 
    P(&sp->mutex); 
    result = sp->buf[(++sp->front)%(sp->n)]; 
    V(&sp->mutex); 
    V(&sp->slots);
    return result;
};

void start_reading_usertable(){
    pthread_mutex_lock(&read_user);
    reader_number++;
    if (reader_number == 1) pthread_mutex_lock(&write_user);
    pthread_mutex_unlock(&read_user);
};

void finish_reading_usertable(){
    pthread_mutex_lock(&read_user);
    reader_number--;
    if (reader_number == 0) pthread_mutex_unlock(&write_user);
    pthread_mutex_unlock(&read_user);
};

void start_writing_table(){
    pthread_mutex_lock(&write_user);
}

void finish_writing_table(){
    pthread_mutex_unlock(&write_user);
}

void start_reading_auction(){
    pthread_mutex_lock(&read_auction);
    auction_reader++;
    if (auction_reader ==1){
        pthread_mutex_lock(&write_auction);
    }
    pthread_mutex_unlock(&read_auction);
}

void finish_read_auction(){
    pthread_mutex_lock(&read_auction);
    auction_reader--;
    if (auction_reader ==0){
        pthread_mutex_unlock(&write_auction);
    }
    pthread_mutex_unlock(&read_auction);
}

void start_writing_auction(){
    pthread_mutex_lock(&write_auction);
}

void finish_writing_auction(){
    pthread_mutex_unlock(&write_auction);
}
