#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

static unsigned int seed_global;

static int rand_between(int a, int b) {
    return a + rand_r(&seed_global) % (b - a + 1);
}

/* ---------- Scenario selector ---------- */
void usage(const char *p) {
    printf("Usage: %s [traffic|elevator|spooler|db|all]\n", p);
    exit(1);
}

/* ---------- Traffic Intersection ---------- */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond[4];
    int waiting[4];
    int light; 
    int green_allow;
    int running;
} Intersection;

void intersection_init(Intersection *i) {
    pthread_mutex_init(&i->mutex, NULL);
    for (int d=0; d<4; d++) pthread_cond_init(&i->cond[d], NULL);
    for (int d=0; d<4; d++) i->waiting[d]=0;
    i->light=0;
    i->green_allow=3;
    i->running=1;
}

void *traffic_light_thread(void *arg) {
    Intersection *I = arg;
    while (1) {
        pthread_mutex_lock(&I->mutex);
        if (!I->running) { pthread_mutex_unlock(&I->mutex); break; }
        int dir = I->light;
        pthread_mutex_unlock(&I->mutex);
        usleep(800000);
        pthread_mutex_lock(&I->mutex);
        I->light = (I->light + 1) % 4;
        int allow = I->green_allow;
        for (int k=0;k<allow;k++) pthread_cond_broadcast(&I->cond[dir]);
        pthread_mutex_unlock(&I->mutex);
        usleep(500000);
    }
    return NULL;
}

void *vehicle_thread(void *arg) {
    Intersection *I = arg;
    int id = rand_between(1,10000);
    int dir = rand_between(0,3);
    pthread_mutex_lock(&I->mutex);
    I->waiting[dir]++;
    while (I->light != dir && I->running) {
        pthread_cond_wait(&I->cond[dir], &I->mutex);
    }
    if (!I->running) {
        I->waiting[dir]--;
        pthread_mutex_unlock(&I->mutex);
        return NULL;
    }
    I->waiting[dir]--;
    pthread_mutex_unlock(&I->mutex);
    usleep(rand_between(100000,300000));
    return NULL;
}

void run_traffic(int vehicles) {
    Intersection I;
    intersection_init(&I);
    pthread_t light;
    pthread_create(&light, NULL, traffic_light_thread, &I);
    pthread_t vt[vehicles];
    for (int i=0;i<vehicles;i++) {
        usleep(rand_between(20000,100000));
        pthread_create(&vt[i], NULL, vehicle_thread, &I);
    }
    for (int i=0;i<vehicles;i++) pthread_join(vt[i], NULL);
    pthread_mutex_lock(&I.mutex);
    I.running = 0;
    for (int d=0;d<4;d++) pthread_cond_broadcast(&I.cond[d]);
    pthread_mutex_unlock(&I.mutex);
    pthread_join(light, NULL);
    printf("Traffic scenario finished\n");
}

/* ---------- Elevator System ---------- */
#define MAX_FLOORS 10
#define MAX_ELEV 3
typedef struct {
    int floor;
    int target[MAX_FLOORS];
    int pending;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int running;
} Elevator;

typedef struct {
    Elevator elev[MAX_ELEV];
    int requests[MAX_FLOORS];
    pthread_mutex_t req_mutex;
} ElevatorSystem;

void elevator_system_init(ElevatorSystem *S) {
    pthread_mutex_init(&S->req_mutex, NULL);
    for (int e=0;e<MAX_ELEV;e++) {
        S->elev[e].floor = 0;
        S->elev[e].pending = 0;
        S->elev[e].running = 1;
        pthread_mutex_init(&S->elev[e].mutex, NULL);
        pthread_cond_init(&S->elev[e].cond, NULL);
        for (int f=0;f<MAX_FLOORS;f++) S->elev[e].target[f]=0;
    }
    for (int f=0;f<MAX_FLOORS;f++) S->requests[f]=0;
}

void request_elevator(ElevatorSystem *S, int floor) {
    pthread_mutex_lock(&S->req_mutex);
    S->requests[floor]++;
    int best = -1;
    int bestdist = 1e9;
    for (int e=0;e<MAX_ELEV;e++) {
        pthread_mutex_lock(&S->elev[e].mutex);
        int dist = abs(S->elev[e].floor - floor) + (S->elev[e].pending)*2;
        pthread_mutex_unlock(&S->elev[e].mutex);
        if (dist < bestdist) { bestdist=dist; best=e; }
    }
    if (best>=0) {
        pthread_mutex_lock(&S->elev[best].mutex);
        S->elev[best].target[floor]=1;
        S->elev[best].pending++;
        pthread_cond_signal(&S->elev[best].cond);
        pthread_mutex_unlock(&S->elev[best].mutex);
        S->requests[floor]--;
    }
    pthread_mutex_unlock(&S->req_mutex);
}

void *elevator_thread(void *arg) {
    Elevator *E = arg;
    while (1) {
        pthread_mutex_lock(&E->mutex);
        while (!E->pending && E->running) pthread_cond_wait(&E->cond, &E->mutex);
        if (!E->running && !E->pending) { pthread_mutex_unlock(&E->mutex); break; }
        int next = -1;
        for (int f=0; f<MAX_FLOORS; f++) if (E->target[f]) { next=f; break; }
        if (next==-1) { pthread_mutex_unlock(&E->mutex); continue; }
        E->target[next]=0;
        E->pending--;
        pthread_mutex_unlock(&E->mutex);
        while (E->floor != next) {
            if (E->floor < next) { E->floor++; }
            else { E->floor--; }
            usleep(150000);
        }
        usleep(100000);
    }
    return NULL;
}

void run_elevator(int nrequests) {
    ElevatorSystem S;
    elevator_system_init(&S);
    pthread_t et[MAX_ELEV];
    for (int e=0;e<MAX_ELEV;e++) pthread_create(&et[e], NULL, elevator_thread, &S.elev[e]);
    for (int i=0;i<nrequests;i++) {
        int floor = rand_between(0, MAX_FLOORS-1);
        request_elevator(&S, floor);
        usleep(rand_between(50000,200000));
    }
    for (int e=0;e<MAX_ELEV;e++) {
        pthread_mutex_lock(&S.elev[e].mutex);
        S.elev[e].running = 0;
        pthread_cond_signal(&S.elev[e].cond);
        pthread_mutex_unlock(&S.elev[e].mutex);
    }
    for (int e=0;e<MAX_ELEV;e++) pthread_join(et[e], NULL);
    printf("Elevator scenario finished\n");
}

/* ---------- Print Spooler ---------- */
typedef struct Job {
    int id;
    int pages;
    struct Job *next;
} Job;

typedef struct {
    Job *head, *tail;
    int count;
    int cap;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    int running;
} Spooler;

void spooler_init(Spooler *S, int cap) {
    S->head=S->tail=NULL; S->count=0; S->cap=cap; S->running=1;
    pthread_mutex_init(&S->mutex, NULL);
    pthread_cond_init(&S->not_empty, NULL);
    pthread_cond_init(&S->not_full, NULL);
}

void submit_job(Spooler *S, Job *j) {
    pthread_mutex_lock(&S->mutex);
    while (S->count == S->cap && S->running) pthread_cond_wait(&S->not_full, &S->mutex);
    if (!S->running) { pthread_mutex_unlock(&S->mutex); return; }
    j->next = NULL;
    if (S->tail == NULL) S->head = S->tail = j;
    else { S->tail->next = j; S->tail = j; }
    S->count++;
    pthread_cond_signal(&S->not_empty);
    pthread_mutex_unlock(&S->mutex);
}

Job *fetch_job(Spooler *S) {
    pthread_mutex_lock(&S->mutex);
    while (S->count == 0 && S->running) pthread_cond_wait(&S->not_empty, &S->mutex);
    if (S->count == 0 && !S->running) { pthread_mutex_unlock(&S->mutex); return NULL; }
    Job *j = S->head;
    S->head = j->next;
    if (S->head==NULL) S->tail=NULL;
    S->count--;
    pthread_cond_signal(&S->not_full);
    pthread_mutex_unlock(&S->mutex);
    return j;
}

void *printer_thread(void *arg) {
    Spooler *S = arg;
    while (1) {
        Job *j = fetch_job(S);
        if (!j) break;
        usleep(50000 * (j->pages));
        free(j);
    }
    return NULL;
}

void *user_thread(void *arg) {
    Spooler *S = arg;
    for (int i=0;i<5;i++) {
        Job *j = malloc(sizeof(Job));
        j->id = rand_between(1,10000);
        j->pages = rand_between(1,5);
        submit_job(S, j);
        usleep(rand_between(50000,200000));
    }
    return NULL;
}

void run_spooler(int users, int printers) {
    Spooler S;
    spooler_init(&S, 10);
    pthread_t up[users], pp[printers];
    for (int i=0;i<printers;i++) pthread_create(&pp[i], NULL, printer_thread, &S);
    for (int i=0;i<users;i++) pthread_create(&up[i], NULL, user_thread, &S);
    for (int i=0;i<users;i++) pthread_join(up[i], NULL);
    pthread_mutex_lock(&S.mutex);
    S.running = 0;
    pthread_cond_broadcast(&S.not_empty);
    pthread_cond_broadcast(&S.not_full);
    pthread_mutex_unlock(&S.mutex);
    for (int i=0;i<printers;i++) pthread_join(pp[i], NULL);
    printf("Spooler scenario finished\n");
}

/* ---------- Database Connection Pool ---------- */
typedef struct {
    int cap;
    int used;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} DBPool;

void dbpool_init(DBPool *P, int cap) {
    P->cap = cap; P->used = 0;
    pthread_mutex_init(&P->mutex, NULL);
    pthread_cond_init(&P->cond, NULL);
}

void db_acquire(DBPool *P) {
    pthread_mutex_lock(&P->mutex);
    while (P->used == P->cap) pthread_cond_wait(&P->cond, &P->mutex);
    P->used++;
    pthread_mutex_unlock(&P->mutex);
}

void db_release(DBPool *P) {
    pthread_mutex_lock(&P->mutex);
    P->used--;
    pthread_cond_signal(&P->cond);
    pthread_mutex_unlock(&P->mutex);
}

void *db_worker(void *arg) {
    DBPool *P = arg;
    db_acquire(P);
    usleep(rand_between(100000,400000));
    db_release(P);
    return NULL;
}

void run_dbpool(int threads, int poolsize) {
    DBPool P;
    dbpool_init(&P, poolsize);
    pthread_t t[threads];
    for (int i=0;i<threads;i++) pthread_create(&t[i], NULL, db_worker, &P);
    for (int i=0;i<threads;i++) pthread_join(t[i], NULL);
    printf("DB pool scenario finished\n");
}

/* ---------- main ---------- */
int main(int argc, char **argv) {
    seed_global = time(NULL) ^ getpid();
    if (argc < 2) usage(argv[0]);
    if (strcmp(argv[1], "traffic")==0) {
        run_traffic(20);
    } else if (strcmp(argv[1], "elevator")==0) {
        run_elevator(25);
    } else if (strcmp(argv[1], "spooler")==0) {
        run_spooler(4, 2);
    } else if (strcmp(argv[1], "db")==0) {
        run_dbpool(12, 4);
    } else if (strcmp(argv[1], "all")==0) {
        run_traffic(20);
        run_elevator(25);
        run_spooler(4, 2);
        run_dbpool(12,4);
    } else usage(argv[0]);
    return 0;
}
