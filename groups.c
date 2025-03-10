#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_PATH_LENGTH 256 
#define MAX_USERS 50 
#define MAX_MSGS 1000 
#define BUFFER_SIZE 256 
#define MAX_NUMBER_GROUPS 30

typedef struct 
{ 
    long mtype; 
    int timestamp; 
    int user; 
    char mtext[256]; 
    int modifyingGroup; 
} Message;

typedef struct 
{ 
    int timestamp; 
    int user; 
    char mtext[256]; 
} UserMessage;

int compareMessages(const void *a, const void *b) 
{ 
    UserMessage *msgA = (UserMessage *)a;
    UserMessage *msgB = (UserMessage *)b;
    return msgA->timestamp - msgB->timestamp;
}


int extract_y(const char *filename) {
    const char *last_underscore = strrchr(filename, '_'); 
    if (!last_underscore)
        return -1;
    int y;
    if (sscanf(last_underscore + 1, "%d.txt", &y) == 1) {
        return y;
    }
    return -1;
}

int main(int argc, char *argv[]) 
{ 
    if(argc != 3)
    { 
        fprintf(stderr, "Usage: %s <testcase number> <group file path>\n", argv[0]); 
        exit(EXIT_FAILURE); 
    }
    int testcase = atoi(argv[1]);
    char *groupFilePath = argv[2];


    int groupNum = 0;
    char *p = strstr(groupFilePath, "group_");
    if(p) {
        sscanf(p, "group_%d", &groupNum);
        printf("Debug: groupNum = %d\n", groupNum);
    }


    char inputFilePath[MAX_PATH_LENGTH];
    snprintf(inputFilePath, sizeof(inputFilePath), "testcase_%d/input.txt", testcase);
    FILE *inputFP = fopen(inputFilePath, "r");
    if(!inputFP) {
        perror("Failed to open input.txt in groups.c");
        exit(EXIT_FAILURE);
    }
    int dummy, numGroups, mq_validation_key, mq_app_key, mq_moderator_key, threshold;
    if(fscanf(inputFP, "%d %d %d %d %d", &dummy, &mq_validation_key, &mq_app_key, &mq_moderator_key, &threshold) != 5) {
        fprintf(stderr, "Error reading IPC keys and threshold from input.txt\n");
        fclose(inputFP);
        exit(EXIT_FAILURE);
    }
    fclose(inputFP);


    int mq_validation = msgget(mq_validation_key, 0666 | IPC_CREAT);
    if(mq_validation == -1) {
        perror("msgget failed for validation queue");
        exit(EXIT_FAILURE);
    }
    int mq_app = msgget(mq_app_key, 0666 | IPC_CREAT);
    if(mq_app == -1) {
        perror("msgget failed for app queue");
        exit(EXIT_FAILURE);
    }
    int mq_moderator = msgget(mq_moderator_key, 0666 | IPC_CREAT);
    if(mq_moderator == -1) {
        perror("msgget failed for moderator queue");
        exit(EXIT_FAILURE);
    }


    Message groupCreateMsg;
    groupCreateMsg.mtype = 1;
    groupCreateMsg.modifyingGroup = groupNum;
    if(msgsnd(mq_validation, &groupCreateMsg, sizeof(Message) - sizeof(groupCreateMsg.mtype), 0) == -1) {
        perror("msgsnd failed for group creation");
        exit(EXIT_FAILURE);
    }

    char fullGroupPath[MAX_PATH_LENGTH];
    snprintf(fullGroupPath, sizeof(fullGroupPath), "testcase_%d/%s", testcase, groupFilePath);
    printf("Debug: fullGroupPath = %s\n", fullGroupPath);
    FILE *groupFile = fopen(fullGroupPath, "r");
    if(!groupFile) {
        perror("Failed to open group file");
        exit(EXIT_FAILURE);
    }

    int numUsers;
    if(fscanf(groupFile, "%d", &numUsers) != 1) {
        fprintf(stderr, "Error reading number of users from group file\n");
        fclose(groupFile);
        exit(EXIT_FAILURE);
    }
    printf("Debug: numUsers = %d\n", numUsers);


    char userFilePaths[MAX_USERS][MAX_PATH_LENGTH];
    int userMapping[MAX_USERS]; 
    for (int i = 0; i < numUsers; i++) {
        if(fscanf(groupFile, "%s", userFilePaths[i]) != 1) {
            fprintf(stderr, "Error reading user file path for user %d\n", i);
            fclose(groupFile);
            exit(EXIT_FAILURE);
        }
        int y = extract_y(userFilePaths[i]);
        if(y == -1) {
            fprintf(stderr, "Error extracting y from filename: %s\n", userFilePaths[i]);
            exit(EXIT_FAILURE);
        }
        userMapping[i] = y;
        printf("Debug: User %d file: %s => y = %d\n", i, userFilePaths[i], y);
    }
    fclose(groupFile);

    int userActive[MAX_USERS] = {0}; 
    for (int i = 0; i < numUsers; i++) {
        int y = userMapping[i];
        if (y >= 0 && y < MAX_USERS)
            userActive[y] = 1; 
        else {
            fprintf(stderr, "Extracted y value %d out of bounds for user %d\n", y, i);
            exit(EXIT_FAILURE);
        }
    }


    int lastMsgIndex[MAX_USERS];
    for (int i = 0; i < MAX_USERS; i++) {
        lastMsgIndex[i] = -1;
    }

    int pipes[numUsers][2];
    pid_t userPIDs[numUsers];
    for (int i = 0; i < numUsers; i++) {
        if(pipe(pipes[i]) == -1) {
            perror("pipe creation failed");
            exit(EXIT_FAILURE);
        }
        pid_t pid = fork();
        if(pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        if(pid == 0) { 
            close(pipes[i][0]); 
            char fullUserPath[MAX_PATH_LENGTH];
            snprintf(fullUserPath, sizeof(fullUserPath), "testcase_%d/%.240s", testcase, userFilePaths[i]);
            FILE *userFP = fopen(fullUserPath, "r");
            if(!userFP) {
                perror("Failed to open user file");
                exit(EXIT_FAILURE);
            }
            char line[BUFFER_SIZE];
            while(fgets(line, sizeof(line), userFP)) {
                char padded[BUFFER_SIZE];
                memset(padded, ' ', BUFFER_SIZE);
                strncpy(padded, line, BUFFER_SIZE);
                if(write(pipes[i][1], padded, BUFFER_SIZE) == -1) {
                    perror("write to pipe failed");
                    fclose(userFP);
                    exit(EXIT_FAILURE);
                }
            }
            fclose(userFP);
            close(pipes[i][1]);
            exit(EXIT_SUCCESS);
        }
        else { 
            userPIDs[i] = pid;
            close(pipes[i][1]);
            Message userCreateMsg;
            userCreateMsg.mtype = 2;
            userCreateMsg.modifyingGroup = groupNum;
            userCreateMsg.user = userMapping[i]; 
            if(msgsnd(mq_validation, &userCreateMsg, sizeof(Message) - sizeof(userCreateMsg.mtype), 0) == -1) {
                perror("msgsnd failed for user creation");
                exit(EXIT_FAILURE);
            }
        }
    }


    UserMessage messages[MAX_MSGS];
    int msgCount = 0;
    char buf[BUFFER_SIZE];
    for (int i = 0; i < numUsers; i++) {
        ssize_t n;
        while((n = read(pipes[i][0], buf, BUFFER_SIZE)) > 0) {
            int ts;
            char text[256];
            if(sscanf(buf, "%d %s", &ts, text) != 2) {
                fprintf(stderr, "Error parsing message from user %d\n", i);
                continue;
            }
            messages[msgCount].timestamp = ts;

            messages[msgCount].user = userMapping[i];
            strncpy(messages[msgCount].mtext, text, sizeof(messages[msgCount].mtext));
            msgCount++;
            if(msgCount >= MAX_MSGS) {
                fprintf(stderr, "Too many messages; increase MAX_MSGS\n");
                break;
            }
        }
        close(pipes[i][0]);
    }


    qsort(messages, msgCount, sizeof(UserMessage), compareMessages);


    for (int i = 0; i < msgCount; i++) {
        int y = messages[i].user;
        lastMsgIndex[y] = i;
    }


    int activeCount = numUsers;
    int usersRemoved = 0;


    for (int i = 0; i < msgCount; i++) {
        int y = messages[i].user; 
        if(!userActive[y])
            continue;

        Message outMsg;
        outMsg.mtype = MAX_NUMBER_GROUPS + groupNum;  
        outMsg.timestamp = messages[i].timestamp;
        outMsg.user = y; 
        strncpy(outMsg.mtext, messages[i].mtext, sizeof(outMsg.mtext));
        outMsg.modifyingGroup = groupNum;


        if(msgsnd(mq_validation, &outMsg, sizeof(Message) - sizeof(outMsg.mtype), 0) == -1) {
            perror("msgsnd failed sending message to validation");
        }

        if(msgsnd(mq_moderator, &outMsg, sizeof(Message) - sizeof(outMsg.mtype), 0) == -1) {
            perror("msgsnd failed sending message to moderator");
        }


        Message modReply;
        if(msgrcv(mq_moderator, &modReply, sizeof(Message) - sizeof(modReply.mtype), groupNum+100, 0) == -1) {
            perror("msgrcv failed waiting for moderator reply");
        } else {
            if(strncmp(modReply.mtext, "user has to be removed", 22) == 0) {
                if(userActive[y]) {
                    userActive[y] = 0;
                    activeCount--;
                    usersRemoved++;
                }
            }
        }


        if(i == lastMsgIndex[y] && userActive[y]) {
            userActive[y] = 0;
            activeCount--;
        }

        if(activeCount < 2) {
            Message termMsg;
            termMsg.mtype = 3;  
            termMsg.modifyingGroup = groupNum;
            termMsg.user = usersRemoved;  
            if(msgsnd(mq_validation, &termMsg, sizeof(Message) - sizeof(termMsg.mtype), 0) == -1) {
                perror("msgsnd failed sending termination to validation");
            }
            if(msgsnd(mq_app, &termMsg, sizeof(Message) - sizeof(termMsg.mtype), 0) == -1) {
                perror("msgsnd failed sending termination to app");
            }
            break;
        }
    }

    for (int i = 0; i < numUsers; i++) {
        waitpid(userPIDs[i], NULL, 0);
    }

    printf("Processing complete. Users removed: %d\n", usersRemoved);
    return EXIT_SUCCESS;
}
