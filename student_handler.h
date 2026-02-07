#ifndef STUDENT_HANDLER_H
#define STUDENT_HANDLER_H

#include "common.h"

void process_student_request(const struct Request *req, struct Response *res);

int student_login_impl(const char* login_id_str, const char* password, struct Student *s_out);
int student_view_all_courses_impl(struct Course courses_out[], int *count);
int student_enroll_course_impl(int studentRollNo, const char *courseID);
int student_drop_course_impl(int studentRollNo, const char *courseID);
int student_view_enrolled_courses_impl(int studentRollNo, struct Course courses_out[], int *count);
int student_change_password_impl(int studentRollNo, const char *new_password);

#endif
