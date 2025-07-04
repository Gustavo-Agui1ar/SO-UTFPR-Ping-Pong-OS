#include "ppos_disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/**
 * 
 #define READ
 #define WRITE
 #define MANAGER
 #define SIGNAL
 #define FCFS_FUN
 #define SSTF_FUN
 */

#define CSCAN_FUN

disk_t* disk = NULL;
task_t* taskDiskMgr = NULL;
struct sigaction sigDisk;

disk_request* disk_scheduler() {
    disk_request* request = NULL;

    #ifdef FCFS_FUN
        request = FCFS();
        if(request != NULL)
            disk->totalBlocks += abs(disk->currentBlock - request->block);
    #elif defined(SSTF_FUN)
        request = SSTF();
        if(request != NULL)
        disk->totalBlocks += abs(disk->currentBlock - request->block);
    #elif defined(CSCAN_FUN)
        request = CSCAN();
        if(request != NULL)
            disk->totalBlocks += (request->block > disk->currentBlock) ? 
                             (request->block - disk->currentBlock) : 
                             (disk->numBlocks - disk->currentBlock + request->block);
    #else
        printf("Erro: Nenhum algoritmo de escalonamento definido.\n");
    #endif

    return request;
}


void print_queue_disk(char* nome, disk_request* queue) {
    if (!queue) {
        printf("\nQueue %s is empty\n", nome);
        return;
    }

    disk_request* current = queue;
    printf("\nQueue %s: ", nome);
    do {
        printf("%d ", current->task->id);
        current = current->next;
    } while (current != queue);
    printf("\n");
}

int disk_mgr_init(int *numBlocks, int *blockSize) {

    #ifdef DEBUG
        printf("\nInitializing disk manager...\n");
    #endif

    
    if(disk_cmd(DISK_CMD_INIT, 0, 0) < 0) {
        perror("Failed to initialize disk driver");
        return -1;
    }
    
    #ifdef DEBUG
    printf("\nDisk driver initialized successfully\n");
    #endif
    
    disk = malloc(sizeof(disk_t));

    if (!disk) {
        perror("Failed to allocate memory for disk");
        return -1;
    }

    #ifdef DEBUG
        printf("\nDisk allocated at %p\n", disk);
    #endif

    *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    if (*numBlocks <= 0 || *blockSize <= 0) {
        #ifdef DEBUG
            printf("\nBlock size or disk size is invalid: numBlocks=%d, blockSize=%d\n", *numBlocks, *blockSize);
        #endif
        perror("Failed to get disk size or block size");
        free(disk);
        return -1;
    }

    disk->numBlocks = *numBlocks;
    disk->blockSize = *blockSize;

    if( disk->blockSize <= 0 || disk->numBlocks <= 0) {
        perror("Invalid disk size or block size");
        free(disk);
        return -1;
    }

    #ifdef DEBUG
        printf("\nDisk size: %d blocks, Block size: %d bytes\n", disk->numBlocks, disk->blockSize);
    #endif

    if(sem_create(&disk->sem_work, 1) < 0) {
    perror("Failed to create disk work semaphore");
    sem_destroy(&disk->sem_queue);
    free(disk);
    return -1;
}

    #ifdef DEBUG
        printf("\nDisk semaphore created\n");
    #endif

    if(sem_create(&disk->sem_queue, 1) < 0) {
        perror("Failed to create disk queue semaphore");
        sem_destroy(&disk->sem_work);
        free(disk);
        return -1;
    }

    #ifdef DEBUG
        printf("\nDisk queue semaphore created\n");
    #endif

    disk->signal = 0;
    disk->totalBlocks = 0;
    disk->currentBlock = 0;
    disk->empty = 1;
    disk->requestQueue = NULL;
    disk->currentRequest = NULL;
    disk->taskQueue = NULL;


    taskDiskMgr = malloc(sizeof(task_t));
    if (!taskDiskMgr) {
        perror("Failed to allocate memory for disk manager task");
        sem_destroy(&disk->sem_work);
        sem_destroy(&disk->sem_queue);
        free(disk);
        return -1;
    }
    
    if(task_create(taskDiskMgr, disk_manager, NULL) < 0) {
        perror("Failed to create disk manager task");
        sem_destroy(&disk->sem_work);
        sem_destroy(&disk->sem_queue);
        free(disk);
        free(taskDiskMgr);
        return -1;
    } 

    sigDisk.sa_handler = handler_signal_disk;
    sigemptyset(&sigDisk.sa_mask);
    sigDisk.sa_flags = 0;

    if (sigaction(SIGUSR1, &sigDisk, 0) < 0) {
        perror("Failed to set signal handler for disk manager");
        return -1;
    }

    #ifdef DEBUG
        printf("\nDisk manager initialized successfully\n");
    #endif
    
    return 0;
}

int disk_block_write(int block, void *buffer) {

   if(block < 0 || block > disk->numBlocks) {
        return -1;
   }

   
   mutex_lock(&disk->mut_disk);

   #ifdef WRITE
        printf("\n[disk_block_write] Initiating write operation for block %d\n", block);
   #endif

   disk_request* request = malloc(sizeof(disk_request));
   if (!request) {
        mutex_unlock(&disk->mut_disk);
        return -1;
   }

    request->task = taskExec;
    request->operation = DISK_CMD_WRITE;
    request->buffer = buffer;
    request->block = block;

    sem_down(&disk->sem_queue);

    queue_append((queue_t**)&disk->requestQueue, (queue_t*)request);

    sem_up(&disk->sem_queue);

    if(taskDiskMgr->state == 'S') {
        #ifdef WRITE
            printf("\n[disk_block_write] Disk manager is suspended, waking it up\n");
        #endif
        disk->signal = 1;
        task_resume(taskDiskMgr);   
     }

   mutex_unlock(&disk->mut_disk);
     
   task_suspend(taskExec, &disk->taskQueue);
   task_yield();
   return 0;
}

int disk_block_read(int block, void *buffer) {

   if (block < 0 || block > disk->numBlocks) {
        return -1;
    }
     
   mutex_lock(&disk->mut_disk);

   #ifdef WRITE
        printf("\n[disk_block_write] Initiating write operation for block %d\n", block);
   #endif

   disk_request* request = malloc(sizeof(disk_request));
   if (!request) {
        mutex_unlock(&disk->mut_disk);
        return -1;
   }

    request->task = taskExec;
    request->operation = DISK_CMD_READ;
    request->buffer = buffer;
    request->block = block;

    sem_down(&disk->sem_queue);

    queue_append((queue_t**)&disk->requestQueue, (queue_t*)request);

    sem_up(&disk->sem_queue);

    if(taskDiskMgr->state == 'S') {
        #ifdef WRITE
            printf("\n[disk_block_write] Disk manager is suspended, waking it up\n");
        #endif
        disk->signal = 1;
        task_resume(taskDiskMgr);   
     }

   mutex_unlock(&disk->mut_disk);
     
   task_suspend(taskExec, &disk->taskQueue);
   task_yield();    
   return 0;
}

disk_request* FCFS() {

    if(disk->requestQueue == NULL)
        return NULL;

    return disk->requestQueue;
}

disk_request* SSTF() {
    sem_down(&disk->sem_queue);

    if(disk->requestQueue == NULL) {
        sem_up(&disk->sem_queue);
        return NULL;
    }

    if(disk->requestQueue->next == disk->requestQueue) {
        sem_up(&disk->sem_queue);
        return disk->requestQueue;
    }

    disk_request* current = disk->requestQueue;
    disk_request* closest = current;
    disk_request* head = disk->requestQueue;

    do {
        if(abs(disk->currentBlock - current->block) < abs(disk->currentBlock  - closest->block))
            closest = current;
        current = current->next;
    } while(current != head);

    sem_up(&disk->sem_queue);

    return closest;
}

disk_request* CSCAN() {
    sem_down(&disk->sem_queue);

    if(disk->requestQueue == NULL) {
        sem_up(&disk->sem_queue);
        return NULL;
    }

    if(disk->requestQueue->next == disk->requestQueue) {
        sem_up(&disk->sem_queue);
        return disk->requestQueue;
    }

    int existsgreater = 0;
    disk_request* current = disk->requestQueue;
    disk_request* best = disk->requestQueue;

    do {
        if(current->block >= disk->currentBlock) {
            if(current->block < best->block) {
                best = current;
                existsgreater = 1;
            }
        }
        current = current->next;
    } while(current != disk->requestQueue);

    if(existsgreater) {
        sem_up(&disk->sem_queue);
        return best;
    }

    current = disk->requestQueue;

    do {
        if(current->block <= disk->currentBlock) {
            if(current->block > best->block) {
                best = current;
            }
        }
        current = current->next;
    } while(current != disk->requestQueue);


    sem_up(&disk->sem_queue);
    return disk->requestQueue;
}

void disk_manager() {

    #ifdef MANAGER
        printf("\nDisk manager started\n");
    #endif

    while(1) {

        #ifdef MANAGER
            printf("\nDisk manager waiting mutex\n");
        #endif

        mutex_lock(&disk->mut_disk);

        #ifdef MANAGER
            printf("\nDisk manager acquired mutex\n");
        #endif

        if(disk->signal) {
            disk->signal = 0;
            
            if(disk->currentRequest) { 
                #ifdef MANAGER
                    printf("\nDisk manager task %d request completed\n", disk->currentRequest->task->id);
                #endif
                
                task_resume(disk->currentRequest->task);

                disk->currentBlock = disk->currentRequest->block;
                disk->currentRequest = NULL;
                disk->empty = 1;
            }
        }

        if(disk->requestQueue != NULL && disk->empty) {
            disk_request* request = disk_scheduler();
            
            if (request) {

                #ifdef MANAGER
                    printf("\nDisk manager processing request from task %d\n", request->task->id);
                #endif  

                disk->currentRequest = request;
                disk_cmd(request->operation, request->block, request->buffer);
                sem_down(&disk->sem_queue);
                queue_remove((queue_t**)&disk->requestQueue, (queue_t*)request);
                sem_up(&disk->sem_queue);
                disk->empty = 0;
            }       
        }
        mutex_unlock(&disk->mut_disk);
        /*
        */
        if(countTasks <= 2) {
            printf("\nTotal Blocks Processed: %d\n", disk->totalBlocks);
            task_exit(1);
        }
    }
}

void handler_signal_disk(int signum) {
    if(signum != SIGUSR1)
        return;
    
    #ifdef SIGNAL
        printf("\nDisk signal handler triggered\n");
    #endif

    disk->signal = 1;
}