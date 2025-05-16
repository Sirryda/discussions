#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define NUM_BUYERS     25          // Max number of buyers
#define TOTAL_TICKETS  70          // Max number of tickets available
#define MAX_REQUEST    6           // Max number of tickets a buyer can purchase

sem_t tickets_sem;                 // Remaining tickets
sem_t print_sem;                   // Output
sem_t order_sem[NUM_BUYERS];       // Keeps buyers in order
sem_t soldout_sem;                 // Tickets are sold out

// Thread function for each buyer
void* buyer_thread(void* arg) {
    int id = *(int*)arg;
    free(arg);
    unsigned int seed = (unsigned)time(NULL) ^ (id * 0x9e3779b9);

    // Thread-local seed for rand_r()
    sem_wait(&order_sem[id - 1]);

    // Decides how many tickets to purchase
    int want = rand_r(&seed) % MAX_REQUEST + 1;
    int bought = 0;

    // Trying to buy number of tickets available
    for (int i = 0; i < want; i++) {
        if (sem_trywait(&tickets_sem) == 0) {
            bought++;
        } else {
            break;  // Tickets are sold out
        }
    }

    // Checks to see how many tickets are still left
    int remaining;
    sem_getvalue(&tickets_sem, &remaining);

    // Prints the results
    sem_wait(&print_sem);
    if (bought > 0) {
        printf("Buyer %d has bought %d tickets. There are %d tickets remaining.\n", id, bought, remaining);
    } else {
        if (sem_trywait(&soldout_sem) ==  0) {
            printf("\n ** Tickets are sold out. ** \n");
        }
    }

    sem_post(&print_sem);

    // Pauses 1 second before next buyer can go
    sleep(1);

    // signal next buyer in sequence
    if (id < NUM_BUYERS) {
        sem_post(&order_sem[id]);
    }

    return NULL;
}

int main() {
    pthread_t buyers[NUM_BUYERS];
    srand((unsigned)time(NULL));

    // Initialize semaphores
    sem_init(&tickets_sem, 0, TOTAL_TICKETS);
    sem_init(&print_sem,   0, 1);

    // Initialize ordering semaphores
    for (int i = 0; i < NUM_BUYERS; i++) {
        // buyer 1 starts immediately, others wait
        sem_init(&order_sem[i], 0, (i == 0) ? 1 : 0);
    }
    sem_init(&soldout_sem, 0, 1);

    // Launches all the buyer threads
    for (int i = 0; i < NUM_BUYERS; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&buyers[i], NULL, buyer_thread, id);
    }

    // Waits for each buyer to finish
    for (int i = 0; i < NUM_BUYERS; i++) {
        pthread_join(buyers[i], NULL);
    }

    // Cleans up all semaphores
    sem_destroy(&tickets_sem);
    sem_destroy(&print_sem);
    for (int i = 0; i < NUM_BUYERS; i++) {
        sem_destroy(&order_sem[i]);
    }

    return 0;
}
