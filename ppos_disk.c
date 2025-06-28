#include "ppos_disk.h"
#include <stdio.h> // <-- para printf

semaphore_t* semaphore_disk;
task_t* disk_mgr = NULL;
disk_t* last_request = NULL;
disk_t* queue_disk = NULL;
struct sigaction disk_action;

int* numBlocks_global = NULL;
int* blockSize_global = NULL;
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

    if(!sem_create(semaphore_disk, 1)) {
        #ifdef DEBUG
            printf("[ERRO] sem_create falhou\n");
        #endif
        return -1;
    }

    #ifdef DEBUG
        printf("[DEBUG] Criando tarefa do gerenciador de disco\n");
    #endif
    disk_mgr = (task_t*)malloc(sizeof(task_t));
    task_create(disk_mgr, disk_manager, NULL);

    disk_action.sa_handler = handler_signal_disk;
    sigemptyset(&disk_action.sa_mask);
    disk_action.sa_flags = 0;

    if (sigaction(SIGUSR1, &disk_action, 0) < 0) {
        perror("[ERRO] sigaction");
        return -1;
    }

    numBlocks_global = numBlocks;
    blockSize_global = blockSize;

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

    disk_t* aux = queue_disk;
    queue_remove((queue_t**)(&queue_disk), (queue_t*)aux);
    lastBlock = aux->block;

    #ifdef DEBUG
        printf("[DEBUG] FCFS selecionou bloco %d\n", aux->block);
    #endif
    return aux;   
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
    sem_down(semaphore_disk);    

    disk_t* disk_config = (disk_t*)malloc(sizeof(disk_t));
    disk_config->next = disk_config->prev = NULL;
    disk_config->operation = DISK_CMD_READ;
    disk_config->buffer = buffer;
    disk_config->block = block;
    disk_config->task = taskExec;

    queue_append((queue_t**)&queue_disk, (queue_t*)disk_config);

    if (queue_contains((queue_t*)disk_mgr, (queue_t**)&sleepQueue)) {
        #ifdef DEBUG
            printf("[DEBUG] Tarefa do disco já está na fila. Reativando.\n");
        #endif
        task_resume(disk_mgr);
    }

    sem_up(semaphore_disk);
    #ifdef DEBUG
        printf("[DEBUG] Tarefa do usuário será suspensa após leitura\n");
    #endif
    task_suspend(taskExec, &sleepQueue);

    return 0;
}

int disk_block_write (int block, void *buffer) {
    #ifdef DEBUG
        printf("[DEBUG] Requisição de escrita no bloco %d\n", block);
    #endif
    sem_down(semaphore_disk);    

    disk_t* disk_config = (disk_t*)malloc(sizeof(disk_t));
    disk_config->next = disk_config->prev = NULL;
    disk_config->operation = DISK_CMD_WRITE;
    disk_config->buffer = buffer;
    disk_config->block = block;
    disk_config->task = taskExec;

    queue_append((queue_t**)&queue_disk, (queue_t*)disk_config);

    if (queue_contains((queue_t*)disk_mgr, (queue_t**)&queue_disk)) {
        #ifdef DEBUG
            printf("[DEBUG] Tarefa do disco já está na fila. Reativando.\n");
        #endif
        task_resume(disk_mgr);
    }

    sem_up(semaphore_disk);
    #ifdef DEBUG
        printf("[DEBUG] Tarefa do usuário será suspensa após escrita\n");
    #endif
    task_suspend(taskExec, &sleepQueue);

    return 0;
}

void disk_manager() {
    #ifdef DEBUG
        printf("[DEBUG] Iniciando disk_manager\n");
    #endif

    while(1) {
        sem_down(semaphore_disk);

        int status = disk_cmd(DISK_CMD_STATUS, *numBlocks_global, blockSize_global);
        if(status == DISK_STATUS_IDLE && queue_disk != NULL){
            #ifdef DEBUG
                printf("[DEBUG] Disco está livre, buscando próxima requisição\n");
            #endif
            last_request = FCFS();
            #ifdef DEBUG
                printf("[DEBUG] Enviando comando ao disco: op=%d bloco=%d\n",
                       last_request->operation, last_request->block);
            #endif
            disk_cmd(last_request->operation, last_request->block, last_request->buffer);
        }

        sem_up(semaphore_disk);
        task_suspend(disk_mgr, &sleepQueue);
    }
}

void handler_signal_disk(int signum) {
    #ifdef DEBUG
        printf("[DEBUG] Sinal recebido: %d\n", signum);
    #endif
    if(signum == SIGUSR1) {
        #ifdef DEBUG
            printf("[DEBUG] Requisição concluída, reativando tarefa %p\n", last_request->task);
        #endif
        task_resume(last_request->task);
        task_switch(disk_mgr);
    }
}

int queue_contains(queue_t *task, queue_t **queue) {
    #ifdef DEBUG
        printf("[DEBUG] queue_contains: verificando se tarefa %p está na fila %p\n", (void*)task, (void*)queue);
    #endif

    if(queue == NULL || (*queue) == NULL) {
        #ifdef DEBUG
            printf("[DEBUG] queue_contains: fila é NULL\n");
        #endif
        return 0;
    }

    queue_t* current = (*queue);

    do {
        #ifdef DEBUG
            printf("[DEBUG] queue_contains: verificando elemento %p\n", (void*)current);
        #endif
        if(current == task) {
            #ifdef DEBUG
                printf("[DEBUG] queue_contains: tarefa encontrada na fila\n");
            #endif
            return 1;
        }
        current = current->next;
    } while (current != (*queue));

    #ifdef DEBUG
        printf("[DEBUG] queue_contains: tarefa não encontrada na fila\n");
    #endif
    return 0;
}
