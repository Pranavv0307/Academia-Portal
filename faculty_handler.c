#include "faculty_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

static void set_lock(int fd, short type) {
    struct flock fl = { .l_type = type, .l_whence = SEEK_SET, .l_start = 0, .l_len = 0, .l_pid = getpid() };
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("fcntl (set_lock)");
    }
}

static void unlock_file(int fd) {
    struct flock fl = { .l_type = F_UNLCK, .l_whence = SEEK_SET, .l_start = 0, .l_len = 0, .l_pid = getpid() };
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        perror("fcntl (unlock_file)");
    }
}

static bool course_id_exists(const char *courseID) {
    int fd = open(COURSE_DATA_FILE, O_RDONLY);
    if (fd == -1) return false;

    set_lock(fd, F_RDLCK);
    struct Course c;
    bool found = false;

    while (read(fd, &c, sizeof(struct Course)) == sizeof(struct Course)) {
        if (strcmp(c.courseID, courseID) == 0) {
            found = true;
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return found;
}

int faculty_login_impl(const char* login_id_str, const char* password, struct Faculty *f_out) {
    int rollNo = atoi(login_id_str);
    if (rollNo <= 0) return LOGIN_FAILURE;

    int fd = open(FACULTY_DATA_FILE, O_RDONLY);
    if (fd == -1) return LOGIN_FAILURE;

    set_lock(fd, F_RDLCK);
    struct Faculty f;
    int status = LOGIN_FAILURE;

    while (read(fd, &f, sizeof(struct Faculty)) == sizeof(struct Faculty)) {
        if (f.rollNo == rollNo && strcmp(f.password, password) == 0) {
            *f_out = f;
            status = LOGIN_SUCCESS_FACULTY;
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return status;
}

int faculty_view_offering_courses_impl(int facultyRollNo, struct Course courses_out[], int *count) {
    int fd = open(COURSE_DATA_FILE, O_RDONLY);
    if (fd == -1) {
        *count = 0;
        return GENERIC_SUCCESS;
    }

    set_lock(fd, F_RDLCK);
    struct Course c;
    *count = 0;

    while (read(fd, &c, sizeof(struct Course)) == sizeof(struct Course)) {
        if (c.facultyRollNo == facultyRollNo && c.isActive && *count < MAX_COURSES_IN_SYSTEM) {
            courses_out[(*count)++] = c;
        }
    }

    unlock_file(fd);
    close(fd);
    return GENERIC_SUCCESS;
}

int faculty_add_new_course_impl(struct Course new_course) {
    if (course_id_exists(new_course.courseID)) return RECORD_EXISTS;

    int fd = open(COURSE_DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) return GENERIC_FAILURE;

    set_lock(fd, F_WRLCK);
    new_course.availableSeats = new_course.totalSeats;
    new_course.isActive = true;

    ssize_t written = write(fd, &new_course, sizeof(struct Course));

    unlock_file(fd);
    close(fd);

    return (written == sizeof(struct Course)) ? GENERIC_SUCCESS : GENERIC_FAILURE;
}

int faculty_remove_course_impl(int facultyRollNo, const char *courseID) {
    int fd = open(COURSE_DATA_FILE, O_RDWR);
    if (fd == -1) return RECORD_NOT_FOUND;

    set_lock(fd, F_WRLCK);
    struct Course c;
    bool updated = false;
    off_t offset = 0;

    while (read(fd, &c, sizeof(struct Course)) == sizeof(struct Course)) {
        if (strcmp(c.courseID, courseID) == 0) {
            if (c.facultyRollNo != facultyRollNo) {
                unlock_file(fd); close(fd);
                return UNAUTHORIZED_ACTION;
            }
            c.isActive = false;
            lseek(fd, offset, SEEK_SET);
            updated = (write(fd, &c, sizeof(struct Course)) == sizeof(struct Course));
            break;
        }
        offset = lseek(fd, 0, SEEK_CUR);
    }

    unlock_file(fd);
    close(fd);
    return updated ? GENERIC_SUCCESS : RECORD_NOT_FOUND;
}

int faculty_update_course_details_impl(int facultyRollNo, const char *courseID, const char *new_name, int new_total_seats) {
    int fd = open(COURSE_DATA_FILE, O_RDWR);
    if (fd == -1) return RECORD_NOT_FOUND;

    set_lock(fd, F_WRLCK);
    struct Course c;
    bool updated = false;
    off_t offset = 0;

    while (read(fd, &c, sizeof(struct Course)) == sizeof(struct Course)) {
        if (strcmp(c.courseID, courseID) == 0) {
            if (c.facultyRollNo != facultyRollNo) {
                unlock_file(fd); close(fd);
                return UNAUTHORIZED_ACTION;
            }
            if (new_name && strlen(new_name) > 0) {
                strncpy(c.courseName, new_name, MAX_COURSE_NAME_LEN - 1);
                c.courseName[MAX_COURSE_NAME_LEN - 1] = '\0';
            }
            if (new_total_seats >= 0) {
                int diff = new_total_seats - c.totalSeats;
                c.totalSeats = new_total_seats;
                c.availableSeats += diff;
                if (c.availableSeats < 0) c.availableSeats = 0;
                if (c.availableSeats > c.totalSeats) c.availableSeats = c.totalSeats;
            }
            lseek(fd, offset, SEEK_SET);
            updated = (write(fd, &c, sizeof(struct Course)) == sizeof(struct Course));
            break;
        }
        offset = lseek(fd, 0, SEEK_CUR);
    }

    unlock_file(fd);
    close(fd);
    return updated ? GENERIC_SUCCESS : RECORD_NOT_FOUND;
}

int faculty_change_password_impl(int facultyRollNo, const char *new_password) {
    int fd = open(FACULTY_DATA_FILE, O_RDWR);
    if (fd == -1) return RECORD_NOT_FOUND;

    set_lock(fd, F_WRLCK);
    struct Faculty f;
    bool updated = false;
    off_t offset = 0;

    while (read(fd, &f, sizeof(struct Faculty)) == sizeof(struct Faculty)) {
        if (f.rollNo == facultyRollNo) {
            strncpy(f.password, new_password, MAX_PASSWORD_LEN - 1);
            f.password[MAX_PASSWORD_LEN - 1] = '\0';
            lseek(fd, offset, SEEK_SET);
            updated = (write(fd, &f, sizeof(struct Faculty)) == sizeof(struct Faculty));
            break;
        }
        offset = lseek(fd, 0, SEEK_CUR);
    }

    unlock_file(fd);
    close(fd);
    return updated ? GENERIC_SUCCESS : RECORD_NOT_FOUND;
}

void process_faculty_request(const struct Request *req, struct Response *res) {
    res->status_code = GENERIC_FAILURE;
    strcpy(res->message, "Faculty operation failed or not implemented.");

    switch (req->operation) {
        case FACULTY_VIEW_OFFERING_COURSES:
            res->data_count = 0;
            res->status_code = faculty_view_offering_courses_impl(req->current_user_rollno, res->data_payload.course_array, &res->data_count);
            strcpy(res->message, res->status_code == GENERIC_SUCCESS ?
                   (res->data_count > 0 ? "Retrieved courses you are offering." : "You are not currently offering any courses.") :
                   "Failed to retrieve your courses.");
            break;

        case FACULTY_ADD_NEW_COURSE:
            struct Course course_to_add = req->data_payload.course_data;
            course_to_add.facultyRollNo = req->current_user_rollno;
            res->status_code = faculty_add_new_course_impl(course_to_add);
            strcpy(res->message,
                   res->status_code == GENERIC_SUCCESS ? "Course added successfully." :
                   res->status_code == RECORD_EXISTS ? "Course ID already exists." : "Failed to add course.");
            break;

        case FACULTY_REMOVE_COURSE:
            res->status_code = faculty_remove_course_impl(req->current_user_rollno, req->data_payload.target_courseID_generic);
            strcpy(res->message,
                   res->status_code == GENERIC_SUCCESS ? "Course removed (deactivated) successfully." :
                   res->status_code == RECORD_NOT_FOUND ? "Course ID not found." :
                   res->status_code == UNAUTHORIZED_ACTION ? "Cannot remove: You do not own this course." : "Failed to remove course.");
            break;

        case FACULTY_UPDATE_COURSE_DETAILS:
            res->status_code = faculty_update_course_details_impl(
                req->current_user_rollno,
                req->data_payload.course_update_data.target_courseID,
                req->data_payload.course_update_data.new_course_name,
                req->data_payload.course_update_data.new_total_seats
            );
            strcpy(res->message,
                   res->status_code == GENERIC_SUCCESS ? "Course details updated successfully." :
                   res->status_code == RECORD_NOT_FOUND ? "Course ID not found for update." :
                   res->status_code == UNAUTHORIZED_ACTION ? "Cannot update: You do not own this course." : "Failed to update course details.");
            break;

        case FACULTY_CHANGE_PASSWORD:
            res->status_code = faculty_change_password_impl(req->current_user_rollno, req->data_payload.new_password_only);
            strcpy(res->message,
                   res->status_code == GENERIC_SUCCESS ? "Password changed successfully." :
                   res->status_code == RECORD_NOT_FOUND ? "Faculty record not found (should not happen if logged in)." : "Failed to change password.");
            break;

        default:
            res->status_code = INVALID_CHOICE;
            strcpy(res->message, "Invalid faculty operation choice.");
            break;
    }
}