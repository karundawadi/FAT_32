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
#include <string.h>
#include <ctype.h>

#define MAX_NUM_ARGUMENTS 4

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

// Packed makes sure to size layout how it looks
struct __attribute__((__packed__)) DirectoryEntry{
    char DIR_NAME[11]; // The name of the directory {Specs on the whitepaper document}
    uint8_t DIR_Attr; // The file attribute {Specs also on the sheet}
    uint8_t Unused1[8]; // Skipped from DIR-NTRes to DIR_LastAccDate
    uint16_t DIR_FirstClusterHigh; // High word of this entry's first cluster 
    uint8_t Unused2[4]; // FstClusHI ignored
    uint16_t DIR_FirstCLusterLow; // Low word of the entry's first cluster member
    uint32_t DIR_FileSize; // 32-bit word holding the file size in bytes
};

struct DirectoryEntry dir[16]; // Since we can only have 16 of these represented by the fat

// Functions defined here 

// From professor's github code, compares FOO TXT and foo.txt in short
int compare_Name( char * input2, char * IMG_Name )
  {
    char input[12];
    memset( input, 0, 12 );
    strncpy( input ,input2, 11 );
    //printf("comparing %s %s\n", input, IMG_Name );
    if( strncmp( input, "..", 2 ) == 0)
    {
      if( strncmp( input, IMG_Name, 2 ) == 0)
      {
        return 1;
      }
      else
      {
        return 0;
      }
    }
    char expanded_name[12];
    memset( expanded_name, ' ', 12 );
    char *token = strtok( input, "." );
    strncpy( expanded_name, token, strlen( token ) );
    token = strtok( NULL, "." );
    if( token )
    {
      strncpy( (char*)(expanded_name+8), token, strlen(token ) );
    }
    expanded_name[11] = '\0';
    int i;
    for( i = 0; i < 11; i++ )
    {
      expanded_name[i] = toupper( expanded_name[i] );
    }
    if( strncmp( expanded_name, IMG_Name, 11 ) == 0 )
    {
      return 1;
    }
    return 0;
  }

// Finds the starting address of a block of data given the sector number 
int LABToOffset(int32_t sector, int16_t BPB_BytesPerSec, int16_t BPB_RsvdSecCnt, int8_t BPB_NumFATs, int32_t BPB_FATz32){
  return ((sector-2)* BPB_BytesPerSec) + (BPB_BytesPerSec* BPB_RsvdSecCnt)+(BPB_NumFATs*BPB_FATz32*BPB_BytesPerSec);
}

int16_t NextLB(uint32_t sector, FILE *fptr, int16_t BPB_BytesPerSec, int16_t BPB_RsvdSecCnt, int8_t BPB_NumFATs, int32_t BPB_FATz32){
  uint32_t FATAddress = (BPB_BytesPerSec * BPB_RsvdSecCnt) + (sector * 4);
  int16_t val;
  fseek(fptr,FATAddress,SEEK_SET);
  fread(&val,2,1,fptr);
  return val;
}

int main()
{
  // Variables used in the program 
  int is_the_file_open= 0; // 1 means file is open
  FILE *fptr;
  int offset_of_current_working_dir = 0;
  
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
    int token_index  = 0;
    // Now print the tokenized input as a debug check
     for( token_index = 0; token_index < token_count; token_index ++ ) 
    {
      printf("token[%d] = %s\n", token_index, token[token_index] );  
    }

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

            // Reading details about the file system 
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

            // For the clusters 
            int reserved_sector = BPB_RsvdSecCnt * BPB_BytesPerSec;
            int totalFatSize = BPB_NumFATs * BPB_FATz32 * BPB_BytesPerSec;
            int cluster_starts_at = reserved_sector + totalFatSize;
            
            // Reading to the DIR array 
            fseek(fptr,cluster_starts_at,SEEK_SET);
            fread(dir, 16, sizeof(struct DirectoryEntry), fptr);
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
        // Printing out details with caution 
        int i = 0;
        for (i = 0;i<16;i++){
          if(compare_Name(token[1],dir[i].DIR_NAME)){
            // In case it is a directory 
            if(dir[i].DIR_FileSize == 0){
              // The entered value if a directory
              printf("The entered value : %s is a directory. \n", token[1]);
              printf("The cluster number is %d and the arributed are %d \n", dir[i].DIR_FirstCLusterLow, dir[i].DIR_Attr); 
              // The above statement means that data starts at first cluster low
            }else{
              printf("The entered value : %s is a file. \n", token[1]);
              printf("The cluster number is %d and the arributed are %d \n", dir[i].DIR_FirstCLusterLow, dir[i].DIR_Attr); 
            }
          }
          else{
            printf("Error : File not found \n");
          }
        }
      }
    }

    else if(strcmp(token[0],"ls")==0){
      int file_found = 0;
      if(is_the_file_open == 0){
        printf("Error: File system image must be opened first. \n");
      }else{
            for (int i = 0; i<16;i++){
               if((dir[i].DIR_Attr == 0x001||(dir[i].DIR_Attr == 0x10)||(dir[i].DIR_Attr == 0x20)) && 
               ((dir[i].DIR_NAME[0] != '.') && (dir[i].DIR_NAME[0] != -27))){ // From pg 24
                char input[12];
                memset( input, ' ', 12 );
                strncpy( input ,dir[i].DIR_NAME, 11 );
                input[11] ='\0'; //Inserting a null terminator
                file_found = 1;
                printf("%s \n",input);
              }
            }
            if(file_found == 0){
              printf("File not found \n");
            }
      }
    }

    else if(strcmp(token[0],"cd")==0){
      if(is_the_file_open == 0){
        printf("Error: File system image must be opened first. \n");
      }else{
        uint16_t ClusterLow;
        int i;
          for (i=0; i<16; i++)
          {
            //compares input with directory name
            //this gets executed, when cd foo.txt is entered
            if(compare_Name( token[1], dir[i].DIR_NAME))
            {
              ClusterLow = dir[i].DIR_FirstCLusterLow;
              if( ClusterLow == 0 ) {
                ClusterLow = 2;
                printf("You are in root directory can not go further back. \n");
              }
              fseek(fptr, LABToOffset( ClusterLow,BPB_BytesPerSec,BPB_RsvdSecCnt,BPB_NumFATs,BPB_FATz32), SEEK_SET);
              fread(dir, 16, sizeof( struct DirectoryEntry), fptr);
              break;
            } 
          }
      }
    }
    
    else if(strcmp(token[0],"read") == 0){
      if(is_the_file_open == 0){
        printf("Error: File system image must be opened first. \n");
      }else{
        char fileName[100];
        int position; 
        int number_of_bytes_from_position;
        int found_position;
        strcpy(fileName,token[1]);
        position = atoi(token[2]);
        number_of_bytes_from_position = atoi(token[3]);
        char characters[number_of_bytes_from_position]; // Is an unsigned int so we can print hex value
        for(int i = 0;i<16;i++){
          if(compare_Name(fileName,dir[i].DIR_NAME)){
            // This means we got the match in which we need to go number_of_bytes_from_position
            printf("%s \n",dir[i].DIR_NAME);
            int actual_position_to_print_from = dir[i].DIR_FirstCLusterLow;
            found_position = LABToOffset(actual_position_to_print_from,BPB_BytesPerSec,BPB_RsvdSecCnt,BPB_NumFATs,BPB_FATz32);
            fseek(fptr,found_position + position,SEEK_SET);
            fread(&characters,number_of_bytes_from_position,1,fptr);
            for(int k = 0; k<number_of_bytes_from_position;k++){
              printf("%c",characters[k]);
            }
            printf("\n");
          }
        }
      }
     }

    free( working_root );
  } 
  return 0;
}
