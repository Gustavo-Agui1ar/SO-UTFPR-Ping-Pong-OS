#include "ppos_disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define DEBUG
disk_t* disk = NULL;
task_t* taskDiskMgr = NULL;
struct sigaction sigDisk;

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

    if(sem_create(&disk->sem_disk, 1) < 0) {
        perror("Failed to create disk semaphore");
        free(disk);
        return -1;
    }

    #ifdef DEBUG
        printf("\nDisk semaphore created\n");
    #endif

    if(sem_create(&disk->sem_queue, 1) < 0) {
        perror("Failed to create disk queue semaphore");
        sem_destroy(&disk->sem_disk);
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
        sem_destroy(&disk->sem_disk);
        sem_destroy(&disk->sem_queue);
        free(disk);
        return -1;
    }
    
    if(task_create(taskDiskMgr, disk_manager, NULL) < 0) {
        perror("Failed to create disk manager task");
        sem_destroy(&disk->sem_disk);
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

    if(block < 0 || block >= disk->numBlocks) {
        perror("Invalid block number for write operation");
        return -1;
    }

    if(disk == NULL) {
        perror("Disk manager not initialized");
        return -1;
    }

    #ifdef DEBUG
        printf("\nInitiating write operation for block %d\n", block);
    #endif

    sem_down(&disk->sem_disk);

    #ifndef DEBUG
        printf("\nGetting disk semaphore for write operation...\n");
    #endif

    disk_request* request = malloc(sizeof(disk_request));

    if (!request) {
        perror("Failed to allocate memory for disk request");
        sem_up(&disk->sem_disk);
        return -1;
    }

    request->task = taskExec;
    request->operation = DISK_CMD_WRITE;
    request->buffer = buffer;
    request->block = block;

    #ifdef DEBUG
        printf("\nDisk request created for write operation on block %d\n", block);
    #endif

    sem_down(&disk->sem_queue);
    
    queue_append((queue_t**)&disk->requestQueue, (queue_t*)request);

    sem_up(&disk->sem_queue);

    #ifdef DEBUG
        printf("\nDisk request appended to request queue\n");
    #endif

    if(disk->signal == 0 && disk->empty == 1) {
        task_resume(taskDiskMgr);
    } 

    #ifdef DEBUG
        printf("\nDisk manager task resumed for write operation\n");
    #endif

    sem_up(&disk->sem_disk);

    sem_down(&disk->sem_queue);
    task_suspend(taskExec, &disk->taskQueue);
    sem_up(&disk->sem_queue);    task_yield();
    
    return 0;
}

int disk_block_read(int block, void *buffer) {

    if(block < 0 || block >= disk->numBlocks) {
        perror("Invalid block number for read operation");
        return -1;
    }

    if(disk == NULL) {
        perror("Disk manager not initialized");
        return -1;
    }

    #ifdef DEBUG
        printf("\nInitiating read operation for block %d\n", block);
    #endif

    sem_down(&disk->sem_disk);

    #ifndef DEBUG
        printf("\nGetting disk semaphore for read operation...\n");
    #endif

    disk_request* request = malloc(sizeof(disk_request));

    if (!request) {
        perror("Failed to allocate memory for disk request");
        sem_up(&disk->sem_disk);
        return -1;
    }

    request->task = taskExec;
    request->operation = DISK_CMD_READ;
    request->buffer = buffer;
    request->block = block;

    #ifdef DEBUG
        printf("\nDisk request created for read operation on block %d\n", block);
    #endif

    sem_down(&disk->sem_queue);
    
    queue_append((queue_t**)&disk->requestQueue, (queue_t*)request);

    sem_up(&disk->sem_queue);

    #ifdef DEBUG
        printf("\nDisk request appended to request queue\n");
    #endif

    if(disk->signal == 0 && disk->empty == 1) {
        task_resume(taskDiskMgr);
    } 

    #ifdef DEBUG
        printf("\nDisk manager task resumed for read operation\n");
    #endif

    sem_up(&disk->sem_disk);

    sem_down(&disk->sem_queue);
    task_suspend(taskExec, &disk->taskQueue);
    sem_up(&disk->sem_queue);
    task_yield();
    return 0;
}

void handler_signal_disk( int signum) {

    #ifdef DEBUG
        printf("\nDisk signal handler invoked for signal %d\n", signum);
    #endif

    if(signum != SIGUSR1) {
        return;
    }

    #ifdef DEBUG
        printf("\nReceived disk signal, processing...\n");
        printf("\nHandling disk signal...\n");
    #endif

    if(disk == NULL || taskDiskMgr == NULL) {
        perror("Disk manager not initialized");
        return;
    }

    disk->signal = 1;
    disk->empty = 0;
    task_resume(taskDiskMgr);

    #ifdef DEBUG
        printf("\nDisk signal handler completed\n");
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

    #ifdef DEBUG
    printf(("Disk Manager started\n"));
    #endif

    while(1) {
        sem_down(&disk->sem_disk);

        #ifndef DEBUG
            printf("\nDisk Manager getting disk semaphore...\n");
        #endif

        if(disk->signal == 1) {
            sem_down(&disk->sem_queue);
            disk->signal = 0;

            if(disk->currentRequest == NULL && disk->currentRequest->task == NULL) {
                perror("No task waiting for disk request");
                sem_up(&disk->sem_queue);
                continue;
            }

            task_t* task = disk->currentRequest->task;
            disk->currentRequest = NULL;
            
            task_resume(task);
            
            #ifdef DEBUG
                printf("\nDisk Manager resumed task %d from disk queue\n", task->id);
            #endif
            sem_up(&disk->sem_queue);
        }

        
        
        if(disk->requestQueue != NULL && disk->empty == 1) {
            disk->empty = 0;
            disk_request* request = FCFS();
            
            if(request == NULL) {
                #ifdef DEBUG
                    printf("\nNo disk requests to process, waiting...\n");
                #endif
                sem_up(&disk->sem_disk);
                task_suspend(taskDiskMgr, &sleepQueue);
            }

            disk_cmd(request->operation, request->block, request->buffer);
            disk->currentRequest = request;

            #ifndef DEBUG 
                printf("\nDisk MAnager request processed: Task %d, Operation %d, Block %d\n", 
                       request->task->id, request->operation, request->block);
            #endif
        }

        sem_up(&disk->sem_disk);

        #ifdef DEBUG
            printf("\nDisk Manager completed a cycle, waiting for next request...\n");
        #endif
        task_suspend(taskDiskMgr, &sleepQueue);
    }
}