#ifndef ADMIN_HANDLER_H
#define ADMIN_HANDLER_H

#include "common.h"

void process_admin_request(const struct Request *req, struct Response *res);

int admin_add_student_impl(struct Student new_student);
int admin_view_student_details_impl(int rollNo, struct Student *s_out);
int admin_add_faculty_impl(struct Faculty new_faculty);
int admin_view_faculty_details_impl(int rollNo, struct Faculty *f_out);
int admin_activate_student_impl(int rollNo);
int admin_block_student_impl(int rollNo);
int admin_modify_student_impl(int rollNo, const char *new_name, const char *new_password);
int admin_modify_faculty_impl(int rollNo, const char *new_name, const char *new_password);

#endif
