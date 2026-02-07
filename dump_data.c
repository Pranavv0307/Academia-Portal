#include <stdio.h>
#include <stdlib.h>
#include "common.h"

void dump_students() {
    FILE *fp = fopen(STUDENT_DATA_FILE, "rb");
    if (!fp) {
        perror("Failed to open student data file");
        return;
    }

    struct Student s;
    printf("\n--- Students ---\n");
    while (fread(&s, sizeof(struct Student), 1, fp) == 1) {
        printf("Roll No: %d\n", s.rollNo);
        printf("Name: %s\n", s.name);
        printf("Password: %s\n", s.password);
        printf("Status: %s\n", s.isActive ? "Active" : "Blocked");
        printf("----------------------\n");
    }

    fclose(fp);
}

void dump_faculty() {
    FILE *fp = fopen(FACULTY_DATA_FILE, "rb");
    if (!fp) {
        perror("Failed to open faculty data file");
        return;
    }

    struct Faculty f;
    printf("\n--- Faculty ---\n");
    while (fread(&f, sizeof(struct Faculty), 1, fp) == 1) {
        printf("Roll No: %d\n", f.rollNo);
        printf("Name: %s\n", f.name);
        printf("Password: %s\n", f.password);
        printf("----------------------\n");
    }

    fclose(fp);
}

int main() {
    dump_students();
    dump_faculty();
    return 0;
}
