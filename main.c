/*  
* Reservation System for Passenger and Agents using POSIX Threads
* 
* The final version of this code has been developed by Mustafa SARAÇ and Musa ÇIBIK as a project
* of Operating System (COMP 304) course. Koç University's code of ethics can be applied to this
* code and liability can not be accepted for any negative situation. Therefore, be careful when
* you get content from here.
*
* Description: In this reservation sysem, there are passenger and agent threads that
* can buy, reserve, cancel and view bus tickets. In order to handle the multiple operations
* that might be done concurrently such as, buy, reserve and cancel operations, we used Pthreads
* in this project. Passengers can cancel and/or view their bought or reserved tickets. 
* Agents can view all tickets and buy, reserve, cancel tickets for their passengers.
* 
* You can also contact me at this email address: msarac13@ku.edu.tr
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

typedef int bool;
#define true 1
#define false 0

typedef struct 
{
    char* time;
    int p_id; // unique Passenger ID
    int a_id; // unique Agent ID
    char operation; // Operations (Buy, Reserve, Cancel, View)
    int seat_no; // unique seat no
    int tour_no; // tour no
} Log;

/* GLOBAL VARIABLES FOR 'SHARED'  RESOURCES */
double start_time; // A variable for start time of simulation in second
int simulation_time; // A variable for simulation time in second

int s_num; // Number of Seats with Unique IDs from 1 to S
int t_num; // Number of Tours with Unique IDs (t=1 if not provided)
int r_num; // Value of Random Number Generator Seed.
int d_num; // the total simulation time in days
int p_num; // Number of Passengers with Unique IDs
int a_num; // Number of Agents with Unique IDs

int **tourSeats; 
int **tourOwners; 
//int **reservedTimes;

FILE *log_file;

int log_counter = 0;
int currentDay = 0;

pthread_mutex_t *seat_locks;
pthread_mutex_t log_lock;
pthread_mutex_t print_lock;
//pthread_mutex_t reservedTime_lock;

// Getting the current time in the format of HH:MM:SS
char* getCurrentTime()
{    
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    char *time = malloc(8);
    char hour[3];
    char min[3];
    char sec[3];

    if(timeinfo->tm_hour < 10)
        sprintf(hour, "0%d", timeinfo->tm_hour);
    else
        sprintf(hour, "%d", timeinfo->tm_hour);

    if(timeinfo->tm_min < 10)
        sprintf(min, "0%d", timeinfo->tm_min);
    else
        sprintf(min, "%d", timeinfo->tm_min);

    if(timeinfo->tm_sec < 10)
        sprintf(sec, "0%d", timeinfo->tm_sec);
    else
        sprintf(sec, "%d", timeinfo->tm_sec);

    strcpy(time, hour);
    strcat(time, ":");
    strcat(time, min);
    strcat(time, ":");
    strcat(time, sec);

    return time;
}

char* getLogAsStr(Log log)
{
    char *str_log = malloc(50);
    char p_id[5];
    char a_id[5];
    char operation[2];
    char seat_no[5];
    char tour_no[5];

    sprintf(p_id, "%d", log.p_id);
    sprintf(a_id, "%d", log.a_id);
    operation[0] = log.operation;
    operation[1] = '\0';
    sprintf(seat_no, "%d", log.seat_no);
    sprintf(tour_no, "%d", log.tour_no);

    strcpy(str_log, log.time);

    strcat(str_log, "\t ");
    strcat(str_log, p_id);
    strcat(str_log, "\t\t ");
    strcat(str_log, a_id);
    strcat(str_log, "\t\t\t");
    strcat(str_log, operation);
    strcat(str_log, "\t\t   ");
    strcat(str_log, seat_no);
    strcat(str_log, "\t\t   ");
    strcat(str_log, tour_no);

    return str_log;
}

/* Giving output the summary of the system at the end of each day */
void summaryDay(int i)
{
        printf("\n==============\n");
        printf("Day%d\n", i);
        printf("==============\n");
        
        printf("Reserved Seats:\n");

        for(int j=0; j<t_num; j++)
        {
            printf("Tour %d: ", j+1);

            for(int k=0; k<s_num; k++)
            {
                if(tourSeats[j][k] == 1)
                    printf(" %d,", k+1);
            }
            
            printf("\n");
        }

        printf("Bought Seats:\n");

        for(int j=0; j<t_num; j++)
        {
            printf("Tour %d: ", j+1);

            for(int k=0; k<s_num; k++)
            {
                if(tourSeats[j][k] == 2)
                    printf(" %d,", k+1);
            }
            
            printf("\n");
        }

/*
        printf("Wait List (Passenger ID)\n");
        bool isWait = false;

        for(int j=1; j<=t_num; j++)
        {
            printf("Tour %d: ", j);

            for(int k=1; k<=s_num; k++)
            {
                if(tourSeats[j][k] == 0)
                {
                    printf("%d", tourOwners[j][k]);
                    isWait = true;
                }
            }
            
            if(isWait != true)
                printf("no passenger is waiting since the tour is not full\n");

            printf("\n");
        }
*/

}

// Taking the input values (days(d), passengers(p), agents(a), tours(t), seats(s), seed(r)) 
// from the command line and setting these values to the variables.
void setInputs(int argc, char *argv[], int *d, int *p, int *a, int *t, int *s, int *r)
{
    // by starting from index 1, we are ignoring the executable file argument (Example: ./main)

    bool is_t_provided = false;

    for(int i=1; i<argc; i++){
        if(strcmp(argv[i], "-d") == 0)
               *d = (int) strtol(argv[i+1], NULL, 10);
        
        if(strcmp(argv[i], "-p") == 0)
            *p = (int) strtol(argv[i+1], NULL, 10);
        
        if(strcmp(argv[i], "-a") == 0)
            *a = (int) strtol(argv[i+1], NULL, 10);
        
        if(strcmp(argv[i], "-t") == 0)
        {
            is_t_provided = true;
            *t = (int) strtol(argv[i+1], NULL, 10);
        }
        
        if(strcmp(argv[i], "-s") == 0)
            *s = (int) strtol(argv[i+1], NULL, 10);

        if(strcmp(argv[i], "-r") == 0)
            *r = (int) strtol(argv[i+1], NULL, 10);

        if(is_t_provided != true)
            *t = 1; // t=1 if not provided

    }
}

// Getting the double value of the current time in terms of second
double getCurrentTimeAsSeconds()
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    struct timeval msec_time;
    gettimeofday(&msec_time, NULL);

    return (timeinfo->tm_hour * 3600) + (timeinfo->tm_min * 60) + timeinfo->tm_sec +  msec_time.tv_usec / 1000000 ;
}


bool isReservationAllowed(int p_id, int s_num, int t_num, int **tourSeats, int **tourOwners)
{    
    int counter = 0;


    for(int i=0; i<t_num; i++){
        for(int j=0; j<s_num; j++){
            if(tourOwners[i][j] == p_id && tourSeats[i][j] == 1) {
                counter++;
            }
        }
    }

    if(counter < 2){
    
        return true;
    }
    else
        return false;
}

void *print_function()
{
    while(getCurrentTimeAsSeconds() - start_time <= simulation_time)
    { 
        pthread_mutex_lock(&print_lock);

        int tempDay = (getCurrentTimeAsSeconds() - start_time)/5 ;
        if(tempDay != currentDay)
        {
            summaryDay(tempDay);
            currentDay++;
        }

        pthread_mutex_unlock(&print_lock);
    }

    pthread_exit(0);
}

/*
void *cancel_automatically()
{
    while(getCurrentTimeAsSeconds() - start_time <= simulation_time)
    {

    	pthread_mutex_lock(&reservedTime_lock);

    	double currTime = getCurrentTimeAsSeconds();


    	for(int i=0; i<t_num; i++){
    		for(int j=0; j<s_num; j++){

    			if(currTime - reservedTimes[i][j] < 5.0){

	    		reservedTimes[i][j] = 0;
	           
	            int k = j + i * s_num;

	    		pthread_mutex_lock(&seat_locks[k]);

	    		tourSeats[i][j] = 0;
	            tourOwners[i][j] = 0;
	    		
	    		pthread_mutex_unlock(&seat_locks[k]);

    			}

			}
    	}

        pthread_mutex_unlock(&reservedTime_lock);
    }

	pthread_exit(0);
}
*/

void *passenger_function(void *my_ID)
{
    int ID = (int)  my_ID;
    srand(r_num);
    while(getCurrentTimeAsSeconds() - start_time <= simulation_time)
    { 

    int ticket_operation = rand() % 10; // generate a random ticket operation
    
    if(ticket_operation < 4){ // reservation
        
            int seat_no = rand() % s_num;
            int tour_no = rand() % t_num;

            int j = seat_no + tour_no * s_num;
            pthread_mutex_lock(&seat_locks[j]);

            if(tourSeats[tour_no][seat_no] == 0){
                if(isReservationAllowed(ID, s_num,  t_num, tourSeats, tourOwners)){
                            tourSeats[tour_no][seat_no] = 1;
                            tourOwners[tour_no][seat_no] = ID;

                            /*
                            pthread_mutex_lock(&reservedTime_lock);
                            reservedTimes[tour_no][seat_no] = getCurrentTimeAsSeconds();
                            pthread_mutex_unlock(&reservedTime_lock);
							*/

                            pthread_mutex_lock(&log_lock);

                            char* time = malloc(8);
                            time = getCurrentTime();

                            Log tempLog; 
                            tempLog.time = time;
                            tempLog.p_id = ID + 1;
                            tempLog.a_id =  0 ;
                            tempLog.operation = 'R';
                            tempLog.seat_no = seat_no+1;
                            tempLog.tour_no = tour_no+1;

                            log_file = fopen("tickets.log", "a"); 

                            if (log_file == NULL) 
                            {
                                printf("Could not open the tickets.log file");
                            } else {
                                fprintf(log_file, "%s\n", getLogAsStr(tempLog));
                                fclose(log_file);
                            }

                            pthread_mutex_unlock(&log_lock);


                }

                
            }

            pthread_mutex_unlock(&seat_locks[j]);


    } else if(4 <= ticket_operation &&  ticket_operation < 6){
    // cancel the reserved/bought ticket

        int seat_no = rand() % s_num;
        int tour_no = rand() % t_num;

        int j = seat_no + tour_no * s_num;
        pthread_mutex_lock(&seat_locks[j]);

        if(tourSeats[tour_no][seat_no] != 0){
            if(tourOwners[tour_no][seat_no] == ID){
                        tourSeats[tour_no][seat_no] = 0;
                        tourOwners[tour_no][seat_no] = 0;

                        /*
                        pthread_mutex_lock(&reservedTime_lock);
                        reservedTimes[tour_no][seat_no] = 0;
                        pthread_mutex_unlock(&reservedTime_lock);
						*/

                        pthread_mutex_lock(&log_lock);

                        char* time = malloc(8);
                        time = getCurrentTime();

                        Log tempLog; 
                        tempLog.time = time;
                        tempLog.p_id = ID + 1;
                        tempLog.a_id =  0 ;
                        tempLog.operation = 'C';
                        tempLog.seat_no = seat_no+1;
                        tempLog.tour_no = tour_no+1;

                        log_file = fopen("tickets.log", "a"); 

                        if (log_file == NULL) 
                        {
                            printf("Could not open the tickets.log file");
                        } else {
                            fprintf(log_file, "%s\n", getLogAsStr(tempLog));
                            fclose(log_file);
                        }

                        pthread_mutex_unlock(&log_lock);


            }

            
        }

        pthread_mutex_unlock(&seat_locks[j]);


    // notify the passenger waiting on the condition of an empty seat


    } else if(6 <= ticket_operation && ticket_operation < 8){
    // view the reserved/bought ticket

            int seat_no = rand() % s_num;
            int tour_no = rand() % t_num;

            int j = seat_no + tour_no * s_num;

            if(tourOwners[tour_no][seat_no] == ID){

                    pthread_mutex_lock(&log_lock);

                    char* time = malloc(8);
                    time = getCurrentTime();

                    Log tempLog; 
                    tempLog.time = time;
                    tempLog.p_id = ID + 1;
                    tempLog.a_id = 0;
                    tempLog.operation = 'V';
                    tempLog.seat_no = seat_no+1;
                    tempLog.tour_no = tour_no+1;

                    log_file = fopen("tickets.log", "a"); 

                    if (log_file == NULL) 
                    {
                        printf("Could not open the tickets.log file");
                    } else {
                        fprintf(log_file, "%s\n", getLogAsStr(tempLog));
                        fclose(log_file);
                    }

                    pthread_mutex_unlock(&log_lock);


            }


    } else {
    // buy the reserved ticket

        int seat_no = rand() % s_num;
        int tour_no = rand() % t_num;

        int j = seat_no + tour_no * s_num;
        pthread_mutex_lock(&seat_locks[j]);

        if(tourSeats[tour_no][seat_no] == 0 || (tourSeats[tour_no][seat_no] == 1 && tourOwners[tour_no][seat_no] == ID)){

                tourSeats[tour_no][seat_no] = 2;
                tourOwners[tour_no][seat_no] = ID;

                /*
                pthread_mutex_lock(&reservedTime_lock);
                reservedTimes[tour_no][seat_no] = 0;
                pthread_mutex_unlock(&reservedTime_lock);
				*/

                pthread_mutex_lock(&log_lock);

                char* time = malloc(8);
                time = getCurrentTime();

                Log tempLog; 
                tempLog.time = time;
                tempLog.p_id = ID + 1;
                tempLog.a_id =  0 ;
                tempLog.operation = 'B';
                tempLog.seat_no = seat_no+1;
                tempLog.tour_no = tour_no+1;

                log_file = fopen("tickets.log", "a"); 

                if (log_file == NULL) 
                {
                    printf("Could not open the tickets.log file");
                } else {
                    fprintf(log_file, "%s\n", getLogAsStr(tempLog));
                    fclose(log_file);
                }

                pthread_mutex_unlock(&log_lock);


        }

        pthread_mutex_unlock(&seat_locks[j]);

    }


    }

    pthread_exit(0);
}

void agent_function(void *my_ID)
{
    int ID = (int)  my_ID;
    srand(r_num);

    while(getCurrentTimeAsSeconds() - start_time <= simulation_time)
    { 

    int ticket_operation = rand() % 10; // generate a random ticket operation
    
    if(ticket_operation < 4){

            int seat_no = rand() % s_num;
            int tour_no = rand() % t_num;
            int pass_no = rand() % p_num;

            int j= seat_no + tour_no * s_num;
            pthread_mutex_lock(&seat_locks[j]);

            if(tourSeats[tour_no][seat_no] == 0){
                if(isReservationAllowed(pass_no, s_num,  t_num, tourSeats, tourOwners)){
                            tourSeats[tour_no][seat_no] = 1;
                            tourOwners[tour_no][seat_no] = pass_no;

                            /*
                            pthread_mutex_lock(&reservedTime_lock);
                            reservedTimes[tour_no][seat_no] = getCurrentTimeAsSeconds();
                            pthread_mutex_unlock(&reservedTime_lock);
							*/

                            pthread_mutex_lock(&log_lock);

                            char* time = malloc(8);
                            time = getCurrentTime();

                            Log tempLog; 
                            tempLog.time = time;
                            tempLog.p_id = pass_no + 1;
                            tempLog.a_id = ID + 1;
                            tempLog.operation = 'R';
                            tempLog.seat_no = seat_no + 1;
                            tempLog.tour_no = tour_no + 1;

                            log_file = fopen("tickets.log", "a"); 

                            if (log_file == NULL) 
                            {
                                printf("Could not open the tickets.log file");
                            } else {
                                fprintf(log_file, "%s\n", getLogAsStr(tempLog));
                                fclose(log_file);
                            }

                            pthread_mutex_unlock(&log_lock);


                }

                
            }

            pthread_mutex_unlock(&seat_locks[j]);

    } else if(4 <= ticket_operation &&  ticket_operation < 6){
    // cancel the reserved/bought ticket,

        int seat_no = rand() % s_num;
        int tour_no = rand() % t_num;

        int j = seat_no + tour_no * s_num;
        pthread_mutex_lock(&seat_locks[j]);

        if(tourSeats[tour_no][seat_no] != 0){

                tourSeats[tour_no][seat_no] = 0;
                int pass_no = tourOwners[tour_no][seat_no];
                tourOwners[tour_no][seat_no] = 0;

                /*
                pthread_mutex_lock(&reservedTime_lock);
                reservedTimes[tour_no][seat_no] = 0;
                pthread_mutex_unlock(&reservedTime_lock);   
                */

                pthread_mutex_lock(&log_lock);

                char* time = malloc(8);
                time = getCurrentTime();

                Log tempLog; 
                tempLog.time = time;
                tempLog.p_id = pass_no + 1;
                tempLog.a_id = ID + 1;
                tempLog.operation = 'C';
                tempLog.seat_no = seat_no+1;
                tempLog.tour_no = tour_no+1;

                log_file = fopen("tickets.log", "a"); 

                if (log_file == NULL) 
                {
                    printf("Could not open the tickets.log file");
                } else {
                    fprintf(log_file, "%s\n", getLogAsStr(tempLog));
                    fclose(log_file);
                }

                pthread_mutex_unlock(&log_lock);

            
        }

        pthread_mutex_unlock(&seat_locks[j]);

    // notify the passenger waiting on the condition of an empty seat


    } else if(6 <= ticket_operation && ticket_operation < 8){
    // view the reserved/bought ticket

            int seat_no = rand() % s_num;
            int tour_no = rand() % t_num;
            int pass_no = rand() % p_num;

            int j = seat_no + tour_no * s_num;

            if(tourOwners[tour_no][seat_no] == pass_no){

                    pthread_mutex_lock(&log_lock);

                    char* time = malloc(8);
                    time = getCurrentTime();

                    Log tempLog; 
                    tempLog.time = time;
                    tempLog.p_id = pass_no+1;
                    tempLog.a_id = ID + 1;
                    tempLog.operation = 'V';
                    tempLog.seat_no = seat_no+1;
                    tempLog.tour_no = tour_no+1;

                    log_file = fopen("tickets.log", "a"); 

                    if (log_file == NULL) 
                    {
                        printf("Could not open the tickets.log file");
                    } else {
                        fprintf(log_file, "%s\n", getLogAsStr(tempLog));
                        fclose(log_file);
                    }

                    pthread_mutex_unlock(&log_lock);


            }

    } else {
    // buy the reserved ticket

            int seat_no = rand() % s_num;
            int tour_no = rand() % t_num;
            int pass_no = rand() % p_num;

            int j= seat_no + tour_no * s_num;
            pthread_mutex_lock(&seat_locks[j]);

            if(tourSeats[tour_no][seat_no] == 0 || (tourSeats[tour_no][seat_no] == 1 && tourOwners[tour_no][seat_no] == pass_no)){

                            tourSeats[tour_no][seat_no] = 2;
                            tourOwners[tour_no][seat_no] = pass_no;

                            /*
                            pthread_mutex_lock(&reservedTime_lock);
                			reservedTimes[tour_no][seat_no] = 0;
                			pthread_mutex_unlock(&reservedTime_lock);
							*/

                            pthread_mutex_lock(&log_lock);

                            char* time = malloc(8);
                            time = getCurrentTime();

                            Log tempLog; 
                            tempLog.time = time;
                            tempLog.p_id = pass_no + 1;
                            tempLog.a_id = ID + 1;
                            tempLog.operation = 'R';
                            tempLog.seat_no = seat_no + 1;
                            tempLog.tour_no = tour_no + 1;

                            log_file = fopen("tickets.log", "a"); 

                            if (log_file == NULL) 
                            {
                                printf("Could not open the tickets.log file");
                            } else {
                                fprintf(log_file, "%s\n", getLogAsStr(tempLog));
                                fclose(log_file);
                            }

                            pthread_mutex_unlock(&log_lock);
                
            }

            pthread_mutex_unlock(&seat_locks[j]);

    	}

    }
  
  	pthread_exit(0);

}


int main(int argc, char *argv[]){
    
    // Read command line arguments
    setInputs(argc, argv, &d_num, &p_num, &a_num, &t_num, &s_num, &r_num);
   
    // MEMORY ALLOCATION
    tourSeats = malloc(sizeof(int*) * t_num);
    tourOwners = malloc(sizeof(int*) * t_num);
    //reservedTimes = malloc(sizeof(int*) * t_num);

    for(int i=0; i<t_num; i++){
        tourSeats[i] = malloc(sizeof(int) * s_num);
        tourOwners[i] = malloc(sizeof(int) * s_num);
        //reservedTimes[i] = malloc(sizeof(int) * s_num);
    }

    seat_locks = malloc(sizeof(pthread_mutex_t) * s_num * t_num);


    // INITIALIZE
    for(int i=0; i<t_num; i++){
        for(int j=0; j<s_num; j++){
            tourSeats[i][j] = 0;
            tourOwners[i][j] = 0;
            //reservedTimes[i][j] = 0;
        }
    }

    start_time = getCurrentTimeAsSeconds();
    simulation_time = d_num * 5;


    // THREADS
    pthread_t passenger_threads[p_num];
    pthread_t agent_threads[a_num];
    pthread_t time_thread;
    //pthread_t reservedTime_thread;

    // MUTEX INIT
    for(int i=0; i<t_num * s_num; i++)   
        pthread_mutex_init(&seat_locks[i], NULL);

    pthread_mutex_init(&log_lock, NULL);
    pthread_mutex_init(&print_lock, NULL);
    //pthread_mutex_init(&reservedTime_lock, NULL);

    // OPENING LOG FILE AND WRITING JUST THE HEADER PART 
    log_file = fopen("tickets.log", "w"); 

    if (log_file == NULL) 
    {
        printf("Could not open the tickets.log file");
    } else {
        fprintf(log_file, "%s\n", "Time\t\tP_ID\tA_ID\tOperation\tSeat No\t\tTour No");
        fclose(log_file);
    }

    // CREATE THREADS

    for(int i=0; i<p_num; i++){
        pthread_create(&passenger_threads[i], NULL, passenger_function, (void *)i);
    }

    for(int i=0; i<a_num; i++){
        pthread_create(&agent_threads[i], NULL, agent_function, (void *)i);
    }

    pthread_create(&time_thread, NULL, print_function, NULL);

	//pthread_create(&reservedTime_thread, NULL, cancel_automatically, NULL);

    // JOIN THREADS
    for(int i=0; i<p_num; i++){
        pthread_join(passenger_threads[i], NULL);
    }

    for(int i=0; i<a_num; i++){
        pthread_join(agent_threads[i], NULL);
    }

    pthread_join(time_thread, NULL);

    //pthread_join(reservedTime_thread, NULL);

    // DESTROY THREADS
    int destroy_seatLocks = -1;
    int destroy_logLock = -1;
    int destroy_printLock = -1;
    //int destroy_reservedTimeLock = -1;

    for(int i=0; i<t_num * s_num; i++)
    	destroy_seatLocks = pthread_mutex_destroy(&seat_locks[i]);

   	destroy_logLock = pthread_mutex_destroy(&log_lock);
    destroy_printLock = pthread_mutex_destroy(&print_lock);
    //destroy_reservedTimeLock = pthread_mutex_destroy(&reservedTime_lock);

    if(destroy_seatLocks != 0 || destroy_logLock != 0 || destroy_printLock != 0 )
    	printf("\nCould not successfully destroy mutex\n");

    // FREE MEMORY
    for(int i=0; i<t_num; i++){
        free(tourSeats[i]);
        free(tourOwners[i]);
        //free(reservedTimes[i]);
    }

    free(tourSeats);
    free(tourOwners);
    free(seat_locks);
    //free(reservedTimes);

    return 0;
}