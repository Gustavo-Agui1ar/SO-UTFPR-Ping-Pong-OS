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
} disk_t;

int disk_mgr_init(int *numBlocks, int *blockSize);

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer);

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer);

disk_t* FCFS();
disk_t* SSTF();
disk_t* CSCAN(int block);

void handler_signal_disk( int signum);
void disk_manager();
int queue_contains(queue_t *task, queue_t **queue);

#endif