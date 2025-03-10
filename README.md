Chat Management and Moderation System
Course: Operating Systems
Semester: Second Semester 2024-25
Submission Date: February 28,2025
Author: Lokesh Sathish
Language: POSIX-compliant C
Tested Environment: Ubuntu 22.04 / Ubuntu 24.04

Project Overview
This project implements a real-time chat management and moderation system designed to monitor user conversations within chat groups. The system identifies restricted words (filtered words) in user messages, tracks violations, and removes users who exceed a predefined violation threshold. Each group terminates automatically when fewer than two active users remain.

Project Structure
The main components of this project are:

app.c: Manages overall application flow, spawns group processes, and tracks active groups.

groups.c: Represents individual chat groups, manages user processes, message ordering, communication with moderator and validation processes.

moderator.c: Monitors messages for violations, tracks user violations count, and decides on user removal based on violation thresholds.

The provided executable validation.out validates message ordering and correctness as per the project guidelines.

Directory Structure
Ensure your directory structure matches the following format:

text
.
├── app.c
├── groups.c
├── moderator.c
├── validation.out
└── testcase_X
    ├── input.txt
    ├── filtered_words.txt
    ├── groups
    │   ├── group_0.txt
    │   ├── group_1.txt
    │   └── ...
    └── users
        ├── user_0_0.txt
        ├── user_0_1.txt
        └── ...
Replace X with your test case number.

Compilation Instructions
Compile the three provided source files separately as follows:

bash
gcc -Wall -Wextra -o app.out app.c
gcc -Wall -Wextra -o groups.out groups.c
gcc -Wall -Wextra -o moderator.out moderator.c
Set permissions for the provided validation executable:

bash
chmod 777 validation.out
Execution Instructions
To run the project for a specific test case (replace X with your test case number):

Step 1: Run Validation Process
In Terminal 1:

bash
./validation.out X
Step 2: Run Moderator Process
In Terminal 2:

bash
./moderator.out X
Step 3: Run App Process (spawns all groups automatically)
In Terminal 3:

bash
./app.out X
No need to execute groups.out directly; it's automatically executed by app.out.

Expected Output
Upon successful execution of a test case, you will see outputs similar to the following:

Validation Terminal:

text
Number of groups created: N
Number of users created: M
Number of messages received: K
Number of users deleted: D
Number of groups deleted: G
Moderator Terminal (example):

text
User 5 from group 2 has been removed due to 6 violations.
User 3 from group 1 has been removed due to 5 violations.
...
App Terminal (example):

text
All users terminated. Exiting group process 0.
All users terminated. Exiting group process 1.
...
