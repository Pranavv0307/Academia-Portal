#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "common.h"
#include "admin_handler.h"
#include "student_handler.h"
#include "faculty_handler.h"

#define SERVER_PORT 8080
#define MAX_PENDING_CONNECTIONS 10

sem_t operations_semaphore;
long long total_successful_ops = 0;

static ssize_t read_fully(int fd, void *buffer, size_t count) {
    size_t total = 0;
    ssize_t n;
    char *buf = buffer;

    while (total < count) {
        n = read(fd, buf + total, count - total);
        if (n < 0) 
            return -1;
        else if (n==0)
            return total;
        total += n;
    }
    return total;
}

static ssize_t write_fully(int fd, const void *buffer, size_t count) {
    size_t total = 0;
    ssize_t n;
    const char *buf = buffer;

    while (total < count) {
        n = write(fd, buf + total, count - total);
        if (n < 0) 
            return -1;
        else if (n==0)
            return total;
        total += n;
    }
    return total;
}

static void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    struct Request req = {0};
    struct Response res = {0};
    int user_type = 0, user_rollno = -1;

    printf("Thread %lu: Connected to socket %d\n", pthread_self(), client_fd);

    if (read_fully(client_fd, &req, sizeof(req)) == sizeof(req) && req.operation == LOGIN_REQUEST) {
        switch (req.user_type) {
            case USER_TYPE_ADMIN:
                if (!strcmp(req.login_id, ADMIN_LOGIN_ID) && !strcmp(req.password, ADMIN_PASSWORD)) {
                    res.status_code = LOGIN_SUCCESS_ADMIN;
                    strcpy(res.message, "Admin login successful.");
                    user_type = USER_TYPE_ADMIN;
                } else {
                    res.status_code = LOGIN_FAILURE;
                    strcpy(res.message, "Admin login failed.");
                }
                break;

            case USER_TYPE_FACULTY: {
                struct Faculty fac = {0};
                res.status_code = faculty_login_impl(req.login_id, req.password, &fac);
                if (res.status_code == LOGIN_SUCCESS_FACULTY) {
                    strcpy(res.message, "Faculty login successful.");
                    user_type = USER_TYPE_FACULTY;
                    user_rollno = fac.rollNo;
                    res.data_payload.logged_in_user_rollno = user_rollno;
                } else {
                    strcpy(res.message, "Faculty login failed.");
                }
                break;
            }

            case USER_TYPE_STUDENT: {
                struct Student stu = {0};
                res.status_code = student_login_impl(req.login_id, req.password, &stu);
                if (res.status_code == LOGIN_SUCCESS_STUDENT) {
                    strcpy(res.message, "Student login successful.");
                    user_type = USER_TYPE_STUDENT;
                    user_rollno = stu.rollNo;
                    res.data_payload.logged_in_user_rollno = user_rollno;
                } else if (res.status_code == STUDENT_BLOCKED) {
                    strcpy(res.message, "Student account is blocked.");
                } else {
                    strcpy(res.message, "Student login failed.");
                }
                break;
            }

            default:
                res.status_code = LOGIN_FAILURE;
                strcpy(res.message, "Invalid user type.");
                break;
        }
    } else {
        res.status_code = LOGIN_FAILURE;
        strcpy(res.message, "Expected login request.");
    }

    if (write_fully(client_fd, &res, sizeof(res)) != sizeof(res) || user_type == 0) {
        close(client_fd);
        return NULL;
    }

    while (1) {
        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));

        if (read_fully(client_fd, &req, sizeof(req)) != sizeof(req)) break;

        req.current_user_rollno = user_rollno;
        bool op_success = false;

        if ((user_type == USER_TYPE_ADMIN && req.operation == ADMIN_LOGOUT) ||
            (user_type == USER_TYPE_FACULTY && req.operation == FACULTY_LOGOUT) ||
            (user_type == USER_TYPE_STUDENT && req.operation == STUDENT_LOGOUT)) {

            strcpy(res.message, "User logged out.");
            res.status_code = GENERIC_SUCCESS;
            write_fully(client_fd, &res, sizeof(res));
            break;
        }

        if (user_type == USER_TYPE_ADMIN) {
            process_admin_request(&req, &res);
        } else if (user_type == USER_TYPE_FACULTY) {
            process_faculty_request(&req, &res);
        } else if (user_type == USER_TYPE_STUDENT) {
            process_student_request(&req, &res);
        } else {
            strcpy(res.message, "Unknown user type.");
            res.status_code = GENERIC_FAILURE;
        }

        if (res.status_code == GENERIC_SUCCESS) op_success = true;

        if (write_fully(client_fd, &res, sizeof(res)) != sizeof(res)) break;

        if (op_success) {
            sem_wait(&operations_semaphore);
            total_successful_ops++;
            printf("Thread %lu: Successful ops = %lld\n", pthread_self(), total_successful_ops);
            sem_post(&operations_semaphore);
        }
    }

    printf("Thread %lu: Disconnecting socket %d\n", pthread_self(), client_fd);
    close(client_fd);
    return NULL;
}

int main() {
    int server_fd, *client_fd_ptr;
    struct sockaddr_in server_addr = {0}, client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    if (sem_init(&operations_semaphore, 0, 1) != 0) {
        perror("Semaphore init failed");
        exit(EXIT_FAILURE);
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_PENDING_CONNECTIONS) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Academia server listening on port %d...\n", SERVER_PORT);

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        client_fd_ptr = malloc(sizeof(int));
        if (!client_fd_ptr) {
            perror("Malloc failed");
            close(client_fd);
            continue;
        }

        *client_fd_ptr = client_fd;
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_fd_ptr) != 0) {
            perror("Thread creation failed");
            free(client_fd_ptr);
            close(client_fd);
        } else {
            pthread_detach(tid);
        }
    }

    close(server_fd);
    sem_destroy(&operations_semaphore);
    return 0;
}