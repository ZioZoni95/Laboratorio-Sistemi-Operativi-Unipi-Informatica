#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<pthread.h>


typedef struct fd_node
{
    void *content;
    struct fd_node *next;
}fd_node_t;

typedef struct queue_fd
{
    fd_node_t *que;
    int len_que_fd;
}queue_fd_t;

typedef struct threads_args{

    int k;
    void *content_array;
    int size_array;
}

static pthread_mutex_t mtx_fd_queue = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_fd_queue = PTHREAD_COND_INITIALIZER;

/**
 * @funct : CreateQueueFD
 * @brief : alloca la struttura queue_fd_t e la inizializza
 * @return : puntatore alla conda inizializzata, NULL su errore con errno settato7
 */
queue_fd_t *CreateQueueFD(){

    queue_fd_t *queue = malloc(sizeof(queue_fd_t));
    if(!queue)return NULL;

    queue->que=NULL;
    queue->len_que_fd = 0;
    return queue;
}


/**
 * @funct : push_fd
 * @brief : inserisce un nodo alla coda secondo la politica FIFO e secondo le regole della mutua esclusione
 * @param q : coda a cui aggiungere il nodo
 * @param new_data : contenuto del nuovo nodo
 * @return : 0 se successo,-1 su errore con errno settato opportunamente
 */

int push_fd(queue_fd_t *q,void *new_data){

    if(!q || !new_data){errno=EINVAL;return -1;}

    fd_node_t *tmp = malloc(sizeof(fd_node_t));
    if(!tmp)return -1;
    tmp->content = new_data;
    lock(&mtx_fd_queue);
    if(q->que == NULL){
        q->que = tmp;
        tmp->next = NULL;
    }
    else{
        tmp->next = q->que;
        q->que = tmp;
    }
    cond_signal(&cond_fd_queue);
    unlock(&mtx_fd_queue);
    return 0;
}

/**
 * @funct : pop_fd
 * @brief : elimina un nodo dalla coda secondo la politica FIFO rispettando la mutua esclusione
 * @param queue : coda da cui prendere il nodo e prelevarne il contenuto
 * @return : contenuto del nodo espulso, NULL su errore con errno settato opportunamente
 */

void *pop_fd(queue_fd_t *q){

    if(!q){errno = EINVAL;return NULL;}

    void *expelled_cont; //puntatore al contenuto della coda
    fd_node_t *corr = q->que;
    fd_node_t *prec = NULL;

    while(corr->next != NULL){
        corr=corr->next;
    }
    lock(&mtx_fd_queue);
    if(prec == NULL){
        expelled_cont = corr->content;
        free(corr);
    }
    else{
        prec->next = corr->next;
        expelled_cont = corr->content;
        free(corr);
    }
    q->len_que_fd -= 1;
    cond_signal(&cond_fd_queue);
    unlock(&mtx_fd_queue);

    return expelled_cont;
}

/**
 * @funct DestroyQueue
 * @param queue : coda di fd da deallocare
 */

void DestroyQueueFD(queue_fd_t *queue){

    if(!queue){errno = EINVAL;return;}
    if(!queue->que){free(queue);return;}

    fd_node_t *corr=queue->que;
    while(corr != NULL){
        if(corr->content)free(corr->content);
        corr=corr->next;
    }
    free(queue);
}

int WaitQueueEmpty(queue_fd_t *queue){

    if(!queue){errno=EINVAL;return -1;}

    while(queue->len_que_fd == 0){
        cond_wait(&cond_fd_queue,&mtx_fd_queue);
    }
    return 0;
}

void *Master(void *arg){
    int k = ()
}