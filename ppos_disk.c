#include "ppos_disk.h"

semaphore_t* semaphore_disk;
task_t* disk_mgr = NULL;
disk_t* last_request = NULL;
disk_t* queue_disk = NULL;
struct sigaction disk_action;


int* numBlocks_global = NULL;
int* blockSize_global = NULL;
int lastBlock = 0;

int disk_mgr_init(int *numBlocks, int *blockSize) {

    if(disk_cmd(DISK_CMD_INIT, (int)(*numBlocks), blockSize))
       return -1; 

    if(disk_cmd(DISK_CMD_STATUS, 0, 0) < 0)
       return -1;
    
    if(!sem_create(semaphore_disk, 1))
       return -1;
      
       
    // inicializar tarefa disco
    disk_mgr = (disk_t*)malloc(sizeof(disk_t));
    task_create(disk_mgr, disk_manager, NULL);
    disk_action.sa_handler = handler_signal_disk;
    sigemptyset(&disk_action.sa_mask);
    disk_action.sa_flags = 0;

    if (sigaction(SIGUSR1, &disk_action, 0) < 0) {
        perror("Erro ao configurar tratador de sinal");
        return -1;
    }

    numBlocks_global = numBlocks;
    blockSize_global = blockSize;
    return 0;
}

disk_t* FCFS() {
    
    if(queue_disk == NULL)
        return NULL;
    
    disk_t* aux = queue_disk;
    queue_remove((queue_t**)(&queue_disk), (queue_t*)aux);
    lastBlock = aux->block;
    return aux;   
}

disk_t* SSTF() {
    if(queue_disk == NULL)
        return NULL;

    disk_t* current = queue_disk;
    disk_t* best = current;

    do {
        if(abs(lastBlock - current->block) < abs(lastBlock - best->block))
            best = current;
        current = queue_disk->next; 
    } while (current != queue_disk);
    lastBlock = best->block;
    return best;
}

disk_t* CSCAN(int lastBlock) {
    if(queue_disk == NULL)
        return NULL;

    disk_t* current = queue_disk;
    disk_t* best = current;

    do {
       
        current = queue_disk->next; 
    } while (current != queue_disk);
    lastBlock = best->block;
    return best;
}


int disk_block_read (int block, void *buffer)
{
  sem_down(semaphore_disk);    
  
  disk_t* disk_config =  (disk_t*)malloc(sizeof(disk_t));
  disk_config->next = disk_config->prev = NULL;
  disk_config->operation = DISK_CMD_READ;
  disk_config->buffer = buffer;
  disk_config->block = block;
  disk_config->task = taskExec;


  queue_append((queue_t**)&queue_disk, (queue_t*)disk_config);

  if (queue_contains((queue_t*)disk_mgr, (queue_t**)queue_disk))
  {
    task_resume(disk_mgr);
  }
  
  sem_up(semaphore_disk);
  
  task_suspend(taskExec, &sleepQueue);
  return 0;
}

int disk_block_write (int block, void *buffer)
{
  sem_down(semaphore_disk);    
  
  disk_t* disk_config =  (disk_t*)malloc(sizeof(disk_t));
  disk_config->next = disk_config->prev = NULL;
  disk_config->operation = DISK_CMD_WRITE;
  disk_config->buffer = buffer;
  disk_config->block = block;
  disk_config->task = taskExec;

  queue_append((queue_t**)&queue_disk, (queue_t*)disk_config);

  if (queue_contains((queue_t*)disk_mgr, (queue_t**)queue_disk))
  {
    task_resume(disk_mgr);
  }
  
  sem_up(semaphore_disk);
  
  task_suspend(taskExec, &sleepQueue);
  return 0;
}

void disk_manager() {

    while(1) {

        sem_down(semaphore_disk);

        if((disk_cmd(DISK_CMD_STATUS, (int)(*numBlocks_global), blockSize_global) == DISK_STATUS_IDLE) && (queue_disk != NULL)){
            last_request = FCFS();
            disk_cmd(last_request->operation, last_request->block, last_request->buffer);
        }

        sem_up(semaphore_disk);
        task_suspend(disk_mgr, &sleepQueue);
    }
}

void handler_signal_disk( int signum) {
    if(signum == SIGUSR1) {
        task_resume(last_request->task);
        task_switch(disk_mgr);
    }
} 

int queue_contains(queue_t *task, queue_t **queue) {
    if(queue == NULL)
        return -1;
    
    queue_t* current = (*queue);

    do {
        current = (*queue)->next; 
        if(current == task)
            return 1;
    } while (current != (*queue));
    return -1;
}