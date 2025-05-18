#include "ppos.h"
#include "ppos-core-globals.h"
#include "ppos-disk-manager.h"
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

unsigned int _systemTime = 0;

struct itimerval timer;
struct sigaction action;

task_t* verifyPriority(task_t* current, task_t* highest);
void task_setProperties(task_t* task, int priority);
void task_setDynamicPrio(task_t* task, int prio);
task_t* findByPriority(task_t* queue);
void growAll(task_t* queue);
int initHandleTimer();
void handleTimer(int signum);
int initTimer();


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
    _systemTime = 0;
   
    task_setProperties(taskMain, 20);
    
    if(!taskMain) {
        perror("error initializing task main\n");
        exit(1);
    }

    if(initTimer()) {
        perror("ppos_init: error in setitimer");
        exit(1);
    }
  
    if(initHandleTimer()) {
        perror("ppos_init: error to define a handle timer");
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
    task_setProperties(task, 0);
}

void before_task_exit () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_exit () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif
    
}

void before_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif
    
}

void after_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif
}

void before_task_yield () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif
}
void after_task_yield () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
}


void before_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif
}

void after_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif
}

void before_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif
}

void after_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
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

task_t* scheduler() {
    // Encontra a task com maior prioridade
    task_t* selected_task = findByPriority(readyQueue);
    
    if (!selected_task)
        return NULL;

    #ifdef DEBUG
    printf("\n[scheduler] Selecionada tarefa %d com prioridade dinâmica %d\n", 
           selected_task->id, selected_task->prio_dynamic);
    #endif

    // Envelhece todas as tarefas (exceto a selecionada)
    growAll(readyQueue);

    // Restaura a prioridade dinâmica da tarefa selecionada
    task_setDynamicPrio(selected_task, selected_task->prio_static);

    return selected_task;
}

task_t* findByPriority(task_t* queue) {
    if (!queue)
        return NULL;

    if (queue->next == queue)
        return queue;

    task_t* current = queue;
    task_t* best = queue;

    // Varre a fila circular procurando a task com maior prioridade
    do {
        best = verifyPriority(current, best);
        current = current->next;
    } while (current != queue);

    return best;
}

task_t* verifyPriority(task_t* current, task_t* best) {
    if (current->prio_dynamic < best->prio_dynamic)
        return current;

    if (current->prio_dynamic > best->prio_dynamic)
        return best;

    if (current->prio_static < best->prio_static)
        return current;

    if (current->prio_static > best->prio_static)
        return best;

    if (current->id < best->id)
        return current;

    return best;
}

void growAll(task_t* queue) {
    if (!queue)
        return;

    task_t* current = queue;

    do {
        // Evita envelhecer a task que será executada (ela ainda não foi retirada da fila)
        if (current != taskExec) {
            task_setDynamicPrio(current, current->prio_dynamic + GROWTH_FACTOR);
            #ifdef DEBUG
            printf("[growAll] Tarefa %d envelheceu para prioridade %d\n", current->id, current->prio_dynamic);
            #endif
        }
        current = current->next;
    } while (current != queue);
}


//------------------------------------------------------------ PRIORIDADE -----------------------------------------------------------------

void task_setDynamicPrio(task_t* task, int prio) {
    if (!task)
        return;

    // Limita prioridade dentro dos valores permitidos
    if (prio > PRIO_MAX_TASK) prio = PRIO_MAX_TASK;
    if (prio < PRIO_MIN_TASK) prio = PRIO_MIN_TASK;

    task->prio_dynamic = prio;

    #ifdef DEBUG
    printf("[task_setDynamicPrio] Tarefa %d nova prioridade dinâmica: %d\n", task->id, task->prio_dynamic);
    #endif
}

void task_setprio(task_t* task, int prio) {
    if (prio > PRIO_MAX_TASK) prio = PRIO_MAX_TASK;
    if (prio < PRIO_MIN_TASK) prio = PRIO_MIN_TASK;

    if (!task) {
        if (taskExec) {
            taskExec->prio_static = prio;
            taskExec->prio_dynamic = prio;
            #ifdef DEBUG
            printf("[task_setprio] Tarefa atual %d definida com prioridade estática %d\n", taskExec->id, prio);
            #endif
        }
        return;
    }

    task->prio_static = prio;
    task->prio_dynamic = prio;

    #ifdef DEBUG
    printf("[task_setprio] Tarefa %d definida com prioridade estática %d\n", task->id, prio);
    #endif
}

int task_getprio(task_t* task) {
    if (!task && taskExec)
        return taskExec->prio_static;

    if (task)
        return task->prio_static;

    return 21; // valor padrão se não houver contexto
}


//------------------------------------------------------------ TEMPO E EXECUÇÃO -----------------------------------------------------------------

unsigned int systime() {
    return _systemTime;
}

int task_get_eet(task_t* task) {
    if (!task)
        return 0;

    return task->running_time;
}

void task_set_eet(task_t* task, int eet) {
    if (!task)
        return;

    task->running_time = eet;

    #ifdef DEBUG
    printf("[task_set_eet] Tarefa %d tempo de execução definido para %d\n", task->id, eet);
    #endif
}


//------------------------------------------------------------ TRATADORES E TIMER -----------------------------------------------------------------

void handleTimer(int signum) {
    if (!taskExec)
        return;

    taskExec->running_time++;
    taskExec->quantum--;
    _systemTime++;

    #ifdef DEBUG
        printf("[handleTimer] Tarefa %d: Quantum %d | Tempo sistema: %d\n", 
           taskExec->id, taskExec->quantum, _systemTime);
    #endif

    if (taskExec->quantum <= 0) {
        taskExec->quantum = QUANTUM;
        task_yield(); 
    }
}

int initHandleTimer() {
    action.sa_handler = handleTimer;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, 0) < 0) {
        perror("Erro ao configurar tratador de sinal");
        return -1;
    }

    return 0;
}

int initTimer() {
    timer.it_value.tv_usec = 1000;
    timer.it_value.tv_sec = 0;
    timer.it_interval.tv_usec = 1000;
    timer.it_interval.tv_sec = 0;

    if (setitimer(ITIMER_REAL, &timer, 0) < 0) {
        perror("Erro ao iniciar timer");
        return -1;
    }

    return 0;
}


//------------------------------------------------------------ INICIALIZAÇÃO DE TAREFAS -----------------------------------------------------------------

void task_setProperties(task_t* task, int priority) {
    if (!task) {
        perror("task_setProperties: tarefa não pode ser NULL\n");
        exit(1);
    }

    task->state = PPOS_TASK_STATE_READY;
    task->quantum = QUANTUM;
    task->task_create_time = systime();
    task->running_time = 0;
    task->activations = 0;
    task_setprio(task, priority);

    #ifdef DEBUG
    printf("[task_setProperties] Tarefa %d criada com prioridade %d\n", task->id, priority);
    #endif
}
