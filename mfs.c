// The MIT License (MIT)
// 
// Copyright (c) 2020 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// Karun Dawadi 
// Prajwal Rana 

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

#define MAX_NUM_ARGUMENTS 3

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size


struct __attribute__((__packed__)) DirectoryEntry{
    char DIR_NAME[11]; // The name of the directory { Specs on the whitepaper document}
    uint8_t DIR_Attr; 
    uint8_t Unused1[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t Unused2[4];
    uint16_t DIR_FirstCLusterLow;
    uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[16]; // Since we can only have 16 of these represented by the fat


int main()
{
  // Variables used in the program 
  int is_the_file_open= 0; // 1 means file is open
  FILE *fptr;
  
  // This are for bios parameter block (BPB)
  int16_t BPB_BytesPerSec; // Count of bytes per sector
  int8_t BPB_SecPerClus; // No. of sectors per allocation 
  int8_t BPB_NumFATs; // Count of FAT data strucutre in volume 
  int32_t BPB_FATz32;  // FAT32 32-bit count of sectors occupied by One FAT
  int16_t BPB_RsvdSecCnt; // Number of reserved sectors in the Reserved region of the volume 

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your FAT32 functionality

    int token_index  = 0;
    for( token_index = 0; token_index < token_count; token_index ++ ) 
    {
      printf("token[%d] = %s\n", token_index, token[token_index] );  
    }

    free( working_root );

    // At this point token[0] and token[1] and so on has the details about the user input 

    // If the user enters exit or quit then we quit the program 
    if ((strcmp(token[0],"quit") == 0) || (strcmp(token[0],"exit") == 0)){
      return 0;
    }
    // open <File type> 
    else if (strcmp(token[0],"open")==0){
      if (is_the_file_open == 0){
          fptr = fopen(token[1],"r"); // Openig the file to read, 1 means file exists 
          if(fptr == NULL){
            printf("Error: File system not found. \n");
          }
          else{
            is_the_file_open = 1; // 1 means true
          }
      } 
      else{
        printf("Error: File system image already open. \n");
      }
    }
    
    else if (strcmp(token[0],"close")==0){
      if(is_the_file_open == 0){
        printf("Error: File system not open. \n");
      }
      is_the_file_open = 0; // Changing this as the file is already close
      fclose(fptr);
    }
    
    else if (strcmp(token[0],"bpb")==0){
      if (is_the_file_open == 0){
        printf("Error: File system not open \n");
      }
      else{
        // The details are taken from the hardware white paper 
        fseek(fptr,11,SEEK_SET);
        fread((&BPB_BytesPerSec),2,1,fptr);
        fseek(fptr,13,SEEK_SET);
        fread((&BPB_SecPerClus),1,1,fptr);
        fseek(fptr,16,SEEK_SET);
        fread((&BPB_NumFATs),1,1,fptr);
        fseek(fptr,36,SEEK_SET);
        fread((&BPB_FATz32),4,1,fptr);
        fseek(fptr,14,SEEK_SET);
        fread((&BPB_RsvdSecCnt),2,1,fptr);

        // Printing out the details [int and hexadecimal values]
        printf("Bytes per sector (BPB_BytesPerSec) : %d %x \n",BPB_BytesPerSec, BPB_BytesPerSec);
        printf("No of sectors per allocation (BPB_SecPerClus) : %d %x \n",BPB_SecPerClus, BPB_SecPerClus);
        printf("Count of FAT data structure on the volume (BPB_NumFATs) : %d %x \n",BPB_NumFATs, BPB_NumFATs);
        printf("Bit-count of sectors occupied by one fat (BPB_FATz32) : %d %x \n",BPB_FATz32, BPB_FATz32);
        printf("Number of reserved sectors in the section (BPB_RsvdSecCnt): %d %x \n",BPB_RsvdSecCnt, BPB_RsvdSecCnt);
      }
    }

    else if (strcmp(token[0],"stat")==0){
      // Filaname or directory name is token[1]
      if(is_the_file_open == 0){
        printf("Error: File system not found \n");
      }else{

      }
    }
  } 
  return 0;
}
