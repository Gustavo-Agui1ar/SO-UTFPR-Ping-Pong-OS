#include "ppos_disk.h"
#include <stdio.h> // <-- para printf

#define DEBUG

mutex_t* mutex_disk = NULL;
task_t* disk_mgr = NULL;
disk_t* last_request = NULL;
disk_t* queue_disk = NULL;
struct sigaction disk_action;
int disk_awake = 0;
int lastBlock = 0;

int disk_mgr_init(int *numBlocks, int *blockSize) {
    #ifdef DEBUG
        printf("[DEBUG] Iniciando disk_mgr_init\n");
    #endif

    if(disk_cmd(DISK_CMD_INIT, 0, 0) < 0) {
        #ifdef DEBUG
            printf("[ERRO] disk_cmd INIT falhou\n");
        #endif
        return -1; 
    }

    if(disk_cmd(DISK_CMD_STATUS, 0, 0) == 0) {
        #ifdef DEBUG
            printf("[ERRO] disco não está pronto\n");
        #endif
        return -1;
    }

    if(!mutex_create(mutex_disk)) {
        #ifdef DEBUG
            printf("[ERRO] sem_create falhou\n");
        #endif
        return -1;
    }

    #ifdef DEBUG
        printf("[DEBUG] Criando tarefa do gerenciador de disco\n");
    #endif
    disk_mgr = (task_t*)malloc(sizeof(task_t));
    
    if(disk_mgr == NULL) {
        #ifdef DEBUG
            printf("[DEBUG] Erro: Disk_mgr foi inicializado incorretamente!\n");
        #endif
        return -1;
    }
    
   if (task_create(disk_mgr, disk_manager, NULL) < 0) {
        #ifdef DEBUG
            printf("[DEBUG] Erro: task_create falhou!\n");
        #endif
        free(disk_mgr);
        disk_mgr = NULL;
        return -1;
    }
    
    disk_action.sa_handler = handler_signal_disk;
    sigemptyset(&disk_action.sa_mask);
    disk_action.sa_flags = 0;

    if (sigaction(SIGUSR1, &disk_action, 0) < 0) {
        perror("[ERRO] sigaction");
        return -1;
    }

    *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    if (*numBlocks <= 0) {
        #ifdef DEBUG
            printf("[ERRO] disk_cmd DISK_CMD_DISKSIZE falhou\n");
        #endif
        return -1;
    }
    *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    if (*blockSize <= 0) {
        #ifdef DEBUG
            printf("[ERRO] disk_cmd DISK_CMD_BLOCKSIZE falhou\n");
        #endif
        return -1;
    }

    #ifdef DEBUG
        printf("[DEBUG] disk_mgr_init finalizado com sucesso\n");
    #endif
    return 0;
}

disk_t* FCFS() {
    #ifdef DEBUG
        printf("[DEBUG] Executando FCFS\n");
    #endif

    if(queue_disk == NULL)
        return NULL;

    lastBlock = queue_disk->block;

    #ifdef DEBUG
        printf("[DEBUG] FCFS selecionou bloco %d\n", queue_disk->block);
    #endif
    return queue_disk;   
}

disk_t* SSTF() {
    #ifdef DEBUG
        printf("[DEBUG] Executando SSTF\n");
    #endif

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
    #ifdef DEBUG
        printf("[DEBUG] SSTF selecionou bloco %d\n", best->block);
    #endif
    return best;
}

disk_t* CSCAN(int lastBlock) {
    #ifdef DEBUG
        printf("[DEBUG] Executando CSCAN\n");
    #endif

    if(queue_disk == NULL)
        return NULL;

    disk_t* current = queue_disk;
    disk_t* best = current;

    do {
        current = queue_disk->next; 
    } while (current != queue_disk);

    #ifdef DEBUG
        printf("[DEBUG] CSCAN selecionou bloco %d\n", best->block);
    #endif
    lastBlock = best->block;
    return best;
}

int disk_block_read (int block, void *buffer) {
    #ifdef DEBUG
        printf("[DEBUG] Requisição de leitura do bloco %d\n", block);
    #endif
    mutex_lock(mutex_disk);    
    
    disk_t* disk_config = NULL;
    disk_config = (disk_t*)malloc(sizeof(disk_t));
    
    if(disk_config == NULL) {
        #ifdef DEBUG
            printf("[DEBUG] Erro: disk_config mal inicializado!\n");
        #endif

        mutex_unlock(mutex_disk);
        return -1;
    }
    
    disk_config->next = disk_config->prev = NULL;
    disk_config->operation = DISK_CMD_READ;
    disk_config->buffer = buffer;
    disk_config->block = block;
    disk_config->task = taskExec;
    
    queue_append((queue_t**)&queue_disk, (queue_t*)disk_config);
    
    if (disk_awake) {
        #ifdef DEBUG
            printf("[DEBUG] Tarefa do disco já está na fila. Reativando.\n");
        #endif
        task_resume(disk_mgr);
    }

    #ifdef DEBUG
    printf("[DEBUG] Tarefa do usuário será suspensa após leitura\n");
    #endif
    
    task_suspend(taskExec, &sleepQueue);
    mutex_unlock(mutex_disk);
    
    return 0;
}
    
int disk_block_write (int block, void *buffer) {
        #ifdef DEBUG
        printf("[DEBUG] Requisição de escrita no bloco %d\n", block);
    #endif
    mutex_lock(mutex_disk);    
    
    disk_t* disk_config = NULL;
    disk_config = (disk_t*)malloc(sizeof(disk_t));
    
    if(disk_config == NULL) {
        #ifdef DEBUG
        printf("[DEBUG] Erro: disk_config mal inicializado!\n");
        #endif
        mutex_unlock(mutex_disk);
        return -1;
    }

    disk_config->next = disk_config->prev = NULL;
    disk_config->operation = DISK_CMD_WRITE;
    disk_config->buffer = buffer;
    disk_config->block = block;
    disk_config->task = taskExec;

    queue_append((queue_t**)&queue_disk, (queue_t*)disk_config);

    if (disk_awake) {
        #ifdef DEBUG
            printf("[DEBUG] Tarefa do disco já está na fila. Reativando.\n");
        #endif
        task_resume(disk_mgr);
    }

    #ifdef DEBUG
    printf("[DEBUG] Tarefa do usuário será suspensa após escrita\n");
    #endif
    task_suspend(taskExec, &sleepQueue);
    mutex_unlock(mutex_disk);

    return 0;
}

void disk_manager() {
    #ifdef DEBUG
        printf("[DEBUG] Iniciando disk_manager\n");
    #endif

    while(1) {
        mutex_lock(mutex_disk);

        if(disk_awake) {
            disk_awake = 0;
            if(last_request) {
                task_resume(last_request->task);
                free(last_request);
                last_request = NULL;
            }
        }

        if((disk_cmd(DISK_CMD_STATUS, 0, 0) == DISK_STATUS_IDLE) && (queue_size((queue_t*)queue_disk) > 0)){
            #ifdef DEBUG
            printf("[DEBUG] Disco está livre, buscando próxima requisição\n");
            #endif
            last_request = FCFS();
            if (last_request) {
                queue_remove((queue_t**)(&queue_disk), (queue_t*)last_request);
                #ifdef DEBUG
                    printf("[DEBUG] Enviando comando ao disco: op=%d bloco=%d buffer=%p\n",
                        last_request->operation, last_request->block, last_request->buffer);
                #endif
                disk_cmd(last_request->operation, last_request->block, last_request->buffer);
            }
        }
        
        mutex_unlock(mutex_disk);
        task_suspend(disk_mgr, &sleepQueue);
    }
}

void handler_signal_disk(int signum) {
    #ifdef DEBUG
        printf("[DEBUG] Sinal recebido: %d\n", signum);
    #endif

    if(signum == SIGUSR1) {
        mutex_lock(mutex_disk);
        disk_awake = 1;
        task_resume(disk_mgr);
        mutex_unlock(mutex_disk);
    }
}