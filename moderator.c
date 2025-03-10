#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#define MAX_FILTERED_WORDS 50
#define MAX_WORD_LENGTH 20
#define MAX_PATH_LENGTH 256
#define MAX_USERS 1500

typedef struct {
    long mtype;
    int timestamp;
    int user;
    char mtext[256];
    int modifyingGroup;
} Message;

char filtered_words[MAX_FILTERED_WORDS][MAX_WORD_LENGTH];
int num_filtered_words = 0;
int max_violations;
int msgq_moderator;

struct UserViolations {
    int groupid;
    int userid;
    int violations;
};
struct UserViolations users[MAX_USERS];
int num_users = 0;

void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

void load_filtered_words(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open filtered_words.txt");
        exit(1);
    }
    while (num_filtered_words < MAX_FILTERED_WORDS && fscanf(file, "%19s", filtered_words[num_filtered_words]) == 1) {
        to_lowercase(filtered_words[num_filtered_words]);
        num_filtered_words++;
    }
    fclose(file);
}

int find_user(int group_id, int user_id) {
    for (int i = 0; i < num_users; i++) {
        if (users[i].groupid == group_id && users[i].userid == user_id)
            return i;
    }
    return -1;
}

void update_violations(int group_id, int user_id, int count) {
    int index = find_user(group_id, user_id);
    if (index == -1) {
        users[num_users].groupid = group_id;
        users[num_users].userid = user_id;
        users[num_users].violations = count;
        index = num_users;
        num_users++;
    } else {
        users[index].violations += count;
    }

    Message response;
    response.user = user_id;
    response.modifyingGroup = group_id;
    response.mtype = group_id + 100; // Ensure only the correct groups.c instance gets this message

    if (users[index].violations >= max_violations) {
        snprintf(response.mtext, sizeof(response.mtext), "user has to be removed");
        printf("User %d from Group %d has been removed due to %d violations.\n", user_id, group_id, users[index].violations);
    } else {
        snprintf(response.mtext, sizeof(response.mtext), "user can continue");
    }
    msgsnd(msgq_moderator, &response, sizeof(response) - sizeof(long), 0);
}

int count_violations(char *message) {
    int count = 0;
    char temp[256];
    strcpy(temp, message);
    to_lowercase(temp);

    for (int i = 0; i < num_filtered_words; i++) {
        if (strstr(temp, filtered_words[i]) != NULL) {
            count++;
        }
    }
    return count;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <test_case_number>\n", argv[0]);
        return 1;
    }
    int test_case_number = atoi(argv[1]);
    char testcase_dir[MAX_PATH_LENGTH];
    snprintf(testcase_dir, sizeof(testcase_dir), "testcase_%d", test_case_number);
    
    char input_file_path[MAX_PATH_LENGTH];
    snprintf(input_file_path, sizeof(input_file_path), "%.245s/input.txt", testcase_dir);
    
    char filtered_words_path[MAX_PATH_LENGTH];
    snprintf(filtered_words_path, sizeof(filtered_words_path), "%.236s/filtered_words.txt", testcase_dir);
    
    FILE *file = fopen(input_file_path, "r");
    if (!file) {
        perror("Failed to open input.txt");
        return 1;
    }
    
    int dummy, validation_msg_queue_key, app_msg_queue_key;
    if (fscanf(file, "%d %d %d %d %d", &dummy, &validation_msg_queue_key, &app_msg_queue_key, &msgq_moderator, &max_violations) != 5) {
        fprintf(stderr, "Error reading input.txt\n");
        fclose(file);
        return 1;
    }
    fclose(file);
    
    load_filtered_words(filtered_words_path);
    
    msgq_moderator = msgget(msgq_moderator, 0666 | IPC_CREAT);
    if (msgq_moderator == -1) {
        perror("Failed to create message queue");
        return 1;
    }
    
    Message msg;
    while (1) {
        if (msgrcv(msgq_moderator, &msg, sizeof(msg) - sizeof(long), 0, 0) == -1) {
            perror("msgrcv failed");
            exit(1);
        }
        int violations = count_violations(msg.mtext);
        update_violations(msg.modifyingGroup, msg.user, violations);
    }
    return 0;
}
