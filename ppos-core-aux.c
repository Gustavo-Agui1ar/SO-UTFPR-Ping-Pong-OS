#include "ppos.h"
#include "ppos-core-globals.h"
#include <signal.h>
#include <string.h>
#include <sys/time.h>

// ****************************************************************************
// Adicione TUDO O QUE FOR NECESSARIO para realizar o seu trabalho
// Coloque as suas modificações aqui, 
// p.ex. includes, defines variáveis, 
// estruturas e funções
//
// ****************************************************************************

#define GROWTH_FACTOR -1
#define PRIO_MIN_TASK -20
#define PRIO_MAX_TASK 20
#define QUANTUM 20
//#define DEBUG
//#define SWITCH_DEBUG
//#define SCHEDULER_DEBUG

int activationDispatcher = 0;
int createDispatcher = 0;
int deathDispatcher = 0;

#define DEFINE_PRIO(p) ((p) < PRIO_MIN_TASK ? PRIO_MIN_TASK : ((p) > PRIO_MAX_TASK ? PRIO_MAX_TASK : (p)))

unsigned int _systemTime = 0;

struct itimerval timer;
struct sigaction action;

task_t* findByPriority(task_t* queue);
void growAll(task_t* queue, task_t* exclude);
void print_queue(char* nome, task_t* queue);
void handleTimer(int signum);

void before_ppos_init () {

    // put your customization here
    #ifdef DEBUG
        printf("\ninit - BEFORE");
    #endif
        
    setvbuf (stdout, 0, _IONBF, 0);
}

void after_ppos_init () {
    // put your customization here
    #ifdef DEBUG
        printf("\ninit - AFTER");
    #endif
    
    if(!taskMain) {
        perror("error initializing task main\n");
        exit(1);
    }

    task_setprio(taskMain, 0);

   action.sa_handler = handleTimer;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, 0) < 0) {
        perror("Erro ao configurar tratador de sinal");
        exit(1);
    }
    
    timer.it_value.tv_usec = 1000;
    timer.it_value.tv_sec = 0;
    timer.it_interval.tv_usec = 1000;
    timer.it_interval.tv_sec = 0;
    
    if (setitimer(ITIMER_REAL, &timer, 0) < 0) {
        perror("Erro ao iniciar timer");
        exit(1);
    }
}

void before_task_create (task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif
}

void after_task_create (task_t *task ) {
    // put your customization here
    #ifdef DEBUG
        printf("\ntask_create - AFTER - [%d]", task->id);
    #endif
    task->task_create_time = systime();
    task->task_death_time = 0;
    task->running_time = 0;
    task->activations = 0;
    task->prio_static = 0;
    task->prio_dynamic = 0;
    if(task->id != 1) 
        task->quantum = QUANTUM;
}

void before_task_exit () {
    // put your customization here
    #ifdef DEBUG
        printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
    #endif
    if(taskExec->id != 1) {
        taskExec->task_death_time = systime();
        printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", taskExec->id, 
            (taskExec->task_death_time - taskExec->task_create_time),
                                                                                     taskExec->running_time,
                                                                                     taskExec->activations);
    } else {
        printf("Task 1 exit: execution time %d ms, processor time %d ms, %d activations\n",  
            (systime() - createDispatcher),
                                                                                     taskExec->running_time,
                                                                                     activationDispatcher);
    }
}

void after_task_exit () {
    // put your customization here
    #ifdef DEBUG
        printf("\ntask_exit - AFTER- [%d]", taskExec->id);
    #endif
}

void before_task_switch ( task_t *task ) {
    // put your customization here
    #ifdef SwITCH_DEBUG
        printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
    #endif
}

void after_task_switch ( task_t *task ) {
    // put your customization here
    #ifdef SwITCH_DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]\n", taskExec->id, task->id);
    printf("activations: %d id: %d\n", taskExec->activations, taskExec->id);
    #endif
    
    taskExec->id != 1 ? taskExec->activations++ : activationDispatcher++;  
}

void before_task_yield () {
    // put your customization here
    #ifdef SWITCH_DEBUG
        printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
    #endif
}
void after_task_yield () {
    // put your customization here
    #ifdef SWITCH_DEBUG
        printf("\ntask_yield - AFTER - [%d]", taskExec->id);
    #endif
}


void before_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
    print_queue("Prontas" ,readyQueue);
    print_queue("Suspensas", sleepQueue);
#endif
}

void after_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
    print_queue("Prontas" ,readyQueue);
    print_queue("Suspensas", sleepQueue);
#endif
}

void before_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
    print_queue("Prontas" ,readyQueue);
    print_queue("Suspensas", sleepQueue);
#endif

}

void after_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
    print_queue("Prontas" ,readyQueue);
    print_queue("Suspensas", sleepQueue);
#endif
}

void before_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif
}

int before_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}


int before_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

//------------------------------------------------------------ ESCALONAMENTO -----------------------------------------------------------------

task_t* scheduler(void) {
    if (!readyQueue)
        return NULL;

    task_t *head = readyQueue->next;
    task_t *current = head;
    task_t *best = head;

    #ifdef SCHEDULER_DEBUG
        printf("[scheduler] Iniciando escalonamento de tarefas\n");
        print_queue("Antes", readyQueue);
    #endif


    while (current != head || current == best) {
        // Find the best task
        if (current->prio_dynamic < best->prio_dynamic ||
            (current->prio_dynamic == best->prio_dynamic && current->prio_static < best->prio_static)) {
            best = current;
        }
        
        current->prio_dynamic = DEFINE_PRIO(current->prio_dynamic + GROWTH_FACTOR);
        current = current->next;
        // Break after one full traversal
        if (current == head) {
            break;
        }
    }

    #ifdef SCHEDULER_DEBUG
        printf("[scheduler] Melhor tarefa selecionada: %d com prioridade dinâmica %d e estática %d\n",
            best->id, best->prio_dynamic, best->prio_static);

        print_queue("Depois", readyQueue);
    #endif

    // Restore dynamic priority of the best task and remove from queue
    best->prio_dynamic = best->prio_static;
    queue_remove((queue_t**)&readyQueue, (queue_t*)best);

    return best;
}


//------------------------------------------------------------ PRIORIDADE -----------------------------------------------------------------

void task_setprio(task_t* task, int prio) {
    
    task_t* target_task = task ? task : taskExec;

    if (!target_task) {
        return;
    }
    
    prio = DEFINE_PRIO(prio);
    task->prio_static = prio;
    task->prio_dynamic = prio;

    #ifdef DEBUG
    printf("\n[task_setprio] Tarefa %d definida com prioridade estática %d\n", task->id, prio);
    #endif
}

int task_getprio(task_t* task) {
    return task ? task->prio_static : (taskExec ? taskExec->prio_static : 21);
}


//------------------------------------------------------------ TEMPO E EXECUÇÃO -----------------------------------------------------------------

unsigned int systime() {
    return _systemTime;
}

//------------------------------------------------------------ TRATADORES E TIMER -----------------------------------------------------------------

void handleTimer(int signum) {
    // Verifica se a tarefa atual está definida
    if (!taskExec) {
        fprintf(stderr, "handleTimer: taskExec não pode ser NULL\n");
        return;
    }

    // Atualiza tempos
    taskExec->running_time++;
    _systemTime++;

    // Evita preempção do dispatcher (tarefa id == 1)
    if (taskExec->id == 1) {
        return;
    }

    // Se quantum acabou, devolve o processador
    if (--taskExec->quantum == 0) {
        taskExec->quantum = QUANTUM;
        #ifdef DEBUG
            printf("[handleTimer] Tarefa %d atingiu fim do quantum. Chamando task_yield().\n", taskExec->id);
        #endif
        task_yield();
        return;
    }

    // Mensagem de depuração, se ativada
    #ifdef DEBUG
        printf("[handleTimer] Tarefa %d: Quantum restante: %d | Tempo do sistema: %d\n", 
               taskExec->id, taskExec->quantum, _systemTime);
    #endif
}


//------------------------------------------------------------ INICIALIZAÇÃO DE TAREFAS -----------------------------------------------------------------

void print_queue(char* nome , task_t* queue) {
    
    printf("\n%s:\n", nome);
    
    if (!queue) {
        printf("\n <-- Fila Vazia --> \n");
        return;
    }

    task_t* current = queue;

    do {
        printf("\n[print_tcb] Tarefa %d: ", current->id);
        printf("Estado: %c | Prioridade Statica: %d | Prioridade Dinamica: %d | Tempo de execução: %d | Tempo de criação: %d | Tempo de morte: %d | Quantum: %d | Ativações: %d\n", 
               current->state, current->prio_static, current->prio_dynamic, current->running_time, current->task_create_time, current->task_death_time, current->quantum, current->activations);
        current = current->next;
    } while (current != queue);

    printf("\n");
}