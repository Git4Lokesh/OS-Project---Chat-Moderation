#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <errno.h> 
#include <sys/wait.h>
#define MAX_PATH_LENGTH 256 
#define MAX_GROUPS 30
typedef struct 
{ 
    long mtype; 
    int timestamp; 
    int user; 
    char mtext[256]; 
    int modifyingGroup; 
}
Message;
int main(int argc, char *argv[])
{ 
    if(argc != 2)
    { fprintf(stderr, "Usage: %s <testcase number>\n", argv[0]); 
        exit(EXIT_FAILURE); 
    }
    int testcase = atoi(argv[1]);
char inputFilePath[MAX_PATH_LENGTH];
snprintf(inputFilePath, sizeof(inputFilePath), "testcase_%d/input.txt", testcase);

FILE *fp = fopen(inputFilePath, "r");
if(!fp){
    perror("Failed to open input.txt");
    exit(EXIT_FAILURE);
}

int numGroups, mq_validation_key, mq_app_key, mq_moderator_key, threshold;
if(fscanf(fp, "%d", &numGroups) != 1){
    fprintf(stderr, "Error reading number of groups\n");
    exit(EXIT_FAILURE);
}
if(fscanf(fp, "%d", &mq_validation_key) != 1){
    fprintf(stderr, "Error reading validation queue key\n");
    exit(EXIT_FAILURE);
}
if(fscanf(fp, "%d", &mq_app_key) != 1){
    fprintf(stderr, "Error reading app queue key\n");
    exit(EXIT_FAILURE);
}
if(fscanf(fp, "%d", &mq_moderator_key) != 1){
    fprintf(stderr, "Error reading moderator queue key\n");
    exit(EXIT_FAILURE);
}
if(fscanf(fp, "%d", &threshold) != 1){
    fprintf(stderr, "Error reading threshold\n");
    exit(EXIT_FAILURE);
}

char groupPaths[MAX_GROUPS][MAX_PATH_LENGTH];
for (int i = 0; i < numGroups; i++){
    if (fscanf(fp, "%s", groupPaths[i]) != 1){
        fprintf(stderr, "Error reading file path for group %d\n", i);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
}
fclose(fp);

int mq_app = msgget(mq_app_key, 0666 | IPC_CREAT);
if(mq_app == -1){
    perror("Failed to create message queue for app");
    exit(EXIT_FAILURE);
}

pid_t pids[MAX_GROUPS];
for (int i = 0; i < numGroups; i++){
    pid_t pid = fork();
    if(pid < 0){
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    if(pid == 0){
        char testcase_str[16];
        snprintf(testcase_str, sizeof(testcase_str), "%d", testcase);
        execl("./groups.out", "./groups.out", testcase_str, groupPaths[i], (char *)NULL);
        perror("execl failed");
        exit(EXIT_FAILURE);
    } else {
        pids[i] = pid;
    }
}


int active_groups = numGroups;
Message msg;
while(active_groups > 0){
    if(msgrcv(mq_app, &msg, sizeof(msg) - sizeof(long), 3, 0) == -1){
        if(errno == EINTR)
            continue;
        perror("msgrcv failed in app");
        break;
    }

    printf("All users terminated. Exiting group process %d.\n", msg.modifyingGroup);
    active_groups--;
}


for (int i = 0; i < numGroups; i++){
    waitpid(pids[i], NULL, 0);
}

return EXIT_SUCCESS;

}
