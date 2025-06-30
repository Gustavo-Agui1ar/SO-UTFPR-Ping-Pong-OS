#include "ppos_disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/**
 * 
 #define DEBUG
 #define READ
 */
#define MANAGER
#define WRITE
disk_t* disk = NULL;
task_t* ajust = NULL;
task_t* taskDiskMgr = NULL;
struct sigaction sigDisk;

void ajuste() {
    while(1) {
        taskExec->quantum = 0;
    }
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

    if(mutex_create(&disk->sem_disk) < 0) {
        perror("Failed to create disk semaphore");
        free(disk);
        return -1;
    }

    #ifdef DEBUG
        printf("\nDisk semaphore created\n");
    #endif

    if(sem_create(&disk->sem_queue, 1) < 0) {
        perror("Failed to create disk queue semaphore");
        mutex_destroy(&disk->sem_disk);
        free(disk);
        return -1;
    }

    #ifdef DEBUG
        printf("\nDisk queue semaphore created\n");
    #endif

    disk->signal = 0;
    disk->empty = 1;
    disk->requestQueue = NULL;
    disk->currentRequest = NULL;
    disk->taskQueue = NULL;


    taskDiskMgr = malloc(sizeof(task_t));
    if (!taskDiskMgr) {
        perror("Failed to allocate memory for disk manager task");
        mutex_destroy(&disk->sem_disk);
        sem_destroy(&disk->sem_queue);
        free(disk);
        return -1;
    }
    
    if(task_create(taskDiskMgr, disk_manager, NULL) < 0) {
        perror("Failed to create disk manager task");
        mutex_destroy(&disk->sem_disk);
        sem_destroy(&disk->sem_queue);
        free(disk);
        free(taskDiskMgr);
        return -1;
    } 
    
    ajust = (task_t*)malloc(sizeof(task_t));

    if (!ajust) {
        perror("Failed to allocate memory for ajust disk manager task");
        mutex_destroy(&disk->sem_disk);
        sem_destroy(&disk->sem_queue);
        free(disk);
        free(taskDiskMgr);
        return -1;
    }

    if(task_create(ajust, ajuste, NULL) < 0) {
        perror("Failed to create ajust disk manager task");
        mutex_destroy(&disk->sem_disk);
        sem_destroy(&disk->sem_queue);
        free(disk);
        free(taskDiskMgr);
        free(ajust);
        return -1;
    }

    ajust->prio_static = 20;
    ajust->id = -1;

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

    if (!disk || block < 0 || block >= disk->numBlocks) {
        perror("Invalid disk or block number");
        return -1;
    }


    #ifdef WRITE
        printf("\nInitiating write operation for block %d\n", block);
    #endif

    sem_down(&disk->sem_queue);

    #ifdef WRITE
        printf("\nGetting disk semaphore for write operation...\n");
    #endif

    disk_request* request = malloc(sizeof(disk_request));

    if (!request) {
        perror("Failed to allocate memory for disk request");
        return -1;
    }

    request->task = taskExec;
    request->operation = DISK_CMD_WRITE;
    request->buffer = buffer;
    request->block = block;

    #ifdef WRITE
        printf("\nDisk request created for write operation on block %d\n", block);
    #endif

    queue_append((queue_t**)&disk->requestQueue, (queue_t*)request);

    sem_up(&disk->sem_queue);
    
    #ifdef WRITE
    printf("\nDisk request appended to request queue\n");
    #endif
    
    #ifdef WRITE
    printf("\nDisk manager task resumed for write operation\n");
    #endif
    
    mutex_unlock(&disk->sem_disk);

    task_suspend(taskExec, &disk->taskQueue);
    task_yield();
    
    return 0;
}

int disk_block_read(int block, void *buffer) {

   if (!disk || block < 0 || block >= disk->numBlocks) {
        perror("Invalid disk or block number");
        return -1;
    }

    #ifdef READ
        printf("\nInitiating read operation for block %d\n", block);
    #endif

    sem_down(&disk->sem_queue);

    #ifdef READ
        printf("\nGetting disk semaphore for read operation...\n");
    #endif

    disk_request* request = malloc(sizeof(disk_request));

    if (!request) {
        perror("Failed to allocate memory for disk request");
        return -1;
    }

    request->task = taskExec;
    request->operation = DISK_CMD_READ;
    request->buffer = buffer;
    request->block = block;

    #ifdef READ
        printf("\nDisk request created for read operation on block %d\n", block);
    #endif

    queue_append((queue_t**)&disk->requestQueue, (queue_t*)request);

    sem_up(&disk->sem_queue);

    #ifdef READ
        printf("\nDisk request appended to request queue\n");
    #endif

    #ifdef READ
        printf("\nDisk manager task resumed for read operation\n");
    #endif

    mutex_unlock(&disk->sem_disk);

    task_suspend(taskExec,  &disk->taskQueue);
    task_yield();
    return 0;
}

void handler_signal_disk( int signum) {
    if(signum != SIGUSR1) {
        return;
    }
    if(disk != NULL) {
        disk->signal = 1;
        mutex_unlock(&disk->sem_disk); 
    }
    #ifdef WRITE
        printf("\nDisk signal handler invoked and signaled disk manager.\n");
    #endif
}

disk_request* FCFS() {
    #ifdef DEBUG
        printf("\nProcessing disk requests using FCFS algorithm...\n");
    #endif

    if(disk->requestQueue == NULL) {
        return NULL;
    }

    disk_request* request = disk->requestQueue;

    queue_remove((queue_t**)&disk->requestQueue, (queue_t*)request);

    #ifdef DEBUG
        printf("\nFCFS: Processing request for task %d, operation %d, block %d\n", 
               request->task->id, request->operation, request->block);
    #endif

    return request;
}

void disk_manager() {

    #ifdef MANAGER
    printf(("Disk Manager started\n"));
    #endif

    while(1) {

        #ifdef MANAGER
            printf("\nDisk Manager getting disk semaphore...\n");
        #endif

        mutex_lock(&disk->sem_disk);
        
        sem_down(&disk->sem_queue);

        if(disk->signal == 1) {
            disk->signal = 0;
            if(disk->currentRequest == NULL) {
                perror("No task waiting for disk request");
                sem_up(&disk->sem_queue);
                continue;
            }

            task_t* task = disk->currentRequest->task;
            disk->currentRequest = NULL;
            disk->empty = 1;

            task_resume(task);
            
            #ifdef MANAGER
                printf("\nDisk Manager resumed task %d from disk queue\n", task->id);
            #endif
        }
        
        
        
        if(disk->requestQueue != NULL && disk->empty) {
            disk_request* request = FCFS();
            
            if(request) {
                disk->currentRequest = request;
                disk_cmd(request->operation, request->block, request->buffer);
                
                #ifdef MANAGER 
                printf("\nDisk MAnager request processed: Task %d, Operation %d, Block %d\n", 
                    request->task->id, request->operation, request->block);
                #endif
            }
            
        }
        
        sem_up(&disk->sem_queue);
        #ifdef MANAGER
            printf("\nDisk Manager completed a cycle, waiting for next request...\n");
        #endif
    }
}

