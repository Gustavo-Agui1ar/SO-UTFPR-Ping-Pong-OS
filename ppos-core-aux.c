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

//------------------------------------------------------------ funcoes criadas para o escalonamento-----------------------------------------------------------------------------


task_t* scheduler() {

    //encontra o elemento de maior prioridade
    task_t* task = findByPriority(readyQueue);
    //PRINT_READY_QUEUE    
    
    if(!task) return NULL;
    
    #ifdef DEBUG
    printf("\nscheduler: - Maior prioridade: %d - Tarefa: %d\n", task->prio_dynamic, task->id);
    #endif
 
    //envelhece as outras tasks;
    growAll(readyQueue);
    //devolve a task sua prioridade original
    task_setDynamicPrio(task, task->prio_static);
    
  return task;
}

task_t* findByPriority(task_t* queue) {

    //fila sem elementos
    if(!queue)
    return NULL;

    if(queue->next == queue)
    return queue;

    task_t* currentQ = queue;
    task_t* highestPrio = queue;

    do {//escolhe pelo de maior prioridade dinamica, estatica, e por quem foi criado antes

        highestPrio = verifyPriority(currentQ, highestPrio);
        currentQ = currentQ->next; 
    } while (currentQ != queue);  // Continua até voltar ao início da fila



    return highestPrio;
}

task_t* verifyPriority(task_t* current, task_t* highest) {
    
    if(current->prio_dynamic < highest->prio_dynamic) 
    return current;
    
    if(current->prio_dynamic > highest->prio_dynamic)
    return highest;
    
    //se passou pelos 2 ifs significa que dynamic prio é igual
    
    if(current->prio_static < highest->prio_static)
    return current;
    
    if(current->prio_static > highest->prio_static)
    return highest;
    
    //se passou pelos 2 ifs significa que static prio é iguasl
    
    if(current->id < highest->id)
    return current;
    
    return highest;
}

void growAll(task_t* queue) {
    
    //fila sem elementos
    if(!queue)
    return;
    
    task_t* currentQ = queue;
    
    do {
        task_setDynamicPrio(currentQ, currentQ->prio_dynamic + GROWTH_FACTOR);
        currentQ = currentQ->next;
    } while(currentQ->next != queue);
}


//------------------------------------------------------------ funcoes criadas para prioridade-----------------------------------------------------------------------------

void task_setDynamicPrio(task_t* task, int prio) {
    
    //verifica os limites de prio
    if(!task) {
        return;
    }

    #ifdef DEBUG
    //printf("\n set dynamic priority: -  priority: %d - task: %d\n", prio, task->id);
    #endif
    
    if(prio > PRIO_MAX_TASK)
    prio = PRIO_MAX_TASK;
    
    if(prio < PRIO_MIN_TASK)
    prio = PRIO_MIN_TASK;
    
    task->prio_dynamic = prio;
}

void task_setprio (task_t *task, int prio) {

     //ajusta a prioridade 
    if(prio > PRIO_MAX_TASK)
        prio = PRIO_MAX_TASK;
    
    if(prio < PRIO_MIN_TASK)
        prio = PRIO_MIN_TASK;

    
    if(!task) {
        if(taskExec) {
            taskExec->prio_static  = prio;
            taskExec->prio_dynamic = prio;
            #ifdef DEBUG
            printf("\n task_setprio: -  priority: %d - dynamic: %d - task: %d\n", task->prio_static, task->prio_dynamic, task->id);
            #endif   
        }
        return;
    }

    task->prio_static  = prio;
    task->prio_dynamic = prio;

    #ifdef DEBUG
    printf("\n task_setprio: -  priority: %d - dynamic: %d - task: %d\n", task->prio_static, task->prio_dynamic, task->id);
    #endif
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task) {
  if(!task && taskExec)
        return taskExec->prio_static; 
    if(task)
        return task->prio_static;
    return 21;  
}

unsigned int systime() {
  return _systemTime;
}

int task_get_eet(task_t* task) {
    if(!task)
        return 0;
    return task->running_time;
}

void task_set_eet(task_t* task, int eet) {
    if(!task)
        return;
    task->running_time = eet;
}

void handleTimer(int signum) {
  
    #ifdef DEBUG
      //printf("\nhandleTimer: - [%d] Quantum: %d - Time: %d\n", taskExec->id, taskExec->quantum, _systemTime);
    #endif
    
  taskExec->running_time++;
  taskExec->quantum--;
  _systemTime++;

  if(taskExec == NULL)
    return;
  
  if(taskExec->quantum <= 0) {
    task_yield();
    return;
  }
}

int initHandleTimer() {
  
  //define o tratador de interrupções do timer
  action.sa_handler = handleTimer;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  
  if(sigaction(SIGALRM, &action, 0) < 0) {
    return -1;
  }

  return 0;
}

int initTimer() {
   
  //define um timer para a preempção por tempo
   timer.it_value.tv_usec = 1000 ;       // primeiro disparo, em micro-segundos
   timer.it_value.tv_sec  = 0 ;          // primeiro disparo, em segundos
   timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos
   timer.it_interval.tv_sec  = 0 ;       // disparos subsequentes, em segundos
   
   if(setitimer(ITIMER_REAL, &timer, 0) < 0) {
      return -1;
   }

   return 0;
}

void task_setProperties(task_t* task, int priority) {

  if(!task) {
    perror("task_setProperties: task must no be a null\n");
    exit(1);
  }

  task->state = PPOS_TASK_STATE_READY;
  task->quantum = QUANTUM;
  task->task_create_time = systime();
  task->running_time = 0;
  task->activations = 0;
  task_setprio(task, priority);
}