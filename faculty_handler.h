#ifndef FACULTY_HANDLER_H
#define FACULTY_HANDLER_H

#include "common.h"

void process_faculty_request(const struct Request *req, struct Response *res);

int faculty_login_impl(const char* login_id_str, const char* password, struct Faculty *f_out);
int faculty_view_offering_courses_impl(int facultyRollNo, struct Course courses_out[], int *count);
int faculty_add_new_course_impl(struct Course new_course);
int faculty_remove_course_impl(int facultyRollNo, const char *courseID);
int faculty_update_course_details_impl(int facultyRollNo, const char *courseID, const char *new_name, int new_total_seats);
int faculty_change_password_impl(int facultyRollNo, const char *new_password);

#endif
