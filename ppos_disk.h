#ifndef __DISK_MGR__
#define __DISK_MGR__
#include "ppos.h"
#include "ppos-data.h"
#include "disk-driver.h"
#include "ppos-core-globals.h"
#include "queue.h"
#include <signal.h>


typedef struct t{
    struct t *next, *prev;
    task_t* task;
    int operation;
    void *buffer;
    int block;
} disk_request;

typedef struct {
    int numBlocks;
    int blockSize;

    semaphore_t sem_work;  // semaphore for disk operations
    mutex_t mut_disk;
    unsigned char signal;         // indicates if the disk is awake
    unsigned char empty;          // indicates if the disk is free

    task_t* taskQueue;      // queue of tasks waiting for disk operations
    disk_request* currentRequest; // queue of tasks waiting for disk operations
    semaphore_t sem_queue;        // semaphore for the disk queue
    disk_request* requestQueue;   // queue of disk requests 
} disk_t;


int disk_mgr_init(int *numBlocks, int *blockSize);

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer);

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer);

disk_request* FCFS();
disk_request* SSTF();
disk_request* CSCAN();

void handler_signal_disk( int signum);
void disk_manager();

#endif