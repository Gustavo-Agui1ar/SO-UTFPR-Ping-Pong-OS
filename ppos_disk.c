#include "ppos_disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/**
 * 
 #define READ
 #define WRITE
 #define FCFS_DEF
 #define SIGNAL
 #define MANAGER
 */
disk_t* disk = NULL;
task_t* ajust = NULL;
task_t* taskDiskMgr = NULL;
struct sigaction sigDisk;

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

    if(sem_create(&disk->sem_work, 0) < 0) {
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
    
    ajust = (task_t*)malloc(sizeof(task_t));

    if (!ajust) {
        perror("Failed to allocate memory for ajust disk manager task");
        sem_destroy(&disk->sem_work);
        sem_destroy(&disk->sem_queue);
        free(disk);
        free(taskDiskMgr);
        return -1;
    }

    if(task_create(ajust, ajuste, NULL) < 0) {
        perror("Failed to create ajust disk manager task");
        sem_destroy(&disk->sem_work);
        sem_destroy(&disk->sem_queue);
        free(disk);
        free(taskDiskMgr);
        free(ajust);
        return -1;
    }

    task_setprio(ajust, 20);
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
    printf("\nDisk Manager: Up Semaphore\n");
    #endif
    
    sem_up(&disk->sem_work);

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
    printf("\nDisk Manager: Up semaphore\n");
    #endif

    sem_up(&disk->sem_work);

    task_suspend(taskExec,  &disk->taskQueue);
    task_yield();
    return 0;
}

void handler_signal_disk( int signum) {
    if(signum != SIGUSR1) {
        return;
    }

    #ifdef SIGNAL
        printf("\nReceived signal to wake up disk manager\n");
    #endif

    if(disk != NULL) {
        #ifdef SIGNAL
            printf("\nWaking up disk manager...\n");
        #endif
        disk->signal = 1;
        sem_up(&disk->sem_work); 
    }
}

disk_request* FCFS() {
    #ifdef FCFS_DEF
        printf("\nProcessing disk requests using FCFS algorithm...\n");
    #endif

    print_queue_disk("Disk Request Queue", disk->requestQueue);

    if(disk->requestQueue == NULL) {
        return NULL;
    }

    disk_request* request = disk->requestQueue;

    queue_remove((queue_t**)&disk->requestQueue, (queue_t*)request);

    #ifdef FCFS_DEF
        printf("\nAfter remove queue: Disk Request Queue\n");
        print_queue_disk("Disk Request Queue", disk->requestQueue);

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
            printf("\nDisk Manager: Waiting semaphore disk \n");
        #endif

        sem_down(&disk->sem_work);
        
        #ifdef MANAGER
            printf("\nDisk Manager: Semaphore acquired\n");
        #endif

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

