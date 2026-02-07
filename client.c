#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "common.h"

#define SERVER_HOST_IP "127.0.0.1"
#define SERVER_PORT 8080

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void read_string(char *buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
    } else {
        buffer[0] = '\0';
    }
}

void display_course_details(const struct Course *c) {
    printf("  Course ID: %s, Name: %s, Faculty Roll: %d, Total Seats: %d, Available: %d, Active: %s\n",
           c->courseID, c->courseName, c->facultyRollNo, c->totalSeats, c->availableSeats, c->isActive ? "Yes" : "No");
}

void display_student_details_client(const struct Student *s) {
    printf("  Student Roll No: %d, Name: %s, Password: %s, Active: %s\n",
           s->rollNo, s->name, s->password, s->isActive ? "Yes" : "No");
}

void display_faculty_details_client(const struct Faculty *f) {
    printf("  Faculty Roll No: %d, Name: %s, Password: %s\n",
           f->rollNo, f->name, f->password);
}


void display_admin_menu() {
    printf("\n.... Welcome to Admin Menu ....\n");
    printf("1. Add Student\n");
    printf("2. View Student Details\n");
    printf("3. Add Faculty\n");
    printf("4. View Faculty Details\n");
    printf("5. Activate Student\n");
    printf("6. Block Student\n");
    printf("7. Modify Student Details\n");
    printf("8. Modify Faculty Details\n");
    printf("9. Logout and Exit\n");
    printf("Enter Your Choice: ");
}

void handle_admin_session(int sock_fd) {
    struct Request req;
    struct Response res;
    int choice;

    while (1) {
        display_admin_menu();
        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        clear_input_buffer();

        memset(&req, 0, sizeof(struct Request));

        switch (choice) {
            case 1:
                req.operation = ADMIN_ADD_STUDENT;
                printf("Enter Student Roll No: ");
                if (scanf("%d", &req.data_payload.student_data.rollNo) != 1) { clear_input_buffer(); printf("Invalid Roll No.\n"); continue;}
                clear_input_buffer();
                printf("Enter Student Name: ");
                read_string(req.data_payload.student_data.name, MAX_NAME_LEN);
                printf("Enter Student Password: ");
                read_string(req.data_payload.student_data.password, MAX_PASSWORD_LEN);
                break;
            case 2:
                req.operation = ADMIN_VIEW_STUDENT_DETAILS;
                printf("Enter Student Roll No to View: ");
                if (scanf("%d", &req.data_payload.target_rollNo_generic) != 1) { clear_input_buffer(); printf("Invalid Roll No.\n"); continue;}
                clear_input_buffer();
                break;
            case 3:
                req.operation = ADMIN_ADD_FACULTY;
                printf("Enter Faculty Roll No: ");
                 if (scanf("%d", &req.data_payload.faculty_data.rollNo) != 1) { clear_input_buffer(); printf("Invalid Roll No.\n"); continue;}
                clear_input_buffer();
                printf("Enter Faculty Name: ");
                read_string(req.data_payload.faculty_data.name, MAX_NAME_LEN);
                printf("Enter Faculty Password: ");
                read_string(req.data_payload.faculty_data.password, MAX_PASSWORD_LEN);
                break;
            case 4:
                req.operation = ADMIN_VIEW_FACULTY_DETAILS;
                printf("Enter Faculty Roll No to View: ");
                if (scanf("%d", &req.data_payload.target_rollNo_generic) != 1) { clear_input_buffer(); printf("Invalid Roll No.\n"); continue;}
                clear_input_buffer();
                break;
            case 5:
                req.operation = ADMIN_ACTIVATE_STUDENT;
                printf("Enter Student Roll No to Activate: ");
                if (scanf("%d", &req.data_payload.target_rollNo_generic) != 1) { clear_input_buffer(); printf("Invalid Roll No.\n"); continue;}
                clear_input_buffer();
                break;
            case 6:
                req.operation = ADMIN_BLOCK_STUDENT;
                printf("Enter Student Roll No to Block: ");
                if (scanf("%d", &req.data_payload.target_rollNo_generic) != 1) { clear_input_buffer(); printf("Invalid Roll No.\n"); continue;}
                clear_input_buffer();
                break;
            case 7:
                req.operation = ADMIN_MODIFY_STUDENT;
                printf("Enter Student Roll No to Modify: ");
                if (scanf("%d", &req.data_payload.user_modification_data.target_rollNo) != 1) { clear_input_buffer(); printf("Invalid Roll No.\n"); continue;}
                clear_input_buffer();
                printf("Enter New Student Name: ");
                read_string(req.data_payload.user_modification_data.new_name, MAX_NAME_LEN);
                printf("Enter New Student Password: ");
                read_string(req.data_payload.user_modification_data.new_password, MAX_PASSWORD_LEN);
                break;
            case 8:
                req.operation = ADMIN_MODIFY_FACULTY;
                printf("Enter Faculty Roll No to Modify: ");
                if (scanf("%d", &req.data_payload.user_modification_data.target_rollNo) != 1) { clear_input_buffer(); printf("Invalid Roll No.\n"); continue;}
                clear_input_buffer();
                printf("Enter New Faculty Name: ");
                read_string(req.data_payload.user_modification_data.new_name, MAX_NAME_LEN);
                printf("Enter New Faculty Password: ");
                read_string(req.data_payload.user_modification_data.new_password, MAX_PASSWORD_LEN);
                break;
            case 9:
                req.operation = ADMIN_LOGOUT;
                write(sock_fd, &req, sizeof(struct Request));
                read(sock_fd, &res, sizeof(struct Response));
                printf("Server: %s\n", res.message);
                return;
            default:
                printf("Invalid choice. Please try again.\n");
                continue;
        }

        if (write(sock_fd, &req, sizeof(struct Request)) <= 0) {
            printf("Connection to server lost.\n");
            return;
        }

        memset(&res, 0, sizeof(struct Response));
        if (read(sock_fd, &res, sizeof(struct Response)) <= 0) {
            printf("Connection to server lost.\n");
            return;
        }

        printf("Server: %s\n", res.message);
        if (res.status_code == GENERIC_SUCCESS) {
            if (req.operation == ADMIN_VIEW_STUDENT_DETAILS) {
                display_student_details_client(&res.data_payload.student_data);
            } else if (req.operation == ADMIN_VIEW_FACULTY_DETAILS) {
                display_faculty_details_client(&res.data_payload.faculty_data);
            }
        }
    }
}

void display_student_menu() {
    printf("\n.... Welcome to Student Menu ....\n");
    printf("1. View All Courses\n");
    printf("2. Enroll (pick) New Course\n");
    printf("3. Drop Course\n");
    printf("4. View Enrolled Course Details\n");
    printf("5. Change Password\n");
    printf("6. Logout and Exit\n");
    printf("Enter Your Choice: ");
}

void handle_student_session(int sock_fd, int student_rollno) {
    struct Request req;
    struct Response res;
    int choice;

    req.current_user_rollno = student_rollno;

    while (1) {
        display_student_menu();
        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        clear_input_buffer();

        memset(&req, 0, sizeof(struct Request));
        req.current_user_rollno = student_rollno;

        switch (choice) {
            case 1:
                req.operation = STUDENT_VIEW_ALL_COURSES;
                break;
            case 2:
                req.operation = STUDENT_ENROLL_COURSE;
                printf("Enter Course ID to Enroll: ");
                read_string(req.data_payload.target_courseID_generic, MAX_COURSE_ID_LEN);
                break;
            case 3:
                req.operation = STUDENT_DROP_COURSE;
                printf("Enter Course ID to Drop: ");
                read_string(req.data_payload.target_courseID_generic, MAX_COURSE_ID_LEN);
                break;
            case 4:
                req.operation = STUDENT_VIEW_ENROLLED_COURSES;
                break;
            case 5:
                req.operation = STUDENT_CHANGE_PASSWORD;
                printf("Enter New Password: ");
                read_string(req.data_payload.new_password_only, MAX_PASSWORD_LEN);
                break;
            case 6:
                req.operation = STUDENT_LOGOUT;
                write(sock_fd, &req, sizeof(struct Request));
                read(sock_fd, &res, sizeof(struct Response));
                printf("Server: %s\n", res.message);
                return;
            default:
                printf("Invalid choice. Please try again.\n");
                continue;
        }

        if (write(sock_fd, &req, sizeof(struct Request)) <= 0) {
            printf("Connection to server lost.\n");
            return;
        }

        memset(&res, 0, sizeof(struct Response));
        if (read(sock_fd, &res, sizeof(struct Response)) <= 0) {
            printf("Connection to server lost.\n");
            return;
        }

        printf("Server: %s\n", res.message);
        if (res.status_code == GENERIC_SUCCESS) {
            if (req.operation == STUDENT_VIEW_ALL_COURSES || req.operation == STUDENT_VIEW_ENROLLED_COURSES) {
                if (res.data_count > 0) {
                    printf("Courses (%d):\n", res.data_count);
                    for (int i = 0; i < res.data_count; i++) {
                        display_course_details(&res.data_payload.course_array[i]);
                    }
                } else {
                }
            }
        }
    }
}


void display_faculty_menu() {
    printf("\n.... Welcome to Faculty Menu ....\n");
    printf("1. View Offering Courses\n");
    printf("2. Add New Course\n");
    printf("3. Remove Course from Catalog\n");
    printf("4. Update Course Details\n");
    printf("5. Change Password\n");
    printf("6. Logout and Exit\n");
    printf("Enter Your Choice: ");
}

void handle_faculty_session(int sock_fd, int faculty_rollno) {
    struct Request req;
    struct Response res;
    int choice;

    req.current_user_rollno = faculty_rollno;

    while (1) {
        display_faculty_menu();
        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        clear_input_buffer();

        memset(&req, 0, sizeof(struct Request));
        req.current_user_rollno = faculty_rollno;

        switch (choice) {
            case 1:
                req.operation = FACULTY_VIEW_OFFERING_COURSES;
                break;
            case 2:
                req.operation = FACULTY_ADD_NEW_COURSE;
                printf("Enter New Course ID: ");
                read_string(req.data_payload.course_data.courseID, MAX_COURSE_ID_LEN);
                printf("Enter New Course Name: ");
                read_string(req.data_payload.course_data.courseName, MAX_COURSE_NAME_LEN);
                printf("Enter Total Seats for Course: ");
                if (scanf("%d", &req.data_payload.course_data.totalSeats) != 1) {clear_input_buffer(); printf("Invalid seats.\n"); continue;}
                clear_input_buffer();
                break;
            case 3:
                req.operation = FACULTY_REMOVE_COURSE;
                printf("Enter Course ID to Remove: ");
                read_string(req.data_payload.target_courseID_generic, MAX_COURSE_ID_LEN);
                break;
            case 4:
                req.operation = FACULTY_UPDATE_COURSE_DETAILS;
                printf("Enter Course ID to Update: ");
                read_string(req.data_payload.course_update_data.target_courseID, MAX_COURSE_ID_LEN);
                printf("Enter New Course Name (or press Enter to keep current): ");
                read_string(req.data_payload.course_update_data.new_course_name, MAX_COURSE_NAME_LEN);
                printf("Enter New Total Seats (or -1 to keep current): ");
                if (scanf("%d", &req.data_payload.course_update_data.new_total_seats) != 1) {clear_input_buffer(); printf("Invalid seats.\n"); continue;}
                clear_input_buffer();
                break;
            case 5:
                req.operation = FACULTY_CHANGE_PASSWORD;
                printf("Enter New Password: ");
                read_string(req.data_payload.new_password_only, MAX_PASSWORD_LEN);
                break;
            case 6:
                req.operation = FACULTY_LOGOUT;
                write(sock_fd, &req, sizeof(struct Request));
                read(sock_fd, &res, sizeof(struct Response));
                printf("Server: %s\n", res.message);
                return;
            default:
                printf("Invalid choice. Please try again.\n");
                continue;
        }

        if (write(sock_fd, &req, sizeof(struct Request)) <= 0) {
            printf("Connection to server lost.\n");
            return;
        }

        memset(&res, 0, sizeof(struct Response));
        if (read(sock_fd, &res, sizeof(struct Response)) <= 0) {
            printf("Connection to server lost.\n");
            return;
        }

        printf("Server: %s\n", res.message);
        if (res.status_code == GENERIC_SUCCESS) {
            if (req.operation == FACULTY_VIEW_OFFERING_COURSES) {
                if (res.data_count > 0) {
                    printf("Your Offered Courses (%d):\n", res.data_count);
                    for (int i = 0; i < res.data_count; i++) {
                        display_course_details(&res.data_payload.course_array[i]);
                    }
                } else {
                }
            }
        }
    }
}

int main() {
    int sock_fd;
    struct sockaddr_in serv_addr;
    struct Request login_req;
    struct Response login_res;
    int user_choice_main;

    printf("Welcome Back to Academia :: Course Registration\n");
    printf("Login Type\n");
    printf("Enter Your Choice { 1.Admin , 2.Professor , 3.Student }: ");

    if (scanf("%d", &user_choice_main) != 1) {
        clear_input_buffer();
        printf("Invalid choice. Exiting.\n");
        return 1;
    }
    clear_input_buffer();

    login_req.operation = LOGIN_REQUEST;

    switch (user_choice_main) {
        case 1: login_req.user_type = USER_TYPE_ADMIN; break;
        case 2: login_req.user_type = USER_TYPE_FACULTY; break;
        case 3: login_req.user_type = USER_TYPE_STUDENT; break;
        default:
            printf("Invalid user type selected. Exiting.\n");
            return 1;
    }

    printf("Enter Login ID: ");
    read_string(login_req.login_id, MAX_NAME_LEN);
    printf("Enter Password: ");
    read_string(login_req.password, MAX_PASSWORD_LEN);

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        printf("Socket creation failed.\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_HOST_IP, &serv_addr.sin_addr) <= 0) {
        printf("Invalid server address.\n");
        close(sock_fd);
        return -1;
    }

    if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection to server failed.\n");
        close(sock_fd);
        return -1;
    }
    printf("Connected to server.\n");

    if (write(sock_fd, &login_req, sizeof(struct Request)) <= 0) {
        printf("Failed to send login request.\n");
        close(sock_fd);
        return -1;
    }

    if (read(sock_fd, &login_res, sizeof(struct Response)) <= 0) {
        printf("Failed to receive login response.\n");
        close(sock_fd);
        return -1;
    }

    printf("Server: %s\n", login_res.message);

    if (login_res.status_code == LOGIN_SUCCESS_ADMIN) {
        handle_admin_session(sock_fd);
    } else if (login_res.status_code == LOGIN_SUCCESS_STUDENT) {
        handle_student_session(sock_fd, login_res.data_payload.logged_in_user_rollno);
    } else if (login_res.status_code == LOGIN_SUCCESS_FACULTY) {
        handle_faculty_session(sock_fd, login_res.data_payload.logged_in_user_rollno);
    } else {
    }

    close(sock_fd);
    printf("Disconnected from server. Exiting client.\n");
    return 0;
}
