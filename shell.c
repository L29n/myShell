#define _GNU_SOURCE
#include <termios.h>z
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>  


typedef struct job //create nodes for linked list
{
    int job_id; //incremements by 1 everytime we add a new job
    int bg;
	pid_t pid;
    int status; //0 for terminated, 1 for paused, 2 for running 
    char* filename;
	struct job* next;
}job;

job* head = NULL;
job* last = NULL; //last node of the linked list for better insertion into linked list
int numJobs = 0; //number of jobs in linked list, this is for the job_id
pid_t currpid;

int getNumFromStr(char** str){
    if(str[1][0] == '%'){
        int ans = 0;
        int p = 1;
        while(str[1][p] != '\0'){
            if(!isdigit(str[1][p])){
                return -1;
            }
            ans *= 10;
            ans += (str[1][p] - '0');
            p++;
        }
        return ans;
    }else{  
        return -1;
    }
}


char** stringTokenizer(char* input, int len, int* count){
    int mallocSizesBuffer = 10;
    int* mallocSizes = (int*)malloc(sizeof(int) * mallocSizesBuffer); //hold the sizes to malloc later
    int wordCount = 0;
    int spaceSeen = 1; //starts as true
    for(int i = 0; i < len - 1; i++){ //populate mallocSizes
        if(spaceSeen){
            if(input[i] != ' '){
                wordCount++;
                    
                //realloc
                if(wordCount > mallocSizesBuffer){
                    mallocSizesBuffer *= 2;
                    mallocSizes = (int*)realloc(mallocSizes, sizeof(int)*mallocSizesBuffer);
                }

                mallocSizes[wordCount - 1] = 1;
                spaceSeen = 0;
            }
        }else{
            if(input[i] == ' '){
                spaceSeen = 1;
            }else{
                mallocSizes[wordCount - 1]++;
            }
        }

    }

    // void initialize(){
        
    // }

    //malloc for the formatted input
    char** formattedInput = (char**)malloc(sizeof(char*) * (wordCount + 1));
    for(int i = 0; i < wordCount; i++){
        formattedInput[i] = (char*)malloc(sizeof(char) * mallocSizes[i] + 1);
    }

    //populate formatted input
    int counter = 0;
    int index = 0;
    for(int i = 0; i < len - 1; i++){
        if(input[i] != ' '){
            formattedInput[index][counter] = input[i];
            counter++;
            if(counter == mallocSizes[index]){
                formattedInput[index][counter] = '\0';
                counter = 0;
                index += 1;
            }
        }
    }

    formattedInput[wordCount] = NULL;

    free(mallocSizes);
    *count = wordCount;
    return formattedInput;

}


char* cd(char** formattedInput, int wordCount){
    size_t size = 255;
    char* buf = (char*)malloc(sizeof(char) * size);
    if(buf == NULL){
        perror("Error");
        exit(1);
    }
    char* home = getenv("HOME");
    char* PWD = getcwd(buf, size);
    if(wordCount > 2){
        puts("cd: too many arguments");
        return PWD;
    }else if(wordCount == 1){
        int check = chdir(home);
        if(check == -1){
            puts("wut");
            return PWD;
        }
        PWD = getcwd(buf, size);
    }else if(wordCount == 2){
        int check = chdir(formattedInput[1]);
        if(check == -1){
            puts("cd: invalid file path");
            return PWD;
        }
        PWD = getcwd(buf, size);
    }
    return PWD;
}



void handler(int signal)
{
    sigset_t sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGINT);
    sigaddset(&sigmask, SIGTSTP);
    sigaddset(&sigmask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &sigmask, NULL); //block signals so the signal handler does not get interrupted with other signals.
    



    if(signal == SIGINT){
        job* pointer = head;
        while(pointer != NULL){
            if(pointer -> pid == currpid && pointer -> status != 0){
                pointer -> status = 0;
                printf("\n[%d] %d terminated by signal 2\n", pointer -> job_id, pointer -> pid);
                kill(currpid, SIGKILL);
                waitpid(currpid, NULL, 0);
                break;
            }
            pointer = pointer -> next;
        }

    }else if(signal == SIGCHLD){
        job* pointer = head;
        while(pointer != NULL){
            if(pointer -> pid == currpid){
                pointer -> status = 0;
                kill(currpid, SIGKILL);
                waitpid(currpid, NULL, 0);
                break;
            }
            pointer = pointer -> next;
        }
    }else if(signal == SIGTSTP){
        job* pointer = head;
        while(pointer != NULL){
            if(pointer -> pid == currpid){
                kill(currpid, SIGSTOP);
                pointer -> status = 1;
                puts("");
                break;
            }
            pointer = pointer -> next;
        }
    }

    sigprocmask(SIG_UNBLOCK, &sigmask, NULL); //unblock with this
}

int main(int argc, char** argv){
    
    

    
    while(1){


        printf("> "); //carrot to prompt user for new input

        signal(SIGINT, handler);
        signal(SIGTSTP, handler);
        signal(SIGCHLD, handler);





        sigset_t sigmask;
        sigemptyset(&sigmask);
        sigaddset(&sigmask, SIGINT);
        sigaddset(&sigmask, SIGTSTP);
        sigaddset(&sigmask, SIGCHLD);
        // sigprocmask(SIG_BLOCK, &sigmask, NULL); //block with this
        // sigprocmask(SIG_UNBLOCK, &sigmask, NULL); //unblock with this


        size_t buffersize = 10;
        char* input = (char*)malloc(sizeof(char) * buffersize);
        if(input == NULL){
            perror("Error");
            exit(-1);
        }
        ssize_t len = getline(&input, &buffersize, stdin);
        if(len == -1){
            perror("Error");
            exit(-1);
        }

        sigprocmask(SIG_BLOCK, &sigmask, NULL); //block
        int wordCount;
        char** formattedInput = stringTokenizer(input, len, &wordCount);
        free(input);
        sigprocmask(SIG_UNBLOCK, &sigmask, NULL); //unblock

        if(wordCount == 0){
            for(int i = 0; i < wordCount; i++){
                free(formattedInput[i]);
            }
            free(formattedInput);
            continue;  
        }else if(formattedInput[0][0] == '.' || formattedInput[0][0] == '/'){
            if(access(formattedInput[0], F_OK) == 0){
                if(access(formattedInput[0], X_OK) == 0){
                    pid_t p1 = fork();

                    //update currpid
                    currpid = p1;

                    if(p1 == 0){
                        if(strcmp(formattedInput[wordCount - 1], "&") == 0){
                            formattedInput[wordCount - 1] = NULL;
                        }
                        int val = execve(formattedInput[0], formattedInput, NULL);
                        if (val == -1){
                            printf("%s: Command not found.\n", formattedInput[0]);
                        }
                        exit(0);
                        
                    }else{
                        job* newJob = (job*)malloc(sizeof(job));
                        newJob -> job_id = ++numJobs;
                        newJob -> pid = p1;
                        newJob -> status = 2; //running
                        newJob -> filename = (char*)malloc(sizeof(char) * (wordCount + 100));
                        newJob -> bg = 0;
                        strcpy(newJob -> filename, formattedInput[0]);
                        for(int i = 1; i < wordCount; i++){
                            strcat(newJob -> filename, " ");
                            strcat(newJob -> filename, formattedInput[i]);
                        }
                        newJob -> next = NULL; 
                        if(head == NULL && last == NULL){
                            head = newJob;
                            last = newJob;
                        }else{
                            last -> next = newJob;
                            last = last -> next;
                        }
                        int stat;
                        if(strcmp(formattedInput[wordCount - 1], "&") == 0){
                            newJob -> status = 2;
                            printf("[%d] %d\n", numJobs, currpid);
                            waitpid(p1, &stat, WNOHANG);
                        }else{
                            waitpid(p1, &stat, WUNTRACED);
                        }
                        if(WIFSTOPPED(stat)){
                            newJob -> status = 1;
                        }
                    }
                   
                }else{
                    puts("Not an executable file");
                }
            }else{
                puts("No such file or directory");
            }
        
        }else if(strcmp(formattedInput[0], "bg") == 0){
            if(wordCount < 2){
                puts("bg: Not enough arguments");
            
            }else if(wordCount > 2){
                puts("bg: Too many arguments");
            }else{
                job* ptr3 = head;
                int a = getNumFromStr(formattedInput);
                if(a > numJobs || a == -1){
                    puts("Not a valid job");
                }else{
                    for(int i = 1; i < a; i++){
                        ptr3 = ptr3 -> next;
                    }
                    if(ptr3 -> status == 1){
                        ptr3 -> status = 2;
                        ptr3 -> bg = 1; //set to background
                        int stat;
                        waitpid(ptr3 -> pid, &stat, WNOHANG);
                        if(kill(ptr3 -> pid, SIGCONT) < 0){
                            kill(ptr3 -> pid, SIGTERM);
                            ptr3 -> status = 0;
                        }
                    }else{
                        puts("Not a valid job or cannot call bg on this job");
                    }
                }
            }
        }else if(strcmp(formattedInput[0], "fg") == 0){
            if(wordCount < 2){
                puts("fg: Not enough arguments");
            
            }else if(wordCount > 2){
                puts("fg: Too many arguments");
            }else{
                job* ptr3 = head;
                int a = getNumFromStr(formattedInput);
                if(a > numJobs || a == -1){
                    puts("Not a valid job");
                }else{
                    for(int i = 1; i < a; i++){
                        ptr3 = ptr3 -> next;
                    }
                    if(ptr3 -> status == 1 || ptr3 -> bg == 1){
                        ptr3 -> bg = 0; //set to foreground
                        if(ptr3 -> status == 1){
                            ptr3 -> status = 2;
                            if(kill(ptr3 -> pid, SIGCONT) < 0){
                                kill(ptr3 -> pid, SIGTERM);
                                ptr3 -> status = 0;
                            }
                            waitpid(ptr3 -> pid, NULL, WUNTRACED);
                        }
                    }else{
                        puts("Not a valid job or cannot call fg on this job");
                    }
                }
            }
        }else if(strcmp(formattedInput[0], "jobs") == 0){
            job* ptr = head;
            while(ptr != NULL){
                if(ptr -> status == 1){ //paused
                    printf("[%d] %d Stopped %s\n", ptr -> job_id, ptr -> pid, ptr -> filename);
                }else if(ptr -> status == 2){ //running
                    printf("[%d] %d Running %s\n", ptr -> job_id, ptr -> pid, ptr -> filename);
                }
                ptr = ptr -> next;
            }
        }else if(strcmp(formattedInput[0], "kill") == 0){
            if(wordCount < 2){
                puts("kill: Not enough arguments");
            
            }else if(wordCount > 2){
                puts("kill: Too many arguments");
            }else{
                job* ptr3 = head;
                int a = getNumFromStr(formattedInput);
                if(a > numJobs || a == -1){
                    puts("Not a valid job");
                }else{
                    for(int i = 1; i < a; i++){
                        ptr3 = ptr3 -> next;
                    }
                    if(ptr3 -> status != 0){
                        ptr3 -> status = 0;
                        kill(ptr3 -> pid, SIGTERM);
                        printf("[%d] %d terminated by signal 15\n", ptr3 -> job_id, ptr3 -> pid);
                    }else{
                        puts("Not a valid job");
                    }
                }
            }
        }else if(strcmp(formattedInput[0], "cd") == 0){
            char* PWD  = cd(formattedInput, wordCount);
            setenv("PWD", PWD, 1);
            printf("%s\n", PWD);
            free(PWD);
        }else if(strcmp(formattedInput[0], "exit") == 0){
            job* pointer = head;
            while(pointer != NULL){
                free(pointer -> filename);
                job* temp = pointer -> next;
                free(pointer);
                pointer = temp;
            }
            for(int i = 0; i < wordCount; i++){
                free(formattedInput[i]);
            }
            free(formattedInput);
            exit(0);
        }else{
            printf("%s: command not found\n", formattedInput[0]);
        }

        
        for(int i = 0; i < wordCount; i++){
            free(formattedInput[i]);
        }
        free(formattedInput);


    }

    return 0;
}
