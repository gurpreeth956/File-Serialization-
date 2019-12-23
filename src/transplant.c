#include "const.h"
#include "transplant.h"
#include "debug.h"

#include <stdio.h>

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int stringCompare(char *string1, char *string2);
int stringLength(char *string1);
int checkMagicSeq();
long getHexToDecimal(int length);
void putChar4Bytes(int length);
void putChar8Bytes(int length);


/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the pathname of the current file or directory
 * as well as other data have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */

/*
 * A function that returns printable names for the record types, for use in
 * generating debugging printout.
 */
static char *record_type_name(int i) {
    switch(i) {
    case START_OF_TRANSMISSION:
    return "START_OF_TRANSMISSION";
    case END_OF_TRANSMISSION:
    return "END_OF_TRANSMISSION";
    case START_OF_DIRECTORY:
    return "START_OF_DIRECTORY";
    case END_OF_DIRECTORY:
    return "END_OF_DIRECTORY";
    case DIRECTORY_ENTRY:
    return "DIRECTORY_ENTRY";
    case FILE_DATA:
    return "FILE_DATA";
    default:
    return "UNKNOWN";
    }
}

/*
 * @brief  Initialize path_buf to a specified base path.
 * @details  This function copies its null-terminated argument string into
 * path_buf, including its terminating null byte.
 * The function fails if the argument string, including the terminating
 * null byte, is longer than the size of path_buf.  The path_length variable
 * is set to the length of the string in path_buf, not including the terminating
 * null byte.
 *
 * @param  Pathname to be copied into path_buf.
 * @return 0 on success, -1 in case of error
 */
int path_init(char *name) {
    // Error if argument length is greater than path length
    if (stringLength(name) > PATH_MAX) {
        return -1;
    }

    // Before initializing, make sure path_buf is empty
    char *pointer = path_buf;
    for (int i = 0; i < PATH_MAX; i++) {
        *pointer = '\0';
        pointer++;
    }

    // No error, so copy argument to path_buf
    pointer = path_buf;
    path_length = 0;

    while (*name != '\0') {
        *pointer = *name;
        path_length++;
        pointer++;
        name++;
    }

    // Add null terminator to the end of array
    *pointer = '\0';
    return 0;
}

/*
 * @brief  Append an additional component to the end of the pathname in path_buf.
 * @details  This function assumes that path_buf has been initialized to a valid
 * string.  It appends to the existing string the path separator character '/',
 * followed by the string given as argument, including its terminating null byte.
 * The length of the new string, including the terminating null byte, must be
 * no more than the size of path_buf.  The variable path_length is updated to
 * remain consistent with the length of the string in path_buf.
 *
 * @param  The string to be appended to the path in path_buf.  The string must
 * not contain any occurrences of the path separator character '/'.
 * @return 0 in case of success, -1 otherwise.
 */
int path_push(char *name) {
    // Error if argument length is greater than path length
    if (stringLength(name) + path_length + 1 > PATH_MAX) {
        return -1;
    }

    // Get to current null terminator position
    char *pointer = path_buf;

    while (*pointer != '\0') {
        pointer++;
    }

    // Add '/' where null terminator is
    *pointer = '/';
    path_length++;
    pointer++;

    // Copy name to path_buff after '/'
    while (*name != '\0') {
        *pointer = *name;
        path_length++;
        pointer++;
        name++;
    }

    // Add null terminator to the end of array
    *pointer = '\0';
    return 0;
}

/*
 * @brief  Remove the last component from the end of the pathname.
 * @details  This function assumes that path_buf contains a non-empty string.
 * It removes the suffix of this string that starts at the last occurrence
 * of the path separator character '/'.  If there is no such occurrence,
 * then the entire string is removed, leaving an empty string in path_buf.
 * The variable path_length is updated to remain consistent with the length
 * of the string in path_buf.  The function fails if path_buf is originally
 * empty, so that there is no path component to be removed.
 *
 * @return 0 in case of success, -1 otherwise.
 */
int path_pop() {
    // Error if path length is 0
    if (path_length == 0) {
        return -1;
    }

    // Pointer for iterating through array and saving last '/'
    char *tempPointer = path_buf;
    char *mainPointer = path_buf;

    // Finding last occurance of '/' if any
    while (*tempPointer != '\0') {
        if (*tempPointer == *"/") {
            mainPointer = tempPointer;
        }
        tempPointer++;
    }

    // Deleting all elements after last '/'
    while (*mainPointer != '\0') {
        *mainPointer = '\0';
        path_length--;
        mainPointer++;
    }

    // Return successfully
    return 0;
}

/*
 * @brief Deserialize directory contents into an existing directory.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory.  It reads (from the standard input) a sequence of DIRECTORY_ENTRY
 * records bracketed by a START_OF_DIRECTORY and END_OF_DIRECTORY record at the
 * same depth and it recreates the entries, leaving the deserialized files and
 * directories within the directory named by path_buf.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * each of the records processed.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including depth fields in the records read that do not match the
 * expected value, the records to be processed to not being with START_OF_DIRECTORY
 * or end with END_OF_DIRECTORY, or an I/O error occurs either while reading
 * the records from the standard input or in creating deserialized files and
 * directories.
 */
int deserialize_directory(int depth) {
    // Check if magic sequence exists
    if (checkMagicSeq() == -1) {
        return -1;
    }

    // Get type byte
    int eofCheck = getchar();
    if (eofCheck == EOF) {
        return -1;
    }
    unsigned char current = eofCheck;

    // Make sure first row is start of directory
    if (current != START_OF_DIRECTORY) {
        return -1;
    }

    // If depth does not match return error
    unsigned int currDepth = getHexToDecimal(4);
    if (currDepth != depth) {
        return -1;
    }

    // Iterate rest of directory entry
    for (int i = 0; i < 8; i++) {
        eofCheck = getchar();
        if (eofCheck == EOF) {
            return -1;
        }
    }


    // Variable used for checking if method return is -1
    int getReturn = 0;

    // Boolean values, 0 for false 1 for true
    int endOfCurrDir = 0;

    // Loop for checking until end of directory is found
    while (endOfCurrDir != 1) {
        // Check if magic sequence exists
        if (checkMagicSeq() == -1) {
            return -1;
        }

        // Get type byte
        eofCheck = getchar();
        if (eofCheck == EOF) {
            return -1;
        }
        current = eofCheck;

        // If first header not directory entry or end, return error
        if (current != DIRECTORY_ENTRY && current != END_OF_DIRECTORY) {
            return -1;
        }

        // If depth does not match return error
        currDepth = getHexToDecimal(4);
        if (currDepth != depth) {
            return -1;
        }

        // Get header data
        unsigned long currLength = getHexToDecimal(8);
        int remLength = currLength - 16;

        // If end of directory, then finish header and break out of it
        if (current == END_OF_DIRECTORY) {
            endOfCurrDir = 1;
            if (getReturn == -1) {
                return -1;
            }
            break;
        }


        // Get file meta data since it is a directory entry
        eofCheck = getchar();
        if (eofCheck == EOF) {
            return -1;
        }
        mode_t currType = eofCheck << 24;
        eofCheck = getchar();
        if (eofCheck == EOF) {
            return -1;
        }
        mode_t tempType = eofCheck << 16;
        currType |= tempType;
        eofCheck = getchar();
        if (eofCheck == EOF) {
            return -1;
        }
        tempType = eofCheck << 8;
        currType |= tempType;
        eofCheck = getchar();
        if (eofCheck == EOF) {
            return -1;
        }
        tempType = eofCheck;
        currType |= tempType;

        for (int i = 0; i < 8; i++) {
            eofCheck = getchar();
            if (eofCheck == EOF) {
                return -1;
            }
        }

        // Clear name_buf before using
        char *pointer = name_buf;
        for (int i = 0; i < NAME_MAX; i++) {
            *pointer = '\0';
            pointer++;
        }

        // For storing file name from stdin in name_buf
        pointer = name_buf;
        for (int i = 0; i < remLength - 12; i++) {
            int eofCheck = getchar();
            if (eofCheck == EOF) {
                return -1;
            }
            unsigned char current = eofCheck;
            *pointer = current;
            pointer++;
        }

        // Updata path_buf
        pointer = name_buf;
        getReturn = path_push(pointer);
        if (getReturn == -1) {
            return -1;
        }


        // Check if type is a file or directory
        if (S_ISREG(currType)) {
            // Deserialize File
            getReturn = deserialize_file(currDepth);
            if (getReturn == -1) {
                return -1;
            }
        } else {
            // Try to open directory and deal accordingly
            DIR *dir = opendir(path_buf);
            if (dir) {
                if ((global_options & 0x8) == 0x8) {
                    // Clobber exists so make directory
                    mkdir(path_buf, 0700);
                } else {
                    return -1;
                }
            } else {
                // Make dir if does not exist
                mkdir(path_buf, 0700);
            }

            // Deserialize Directory
            getReturn = deserialize_directory(depth + 1);
            if (getReturn == -1) {
                return -1;
            }
        }

        // Set mode of directory/file
        chmod(path_buf, currType & 0777);
        path_pop();
    }


    // Exit was not end of directory
    if (endOfCurrDir != 1) {
        return -1;
    }

    // Function done, so return success
    return 0;
}


/*
 * @brief Deserialize the contents of a single file.
 * @details  This function assumes that path_buf contains the name of a file
 * to be deserialized.  The file must not already exist, unless the ``clobber''
 * bit is set in the global_options variable.  It reads (from the standard input)
 * a single FILE_DATA record containing the file content and it recreates the file
 * from the content.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * the FILE_DATA record.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including a depth field in the FILE_DATA record that does not match
 * the expected value, the record read is not a FILE_DATA record, the file to
 * be created already exists, or an I/O error occurs either while reading
 * the FILE_DATA record from the standard input or while re-creating the
 * deserialized file.
 */
int deserialize_file(int depth) {
    // Create file and check if it exists to return error
    FILE *f;

    if ((global_options & 0x8) != 0x8) {
        // Check if file exists, if so then return error
        struct stat file_buf;
        if (stat(path_buf, &file_buf) == 0) {
            return -1;
        }
        f = fopen(path_buf, "w");
    } else {
        // For clobber, so overwrite the file
        f = fopen(path_buf, "w+");
    }

    // Check if magic sequence exists
    if (checkMagicSeq() == -1) {
        return -1;
    }


    // Move pointer since magic sequence checked
    int remBytes = 12;
    int eofCheck = getchar();
    if (eofCheck == EOF) {
        return -1;
    }
    unsigned char current = eofCheck;

    // Byte should be 5 since FILE_DATA
    if (current != FILE_DATA) {
        return -1;
    }

    // Convert file depth
    unsigned int fileDepth = getHexToDecimal(4);
    remBytes -= 4;
    if (fileDepth != depth) {
        return -1;
    }

    // Convert file length
    unsigned long thisLength = getHexToDecimal(8);
    remBytes -= 8;
    if (thisLength == -1) {
        return -1;
    }
    remBytes = thisLength - 16;


    // Iterate through the bytes
    while (remBytes > 0) {
        // Get next char and return error if none
        eofCheck = getchar();
        if (eofCheck == EOF) {
            return -1;
        }
        current = eofCheck;

        // No error so put byte in file
        fputc(current, f);
        remBytes--;
    }

    // Close file and return success
    fclose(f);
    return 0;
}


/**
 * @brief Reads serialized data from the standard input and reconstructs from it
 * a tree of files and directories.
 * @details  This function assumes path_buf has been initialized with the pathname
 * of a directory into which a tree of files and directories is to be placed.
 * If the directory does not already exist, it is created.  The function then reads
 * from from the standard input a sequence of bytes that represent a serialized tree
 * of files and directories in the format written by serialize() and it reconstructs
 * the tree within the specified directory.  Options that modify the behavior are
 * obtained from the global_options variable.
 *
 * @return 0 if deserialization completes without error, -1 if an error occurs.
 */
int deserialize() {
    // Check if magic sequence exists
    if (checkMagicSeq() == -1) {
        return -1;
    }

    // Get type byte
    int eofCheck = getchar();
    if (eofCheck == EOF) {
        return -1;
    }
    unsigned char current = eofCheck;

    // If first header not start of transmission, return error
    if (current != START_OF_TRANSMISSION) {
        return -1;
    }

    // Since start of transmission go to next header
    for (int i = 0; i < 12; i++) {
        eofCheck = getchar();
        if (eofCheck == EOF) {
            return -1;
        }
    }

    // If directory does not exist, create it
    DIR *dir = opendir(path_buf);
    if (!dir) {
        mkdir(path_buf, 0700);
    }

    // Call desrialize_directory function
    int getReturn = deserialize_directory(1);
    if (getReturn == -1) {
        return -1;
    }

    // Done deserializing, so return success
    return 0;
}


/*
 * @brief  Serialize the contents of a directory as a sequence of records written
 * to the standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory to be serialized.  It serializes the contents of that directory as a
 * sequence of records that begins with a START_OF_DIRECTORY record, ends with an
 * END_OF_DIRECTORY record, and with the intervening records all of type DIRECTORY_ENTRY.
 *
 * @param depth  The value of the depth field that is expected to occur in the
 * START_OF_DIRECTORY, DIRECTORY_ENTRY, and END_OF_DIRECTORY records processed.
 * Note that this depth pertains only to the "top-level" records in the sequence:
 * DIRECTORY_ENTRY records may be recursively followed by similar sequence of
 * records describing sub-directories at a greater depth.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open files, failure to traverse directories, and I/O errors
 * that occur while reading file content and writing to standard output.
 */
int serialize_directory(int depth) {
    // Open the  directory since it is assumed it exists
    DIR *dir = opendir(path_buf);
    struct dirent *de;
    int getReturn = 0;

    // If directory does not open, return failure
    if (dir == NULL) {
        return -1;
    }

    // Serialize the current directory
    putchar(0x0C);
    putchar(0x0D);
    putchar(0xED);
    putchar(0x02);

    // Calculate needed depth and store it
    putChar4Bytes(depth);

    // For needed length and store it
    for (int i = 0; i < 7; i++) {
        putchar(0x00);
    }
    putchar(0x10);


    // Traverse to directories
    while ((de = readdir(dir)) != NULL) {
        // Check for . or .. dirs
        if (stringCompare(de->d_name, ".") == 0 || stringCompare(de->d_name, "..") == 0) {
            continue;
        }

        // Get file name and push it to the path
        char *namePoint = de->d_name;
        char *namePoint2 = namePoint;
        getReturn = path_push(namePoint);
        if (getReturn == -1) {
            return -1;
        }

        // Creating stat struct for getting file meta daata
        struct stat stat_buf;
        stat(path_buf, &stat_buf);

        // Get total and name length
        int totalLength = 28;
        int nameLength = stringLength(namePoint);
        totalLength += nameLength - 1;


        // Serialize directory entry
        putchar(0x0C);
        putchar(0x0D);
        putchar(0xED);
        putchar(0x04);

        // Get depth char and store it in 4 bytes
        putChar4Bytes(depth);

        // Get length char and store it in 8 bytes
        putChar8Bytes(totalLength);

        // Get depth char and store it in 4 bytes
        mode_t type = stat_buf.st_mode;
        putchar((type & 0xFF000000) >> 24);
        putchar((type & 0xFF0000) >> 16);
        putchar((type & 0xFF00) >> 8);
        putchar(type & 0xFF);

        // Get length char and store it in 8 bytes
        off_t size = stat_buf.st_size;
        putchar((size & 0xFF00000000000000) >> 56);
        putchar((size & 0xFF000000000000) >> 48);
        putchar((size & 0xFF0000000000) >> 40);
        putchar((size & 0xFF00000000) >> 32);
        putchar((size & 0xFF000000) >> 24);
        putchar((size & 0xFF0000) >> 16);
        putchar((size & 0xFF00) >> 8);
        putchar(size & 0xFF);

        // Get name char and store it in leftover bytes
        while (*namePoint2 != '\0') {
            putchar(*namePoint2);
            namePoint2++;
        }


        // Check if file or directory
        if (S_ISREG(stat_buf.st_mode)) {
            // Now serialize file content
            getReturn = serialize_file(depth, stat_buf.st_size);
            if (path_pop() == -1) {
                return -1;
            }
            if (getReturn == -1) {
                return -1;
            }
        } else if (S_ISDIR(stat_buf.st_mode)) {
            // Now serialize directory content
            getReturn = serialize_directory(depth + 1);
            if (path_pop() == -1) {
                return -1;
            }
            if (getReturn == -1) {
                return -1;
            }
        }
    }


    // Complete end of directory entry
    putchar(0x0C);
    putchar(0x0D);
    putchar(0xED);
    putchar(0x03);

    // Get depth char and store it in 4 bytes
    putChar4Bytes(depth);

    // Finish the length field
    for (int i = 0; i < 7; i++) {
        putchar(0x00);
    }
    putchar(0x10);

    // Done serializing directory, so return success
    closedir(dir);
    return 0;
}


/*
 * @brief  Serialize the contents of a file as a single record written to the
 * standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * file to be serialized.  It serializes the contents of that file as a single
 * FILE_DATA record emitted to the standard output.
 *
 * @param depth  The value to be used in the depth field of the FILE_DATA record.
 * @param size  The number of bytes of data in the file to be serialized.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open the file, too many or not enough data bytes read
 * from the file, and I/O errors reading the file data or writing to standard output.
 */
int serialize_file(int depth, off_t size) {
    // Open the file and return error if it does not exist
    FILE *f = fopen(path_buf, "r");
    if (!f) {
        return -1;
    }

    // Calculate total length of file entry
    long totalLength = 16 + size;

    // Put basic directory file entry data
    putchar(0x0C);
    putchar(0x0D);
    putchar(0xED);
    putchar(0x05);

    // Calculate needed depth and store it
    putChar4Bytes(depth);

    // Calculate needed length and store it
    putChar8Bytes(totalLength);


    // Put file content in file entry data
    int eofCheck = 0;
    for (int i = 0; i < size; i++) {
        eofCheck = fgetc(f);
        if (eofCheck == EOF) {
            return -1;
        }
        unsigned char current = eofCheck;
        putchar(current);
    }

    // Close file and return success
    eofCheck = fclose(f);
    if (eofCheck == EOF) {
        return -1;
    }
    return 0;
}


/**
 * @brief Serializes a tree of files and directories, writes
 * serialized data to standard output.
 * @details This function assumes path_buf has been initialized with the pathname
 * of a directory whose contents are to be serialized.  It traverses the tree of
 * files and directories contained in this directory (not including the directory
 * itself) and it emits on the standard output a sequence of bytes from which the
 * tree can be reconstructed.  Options that modify the behavior are obtained from
 * the global_options variable.
 *
 * @return 0 if serialization completes without error, -1 if an error occurs.
 */
int serialize() {
    // Add start of transmission entry
    putchar(0x0C);
    putchar(0x0D);
    putchar(0xED);
    for (int i = 0; i < 12; i++) {
        putchar(0x00);
    }
    putchar(0x10);

    // Call on serialize_directory
    int getReturn = serialize_directory(1);
    if (getReturn == -1) {
        return -1;
    }

    // Add end of transmission entry
    putchar(0x0C);
    putchar(0x0D);
    putchar(0xED);
    putchar(0x10);
    for (int i = 0; i < 11; i++) {
        putchar(0x00);
    }
    putchar(0x10);

    // Serializing done so return success
    fflush(stdout);
    return -0;
}


/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv) {
    // No arguments provided, so return error
    if (argc <= 1) {
        return -1;
    }

    // Go to next argument
    argv++;

    // If -h flag, display usage and exit success
    if (stringCompare("-h", *argv) == 0) {
        global_options |= 0x1;
        return 0;
    }


    // If -s flag, perform serialization
    if (stringCompare("-s", *argv) == 0) {
        // Boolean value, 0 for false 1 for true
        int pathInitiated = 0;
        if (argc > 2) {
            argv++;

            while (*argv != NULL) {
                // If -c flag
                if (stringCompare("-c", *argv) == 0) {
                    return -1;
                }
                // If -p flag
                else if (stringCompare("-p", *argv) == 0) {
                    // Need to check for DIR
                    argv++;
                    if (*argv == NULL) {
                        return -1;
                    }
                    if (**argv == *"-") {
                        return -1;
                    }
                    if (path_init(*argv) == -1) {
                        return -1;
                    }
                    pathInitiated = 1;
                }
                // If other return error
                else {
                    return -1;
                }

                argv++;
            }
        }

        // Else set current directory for serialization
        if (pathInitiated == 0) {
            if (path_init(".") == -1) {
                return -1;
            }
        }

        // Set the global options and return
        global_options |= 0x2;
        return 0;
    }


    // If -d flag, perform deserialization
    if (stringCompare("-d", *argv) == 0) {
        // Boolean value, 0 for false 1 for true
        int pathInitiated = 0;
        if (argc > 2) {
            argv++;
            int extraArgs = argc - 2;

            while (*argv != NULL) {
                // If -c flag
                if (stringCompare("-c", *argv) == 0) {
                    global_options |= 0x8;
                }
                // If -p flag
                else if (stringCompare("-p", *argv) == 0) {
                    // need to check for DIR
                    argv++;
                    if (*argv == NULL) {
                        return -1;
                    }
                    if (**argv == *"-") {
                        return -1;
                    }
                    if (path_init(*argv) == -1) {
                        return -1;
                    }

                    pathInitiated = 1;
                }
                // If other return error
                else {
                    return -1;
                }

                argv++;
            }
        }

        // Else set current directory for deserialization
        if (pathInitiated == 0) {
            if (path_init(".") == -1) {
                return -1;
            }
        }

        // Set global options and return
        global_options |= 0x4;
        return 0;
    }


    // First flag was neither -h, -s, or -d so return error
    return -1;
}


// Function for comparing two char arrays (0 for same, -1 for not)
int stringCompare(char *string1, char *string2) {
    while (*string1 == *string2) {
        if (*string1 == '\0' && *string2 == '\0') return 0;
        if (*string1 == '\0' || *string2 == '\0') return -1;
        string1++;
        string2++;
    }
    return -1;
}


// Function for getting char array length
int stringLength(char *string1) {
    int length = 0;
    while (*string1 != '\0') {
        string1++;
        length++;
    }
    return length + 1;
}


// Function for checking magic sequence in file
int checkMagicSeq() {
    // Create needed pointers using getchar() since stdin
    int firstEOF = getchar();
    int secondEOF = getchar();
    int thirdEOF= getchar();

    // Check if above chars do not exist
    if (firstEOF == EOF || secondEOF == EOF || thirdEOF == EOF) {
        return -1;
    }

    // Set to unsigned chars for no errors
    unsigned char first = firstEOF;
    unsigned char second = secondEOF;
    unsigned char third = thirdEOF;

    // First character
    if (first != MAGIC0) {
        return -1;
    }
    // Second character
    if (second != MAGIC1) {
        return -1;
    }
    // Third character
    if (third != MAGIC2) {
        return -1;
    }

    return 0;
}


// Function for getting the decimal value of hex bytes
long getHexToDecimal(int length) {
    unsigned long value = 0;

    for (int i = 0; i < length; i++) {
        int num = getchar();
        // Make sure num if not eof
        if (num == EOF) {
            return -1;
        }
        value |= num;
        // Do not shift if last byte
        if (i != length - 1) {
            value = value << 8;
        }
    }

    return value;
}


// Function for putting 4 bytes to stdout
void putChar4Bytes(int length) {
    putchar((length & 0xFF000000) >> 24);
    putchar((length & 0xFF0000) >> 16);
    putchar((length & 0xFF00) >> 8);
    putchar(length & 0xFF);
}


// Function for putting 8 bytes to stdout
void putChar8Bytes(int length) {
    putchar((length & 0xFF00000000000000) >> 56);
    putchar((length & 0xFF000000000000) >> 48);
    putchar((length & 0xFF0000000000) >> 40);
    putchar((length & 0xFF00000000) >> 32);
    putchar((length & 0xFF000000) >> 24);
    putchar((length & 0xFF0000) >> 16);
    putchar((length & 0xFF00) >> 8);
    putchar(length & 0xFF);
}

