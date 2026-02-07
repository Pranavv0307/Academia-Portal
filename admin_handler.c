#include "admin_handler.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

static void set_lock(int fd, short type) {
    struct flock fl;
    fl.l_type = type;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("fcntl (set_lock)");
    }
}

static void unlock_file(int fd) {
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        perror("fcntl (unlock_file)");
    }
}

static bool is_roll_no_unique(int rollNo) {
    int fd_s, fd_f;
    struct Student s_check;
    struct Faculty f_check;
    bool unique = true;

    fd_s = open(STUDENT_DATA_FILE, O_RDONLY);
    if (fd_s != -1) {
        set_lock(fd_s, F_RDLCK);
        while (read(fd_s, &s_check, sizeof(struct Student)) == sizeof(struct Student)) {
            if (s_check.rollNo == rollNo) {
                unique = false;
                break;
            }
        }
        unlock_file(fd_s);
        close(fd_s);
    }
    if (!unique) return false;

    fd_f = open(FACULTY_DATA_FILE, O_RDONLY);
    if (fd_f != -1) {
        set_lock(fd_f, F_RDLCK);
        while (read(fd_f, &f_check, sizeof(struct Faculty)) == sizeof(struct Faculty)) {
            if (f_check.rollNo == rollNo) {
                unique = false;
                break;
            }
        }
        unlock_file(fd_f);
        close(fd_f);
    }
    return unique;
}

int admin_add_student_impl(struct Student new_student) {
    if (!is_roll_no_unique(new_student.rollNo)) {
        return RECORD_EXISTS;
    }

    int fd = open(STUDENT_DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) return GENERIC_FAILURE;

    set_lock(fd, F_WRLCK);
    new_student.isActive = true;
    ssize_t bytes_written = write(fd, &new_student, sizeof(struct Student));
    if (bytes_written == -1) perror("write (admin_add_student_impl)");
    unlock_file(fd);
    close(fd);

    return (bytes_written == sizeof(struct Student)) ? GENERIC_SUCCESS : GENERIC_FAILURE;
}

int admin_view_student_details_impl(int rollNo, struct Student *s_out) {
    int fd = open(STUDENT_DATA_FILE, O_RDONLY);
    if (fd == -1) return RECORD_NOT_FOUND;

    set_lock(fd, F_RDLCK);
    struct Student current_student;
    bool found = false;
    while (read(fd, &current_student, sizeof(struct Student)) == sizeof(struct Student)) {
        if (current_student.rollNo == rollNo) {
            *s_out = current_student;
            found = true;
            break;
        }
    }
    unlock_file(fd);
    close(fd);

    return found ? GENERIC_SUCCESS : RECORD_NOT_FOUND;
}

int admin_add_faculty_impl(struct Faculty new_faculty) {
    if (!is_roll_no_unique(new_faculty.rollNo)) {
        return RECORD_EXISTS;
    }

    int fd = open(FACULTY_DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) return GENERIC_FAILURE;

    set_lock(fd, F_WRLCK);
    ssize_t bytes_written = write(fd, &new_faculty, sizeof(struct Faculty));
    if (bytes_written == -1) perror("write (admin_add_faculty_impl)");
    unlock_file(fd);
    close(fd);

    return (bytes_written == sizeof(struct Faculty)) ? GENERIC_SUCCESS : GENERIC_FAILURE;
}

int admin_view_faculty_details_impl(int rollNo, struct Faculty *f_out) {
    int fd = open(FACULTY_DATA_FILE, O_RDONLY);
    if (fd == -1) return RECORD_NOT_FOUND;

    set_lock(fd, F_RDLCK);
    struct Faculty current_faculty;
    bool found = false;
    while (read(fd, &current_faculty, sizeof(struct Faculty)) == sizeof(struct Faculty)) {
        if (current_faculty.rollNo == rollNo) {
            *f_out = current_faculty;
            found = true;
            break;
        }
    }
    unlock_file(fd);
    close(fd);

    return found ? GENERIC_SUCCESS : RECORD_NOT_FOUND;
}

static int admin_toggle_student_status_helper(int rollNo, bool new_status) {
    int fd = open(STUDENT_DATA_FILE, O_RDWR);
    if (fd == -1) return RECORD_NOT_FOUND;

    set_lock(fd, F_WRLCK);
    struct Student current_student;
    bool found = false;
    off_t record_offset = 0;

    while (read(fd, &current_student, sizeof(struct Student)) == sizeof(struct Student)) {
        if (current_student.rollNo == rollNo) {
            current_student.isActive = new_status;
            lseek(fd, record_offset, SEEK_SET);
            if (write(fd, &current_student, sizeof(struct Student)) == sizeof(struct Student)) {
                found = true;
            }
            break;
        }
        record_offset = lseek(fd, 0, SEEK_CUR);
    }
    unlock_file(fd);
    close(fd);

    return found ? GENERIC_SUCCESS : RECORD_NOT_FOUND;
}

int admin_activate_student_impl(int rollNo) {
    return admin_toggle_student_status_helper(rollNo, true);
}

int admin_block_student_impl(int rollNo) {
    return admin_toggle_student_status_helper(rollNo, false);
}

int admin_modify_student_impl(int rollNo, const char *new_name, const char *new_password) {
    int fd = open(STUDENT_DATA_FILE, O_RDWR);
    if (fd == -1) return RECORD_NOT_FOUND;

    set_lock(fd, F_WRLCK);
    struct Student current_student;
    bool found = false;
    off_t record_offset = 0;

    while (read(fd, &current_student, sizeof(struct Student)) == sizeof(struct Student)) {
        if (current_student.rollNo == rollNo) {
            strncpy(current_student.name, new_name, MAX_NAME_LEN -1);
            current_student.name[MAX_NAME_LEN -1] = '\0';
            strncpy(current_student.password, new_password, MAX_PASSWORD_LEN -1);
            current_student.password[MAX_PASSWORD_LEN -1] = '\0';

            lseek(fd, record_offset, SEEK_SET);
            if (write(fd, &current_student, sizeof(struct Student)) == sizeof(struct Student)) {
                found = true;
            }
            break;
        }
        record_offset = lseek(fd, 0, SEEK_CUR);
    }
    unlock_file(fd);
    close(fd);

    return found ? GENERIC_SUCCESS : RECORD_NOT_FOUND;
}

int admin_modify_faculty_impl(int rollNo, const char *new_name, const char *new_password) {
    int fd = open(FACULTY_DATA_FILE, O_RDWR);
    if (fd == -1) return RECORD_NOT_FOUND;

    set_lock(fd, F_WRLCK);
    struct Faculty current_faculty;
    bool found = false;
    off_t record_offset = 0;

    while (read(fd, &current_faculty, sizeof(struct Faculty)) == sizeof(struct Faculty)) {
        if (current_faculty.rollNo == rollNo) {
            strncpy(current_faculty.name, new_name, MAX_NAME_LEN -1);
            current_faculty.name[MAX_NAME_LEN -1] = '\0';
            strncpy(current_faculty.password, new_password, MAX_PASSWORD_LEN -1);
            current_faculty.password[MAX_PASSWORD_LEN -1] = '\0';

            lseek(fd, record_offset, SEEK_SET);
            if (write(fd, &current_faculty, sizeof(struct Faculty)) == sizeof(struct Faculty)) {
                found = true;
            }
            break;
        }
        record_offset = lseek(fd, 0, SEEK_CUR);
    }
    unlock_file(fd);
    close(fd);

    return found ? GENERIC_SUCCESS : RECORD_NOT_FOUND;
}

void process_admin_request(const struct Request *req, struct Response *res) {
    res->status_code = GENERIC_FAILURE;
    strcpy(res->message, "Admin operation failed or not implemented.");

    switch (req->operation) {
        case ADMIN_ADD_STUDENT:
            res->status_code = admin_add_student_impl(req->data_payload.student_data);
            if (res->status_code == GENERIC_SUCCESS) strcpy(res->message, "Student added successfully.");
            else if (res->status_code == RECORD_EXISTS) strcpy(res->message, "Student Roll No already exists or is used by a faculty.");
            else strcpy(res->message, "Failed to add student.");
            break;

        case ADMIN_VIEW_STUDENT_DETAILS:
            res->status_code = admin_view_student_details_impl(req->data_payload.target_rollNo_generic, &res->data_payload.student_data);
            if (res->status_code == GENERIC_SUCCESS) {
                strcpy(res->message, "Student details retrieved.");
            } else if (res->status_code == RECORD_NOT_FOUND) strcpy(res->message, "Student not found.");
            else strcpy(res->message, "Failed to view student details.");
            break;

        case ADMIN_ADD_FACULTY:
            res->status_code = admin_add_faculty_impl(req->data_payload.faculty_data);
            if (res->status_code == GENERIC_SUCCESS) strcpy(res->message, "Faculty added successfully.");
            else if (res->status_code == RECORD_EXISTS) strcpy(res->message, "Faculty Roll No already exists or is used by a student.");
            else strcpy(res->message, "Failed to add faculty.");
            break;

        case ADMIN_VIEW_FACULTY_DETAILS:
            res->status_code = admin_view_faculty_details_impl(req->data_payload.target_rollNo_generic, &res->data_payload.faculty_data);
            if (res->status_code == GENERIC_SUCCESS) {
                strcpy(res->message, "Faculty details retrieved.");
            } else if (res->status_code == RECORD_NOT_FOUND) strcpy(res->message, "Faculty not found.");
            else strcpy(res->message, "Failed to view faculty details.");
            break;

        case ADMIN_ACTIVATE_STUDENT:
            res->status_code = admin_activate_student_impl(req->data_payload.target_rollNo_generic);
            if (res->status_code == GENERIC_SUCCESS) strcpy(res->message, "Student activated successfully.");
            else if (res->status_code == RECORD_NOT_FOUND) strcpy(res->message, "Student not found for activation.");
            else strcpy(res->message, "Failed to activate student.");
            break;

        case ADMIN_BLOCK_STUDENT:
            res->status_code = admin_block_student_impl(req->data_payload.target_rollNo_generic);
            if (res->status_code == GENERIC_SUCCESS) strcpy(res->message, "Student blocked successfully.");
            else if (res->status_code == RECORD_NOT_FOUND) strcpy(res->message, "Student not found for blocking.");
            else strcpy(res->message, "Failed to block student.");
            break;

        case ADMIN_MODIFY_STUDENT:
            res->status_code = admin_modify_student_impl(
                req->data_payload.user_modification_data.target_rollNo,
                req->data_payload.user_modification_data.new_name,
                req->data_payload.user_modification_data.new_password);
            if (res->status_code == GENERIC_SUCCESS) strcpy(res->message, "Student details modified successfully.");
            else if (res->status_code == RECORD_NOT_FOUND) strcpy(res->message, "Student not found for modification.");
            else strcpy(res->message, "Failed to modify student details.");
            break;

        case ADMIN_MODIFY_FACULTY:
            res->status_code = admin_modify_faculty_impl(
                req->data_payload.user_modification_data.target_rollNo,
                req->data_payload.user_modification_data.new_name,
                req->data_payload.user_modification_data.new_password);
            if (res->status_code == GENERIC_SUCCESS) strcpy(res->message, "Faculty details modified successfully.");
            else if (res->status_code == RECORD_NOT_FOUND) strcpy(res->message, "Faculty not found for modification.");
            else strcpy(res->message, "Failed to modify faculty details.");
            break;
        default:
            res->status_code = INVALID_CHOICE;
            strcpy(res->message, "Invalid admin operation choice received by handler.");
            break;
    }
}
