#ifndef CRYPTIO_H
#define CRYPTIO_H

#include <stdio.h>
#include <stdlib.h>

#define MAGIC "CRYPTMAGIC"
#define MAGIC_SIZE 12

typedef struct FileEntry
{
    size_t name_len;
    char *name;
    long offset;
    long size;
}FileEntry;

void update_table(FileEntry *head, int count);
int load_all_files(FileEntry **head);
void extract_from_table(char *filename);
void add_file(FileEntry **head, int *count, const char *filename);
FileEntry *get_file(FileEntry **head, int *count, const char *filename);
void extract_file(FileEntry *head, int *count,const char *filename);
void update_file(char *filename);
void delete_file(FileEntry **head, int *count, const char *filename);
void free_file_entry(FileEntry *file);

#endif