// concert_ticket_sales_ordered.c
// Compile with: gcc -pthread -o concert_ticket_sales_ordered concert_ticket_sales_ordered.c

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define NUM_BUYERS     25
#define TOTAL_TICKETS  70
#define MAX_REQUEST    6  // max tickets any buyer will attempt to buy

sem_t tickets_sem;                 // counts remaining tickets
sem_t print_sem;                   // serializes output
sem_t order_sem[NUM_BUYERS];       // enforces buyer order (1→2→…→10)
sem_t soldout_sem;                 // once tickets are sold out

void* buyer_thread(void* arg) {
    int id = *(int*)arg;
    free(arg);

    // thread‐local PRNG seed
    unsigned int seed = (unsigned)time(NULL) ^ (id * 0x9e3779b9);

    // wait for my turn (buyer 1 goes first, then 2, etc.)
    sem_wait(&order_sem[id - 1]);

    // decide how many tickets to request
    int want = rand_r(&seed) % MAX_REQUEST + 1;
    int bought = 0;

    // try to buy up to 'want' tickets
    for (int i = 0; i < want; i++) {
        if (sem_trywait(&tickets_sem) == 0) {
            bought++;
        } else {
            break;  // sold out
        }
    }

    // get remaining count
    int remaining;
    sem_getvalue(&tickets_sem, &remaining);

    sem_wait(&print_sem);
    if (bought > 0) {
        printf("Buyer %d has bought %d tickets. There are %d tickets remaining.\n", id, bought, remaining);
    } else {
        if (sem_trywait(&soldout_sem) ==  0) {
            printf("\n ** Tickets are sold out. ** \n");
        }
    }

    sem_post(&print_sem);

    sleep(1);

    // signal next buyer (if any)
    if (id < NUM_BUYERS) {
        sem_post(&order_sem[id]);
    }

    return NULL;
}

int main() {
    pthread_t buyers[NUM_BUYERS];
    srand((unsigned)time(NULL));

    // initialize semaphores
    sem_init(&tickets_sem, 0, TOTAL_TICKETS);
    sem_init(&print_sem,   0, 1);
    for (int i = 0; i < NUM_BUYERS; i++) {
        // buyer 1 starts immediately, others wait
        sem_init(&order_sem[i], 0, (i == 0) ? 1 : 0);
    }
    sem_init(&soldout_sem, 0, 1);

    // launch buyer threads
    for (int i = 0; i < NUM_BUYERS; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&buyers[i], NULL, buyer_thread, id);
    }

    // wait for all buyers to finish
    for (int i = 0; i < NUM_BUYERS; i++) {
        pthread_join(buyers[i], NULL);
    }

    // clean up
    sem_destroy(&tickets_sem);
    sem_destroy(&print_sem);
    for (int i = 0; i < NUM_BUYERS; i++) {
        sem_destroy(&order_sem[i]);
    }

    return 0;
}
