#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define SHMSZ     27
#define MAX_CHARS_PER_LINE 100
#define BILLION  1000000000L;

//function to count lines in a file with name filename

int count_lines(char* filename){

    FILE* fp = fopen(filename, "r");                    //opens file with name filename
    int count = 0;
    char c;


    if (fp == NULL){                                    //checks if file was not opened

        printf("Could not open file %s", filename);
        return 0;

    }
  
    for (c = getc(fp); c != EOF; c = getc(fp)){         //gets characters one by one in the file

        if (c == '\n'){                                 //if character is '\n' line changes so we add + 1 to lines counter

            count++;

        }
    }

    fclose(fp);                                         //closes file
  
    return count;                                       //returns lines counted

}




int main(int argc , char* argv []){

    pid_t childpid = 0;                                //variables defined here for the rest of the program
    int i,line_nmb,shmid_parent,file_lines;
    int shmid_child;
    FILE *fptr;
    key_t key_parent;                                   //key for parent and child for shared memory to use
    key_parent = 333;
    key_t key_child;
    key_child = 444;
    char* shm_parent;                                   //define char * variable for shared memoryu to write and read
    char* shm_child;
    char *rd;
    char *wd;
    char line[100];
    sem_t* mutex;
    int error_check;
    char* filename;
    int N,K;
    struct timeval start_time,stop_time;
    double time_used;

    shm_parent = malloc(MAX_CHARS_PER_LINE * sizeof(char));             //call malloc to dynamically allocate memory for variabls to write through shared memory
    shm_child = malloc(MAX_CHARS_PER_LINE * sizeof(char));
    rd = malloc(MAX_CHARS_PER_LINE * sizeof(char));
    wd = malloc(MAX_CHARS_PER_LINE * sizeof(char));

    for(int counter = 0;counter < argc;counter++){                      //iterates all arguments given in execution from main

        if(strcmp(argv[counter],"-f") == 0){                            //if we find -f in arguments after that follows the name of the file

            filename = malloc((strlen(argv[counter + 1]) * sizeof(char)));
            strcpy(filename,argv[counter + 1]);

        }

        else if(strcmp(argv[counter],"-k") == 0){                       //if we find -k in arguments after that follows the number of childs to create

            K = atoi(argv[counter + 1]);

        }

        else if(strcmp(argv[counter],"-n") == 0){                       //if we find -n in arguments after that follows the number of loops each child will do

            N = atoi(argv[counter + 1]);

        }

    }

    file_lines = count_lines(filename);                                 //calculates number of lines in file
    

    for(int counter = 0;counter < 100;counter++){                      

        rd[counter] = '\0';
        wd[counter] = '\0';

    }

    mutex = (sem_t*) mmap(0, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0 );                 //because POSIX semaphores are on-the-stack structs we have to do the sharing in child processes

    if ((void*)mutex == MAP_FAILED){                            //checks for error

        perror("mmap");
        exit(1); 

    } 

    sem_init(mutex,1,0);                                        


    for(i = 0;i < K;i++){                                       //creates K number of child processes

        if((childpid = fork()) <= 0){                           //if childpid <= 0 this the child process       
            
            srand(getpid());                                    //give child pid for seed to srand to get random numbers

            for(int loop = 0;loop < N;loop++){                  //for N child loops

                
                line_nmb = rand() % file_lines + 1;             //get random number to request a random line from the file

                if((shmid_child = shmget(key_child, SHMSZ, IPC_CREAT | 0666)) < 0){                 //call shmget function and check for error

                    perror("shmget");
                    exit(1);

                }


                if((shm_child = shmat(shmid_child, NULL, 0)) == (char *) -1){                       //call shmat to attach shmid_child to shm_child variable and check for error

                    perror("shmat");
                    exit(1);

                }
                
                wd = shm_child;                                         //make wd equal to shm_child (everything written on wd will be shared to parent process)
                printf("Child requests line number : %d \n",line_nmb);
                sprintf(wd, "%d", line_nmb);                            //use sprintf to write int to string wd
                

                //critical!!!!!

                if((error_check = sem_post(mutex)) != 0){               //call sem_post to wake up semaphore in parent and check for error

                    perror("sem_wait");
                    exit(1);

                }

                gettimeofday(&start_time, NULL);                        //call gettimeofday to start counting time after requesting for a line from parent

                sleep(1);


                if((error_check = sem_wait(mutex)) != 0){               //call sem_wait to block semaphore until parent writes the line requested

                    perror("sem_wait");
                    exit(1);

                }


                if ((shmid_parent = shmget(key_parent, SHMSZ, 0666)) < 0){              //call shmget function and check for error

                    perror("shmget");
                    exit(1);

                }

                if ((shm_parent = shmat(shmid_parent, NULL, 0)) == (char *) -1){        //call shmat to attach shmid_parent to shm_parent variable and check for error

                    perror("shmat");
                    exit(1);

                }

                rd = shm_parent;                                        //make rd equal to shm_parent (everything written on shm_parent from parent can be read)                                 

                strcpy(wd,rd);                                          //copy rd string to wd    

                printf("Line is : %s \n",wd);                           //print line given from parent process 

                //critical!!!!

                gettimeofday(&stop_time, NULL);                         //stop counting time

                time_used = (stop_time.tv_usec - start_time.tv_usec);                                   //deduct stop from start time to find the time spent

                printf("Time used to request and get a line in micro seconds : %lf\n", time_used );         //printing time spent

                for(int counter = 0;counter < 100;counter++){                           //empty string variables to avoid mistakes when for loop continues

                    rd[counter] = '\0';
                    wd[counter] = '\0';

                }

            }        
            
            exit(0);                                                    //exit child process (if we dont exit child process will create another child on next loop)

        }

        else if(childpid > 0){                                          //if childpid > 0 this is the parent process


            for(int loop = 0;loop < N;loop++){                          //for loops N times for N requests of each child
                                       
                fptr = fopen(filename,"r");                             //open file with name filename
            
                if(fptr == NULL){                                       //check for errors if fptr == NULL

                    perror("File not open");
                    exit(1);

                }


                //critical!!!!!!!!!!

                if((error_check = sem_wait(mutex)) != 0){               //block semaphore to wait for child to request a line

                    perror("sem_wait");
                    exit(1);

                }

                if ((shmid_child = shmget(key_child, SHMSZ, 0666)) < 0){            //call shmget function and check for error

                    perror("shmget");
                    exit(1);

                }

                if ((shm_child = shmat(shmid_child, NULL, 0)) == (char *) -1){          //call shmat to attach shmid_parent to shm_parent variable and check for error

                    perror("shmat");
                    exit(1);

                }

                rd = shm_child;                                         //make rd equal to shm_child (everything written on shm_child from child can be read)
                line_nmb = atoi(rd);                                    //make string from read to int in line_nmb

                

                if((shmid_parent = shmget(key_parent, SHMSZ, IPC_CREAT | 0666)) < 0){           //call shmget function and check for error

                    perror("shmget");
                    exit(1);

                }


                if ((shm_parent = shmat(shmid_parent, NULL, 0)) == (char *) -1){                //call shmat to attach shmid_parent to shm_parent variable and check for error

                    perror("shmat");
                    exit(1);

                }

                wd = shm_parent;                                    //make wd equal to shm_parent (everything written on wd from parent can be read from child)                       


                int j = 0;

                while(fgets(line,MAX_CHARS_PER_LINE,fptr) != NULL){                 //iterate file with fgets to find line requested

                    j++;

                    if(j == line_nmb){

                        strcpy(wd,line);                                           //copy line to wd

                        break;

                    }

                }

                //critical
            
                if((error_check = sem_post(mutex)) != 0){                           //wake up semaphore and child process

                    perror("sem_post");
                    exit(1);

                }

                sleep(1);

            }

        }

        fclose(fptr);                                                               //close file

    }

    //free(rd);
    //free(wd);
    //free(shm_parent);

    return 0;

}