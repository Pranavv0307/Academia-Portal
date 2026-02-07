#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

// Credentials
#define ADMIN_LOGIN_ID "Admin"
#define ADMIN_PASSWORD "Admin"

// Data files
#define STUDENT_DATA_FILE "students.dat"
#define FACULTY_DATA_FILE "faculty.dat"
#define COURSE_DATA_FILE "courses.dat"
#define ENROLLMENT_DATA_FILE "enrollments.dat"

// Limits
#define MAX_NAME_LEN 50
#define MAX_PASSWORD_LEN 20
#define MAX_COURSE_ID_LEN 10
#define MAX_COURSE_NAME_LEN 50
#define MAX_COURSES_PER_STUDENT 10
#define MAX_COURSES_PER_FACULTY 20
#define MAX_COURSES_IN_SYSTEM 30

struct Student {
    int rollNo;
    char name[MAX_NAME_LEN];
    char password[MAX_PASSWORD_LEN];
    bool isActive;
};

struct Faculty {
    int rollNo;
    char name[MAX_NAME_LEN];
    char password[MAX_PASSWORD_LEN];
};

struct Course {
    char courseID[MAX_COURSE_ID_LEN];
    char courseName[MAX_COURSE_NAME_LEN];
    int facultyRollNo;
    int totalSeats;
    int availableSeats;
    bool isActive;
};

struct Enrollment {
    int studentRollNo;
    char courseID[MAX_COURSE_ID_LEN];
};

#define USER_TYPE_ADMIN 1
#define USER_TYPE_FACULTY 2
#define USER_TYPE_STUDENT 3

#define LOGIN_REQUEST 1

#define ADMIN_ADD_STUDENT 20
#define ADMIN_VIEW_STUDENT_DETAILS 21
#define ADMIN_ADD_FACULTY 22
#define ADMIN_VIEW_FACULTY_DETAILS 23
#define ADMIN_ACTIVATE_STUDENT 24
#define ADMIN_BLOCK_STUDENT 25
#define ADMIN_MODIFY_STUDENT 26
#define ADMIN_MODIFY_FACULTY 27
#define ADMIN_LOGOUT 29

#define STUDENT_VIEW_ALL_COURSES 31
#define STUDENT_ENROLL_COURSE 32
#define STUDENT_DROP_COURSE 33
#define STUDENT_VIEW_ENROLLED_COURSES 34
#define STUDENT_CHANGE_PASSWORD 35
#define STUDENT_LOGOUT 39

#define FACULTY_VIEW_OFFERING_COURSES 41
#define FACULTY_ADD_NEW_COURSE 42
#define FACULTY_REMOVE_COURSE 43
#define FACULTY_UPDATE_COURSE_DETAILS 44
#define FACULTY_CHANGE_PASSWORD 45
#define FACULTY_LOGOUT 49

#define LOGIN_SUCCESS_ADMIN 10
#define LOGIN_SUCCESS_STUDENT 11
#define LOGIN_SUCCESS_FACULTY 12
#define LOGIN_FAILURE 13
#define GENERIC_SUCCESS 100
#define GENERIC_FAILURE 101
#define RECORD_NOT_FOUND 102
#define RECORD_EXISTS 103
#define INVALID_CHOICE 104
#define COURSE_FULL 105
#define ALREADY_ENROLLED 106
#define NOT_ENROLLED 107
#define STUDENT_BLOCKED 108
#define COURSE_NOT_ACTIVE 109
#define UNAUTHORIZED_ACTION 110

struct Request {
    int operation;
    int user_type;
    char login_id[MAX_NAME_LEN];
    char password[MAX_PASSWORD_LEN];
    int current_user_rollno;

    union {
        struct Student student_data;
        struct Faculty faculty_data;
        struct Course course_data;

        struct {
            int target_rollNo;
            char new_name[MAX_NAME_LEN];
            char new_password[MAX_PASSWORD_LEN];
        } user_modification_data;

        int target_rollNo_generic;
        char target_courseID_generic[MAX_COURSE_ID_LEN];

        char new_password_only[MAX_PASSWORD_LEN];

        struct {
            char target_courseID[MAX_COURSE_ID_LEN];
            char new_course_name[MAX_COURSE_NAME_LEN];
            int new_total_seats;
        } course_update_data;

    } data_payload;
};

struct Response {
    int status_code;
    char message[250];
    int data_count;
    union {
        struct Student student_data;
        struct Faculty faculty_data;
        struct Course course_array[MAX_COURSES_IN_SYSTEM];
        int logged_in_user_rollno;
    } data_payload;
};

#endif
