#include "ppos_disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define DEBUG

semaphore_t *semaphore_disk = NULL;
task_t *disk_mgr = NULL;
disk_t *last_request = NULL;
disk_t *queue_disk = NULL;
int disk_awake = 0;
int lastBlock = 0;

disk_t *(*select_request)(void) = NULL;
void print_task_queue(const char *name, queue_t *queue);

int disk_mgr_init(int *numBlocks, int *blockSize) {
    #ifdef DEBUG
    printf("[DEBUG] Iniciando disk_mgr_init\n");
    #endif

    if (disk_cmd(DISK_CMD_INIT, 0, 0) < 0) {
        #ifdef DEBUG
        printf("[ERRO] disk_cmd INIT falhou\n");
        #endif
        return -1;
    }

    if(!sem_create(semaphore_disk, 1)) {
        #ifdef DEBUG
        printf("[ERRO] sem_create falhou\n");
        #endif
        return -1;
    }

    disk_mgr = malloc(sizeof(task_t));
    if (!disk_mgr) return -1;
    if (task_create(disk_mgr, disk_manager, NULL) < 0) {
        #ifdef DEBUG
        printf("[ERRO] disk_mgr falhou\n");
        #endif
        free(disk_mgr);
        disk_mgr = NULL;
        return -1;
    }

    struct sigaction action;
    action.sa_handler = handler_signal_disk;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGUSR1, &action, NULL) < 0) {
        #ifdef DEBUG
        perror("[ERRO] sigaction");
        #endif
        return -1;
    }

    *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    if (*numBlocks <= 0) return -1;
    *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    if (*blockSize <= 0) return -1;

    select_request = FCFS;
    #ifdef DEBUG
    printf("[DEBUG] disk_mgr_init finalizado com sucesso\n");
    #endif
    return 0;
}

disk_t *FCFS(void) {
    return queue_disk;
}

disk_t *SSTF(void) {
    if (!queue_disk) return NULL;
    disk_t *current = queue_disk;
    disk_t *best = current;
    do {
        if (abs(lastBlock - current->block) < abs(lastBlock - best->block))
            best = current;
        current = current->next;
    } while (current != queue_disk);
    return best;
}

disk_t *CSCAN(void) {
    return NULL;
}

int disk_block_write(int block, void *buffer) {
    if (block < 0) return -1;

    disk_t *req = malloc(sizeof(disk_t));
    if (!req) return -1;
    req->operation = DISK_CMD_WRITE;
    req->block = block;
    req->buffer = buffer;
    req->task = taskExec;

    sem_down(semaphore_disk);
    queue_append((queue_t **)&queue_disk, (queue_t *)req);
    sem_up(semaphore_disk);

    if (!disk_awake) task_resume(disk_mgr);

    task_suspend(taskExec, &sleepQueue);
    return 0;
}

int disk_block_read(int block, void *buffer) {
    if (block < 0) return -1;

    disk_t *req = malloc(sizeof(disk_t));
    if (!req) return -1;
    req->operation = DISK_CMD_READ;
    req->block = block;
    req->buffer = buffer;
    req->task = taskExec;

    sem_down(semaphore_disk);
    queue_append((queue_t **)&queue_disk, (queue_t *)req);
    sem_up(semaphore_disk);

    if (!disk_awake) task_resume(disk_mgr);

    task_suspend(taskExec, &sleepQueue);
    return 0;
}

void disk_manager() {

    #ifdef DEBUG
    printf("[DEBUG] disk_manager: Iniciado \n");
    #endif

    while (1) {
        sem_down(semaphore_disk);

        #ifdef DEBUG
        printf("[DEBUG] disk_manager: sem_down realizado\n");
        #endif

        if (disk_awake) {
            disk_awake = 0;

            #ifdef DEBUG
            printf("[DEBUG] disk_manager: disco acordou, retomando tarefa %p\n", last_request ? last_request->task : NULL);
            #endif

            if (last_request) {
                task_resume(last_request->task);
            }
        }

        // Se o disco está ocioso e há requisições pendentes
        if (disk_cmd(DISK_CMD_STATUS, 0, 0) == DISK_STATUS_IDLE && queue_disk) {
            disk_t *next = select_request();

            #ifdef DEBUG
            printf("[DEBUG] disk_manager: disco ocioso e há requisições na fila\n");
            #endif

            if (next) {
                #ifdef DEBUG
                printf("[DEBUG] disk_manager: requisicao selecionada (op=%d, bloco=%d)\n", next->operation, next->block);
                #endif

                queue_remove((queue_t **)&queue_disk, (queue_t *)next);
                lastBlock = next->block;
                last_request = next;
                disk_cmd(next->operation, next->block, next->buffer);

                #ifdef DEBUG
                printf("[DEBUG] disk_manager: comando enviado ao disco (op=%d, bloco=%d)\n", next->operation, next->block);
                #endif
            }
        }

        #ifdef DEBUG
        print_task_queue("[DEBUG] Fila de tarefas suspensas antes de task_suspend", (queue_t *)sleepQueue);
        #endif

        // Libera o semáforo e suspende o gerenciador
        sem_up(semaphore_disk);

        #ifdef DEBUG
        printf("[DEBUG] disk_manager: sem_up realizado, suspendendo tarefa do gerenciador\n");
        #endif

        task_suspend(disk_mgr, &sleepQueue);
    }
}


void handler_signal_disk(int signum) {
    if (signum != SIGUSR1) return;
    sem_down(semaphore_disk);
    disk_awake = 1;
    task_resume(disk_mgr);
    sem_up(semaphore_disk);
}

void print_task_queue(const char *name, queue_t *queue) {
    printf("[DEBUG] %s: ", name);
    if (!queue) {
        printf("<vazia>\n");
        return;
    }
    queue_t *curr = queue;
    do {
        task_t *t = (task_t*) curr;
        printf("%d -> ", t->id);
        curr = curr->next;
    } while (curr != queue);
    printf("(retorno)\n");
}
