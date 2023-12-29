#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <fstab.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define MAX_FILES 32                    // 32 files
#define MAX_FILE_SIZE 200 * 1024 * 1024 // 200 MB

int main(int argc, char **argv)
{

    int i = 0, operationType;

    if (argc < 2)
    {
        printf("Usage: %s -b file1 file2 ... -o output.sau\n", argv[0]);
        printf("or: %s -a archive.sau directory\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "-b") == 0)
    {
        operationType = 1;
    }
    else if (strcmp(argv[1], "-a") == 0)
    {
        if (argc != 4)
        {
            printf("Usage: %s -a archive.sau directory\n", argv[0]);
            exit(1);
        }
        if (strstr(argv[2], ".sau") == NULL)
        {
            printf("Archive file is inappropriate or corrupt!\n");
            exit(1);
        }
        operationType = 0;
    }
    else
    {
        printf("Invalid option: %s\n", argv[1]);
        exit(1);
    }

    if (operationType == 0)
    { // tarsau -a
        printf("tarsau -a\n");
        FILE *arch, *file;
        DIR *dir;
        char *archive = argv[2];
        char *directory = argv[3];
        char c;
        dir = opendir(directory);
        if (dir == NULL)
        {
            printf("Failed to open directory.\n");
            if (ENOENT == errno)
            {
                printf("Directory does not exist. Creating directory.\n");
                mkdir(directory, 0700);
            }
            else
            {
                printf("An error occurred that wasn't ENOENT.\n");
                fprintf(stderr, "Couldn't open the directory %s", directory);
                exit(1);
            }
        }
        else
        {
            printf("Directory exists. Proceeding with the operation.\n");
            closedir(dir);
        }
        arch = fopen(archive, "r");
        if (arch == NULL)
        {
            printf("Cannot open archive file.\n");
            exit(1);
        }

        char filename[100];
        int filepermission, headersize,filesize;
        long location;

        fscanf(arch, " %d", &headersize);
        fseek(arch,10,SEEK_SET);
        int i = 0,addition;
        while (fscanf(arch, "|%[^,],%o,%d|", filename, &filepermission, &filesize) == 3)
        {
            printf("%s", filename);
            char fullpath[256];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", directory, filename);
            file = fopen(fullpath, "w");
            if (file == NULL)
            {
                printf("Cannot open file %s for writing.\n", fullpath);
                exit(1);
            }

            location= ftell(arch);
            if (i == 0){
                addition = headersize;
                i++;
            }
            fseek(arch,addition,SEEK_SET);
            for (int i = 0; i < filesize; i++)
            {
                c = fgetc(arch);
                fputc(c, file);
            }
            addition= ftell(arch);
            fclose(file);
            chmod(fullpath, filepermission);
            fseek(arch,location,SEEK_SET);
        }
    }
    if (operationType == 1)
    { // tarsau -b
        FILE *output_file, *argument, *temp;
        ssize_t size;
        struct stat buf;
        int exists;
        char *fileName;

        for (i = 1; i < argc; i++)
        {

            if (strcmp(argv[i], "-o") == 0)
            {
                fileName = argv[i + 1];
                argc -= 2;
                break;
            }
            else
            {
                fileName = "a.sau";
            }
        }

        printf("tarsau -b\n");
        output_file = fopen(fileName, "w+");
        long headersize = 0;
        if (argc - 2 > MAX_FILES)
        { // The number of input files cannot be more than 32
            fprintf(stderr, "Exceeded max file count:32");
            exit(1);
        }
        fseek(output_file, 10, SEEK_SET);
        for (i = 2; i < argc; i++)
        {
            exists = stat(argv[i], &buf);
            headersize += fprintf(output_file, "|%s,%04o,%ld|", argv[i], buf.st_mode & 0777, buf.st_size);
        }
        headersize += 10;
        fprintf(output_file, "\n");
        rewind(output_file);
        fprintf(output_file, "%-10ld", headersize);

        if (headersize > MAX_FILE_SIZE)
        { // The total size of input files cannot exceed 200 MBytes.
            fprintf(stderr, "Exceeded max file size:200MB");
            exit(1);
        }
        // The first 10 bytes hold the numerical size of the first section
        fseek(output_file, headersize + 1, SEEK_SET);
        char *tmp;
        for (i = 2; i < argc; i++)
        {
            exists = stat(argv[i], &buf);
            if (exists < 0)
            {
                fprintf(stderr, "%s not found\n", argv[i]);
            }
            else
            {

                argument = fopen(argv[i], "r");
                tmp = malloc(buf.st_size);

                size = fread(tmp, 1, buf.st_size, argument);
                if (size > 0)
                {
                    fwrite(tmp, 1, buf.st_size, output_file);
                    size = fread(tmp, 1, buf.st_size, argument);
                }
                free(tmp);
                fclose(argument);
            }
        }
        fclose(output_file);
    }
    return 0;
}