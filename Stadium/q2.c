#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <errno.h>

#define RED "\033[1;91m"
#define GREEN "\033[1;92m"
#define YELLOW "\033[1;93m"
#define BLUE "\033[1;94m"
#define MAGENTA "\033[1;95m"
#define CYAN "\033[1;96m"
#define NORMAL "\033[0m"

#define max_people 1000
#define max_groups 1000 

pthread_mutex_t clasico_lock;

typedef struct person{
    char Name[20];
    char type;
    int appearence_time;
    int patience_time;
    int num_goals; 
    int grp_idx;
    pthread_mutex_t plock;
    pthread_mutex_t prlock;
    pthread_cond_t cond;
    int over;  
    char seat_z; 
}Person;

typedef struct goal{
    char team;
    int chance_time;
    float probability;
}Goal;

int H_cap, A_cap, N_cap;
int max_time;           // max spectating time 
int num_groups;
int H_score, A_score;   
int friends[max_groups];
int checker[max_groups]; 

pthread_cond_t g_signal = PTHREAD_COND_INITIALIZER;

sem_t A_zone;
sem_t H_zone;
sem_t N_zone;

void* Nue_thrfun(void* arg)
{
    time_t* time_taken = (time_t*)malloc(sizeof(time_t));
    (*time_taken) = time(NULL);
    int check = sem_timedwait(&N_zone, (struct timespec*)arg);
    (*time_taken) = time(NULL) - (*time_taken); 

    return time_taken;
}

void* Hme_thrfun(void* arg)
{
    time_t* time_taken = (time_t*)malloc(sizeof(time_t));
    (*time_taken) = time(NULL);
    int check = sem_timedwait(&H_zone, (struct timespec*)arg);
    (*time_taken) = time(NULL) - (*time_taken); 

    return time_taken;
}

void* Awy_thrfun(void* arg)
{
    time_t* time_taken = (time_t*)malloc(sizeof(time_t));
    (*time_taken) = time(NULL);
    int check = sem_timedwait(&A_zone, (struct timespec*)arg);
    (*time_taken) = time(NULL) - (*time_taken); 

    return time_taken;
}

void* processing_goals(void* arg)
{
    // printf("Reached :) %d \n",((Goal*)arg)->chance_time);
    sleep(((Goal*)arg)->chance_time);
    int prob = rand()%100;
    if(((Goal*)arg)->team == 'A')
    {
        if(((Goal*)arg)->probability*100 >= prob)
        {
            A_score++;
            printf(GREEN"Team A have scored their %dth goal\n",A_score);
            pthread_cond_signal(&g_signal); 
        }
        else 
            printf(MAGENTA"Team A missed the chance to score their %dth goal\n",A_score);
    }
    else 
    {
        if(((Goal*)arg)->probability*100 >= prob)
        {
            H_score++;
            printf(GREEN"Team H have scored their %dth goal\n",H_score);
            pthread_cond_signal(&g_signal);
        }
        else 
            printf(MAGENTA"Team H missed the chance to score their %dth goal\n",H_score);
    }
}

void* spec_time(void* arg)
{
    sleep(max_time);
    ((Person*)arg)->over = 1;
    pthread_cond_signal(&g_signal); 

    return NULL;
}

void* allocate_seats(void* arg)
{
    int check;
    int seat = 0;  
    struct timespec ts;
    sleep(((Person*)arg)->appearence_time);
    printf(CYAN"%s has reached the stadium\n", ((Person*)arg)->Name);
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += ((Person*)arg)->patience_time; 
    if(((Person*)arg)->type == 'A')
    {
        check = sem_timedwait(&A_zone, &ts);
        if(check == -1 && errno == ETIMEDOUT)
        {
            printf(RED"%s couldn’t get a seat\n"NORMAL,((Person*)arg)->Name);
            seat = 1;
            // return NULL; 
        }
        else 
        printf(BLUE"%s has got a seat in zone A\n", ((Person*)arg)->Name);
    }
    else if(((Person*)arg)->type == 'H')
    {
        time_t *H_time; 
        time_t* N_time; 
        pthread_t Nue_thr, Hme_thr;
        pthread_create(&Nue_thr, NULL,Nue_thrfun,(void*)(&ts));
        pthread_create(&Hme_thr, NULL,Hme_thrfun,(void*)(&ts));
        
        pthread_join(Hme_thr,(void *)&H_time);       // *H_time = time_taken by the Hme_thread
        pthread_join(Nue_thr, (void*)&N_time); 

        if((*H_time) >= ((Person*)arg)->patience_time && (*N_time) >= ((Person*)arg)->patience_time)
        {
            printf(RED"%s couldn’t get a seat\n"NORMAL,((Person*)arg)->Name);
            seat = 1; 
            // return NULL;
        }
        else if((*H_time) > (*N_time))
        {
            printf(BLUE"%s has got a seat in zone N\n", ((Person*)arg)->Name);
            if((*H_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&H_zone);
            }
        }
        else if((*N_time) > (*H_time))
        {
            printf(BLUE"%s has got a seat in zone H\n", ((Person*)arg)->Name);
            if((*N_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&N_zone);
            }
        }
        else if((*N_time) == (*H_time))
        {
            if((*N_time)%2 == 0)
            {
                printf(BLUE"%s has got a seat in zone N\n", ((Person*)arg)->Name);
                sem_post(&H_zone);
            }
            else 
            {
                printf(BLUE"%s has got a seat in zone H\n", ((Person*)arg)->Name); 
                sem_post(&N_zone);
            }
        }
    }
    else 
    {
        time_t* H_time; 
        time_t* N_time;
        time_t* A_time; 

        pthread_t Nue_thr, Hme_thr, Awy_thr;
        
        pthread_create(&Nue_thr, NULL,Nue_thrfun,(void*)(&ts));
        pthread_create(&Hme_thr, NULL,Hme_thrfun,(void*)(&ts));
        pthread_create(&Awy_thr, NULL,Awy_thrfun,(void*)(&ts));

        pthread_join(Hme_thr,(void *)&H_time);       // *H_time = time_taken by the Hme_thread
        pthread_join(Nue_thr, (void*)&N_time);
        pthread_join(Awy_thr, (void*)&A_time);

        if((*H_time) >= ((Person*)arg)->patience_time && (*N_time) >= ((Person*)arg)->patience_time && (*A_time) >= ((Person*)arg)->patience_time)
        {
            printf(RED"%s couldn’t get a seat\n"NORMAL,((Person*)arg)->Name);
            seat = 1;
            // return NULL;
        }
        else if((*H_time) >= (*A_time) && (*A_time) >= (*N_time))
        {
            printf(BLUE"%s has got a seat in zone N\n", ((Person*)arg)->Name);
            if((*H_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&H_zone);
                sem_post(&A_zone);
            }
            else if((*H_time) >= ((Person*)arg)->patience_time && (*A_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&A_zone);
            }
        }
        else if((*H_time) >= (*N_time) && (*N_time) >= (*A_time))
        {
            printf(BLUE"%s has got a seat in zone A\n", ((Person*)arg)->Name);
            if((*H_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&H_zone);
                sem_post(&N_zone);
            }
            else if((*H_time) >= ((Person*)arg)->patience_time && (*N_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&N_zone);
            }
        }
        else if((*A_time) >= (*H_time) && (*H_time) >= (*N_time))
        {
            printf(BLUE"%s has got a seat in zone N\n", ((Person*)arg)->Name);
            if((*A_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&H_zone);
                sem_post(&A_zone);
            }
            else if((*A_time) >= ((Person*)arg)->patience_time && (*H_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&H_zone);
            }
        }
        else if((*A_time) >= (*N_time) && (*N_time) >= (*H_time))
        {
            printf(BLUE"%s has got a seat in zone H\n", ((Person*)arg)->Name);
            if((*A_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&N_zone);
                sem_post(&A_zone);
            }
            else if((*A_time) >= ((Person*)arg)->patience_time && (*N_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&N_zone);
            }
        }
        else if((*N_time) >= (*A_time) && (*A_time) >= (*H_time))
        {
            printf(BLUE"%s has got a seat in zone H\n", ((Person*)arg)->Name);
            if((*N_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&N_zone);
                sem_post(&A_zone);
            }
            else if((*N_time) >= ((Person*)arg)->patience_time && (*A_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&A_zone);
            }
        }
        else if((*N_time) >= (*H_time) && (*H_time) >= (*A_time))
        {
            printf(BLUE"%s has got a seat in zone A\n", ((Person*)arg)->Name);
            if((*N_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&N_zone);
                sem_post(&H_zone);
            }
            else if((*N_time) >= ((Person*)arg)->patience_time && (*H_time) < ((Person*)arg)->patience_time)
            {
                sem_post(&H_zone);
            }
        }
    }
        if(seat == 0)
        {
            pthread_t spec_thr; 
            pthread_create(&spec_thr, NULL, spec_time, arg); 
            while(((Person*)arg)->over == 0)
            {
                pthread_mutex_lock(&(((Person*)arg)->plock));
                if(((Person*)arg)->type == 'A')
                {
                    if(H_score >= ((Person*)arg)->num_goals)
                    {
                        printf(YELLOW"%s is leaving due to the bad defensive performance of his team\n", ((Person*)arg)->Name);
                        pthread_mutex_unlock(&(((Person*)arg)->plock));
                        // return NULL;
                        break;
                    }
                    else
                        pthread_cond_wait(&g_signal,&(((Person*)arg)->plock));
                }
                else if(((Person*)arg)->type == 'H')
                {
                    if(A_score >= ((Person*)arg)->num_goals)
                    {
                        printf(YELLOW"%s is leaving due to the bad defensive performance of his team\n", ((Person*)arg)->Name);
                        pthread_mutex_unlock(&(((Person*)arg)->plock));
                        // return NULL;
                        break;
                    }   
                    else 
                        pthread_cond_wait(&g_signal,&(((Person*)arg)->plock));
                }
                    pthread_mutex_unlock(&(((Person*)arg)->plock));
            }
            if(((Person*)arg)->over == 1)
            printf(YELLOW"%s watched the match for %d seconds and is leaving\n",((Person*)arg)->Name ,max_time);
        }
    
    printf(RED"%s is waiting for their friends at the exit\n", ((Person*)arg)->Name);
    checker[((Person*)arg)->grp_idx]++;

    if(checker[((Person*)arg)->grp_idx] == friends[((Person*)arg)->grp_idx])
        printf(RED"Group %d is leaving for dinner\n", ((Person*)arg)->grp_idx + 1);

    if(((Person*)arg)->seat_z == 'A')
        sem_post(&A_zone);
    else if(((Person*)arg)->seat_z == 'N')
        sem_post(&N_zone);
    else if(((Person*)arg)->seat_z == 'H')
        sem_post(&H_zone);
}

int main(void)
{
    scanf("%d %d %d", &H_cap, &A_cap, &N_cap);
    scanf("%d", &max_time);
    scanf("%d", &num_groups);

    Person Audiance[max_people];

    int num_people = 0;

    for(int i = 0;i < num_groups; i++)
    {
        scanf("%d", &friends[i]);
        checker[i] = 0;
        for(int j = 0; j < friends[i]; j++)
        {
            scanf("%s", Audiance[num_people].Name);
            getchar();
            scanf("%c %d %d %d", &Audiance[num_people].type, &Audiance[num_people].appearence_time, &Audiance[num_people].patience_time, &Audiance[num_people].num_goals);
            Audiance[num_people].grp_idx = i;
            num_people++;
        }
    }
    int num_goals; 
    scanf("%d", &num_goals);
    Goal chances[num_goals];
    for(int i = 0;i < num_goals; i++)
    {
        getchar();
        scanf("%c %d %f", &chances[i].team, &chances[i].chance_time, &chances[i].probability);
    }

    sem_init(&A_zone, 0, A_cap);       
    sem_init(&H_zone, 0, H_cap);
    sem_init(&N_zone , 0, N_cap);

    pthread_t people[num_people];

    for(int i = 0;i < num_people; i++)
    {
        Audiance[i].over = 0;   
        Audiance[i].seat_z = 'Z'; 
        pthread_mutex_init(&(Audiance[i].plock), NULL);
        pthread_create(&people[i], NULL, allocate_seats, (void *)(&Audiance[i]));
    }

    pthread_t Goals[num_goals];

    // printf("Reached :)\n");
    for(int i = 0; i < num_goals; i++)
        pthread_create(&Goals[i], NULL, processing_goals, (void*)(&chances[i]));
    
    
    for(int i = 0; i < num_goals; i++)
        pthread_join(Goals[i], NULL);
    
    for(int i; i < num_people; i++)
        pthread_join(people[i],NULL);
    return 0;
}