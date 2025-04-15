#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cryptio.h"

void update_table(FileEntry *head, int count)
{
    FILE *fptr = fopen("table.bin", "wb"); // use "wb" to overwrite
    if (!fptr)
    {
        perror("fopen");
        return;
    }

    for (int i = 0; i < count; i++)
    {
        fwrite(&head[i].name_len, sizeof(size_t), 1, fptr);
        fwrite(head[i].name, 1, head[i].name_len, fptr);
        fwrite(&head[i].offset, sizeof(long), 1, fptr);
        fwrite(&head[i].size, sizeof(long), 1, fptr);
    }

    fclose(fptr);
}

int load_all_files(FileEntry **head)
{
    FILE *fptr = fopen("table.bin", "rb");
    if (!fptr)
    {
        // File might not exist yet — that’s okay
        *head = NULL;
        return 0;
    }

    fseek(fptr, 0, SEEK_END);
    long fileSize = ftell(fptr);
    rewind(fptr);

    FileEntry *entries = NULL;
    size_t count = 0;

    while (ftell(fptr) < fileSize)
    {
        FileEntry entry;

        // Read name_len
        if (fread(&entry.name_len, sizeof(size_t), 1, fptr) != 1)
            break;

        // Allocate and read name
        entry.name = malloc(entry.name_len + 1);
        if (!entry.name)
        {
            perror("malloc name");
            break;
        }
        if (fread(entry.name, 1, entry.name_len, fptr) != entry.name_len)
        {
            free(entry.name);
            break;
        }
        entry.name[entry.name_len] = '\0'; // null-terminate

        // Read offset and size
        if (fread(&entry.offset, sizeof(long), 1, fptr) != 1 || fread(&entry.size, sizeof(long), 1, fptr) != 1)
        {
            free(entry.name);
            break;
        }

        // Safe realloc
        FileEntry *temp = realloc(entries, (count + 1) * sizeof(FileEntry));
        if (!temp)
        {
            perror("realloc entries");
            free(entry.name);
            break;
        }

        entries = temp;
        entries[count++] = entry;
    }

    fclose(fptr);
    *head = entries;
    return count;
}


void add_file(FileEntry **head, int *count, const char *filename)
{
    FILE *fptr = fopen("crypt.hdn", "ab+");
    FILE *file = fopen(filename, "rb");
    if (!fptr || !file)
    {
        perror("fopen");
        if (file)
            fclose(file);
        if (fptr)
            fclose(fptr);
        return;
    }

    // Get size of the file to add
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    // Get current offset in the archive
    fseek(fptr, 0, SEEK_END);
    long offset = ftell(fptr);

    // Read and write file content
    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        fwrite(buffer, 1, bytesRead, fptr);
    }

    // Prepare new FileEntry
    FileEntry entry;
    entry.name_len = strlen(filename);
    entry.name = strdup(filename); // dynamically allocate string
    entry.offset = offset;
    entry.size = fileSize;

    // Reallocate head array and add new entry
    FileEntry *resized = realloc(*head, (*count + 1) * sizeof(FileEntry));
    if (!resized)
    {
        perror("realloc");
        fclose(file);
        fclose(fptr);
        return;
    }
    *head = resized;
    (*head)[*count] = entry;
    (*count)++;

    fclose(file);
    fclose(fptr);
}

FileEntry *get_file(FileEntry **head, int *count, const char *filename)
{
    int i = 0;
    for (i; i < *count; i++)
    {
        if (strcmp((*head)[i].name, filename) == 0)
        {
            return &(*head)[i]; // Return a pointer to the matching FileEntry
        }
    }
    return NULL; // Return NULL if file is not found
}

void extract_file(FileEntry *head, int *count, const char *filename)
{
    FileEntry *file = get_file(&head, count, filename);

    if (file == NULL)
    {
        fprintf(stderr, "Could not find file with name: %s", filename);
        return;
    }
    FILE *fptr = fopen("crypt.hdn", "rb");
    FILE *dst = NULL;
    if (!fptr)
    {
        perror("fopen");
        return;
    }
    fseek(fptr, file->offset, SEEK_SET);

    char *buffer = malloc(file->size);
    if (!buffer)
    {
        perror("malloc");
        return;
    }

    size_t readCount = fread(buffer, 1, file->size, fptr);
    if (readCount != file->size)
    {
        perror("fread");
        free(buffer);
        fclose(fptr);
        return;
    }

    dst = fopen(filename, "wb+");
    if (!dst)
    {
        perror("fopen");
        free(buffer);
        fclose(fptr);
        return;
    }

    fwrite(buffer, 1, file->size, dst);

    free(buffer);
    fclose(fptr);
    fclose(dst);
    delete_file(&head, count, filename);
    return;
}

// void update_file();

void delete_file(FileEntry **head, int *count, const char *filename)
{
    // Find the file in the array
    FileEntry *file = get_file(head, count, filename);
    if (!file)
    {
        fprintf(stderr, "File not found in archive: %s\n", filename);
        return;
    }

    // Open the original archive (crypt.hdn) and create a temporary one (crypt_temp.hdn)
    FILE *fptr = fopen("crypt.hdn", "rb");
    FILE *dst = fopen("crypt_temp.hdn", "wb");
    if (!fptr || !dst)
    {
        perror("fopen");
        if (fptr) fclose(fptr);
        if (dst) fclose(dst);
        return;
    }

    // Start reading from the original file and writing to the new one
    long newOffset = 0;
    for (int i = 0; i < *count; ++i)
    {
        if (&(*head)[i] == file)
            continue; // Skip the file we are deleting

        // Copy the file content to the new archive
        char *buffer = malloc((*head)[i].size);
        fseek(fptr, (*head)[i].offset, SEEK_SET);
        fread(buffer, 1, (*head)[i].size, fptr);
        fwrite(buffer, 1, (*head)[i].size, dst);
        free(buffer);

        // Update the offset in the head array
        (*head)[i].offset = newOffset;
        newOffset += (*head)[i].size;
    }

    fclose(fptr);
    fclose(dst);

    // Remove the original archive and rename the temporary file
    remove("crypt.hdn");
    rename("crypt_temp.hdn", "crypt.hdn");

    // Remove the entry from the array and update the count
    free(file->name);
    for (int i = file - *head; i < *count - 1; ++i)
    {
        (*head)[i] = (*head)[i + 1];  // Shift the entries
    }
    (*count)--;

    // Resize the array
    *head = realloc(*head, (*count) * sizeof(FileEntry));

    // Update table.bin with the new metadata
    update_table(*head, *count);
}
