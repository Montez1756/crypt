#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cryptio.h"

int array_includes(char **array, int count, char *include)
{
    for (int i = 0; i < count; i++)
    {
        if (strcmp(array[i], include) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2 || (argc < 3 && strcmp(argv[1], "-l") != 0))
    {
        fprintf(stderr, "Usage: %s [-a|-r|-d] file1 [file2 ...] [-a|-r|-d] ... | -l\n", argv[0]);
        return -1;
    }

    char *args[] = {"-a", "-r", "-d"};
    int i = 1;
    FileEntry *head = NULL;
    int count = load_all_files(&head);
    while (i < argc)
    {
        if (strcmp(argv[i], "-a") == 0)
        {
            i++;
            while (i < argc && !array_includes(args, 2, argv[i]))
            {
                add_file(&head, &count, argv[i]);
                update_table(head, count);
                i++;
            }
        }
        else if (strcmp(argv[i], "-r") == 0)
        {
            i++;
            while (i < argc && !array_includes(args, 2, argv[i]))
            {
                extract_file(head, &count, argv[i]);
                i++;
            }
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            i++;
            while (i < argc && !array_includes(args, 2, argv[i]))
            {
                delete_file(&head, &count, argv[i]);
                i++;
            }
        }
        else if (strcmp(argv[i], "-l") == 0)
        {
            if (!head)
            {
                fprintf(stderr, "No files to list currently");
                return -1;
            }
            puts("Files:");
            for (int i = 0; i < count; i++)
            {
                FileEntry *current = &head[i];
                printf("File Name Length: %zu\n File Name: %s\n File Offset: %ld\n File Offset: %ld\n", current->name_len, current->name, current->offset, current->size);
            }
            i++;
        }
        else
        {
            fprintf(stderr, "Unrecognized argument: %s\n", argv[i]);
            i++;
        }
    }

    return 0;
}