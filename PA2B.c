#include <stdio.h>      // header files: standard in out, arguments
#include <stdlib.h>     // general functions, for malloc(), strtol()
#include <unistd.h>     // for read() write() lseek()
#include <fcntl.h>      // file control, open()
#include <errno.h>      // system call/function errors
#include <string.h>     // manipulating character arrays, strcspn

#define BUFF_SIZE 1024  // buffer size

int main(int argc, char **argv)
{
    int fd = 0;                                 // file descriptor
    // Open the File
    if (argc >= 2)                              // must have 1 argument for file name
    {
        if(access(argv[1], R_OK|W_OK) != 0)     // file must be readable & writeable
        {
            printf("ERROR: File not readable or writeable.\n");
            return -1;
        }
        fd = open(argv[1], O_RDWR);             // open the given file for read/write
        if(fd < 0)                              // successful call returns nonnegative fd
        {
            printf("ERROR (opening the file): %s\n", strerror(errno));
            return -1;                          // non-zero exit
        }
    }
    else                                        // less than 2 args given (no file)
    {
        printf("ERROR: No file name was given from the command line.\n");
        return -1;
    }

    // Prompt the User
    printf("option?\n");
    char choice[3];                             // the user selection of r, w, or s
    char* rbuffer = NULL;                       // the read buffer
    char* wbuffer = malloc(1024);               // the write buffer
    int err;                                    // for error returns
    while(fgets(choice, 2, stdin) != NULL)      // loop until ctrl+d/EOF //while(scanf("%s", choice) != EOF)    
    {
        while(getchar() != '\n' && getchar() != EOF) //removing newlines from enter //choice[strcspn(choice, "\n")] = 0;
            continue;
        // Options
        if(*choice == 'r')                      // Read
        {
            long bsize;                         // number of bytes to read and buffer size
            char num[10];                       // number of bytes to read
            char* ptr;                          // pointer for strtol
            printf("Enter the number of bytes you want to read: ");
            if(fgets(num, 10, stdin) == NULL)   // get the number of bytes
                break;                          // break at a ctrl+d
            bsize = strtol(num, &ptr, 10);      // convert the string into a long, if no number given 0 will be read
            rbuffer = realloc(rbuffer, bsize+1);// readjust the read buffer on heap to right size
            err = read(fd, rbuffer, bsize);     // Read bsize bytes from file descriptor into buffer
            //if(err < 0)                         // -1 indicates error
            //    printf("There was an error reading the file: %s\n", strerror(errno));
            //else if(err == 0)                   // 0 indicates EOF or 0 bytes read,
            //    printf("There is nothing to read from the file.\n");
            //else                                // print the data that was read to the console
            printf("%s\n", rbuffer);
        }
        else if(*choice == 'w')                     // Write
        {
            printf("Enter the data you want to write: ");
            if(fgets(wbuffer, 1024, stdin) == NULL) //gather what to write from the user
                break;
            wbuffer[strcspn(wbuffer, "\n")] = 0;
            err = write(fd, wbuffer, strlen(wbuffer)); //write to the file
            //if(err < 0)
            //    printf("There was an error writing to the file: %s\n", strerror(errno));
            //else if(err == 0)
            //    printf("Nothing was written to the file.\n");
        }
        else if(*choice == 's')
        {
            long offset;                        // the offset value
            char offsetstr[10];                 // character array for offset user input
            char whencestr[10];                 // character array for whence user input
            char* ptr;                          // pointer for strtol       
            printf("Enter an offset value: ");  // Gather the offset
            if(fgets(offsetstr, 10, stdin) == NULL)
                break;                          // get user input, break at a ctrl+d
            offset = strtol(offsetstr, &ptr, 10); //convert the string into a long, if no number given 0
            printf("Enter a value for whence: "); // Gather the whence
            if(fgets(whencestr, 10, stdin) == NULL)
                break;                          // get user input, break at ctrl+d
            whencestr[strcspn(whencestr, "\n")] = 0; // remove \n, get span until \n character in str
            // Choose the right seek
            if((strcmp(whencestr, "SEEK_SET") == 0) || (strcmp(whencestr, "0") == 0))
                err = lseek(fd, offset, SEEK_SET);
            else if((strcmp(whencestr, "SEEK_CUR") == 0) || (strcmp(whencestr, "1") == 0))
                err = lseek(fd, offset, SEEK_CUR);
            else if((strcmp(whencestr, "SEEK_END") == 0) || (strcmp(whencestr, "2") == 0))
                err = lseek(fd, offset, SEEK_END);
            //if(err < 0)
            //    printf("There was an error changing the offset: %s\n", strerror(errno));
        }
        memset(choice, 0, sizeof(choice));      // clear the choice character array
        printf("option?\n");                    // re-prompt the user
    }

    free(rbuffer);                              // free read buffer memory
    free(wbuffer);                              // free write buffer memory
    close(fd);                                  // close the file
    return 0;                                   // return successfully
}
     


