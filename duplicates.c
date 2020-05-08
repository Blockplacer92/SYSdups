/* duplicates.c */

#include "hash.h"
#include "macros.h"
#include "table.h"

#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>

/* Globals */

char * PROGRAM_NAME = NULL;

/* Structures */
typedef struct {
    bool count;
    bool quiet;
} Options;

/* Functions */

void usage(int status) {
    fprintf(stderr, "Usage: %s paths...\n", PROGRAM_NAME);
    fprintf(stderr, "    -c     Only display total number of duplicates\n");
    fprintf(stderr, "    -q     Do not write anything (exit with 0 if duplicate found)\n");
    exit(status);
}

/**
 * Check if path is a directory.
 * @param       path        Path to check.
 * @return      true if Path is a directory, otherwise false.
 */
bool is_directory(const char *path) {
    // use stat on path to then figure out if it is a directory
    struct stat buf;
    int statResult = stat(path, &buf);
    if(statResult < 0) {
        /* if the statResult is less than 0 then it is definitely not a directory or a file and therefore a comandline input was invalid so the program should exit and show the usage*/    

        exit(1);
    }
    if(!S_ISDIR(buf.st_mode)){
        /* if its not a directory, return false and assume its a file*/
        return false;
    }else{
        /* if it is a directory, return true*/
        return true;
    }
}

/**
 * Check if file is in table of checksums.
 *
 *  If quiet is true, then exit if file is in checksums table.
 *
 *  If count is false, then print duplicate association if file is in
 *  checksums table.
 *
 * @param       path        Path to file to check.
 * @param       checksums   Table of checksums.
 * @param       options     Options.
 * @return      0 if Path is not in checksums, otherwise 1.
 */
size_t check_file(const char *path, Table *checksums, Options *options) {
    /*get md5sum of the path to use as the key when searching table*/
    char hexdigest[HEX_DIGEST_LENGTH];
    bool md5 = hash_from_file(path, hexdigest);
    if(!md5){
        return 0;
    }else{
        /* search the table */
        Value *value = table_search(checksums, hexdigest);

        if(value){
        /*if value, means the md5sum of path is in the table already, aka the current path you are checking is a duplicate of another path already in the table.*/
            if(options->quiet){
                free(options);
                table_delete(checksums);
                free(value);
                exit(1);
            }else if (!options->count){ 
                printf("%s",value->string);
                free(value);
                return 1;
            }else{
                free(value);
                return 1;
            }

        }else{
        /*if not value, means that it is not in the table. Must add it to the table with md5sum of path as the key and the path as the value*/
            Value value;
            value.string = (char *)path;
            table_insert(checksums, hexdigest, value, STRING);
            return 0;
        }
    }
}

/**
 * Check all entries in directory (recursively).
 * @param       root        Path to directory to check.
 * @param       checksums   Table of checksums.
 * @param       options     Options.
 * @return      Number of duplicate entries in directory.
 */
size_t check_directory(const char *root, Table *checksums, Options *options) {
    struct dirent *de;
    DIR *dr = opendir(root);
    if(dr == NULL){
        return 0;
    }
    size_t  dups_count = 0;
    while((de = readdir(dr)) != NULL){
        if(is_directory(de->d_name)){
            if(streq(de->d_name,".") || streq(de->d_name, "..")){
                continue;
            }else{
                check_directory(de->d_name, checksums, options);
            }
        }else{
            dups_count += check_file(de->d_name, checksums, options);
        }
    }
    closedir(dr);
    free(de);
    return dups_count;
}

/* Main Execution */

int main(int argc, char *argv[]) {
    /* Parse command line arguments */
    bool carg = false;
    bool qarg = false;
    PROGRAM_NAME = argv[0];
    size_t length = argc - 1;
    int count = 0;
    for (int i = 1; i < length; i++){
        if (streq(argv[i], "-c")){
            carg = true;
            count += 1;
        }else if(streq(argv[i], "-q")){
            qarg = true;
            count += 1;
        }else if(streq(argv[i], "-h")){
                usage(0);
        }
    }
    length -= count;
    char *paths[length];
    for(int j = 0; j < length; j++){
        paths[j] = argv[j+1];
    }
    Options *options = (Options *)malloc(sizeof(Options));
    options->count = carg;
    options->quiet = qarg;
    Table *t = table_create(length);
    int total_count = 0;
    int dups_count;
    /* Check each argument */
    for(int k = 0; k < length; k++){
        dups_count = check_directory(paths[k], t, options);
        total_count += dups_count;
    }
    
    if (options->count && !options->quiet){
        printf("%d",total_count);
        free(options);
        table_delete(t);
        return EXIT_SUCCESS;
    }
    free(options);
    table_delete(t);


       


    /* Display count if necessary */

    return EXIT_FAILURE;
}

/* vim: set sts=4 sw=4 ts=8 expandtab ft=c: */
