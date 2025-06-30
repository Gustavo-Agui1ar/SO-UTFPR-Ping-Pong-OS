#include "ppos_disk.h"

//#define DISK_DEBUG
#define SIGNAL_DEBUG

disk_t disk;
task_t* taskDiskMgr = NULL;
struct sigaction sigDisk;

task_t* ajust = NULL;
void ajuste(void *args) {
    while(1) {
        taskExec->quantum = 1;
    }
}

int disk_mgr_init(int *numBlocks, int *blockSize) {
  #ifdef DISK_DEBUG
    printf("\nDisk manager Init\n");
  #endif 

  if(disk_cmd(DISK_CMD_INIT, 0, 0) < 0) {
    perror("Failed to initialize disk driver");
    return -1;
  }

  *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
  *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);

  disk.numBlocks = *numBlocks;
  disk.blockSize = *blockSize;

  if(mutex_create(&disk.mut_disk) < 0) {
    perror("Failed to create disk semaphore");
    return -1;
  }

  disk.signal = 0;
  disk.empty = 1;
  disk.requestQueue = NULL;
  disk.currentRequest = NULL;
  disk.taskQueue = NULL;

  taskDiskMgr = (task_t*) malloc(sizeof(task_t));

  if(task_create(taskDiskMgr, disk_manager, NULL) < 0) {
    perror("Failed to create disk manager task");
    return -1;
  }

  sigDisk.sa_handler = handler_signal_disk;
  sigemptyset(&sigDisk.sa_mask);
  sigDisk.sa_flags = 0;

  if (sigaction(SIGUSR1, &sigDisk, 0) < 0) {
    perror("Failed to set signal handler for disk manager");
    return -1;
  }

      ajust = (task_t*)malloc(sizeof(task_t));

    if (!ajust) {
        perror("Failed to allocate memory for ajust disk manager task");


        return -1;
    }

    if(task_create(ajust, ajuste, NULL) < 0) {
        perror("Failed to create ajust disk manager task");
   
        return -1;
    }

  ajust->prio_static = 20;

  #ifdef DISK_DEBUG
    printf("\nDisk manager Init finish\n");
  #endif 

  return 0;
}

disk_request* create_disk_request(task_t* task, int operation, void* buffer, int block) {
  #ifdef DISK_DEBUG
    printf("\nCreating disk request\n");
  #endif

  disk_request* request = malloc(sizeof(disk_request));

  request->prev = NULL;
  request->next = NULL;
  request->task = task;
  request->operation = operation;
  request->buffer = buffer;
  request->block = block;

  return request;
}

void suspend_disk_manager() {
  #ifdef DISK_DEBUG
    printf("\nSuspendendo disk manager\n");
  #endif
  queue_remove((queue_t**)&readyQueue, (queue_t*)taskDiskMgr);
  taskDiskMgr->state = 'S';
}

void awake_disk_manager() {
  #ifdef DISK_DEBUG
    printf("\nAcordando disk manager\n");
  #endif
  queue_append((queue_t**)&readyQueue, (queue_t*)taskDiskMgr);
  taskDiskMgr->state = 'R';
}

void suspend_task(task_t* task) {
  #ifdef DISK_DEBUG
    printf("\nSuspendendo tarefa com id %d\n", task->id);
  #endif
  queue_remove((queue_t**)&readyQueue, (queue_t*)task);
  queue_append((queue_t**)&disk.taskQueue, (queue_t*)task);
  task->state = 'S';
}

void awake_task(task_t* task) {
  #ifdef DISK_DEBUG
    printf("\nAcordando tarefa com id %d\n", task->id);
  #endif
  queue_remove((queue_t**)&disk.taskQueue, (queue_t*)task);
  queue_append((queue_t**)&readyQueue, (queue_t*)task);
  task->state = 'R';
}

disk_request* FCFS() {
  if(disk.requestQueue == NULL)
    return NULL;

  #ifdef DISK_DEBUG
    printf("\nFCFS\n");
  #endif

  disk_request* request = disk.requestQueue;
  queue_remove((queue_t**)&disk.requestQueue, (queue_t*)request);
  return request;
}

int disk_block_read(int block, void *buffer) {
  if (block < 0 || block >= disk.numBlocks) {
      perror("Invalid disk or block number");
      return -1;
  }

  #ifdef DISK_DEBUG
    printf("\nIniciando disk_block_read de leitura\n");
  #endif

  mutex_lock(&disk.mut_disk);
  disk_request* request = create_disk_request(taskExec, DISK_CMD_READ, buffer, block);
  queue_append((queue_t**)&disk.requestQueue, (queue_t*)request);
  mutex_unlock(&disk.mut_disk);

  suspend_task(taskExec);
  task_yield();

  return 0;
}

int disk_block_write(int block, void *buffer) {
  if (block < 0 || block >= disk.numBlocks) {
      perror("Invalid disk or block number");
      return -1;
  }

  #ifdef DISK_DEBUG
    printf("\nIniciando disk_block_write de leitura\n");
  #endif

  mutex_lock(&disk.mut_disk);
  disk_request* request = create_disk_request(taskExec, DISK_CMD_WRITE, buffer, block);
  queue_append((queue_t**)&disk.requestQueue, (queue_t*)request);
  mutex_unlock(&disk.mut_disk);

  suspend_task(taskExec);
  task_yield();

  return 0;
}

void handler_signal_disk( int signum) {
  if(signum != SIGUSR1)
    return;

  #ifdef SIGNAl_DEBUG
    printf("\nTratando sinal\n");
  #endif
  
  disk.signal = 1;
  if (taskDiskMgr->state == 'S')
    awake_disk_manager();
}

void disk_manager(void* args) {
  while (1) {
    #ifdef DiSK_DEBUG
    printf("\nIniciando disk manager\n");
    #endif

    mutex_lock(&disk.mut_disk);

    if (disk.signal == 1) {
      disk.signal = 0;

      task_t* task = disk.currentRequest->task;
      disk.currentRequest = NULL;
      disk.empty = 1;

      awake_task(task);
    }

    if(disk.requestQueue != NULL && disk.empty) {
      disk_request* request = FCFS();
      
      if (request) {
        disk.currentRequest = request;
        disk_cmd(request->operation, request->block, request->buffer);
        disk.empty = 0;
      }    
    }
  
    mutex_unlock(&disk.mut_disk);
    suspend_disk_manager();

    #ifdef DISK_DEBUG
    printf("\nFinalizando disk manager\n");
    #endif
    task_yield();
  }
}