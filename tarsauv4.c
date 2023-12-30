#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

#define MAX_FILES 32                    // 32 files
#define MAX_FILE_SIZE 200 * 1024 * 1024 // 200 MB

int main(int argc, char **argv)
{
    // Checking argument count and argument vector which should be appropriate format and
    // Considering second argument of argument vectors to start process.

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
        if (argc < 3)
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

        FILE *arch, *file;
        DIR *dir;
        char *archive = argv[2];
        char *directory;
        char c;
        if (argv[3] == NULL){
            directory = ".";
        }
        else{
            directory = argv[3];
        }
        // Going over the output directory if it does not exist create the particular directory which was given argument by user
        dir = opendir(directory);
        if (dir == NULL)
        {

            if (ENOENT == errno)
            {

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

        // Open the file and handling error.

        arch = fopen(archive, "r");
        if (arch == NULL)
        {
            printf("Cannot open archive file.\n");
            exit(1);
        }

        char filename[100];
        int filepermission, headersize, filesize;
        long location;

        // Start to read until end of the file continue loop.
        // Move the file position indicator 10 bytes ahead in the archive.

        fscanf(arch, " %d", &headersize);
        fseek(arch, 10, SEEK_SET);
        int i = 0, addition;
        while (fscanf(arch, "|%[^,],%o,%d|", filename, &filepermission, &filesize) == 3)
        {
            // Create the full path for the file in the specified directory
            char fullpath[256];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", directory, filename);
            file = fopen(fullpath, "w");
            if (file == NULL)
            {
                printf("Cannot open file %s for writing.\n", fullpath);
                exit(1);
            }

            location = ftell(arch);
            if (i == 0)
            {
                addition = headersize;
                i++;
            }

            // Copy the contents of the file from the archive to the newly created file
            fseek(arch, addition, SEEK_SET);
            for (int i = 0; i < filesize; i++)
            {
                c = fgetc(arch);
                if(c == '\n'){} else{fputc(c, file);}
            }

            // Restore the file position in the archive for the next iteration
            addition = ftell(arch);
            fclose(file);
            chmod(fullpath, filepermission);
            fseek(arch, location, SEEK_SET);
        }

        printf("t1, t2, t3, t4.txt t5.dat files  opened in the %s directory.\n", directory);

    }

    // Logic of condition will play a role according to argument using operationType variable.

    if (operationType == 1)
    { // tarsau -b
        FILE *output_file, *argument, *temp;
        ssize_t size;
        struct stat buf;
        int exists;
        char *fileName;

        // Check the command line second argument which will be archieved file name

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

        // Create a archieve file and control the file arguments is there any problem about file number or file size

        output_file = fopen(fileName, "w+");
        long headersize = 0;
        if (argc - 2 > MAX_FILES)
        { // The number of input files cannot be more than 32
            fprintf(stderr, "Exceeded max file count:32");
            exit(1);
        }

        // The first 10 bytes hold the numerical size of the first section
        // Write metadata to the archive like |filename, permission mode, file size|

        fseek(output_file, 10, SEEK_SET);
        for (i = 2; i < argc; i++)
        {
            exists = stat(argv[i], &buf);
            headersize += fprintf(output_file, "|%s,%04o,%ld|", argv[i], buf.st_mode & 0777, buf.st_size);
        }
        headersize += 11;
        fprintf(output_file, "\n");
        rewind(output_file);
        fprintf(output_file, "%-10ld", headersize);

        if (headersize > MAX_FILE_SIZE)
        { // The total size of input files cannot exceed 200 MBytes.
            fprintf(stderr, "Exceeded max file size:200MB");
            exit(1);
        }

        // Move the file position indicator in the output file to the position.
        fseek(output_file, headersize, SEEK_SET);
        char *tmp;

        // Get into the files which was given as argument on command line.
        //  which represent the paths of the archieve file and output directory
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

                // Allocate memory for the temporary buffer based on the file size
                tmp = malloc(buf.st_size);

                // Read the contents of the file into the temporary buffer
                size = fread(tmp, 1, buf.st_size, argument);
                if (size > 0)
                {
                    for (int j = 0; j < size; j++)       
                    {   // Checking if there is incompatible file format
                        if ((!isascii(tmp[j]) || iscntrl(tmp[j])) && !isspace(tmp[j]))
                        {
                            fprintf(stderr, "Incompatible input file format: %s\n",argv[i]);
                            fclose(argument);
                            exit(1);
                        }
                    }
                    // Write the contents of the temporary buffer to the output file
                    fwrite(tmp, 1, buf.st_size, output_file);
                    size = fread(tmp, 1, buf.st_size, argument);
                }
                free(tmp);
                fclose(argument);
            }
        }
        fclose(output_file);

        printf("The files have been merged.\n");

    }
    return 0;
}
