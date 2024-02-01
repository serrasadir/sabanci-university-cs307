#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

sem_t lock2;
sem_t lock;
int passenger_num = 0;
sem_t sem_a;
sem_t sem_b;
int cid = 0;
bool ready = false;

struct threadArgs {
    sem_t *sem_a;
    sem_t *sem_b;
    int *num_a; 
    int *num_b;  
    char team;
};

void *fan_thread(void *args) {
    struct threadArgs *arg = (struct threadArgs *)args;
    char myteam = arg->team;
    
    int mynum, rivalnum;
    sem_wait(&lock);

    if(myteam == 'A'){ 
        (*(arg->num_a)) += 1;
    }    
    else{
        (*(arg->num_b)) += 1;
    }

    mynum = *(arg->num_a);
    printf("Thread ID: %lu, Team: %c, I am looking for a spot.\n", pthread_self(), myteam);

    if (*(arg->num_a) >= 4 || *(arg->num_b) >= 4) { // phase 2
        if(myteam == 'A'){            
            // found a band now wake the others
            sem_post(arg->sem_a);
            sem_post(arg->sem_a);
            sem_post(arg->sem_a);

            *(arg->num_a) -= 4; // they have found a car

        } else {
            sem_post(arg->sem_b);
            sem_post(arg->sem_b);
            sem_post(arg->sem_b);

            *(arg->num_b) -= 4; // they have found a car
         
        }
        ready = true;
    } else if ((*(arg->num_a) >= 2) && (*(arg->num_b) == 2) || (*(arg->num_b) >= 2) && (*(arg->num_a) == 2)) { // phase 2
        if(myteam == 'A'){
            sem_post(arg->sem_a);
            sem_post(arg->sem_b);
            sem_post(arg->sem_b);       
        } else {
            sem_post(arg->sem_a);
            sem_post(arg->sem_a);
            sem_post(arg->sem_b);
        }       
        *(arg->num_a) -= 2;
     
        *(arg->num_b) -= 2;
        ready = true;
    } else { // phase1
     
        sem_post(&lock);
        if(myteam == 'A'){
        sem_wait(arg->sem_a);
        }else{
        sem_wait(arg->sem_b);
        }
        
        sem_wait(&lock);
    }
    
    if (ready) { // phase 3 
        sem_wait(&lock2);
        printf("Thread ID: %lu, Team: %c, I have found a spot in a car.\n", pthread_self(), myteam);
    	passenger_num++;
    	sem_post(&lock2);
    	if ( passenger_num == 4) {
    	sem_wait(&lock2);
        	printf("Thread ID: %lu, Team: %c, I am the captain and driving the car with ID %d.\n", pthread_self(), myteam, cid);
        	cid += 1;
        	passenger_num=0;
        	ready = false;}
        	sem_post(&lock2);
    }
    sem_post(&lock);  
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "wrong input");
        return EXIT_FAILURE;
    }
    int fan_num_a = atoi(argv[1]);
    int fan_num_b = atoi(argv[2]);

    if (fan_num_a % 2 != 0 || fan_num_b % 2 != 0 ||
        (fan_num_a + fan_num_b) % 4 != 0) {
        fprintf(stderr, "Invalid number of fans.\n");
        return EXIT_FAILURE;
    }
    sem_init(&lock2, 0, 1);
    sem_init(&lock, 0, 1);
    sem_init(&sem_a, 0, 0);
    sem_init(&sem_b, 0, 0);

   pthread_t threads_a[fan_num_a];
    pthread_t threads_b[fan_num_b];

    int num_a = 0;
    int num_b = 0;

    struct threadArgs args_a = {&sem_a, &sem_b, &num_a, &num_b, 'A'};
    for (int a = 0; a < fan_num_a; a++) {
        pthread_create(&threads_a[a], NULL, fan_thread, &args_a);
    }

    struct threadArgs args_b = {&sem_a, &sem_b, &num_a, &num_b, 'B'};
    for (int b = 0; b < fan_num_b; b++) {
        pthread_create(&threads_b[b], NULL, fan_thread, &args_b);
    }

    for (int a = 0; a < fan_num_a; a++) {
        pthread_join(threads_a[a], NULL);
    }

    for (int b = 0; b < fan_num_b; b++) {
        pthread_join(threads_b[b], NULL);
    }
    


    printf("Main terminates\n");
    return 0;
}

