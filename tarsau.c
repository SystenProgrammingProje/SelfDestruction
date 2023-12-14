//PATH=$PATH:$(pwd) ./tarsau yerine tarsau olarak çalıştırmak için system pathine ekledik
//export PATH := bin:$(PATH) makefile içersinde yap

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_FILENAME_LENGTH 256
#define MAX_BUFFER_SIZE 1024
#define MAX_RECORD_SIZE 256
#define MAX_FILES 32
#define MAX_TOTAL_SIZE 200 * 1024 * 1024

void printUsage() {
    printf("Usage:\n");
    printf("tarsau -b file1 file2 ... fileN -o archive_file.sau\n");
    printf("tarsau -a archive_file.sau directory\n");
}

void createArchive(char *archiveFileName, char *inputFiles[], int numFiles) {
    int archiveFile = open(archiveFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (archiveFile == -1) {
        perror("Error creating archive file");
        exit(1);
    }

    // Organization (contents) section
    char sizeStr[11];
    snprintf(sizeStr, sizeof(sizeStr), "%010d|", numFiles);
    write(archiveFile, sizeStr, 11);

    for (int i = 0; i < numFiles; ++i) {
        struct stat fileStat;
        if (stat(inputFiles[i], &fileStat) == -1) {
            perror("Error getting file information");
            exit(1);
        }

        char record[MAX_RECORD_SIZE];
        snprintf(record, MAX_RECORD_SIZE, "|%s,%o,%ld|", inputFiles[i], fileStat.st_mode & 0777, fileStat.st_size);
        write(archiveFile, record, strlen(record));
    }

    // Archived files section
    for (int i = 0; i < numFiles; ++i) {
        int currentFile = open(inputFiles[i], O_RDONLY);

        if (currentFile == -1) {
            perror("Error opening file");
            exit(1);
        }

        char buffer[MAX_BUFFER_SIZE];
        ssize_t bytesRead;

        while ((bytesRead = read(currentFile, buffer, sizeof(buffer))) > 0) {
            if (write(archiveFile, buffer, bytesRead) != bytesRead) {
                perror("Error writing to archive file");
                exit(1);
            }
        }

        close(currentFile);
    }

    close(archiveFile);
    printf("The files have been merged.\n");
}

void extractArchive(char *archiveFileName, char *outputDir) {
    int archiveFile = open(archiveFileName, O_RDONLY);

    if (archiveFile == -1) {
        perror("Error opening archive file");
        exit(1);
    }

    // Read organization (contents) section
    char sizeStr[11];
    read(archiveFile, sizeStr, 11);
    sizeStr[10] = '\0';

    int numFiles = atoi(sizeStr);

    // Check if the directory already exists
    struct stat st = {0};
    if (stat(directoryPath, &st) == -1) {
        // Directory does not exist, create it
        if (mkdir(directoryPath, 0777) == 0) {
            printf("Directory created successfully.\n");
        } else {
            perror("Error creating directory");
        }
    }


    for (int i = 0; i < numFiles; ++i) {
        char record[MAX_RECORD_SIZE];
        char fileName[MAX_FILENAME_LENGTH];
        mode_t permissions;
        off_t fileSize;

        read(archiveFile, record, MAX_RECORD_SIZE);
        sscanf(record, "|%[^,],%o,%ld|", fileName, &permissions, &fileSize);

        char outputPath[MAX_FILENAME_LENGTH + 10];
        snprintf(outputPath, sizeof(outputPath), "%s/%s", outputDir, fileName);

        int outputFile = open(outputPath, O_WRONLY | O_CREAT | O_TRUNC, permissions);
        if (outputFile == -1) {
            perror("Error creating output file");
            exit(1);
        }

        char buffer[MAX_BUFFER_SIZE];
        ssize_t bytesRead;

        while ((bytesRead = read(archiveFile, buffer, sizeof(buffer))) > 0) {
            if (write(outputFile, buffer, bytesRead) != bytesRead) {
                perror("Error writing to output file");
                exit(1);
            }
        }

        close(outputFile);
    }

    close(archiveFile);
    printf("t1, t2, t3, t4.txt t5.dat files  opened in the %s directory.\n", outputDir);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printUsage();
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0) {
        // Create archive
        char *archiveFileName = "a.sau";  // Default archive file name
        int filesCount = 0;
        char *inputFiles[MAX_FILES];

        for (int i = 2; i < argc; ++i) {
            if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 < argc) {
                    archiveFileName = argv[i + 1];
                    i++;  // Skip the next argument as it is the archive file name
                } else {
                    printf("Missing archive file name after -o parameter.\n");
                    return 1;
                }
            } else {
                if (filesCount < MAX_FILES) {
                    inputFiles[filesCount++] = argv[i];
                } else {
                    printf("Too many input files. Maximum allowed is %d.\n", MAX_FILES);
                    return 1;
                }
            }
        }

        createArchive(archiveFileName, inputFiles, filesCount);
    } else if (strcmp(argv[1], "-a") == 0) {
        // Extract archive
        if (argc == 4) {
            char *archiveFileName = argv[2];
            char *outputDir = argv[3];

            extractArchive(archiveFileName, outputDir);
        } else {
            printf("Invalid number of parameters after -a.\n");
            return 1;
        }
    } else {
        printf("Invalid option: %s\n", argv[1]);
        printUsage();
        return 1;
    }

    return 0;
}
