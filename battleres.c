// battle_rez_semaphore_one_standby.c
// Compile with: gcc -o battle_rez_semaphore battle_rez_semaphore_one_standby.c -pthread

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define NUM_PLAYERS 5
#define ROUNDS      5

sem_t death_event;    // wake everyone on a death
sem_t rez_token;      // grants exactly one rez
sem_t standby_token;  // grants exactly one “stand by”
sem_t print_sem;      // serialize printf

void* player_thread(void* arg) {
    int id = *(int*)arg;
    free(arg);

    // thread-local seed
    unsigned int seed = (unsigned int)time(NULL) ^ (id * 0x9e3779b9);

    for (int r = 1; r <= ROUNDS; r++) {
        sem_wait(&death_event);
        usleep(rand_r(&seed) % 100000);

        // first one here grabs the rez
        if (sem_trywait(&rez_token) == 0) {
            sem_wait(&print_sem);
            printf("Round %d: Player %d casts Battle Rez!\n", r, id);
            sem_post(&print_sem);
        }
        // the *next* one here grabs the standby
        else if (sem_trywait(&standby_token) == 0) {
            sem_wait(&print_sem);
            printf("Round %d: Player %d stands by.\n", r, id);
            sem_post(&print_sem);
        }
        // everyone else stays silent
    }
    return NULL;
}

int main() {
    pthread_t players[NUM_PLAYERS];

    // initialize semaphores
    sem_init(&death_event,   0, 0);
    sem_init(&rez_token,     0, 1);
    sem_init(&standby_token, 0, 1);
    sem_init(&print_sem,     0, 1);

    // spin up threads
    for (int i = 0; i < NUM_PLAYERS; i++) {
        int *id = malloc(sizeof(int));
        *id = i+1;
        pthread_create(&players[i], NULL, player_thread, id);
    }

    // ROUNDS of “someone died”
    for (int r = 1; r <= ROUNDS; r++) {
        printf("\n== Death Event: Round %d ==\n", r);
        // wake all players
        for (int i = 0; i < NUM_PLAYERS; i++) {
            sem_post(&death_event);
        }
        sleep(1);

        // reset for next round
        sem_post(&rez_token);
        sem_post(&standby_token);
    }

    // join & cleanup
    for (int i = 0; i < NUM_PLAYERS; i++) {
        pthread_join(players[i], NULL);
    }
    sem_destroy(&death_event);
    sem_destroy(&rez_token);
    sem_destroy(&standby_token);
    sem_destroy(&print_sem);

    return 0;
}
