#include "student_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

// -------------------- File Locking Helpers --------------------

static void set_lock(int fd, short type) {
    struct flock fl = {.l_type = type, .l_whence = SEEK_SET, .l_start = 0, .l_len = 0, .l_pid = getpid()};
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("fcntl (set_lock)");
    }
}

static void unlock_file(int fd) {
    struct flock fl = {.l_type = F_UNLCK, .l_whence = SEEK_SET, .l_start = 0, .l_len = 0, .l_pid = getpid()};
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        perror("fcntl (unlock_file)");
    }
}

// -------------------- Data Access Helpers --------------------

static bool get_course_details(const char *courseID, struct Course *c_out) {
    int fd = open(COURSE_DATA_FILE, O_RDONLY);
    if (fd == -1) return false;

    set_lock(fd, F_RDLCK);
    struct Course course;
    bool found = false;

    while (read(fd, &course, sizeof(course)) == sizeof(course)) {
        if (strcmp(course.courseID, courseID) == 0) {
            *c_out = course;
            found = true;
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return found;
}

static bool is_student_enrolled(int studentRollNo, const char *courseID) {
    int fd = open(ENROLLMENT_DATA_FILE, O_RDONLY);
    if (fd == -1) return false;

    set_lock(fd, F_RDLCK);
    struct Enrollment en;
    bool enrolled = false;

    while (read(fd, &en, sizeof(en)) == sizeof(en)) {
        if (en.studentRollNo == studentRollNo && strcmp(en.courseID, courseID) == 0) {
            enrolled = true;
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return enrolled;
}

// -------------------- Core Student Operations --------------------

int student_login_impl(const char* login_id_str, const char* password, struct Student *s_out) {
    int rollNo = atoi(login_id_str);
    if (rollNo <= 0) return LOGIN_FAILURE;

    int fd = open(STUDENT_DATA_FILE, O_RDONLY);
    if (fd == -1) return LOGIN_FAILURE;

    set_lock(fd, F_RDLCK);
    struct Student student;
    int status = LOGIN_FAILURE;

    while (read(fd, &student, sizeof(student)) == sizeof(student)) {
        if (student.rollNo == rollNo) {
            if (strcmp(student.password, password) == 0) {
                status = student.isActive ? LOGIN_SUCCESS_STUDENT : STUDENT_BLOCKED;
                if (student.isActive) *s_out = student;
            }
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return status;
}

int student_view_all_courses_impl(struct Course courses_out[], int *count) {
    int fd = open(COURSE_DATA_FILE, O_RDONLY);
    if (fd == -1) {
        *count = 0;
        return GENERIC_SUCCESS;
    }

    set_lock(fd, F_RDLCK);
    struct Course course;

    while (read(fd, &course, sizeof(course)) == sizeof(course)) {
        if (course.isActive && *count < MAX_COURSES_IN_SYSTEM) {
            courses_out[(*count)++] = course;
        }
    }

    unlock_file(fd);
    close(fd);
    return GENERIC_SUCCESS;
}

int student_view_enrolled_courses_impl(int studentRollNo, struct Course courses_out[], int *count) {
    int fd = open(ENROLLMENT_DATA_FILE, O_RDONLY);
    if (fd == -1) {
        *count = 0;
        return GENERIC_SUCCESS;
    }

    set_lock(fd, F_RDLCK);
    struct Enrollment en;
    struct Course c;

    while (read(fd, &en, sizeof(en)) == sizeof(en)) {
        if (en.studentRollNo == studentRollNo && get_course_details(en.courseID, &c)) {
            if (*count < MAX_COURSES_IN_SYSTEM) {
                courses_out[(*count)++] = c;
            }
        }
    }

    unlock_file(fd);
    close(fd);
    return GENERIC_SUCCESS;
}

int student_enroll_course_impl(int studentRollNo, const char *courseID) {
    if (is_student_enrolled(studentRollNo, courseID)) return ALREADY_ENROLLED;

    struct Course course;
    if (!get_course_details(courseID, &course)) return RECORD_NOT_FOUND;
    if (!course.isActive) return COURSE_NOT_ACTIVE;
    if (course.availableSeats <= 0) return COURSE_FULL;

    // Append to enrollment file
    int fd_enroll = open(ENROLLMENT_DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd_enroll == -1) return GENERIC_FAILURE;

    set_lock(fd_enroll, F_WRLCK);
    struct Enrollment en = {.studentRollNo = studentRollNo};
    strncpy(en.courseID, courseID, MAX_COURSE_ID_LEN - 1);
    en.courseID[MAX_COURSE_ID_LEN - 1] = '\0';
    ssize_t status = write(fd_enroll, &en, sizeof(en));
    unlock_file(fd_enroll);
    close(fd_enroll);

    if (status != sizeof(en)) return GENERIC_FAILURE;

    // Decrease course seat count
    int fd_course = open(COURSE_DATA_FILE, O_RDWR);
    if (fd_course == -1) return GENERIC_FAILURE;

    set_lock(fd_course, F_WRLCK);
    struct Course c;
    off_t offset = 0;
    bool updated = false;

    while (read(fd_course, &c, sizeof(c)) == sizeof(c)) {
        if (strcmp(c.courseID, courseID) == 0) {
            c.availableSeats--;
            lseek(fd_course, offset, SEEK_SET);
            if (write(fd_course, &c, sizeof(c)) == sizeof(c)) updated = true;
            break;
        }
        offset = lseek(fd_course, 0, SEEK_CUR);
    }

    unlock_file(fd_course);
    close(fd_course);
    return updated ? GENERIC_SUCCESS : GENERIC_FAILURE;
}

int student_drop_course_impl(int studentRollNo, const char *courseID) {
    if (!is_student_enrolled(studentRollNo, courseID)) return NOT_ENROLLED;

    int fd_old = open(ENROLLMENT_DATA_FILE, O_RDONLY);
    int fd_new = open("temp_enroll.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_old == -1 || fd_new == -1) return GENERIC_FAILURE;

    set_lock(fd_old, F_WRLCK);
    struct Enrollment en;
    bool found = false;

    while (read(fd_old, &en, sizeof(en)) == sizeof(en)) {
        if (en.studentRollNo == studentRollNo && strcmp(en.courseID, courseID) == 0) {
            found = true;
            continue;
        }
        if (write(fd_new, &en, sizeof(en)) == -1) {
            close(fd_old); close(fd_new); unlink("temp_enroll.dat");
            return GENERIC_FAILURE;
        }
    }

    close(fd_old); close(fd_new);
    if (!found) {
        unlink("temp_enroll.dat");
        return NOT_ENROLLED;
    }

    if (rename("temp_enroll.dat", ENROLLMENT_DATA_FILE) != 0) {
        unlink("temp_enroll.dat");
        return GENERIC_FAILURE;
    }

    // Increase course seat count
    int fd_course = open(COURSE_DATA_FILE, O_RDWR);
    if (fd_course == -1) return GENERIC_FAILURE;

    set_lock(fd_course, F_WRLCK);
    struct Course c;
    off_t offset = 0;
    bool updated = false;

    while (read(fd_course, &c, sizeof(c)) == sizeof(c)) {
        if (strcmp(c.courseID, courseID) == 0) {
            c.availableSeats++;
            lseek(fd_course, offset, SEEK_SET);
            if (write(fd_course, &c, sizeof(c)) == sizeof(c)) updated = true;
            break;
        }
        offset = lseek(fd_course, 0, SEEK_CUR);
    }

    unlock_file(fd_course);
    close(fd_course);
    return updated ? GENERIC_SUCCESS : GENERIC_FAILURE;
}

int student_change_password_impl(int studentRollNo, const char *new_password) {
    int fd = open(STUDENT_DATA_FILE, O_RDWR);
    if (fd == -1) return RECORD_NOT_FOUND;

    set_lock(fd, F_WRLCK);
    struct Student student;
    off_t offset = 0;
    bool updated = false;

    while (read(fd, &student, sizeof(student)) == sizeof(student)) {
        if (student.rollNo == studentRollNo) {
            strncpy(student.password, new_password, MAX_PASSWORD_LEN - 1);
            student.password[MAX_PASSWORD_LEN - 1] = '\0';
            lseek(fd, offset, SEEK_SET);
            if (write(fd, &student, sizeof(student)) == sizeof(student)) updated = true;
            break;
        }
        offset = lseek(fd, 0, SEEK_CUR);
    }

    unlock_file(fd);
    close(fd);
    return updated ? GENERIC_SUCCESS : RECORD_NOT_FOUND;
}

// -------------------- Request Dispatcher --------------------

void process_student_request(const struct Request *req, struct Response *res) {
    res->status_code = GENERIC_FAILURE;
    strcpy(res->message, "Student operation failed or not implemented.");
    res->data_count = 0;

    switch (req->operation) {
        case STUDENT_VIEW_ALL_COURSES:
            res->status_code = student_view_all_courses_impl(res->data_payload.course_array, &res->data_count);
            strcpy(res->message, res->data_count > 0 ?
                   "Retrieved active courses." : "No active courses available.");
            break;

        case STUDENT_ENROLL_COURSE:
            res->status_code = student_enroll_course_impl(req->current_user_rollno, req->data_payload.target_courseID_generic);
            switch (res->status_code) {
                case GENERIC_SUCCESS: strcpy(res->message, "Successfully enrolled in course."); break;
                case ALREADY_ENROLLED: strcpy(res->message, "Already enrolled in course."); break;
                case COURSE_FULL: strcpy(res->message, "Course is full."); break;
                case COURSE_NOT_ACTIVE: strcpy(res->message, "Course is inactive."); break;
                case RECORD_NOT_FOUND: strcpy(res->message, "Course not found."); break;
                default: strcpy(res->message, "Failed to enroll in course."); break;
            }
            break;

        case STUDENT_DROP_COURSE:
            res->status_code = student_drop_course_impl(req->current_user_rollno, req->data_payload.target_courseID_generic);
            strcpy(res->message,
                   res->status_code == GENERIC_SUCCESS ? "Course dropped successfully." :
                   res->status_code == NOT_ENROLLED ? "Not enrolled in this course." :
                   "Failed to drop course.");
            break;

        case STUDENT_VIEW_ENROLLED_COURSES:
            res->status_code = student_view_enrolled_courses_impl(req->current_user_rollno, res->data_payload.course_array, &res->data_count);
            strcpy(res->message, res->data_count > 0 ?
                   "Enrolled courses retrieved." : "You are not enrolled in any courses.");
            break;

        case STUDENT_CHANGE_PASSWORD:
            res->status_code = student_change_password_impl(req->current_user_rollno, req->data_payload.new_password_only);
            strcpy(res->message, res->status_code == GENERIC_SUCCESS ?
                   "Password changed successfully." : "Failed to change password.");
            break;

        default:
            res->status_code = INVALID_CHOICE;
            strcpy(res->message, "Invalid student operation.");
    }
}
