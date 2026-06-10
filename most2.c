#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define NTHREADS 30
#define MAX_SERIA 5 

typedef struct {
    int priority;
    int id;
} Auto;

typedef struct {
    Auto auta[NTHREADS];
    int size;
} WaitList;

pthread_mutex_t most_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_most = PTHREAD_COND_INITIALIZER;

WaitList pq_A; 
WaitList pq_B; 


bool most_zajety = false;
int id_na_moscie = -1;
int kierunek_na_moscie = 0; 

int preferowany_kierunek = 0; 
int licznik_serii = 0;        
int global_ticket = 0;        


int czekal[NTHREADS]; 
int miasto[NTHREADS]; 


int auta_w_miescie[2] = {0, 0};
int auta_w_kolejce[2] = {0, 0};


void swap_nodes(Auto* a, Auto* b) {
    Auto temp = *a;
    *a = *b;
    *b = temp;
}

void pq_push(WaitList* pq, int priority, int id) {
    if (pq->size >= NTHREADS) return;
    int current = pq->size;
    pq->auta[current].priority = priority;
    pq->auta[current].id = id;
    pq->size++;
    
    while (current > 0) {
        int parent = (current - 1) / 2;
        if (pq->auta[current].priority <= pq->auta[parent].priority) break;
        swap_nodes(&pq->auta[current], &pq->auta[parent]);
        current = parent;
    }
}

void pq_pop(WaitList* pq, Auto* out_node) {
    if (pq->size == 0) return;
    *out_node = pq->auta[0];
    pq->size--;
    pq->auta[0] = pq->auta[pq->size];
    int current = 0;
    while (1) {
        int left_child = 2 * current + 1;
        int right_child = 2 * current + 2;
        int largest = current;
        if (left_child < pq->size && pq->auta[left_child].priority > pq->auta[largest].priority) largest = left_child;
        if (right_child < pq->size && pq->auta[right_child].priority > pq->auta[largest].priority) largest = right_child;
        if (largest == current) break;
        swap_nodes(&pq->auta[current], &pq->auta[largest]);
        current = largest;
    }
}


void wyswietl() {
    int poza_kolejka_A = auta_w_miescie[0] - auta_w_kolejce[0];
    int poza_kolejka_B = auta_w_miescie[1] - auta_w_kolejce[1];
    
    printf("A-%d %d>>> ", poza_kolejka_A, auta_w_kolejce[0]);
    if (most_zajety) {
        printf(kierunek_na_moscie == 0 ? "[>> %d >>]" : "[<< %d <<]", id_na_moscie);
    } else {
        printf("[  PUSTY  ]");
    }
    printf(" <<<%d %d-B\n", auta_w_kolejce[1], poza_kolejka_B);
    fflush(stdout);
	
}

void* przejazd(void* arg) {
    int id = *(int*)arg;
    free(arg);

    while(1) {
        
        sleep(rand() % 3 + 1);

        pthread_mutex_lock(&most_mutex);
        
        int moj_kierunek = miasto[id]; 
        int przeciwny_kierunek = 1 - moj_kierunek;

        auta_w_kolejce[moj_kierunek]++;
        global_ticket++;
        
        
        czekal[id] = 10000 - global_ticket; 

        
        if (moj_kierunek == 0) {
            pq_push(&pq_A, czekal[id], id);
        } else {
            pq_push(&pq_B, czekal[id], id);
        }
        
        wyswietl();

        
        if (moj_kierunek == 0) {
            while (most_zajety || preferowany_kierunek != 0 || pq_A.auta[0].id != id) {
                if (!most_zajety && preferowany_kierunek != 0 && pq_B.size == 0) {
                    preferowany_kierunek = 0;
                    licznik_serii = 0;
                    continue;
                }
                pthread_cond_wait(&cond_most, &most_mutex);
            }
            Auto usuniete;
            pq_pop(&pq_A, &usuniete);
        } else {
            while (most_zajety || preferowany_kierunek != 1 || pq_B.auta[0].id != id) {
                if (!most_zajety && preferowany_kierunek != 1 && pq_A.size == 0) {
                    preferowany_kierunek = 1;
                    licznik_serii = 0;
                    continue;
                }
                pthread_cond_wait(&cond_most, &most_mutex);
            }
            Auto usuniete;
            pq_pop(&pq_B, &usuniete);
        }

        
        most_zajety = true;
        id_na_moscie = id;
        kierunek_na_moscie = moj_kierunek;
        licznik_serii++;
        auta_w_kolejce[moj_kierunek]--;

        wyswietl();
        pthread_mutex_unlock(&most_mutex);

        
        usleep(80000);

        
        pthread_mutex_lock(&most_mutex);
        
        auta_w_miescie[moj_kierunek]--;
        miasto[id] = przeciwny_kierunek;
        auta_w_miescie[przeciwny_kierunek]++;

        most_zajety = false;
        id_na_moscie = -1;

        if (moj_kierunek == 0) {
            if (pq_A.size == 0 || (licznik_serii >= MAX_SERIA && pq_B.size > 0)) {
                preferowany_kierunek = 1;
                licznik_serii = 0;
            }
        } else {
            if (pq_B.size == 0 || (licznik_serii >= MAX_SERIA && pq_A.size > 0)) {
                preferowany_kierunek = 0;
                licznik_serii = 0;
            }
        }

        wyswietl();
        
        pthread_cond_broadcast(&cond_most);
        pthread_mutex_unlock(&most_mutex);
    }
    return NULL;
}

int main() {
    srand(time(NULL));
    pq_A.size = 0;
    pq_B.size = 0;

    pthread_t car[NTHREADS];
    
    pthread_mutex_lock(&most_mutex);
    for(int i = 0; i < NTHREADS; i++) {
        miasto[i] = rand() % 2; 
        auta_w_miescie[miasto[i]]++;
        czekal[i] = 0;
    }
    preferowany_kierunek = rand() % 2;
    pthread_mutex_unlock(&most_mutex);

    
    for(int i = 0; i < NTHREADS; i++) {
        int* id_alloc = malloc(sizeof(int));
        *id_alloc = i;
        pthread_create(&car[i], NULL, przejazd, id_alloc);
    }
	
    for(int j = 0; j < NTHREADS; j++) {
        pthread_join(car[j], NULL); 
    }

    return 0;
}