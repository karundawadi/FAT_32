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

#define MAX_NUM_ARGUMENTS 3

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

struct __attribute__((__packed__)) DirectoryEntry
{
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};
struct DirectoryEntry dir[16];

int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  int val =1;
  int num =3;
  int hold =5;
  int foo=7;
  FILE* fp;

  int16_t BPB_BytesPerSec;
  int8_t BPB_SecPerClus;
  int16_t BPB_RsvdSecCnt;
  int8_t BPB_NumFATS;
  int32_t BPB_FATSz32;

  while( 1 )
  {
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    if(cmd_str[0]=='\n')
    {
      continue;
    }

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

    int LBAToOffset(int32_t sector)
    {
      return ((sector-2) * BPB_BytesPerSec) + (BPB_BytesPerSec * BPB_RsvdSecCnt) + (BPB_NumFATS * BPB_FATSz32 * BPB_BytesPerSec);
    }


  //return value of compare?
  int compare_Name( char * input, char * IMG_Name )
  {
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



 
    // File Open and read value
    if(strcmp(token[0],"open")==0 && val ==1 )
    {
      // printf("token 1:%s\n",token[1]);
      fp = fopen(token[1],"r");

      if(fp == NULL)
      {
        printf("Error. File system image not found\n");
        // continue; // Removing this part, as this is not needed 
      }
      //if file is not null
      else
      {
        val = 2;
        hold++;
        num=7;
        foo++;

        // BPB bytes per sec is read ; others follow similar patters
        fseek(fp,11,SEEK_SET);
        fread(&BPB_BytesPerSec,2,1,fp);


        fseek(fp, 13,SEEK_SET);
        fread(&BPB_SecPerClus,1,1,fp);

        fseek(fp, 14, SEEK_SET);
        fread(&BPB_RsvdSecCnt,2,1,fp);

        fseek(fp, 16, SEEK_SET);
        fread(&BPB_NumFATS,1,1,fp);

        fseek(fp, 36, SEEK_SET);
        fread(&BPB_FATSz32,4,1,fp);

        fseek(fp, 0x100400, SEEK_SET);
        fread(dir, 16, sizeof( struct DirectoryEntry), fp);
      }

    }
    //if file is already open
    else if(strcmp(token[0],"open")==0 && val == 2)
    {
      printf("Error. File system is already open\n");
    }
    //close file
    else if(strcmp(token[0],"close")==0)
    {
      if(foo == 7)
      {
        printf("Error. File system not open\n");
      }
      else
      {
        fclose(fp);
        num = 4;
        val =1;
        foo =7;
      }

    }
    else if(strcmp(token[0],"exit")==0 || strcmp(token[0],"quit")==0)
    {
      printf("Exiting.....\n");
      exit(0);
    }
    //if file is already open
    else
    {
      //file system is closed
      if(num==4)
      {
        printf("Error. File system image must be opened first\n");
      }
      else if(strcmp(token[0],"bpb")==0)
      {
	      printf("BPB_BytesPerSec: %d\n",BPB_BytesPerSec);
	      printf("BPB_SecPerClus: %d\n",BPB_SecPerClus);
 	      printf("BPB_RsvdSecCnt: %d\n", BPB_RsvdSecCnt);
        printf("BPB_NumFATS: %d\n", BPB_NumFATS);
        printf("BPB_RsvdSecCnt: %d\n", BPB_FATSz32);
      }
      else if(strcmp(token[0],"stat")==0)
      {
        //file is open
        if(val==2)
        {
          fseek(fp, 0x100400, SEEK_SET);
          fread(dir, 16, sizeof( struct DirectoryEntry), fp);
          int i;
          int track=2;
          for (i=0; i<16; i++)
          {
            // this line is comparing foo.txt with "FOO     TXT"
            if(compare_Name( token[1], dir[i].DIR_Name))
            {
              printf("inside\n");
              printf("Starting cluster number: %d \n",dir[i].DIR_FirstClusterLow);
              printf("File name :%s\n", dir[i].DIR_Name);
              track = 3;
            }
          }
          if(track == 2)
          {
             printf("Error. File not found\n");
          }
        }
        //file is not open
        else
        {
          printf(" Error. File needs to be opened first");
        }

      }
      //how to check if cd is working ?
      //segfaulting when .. is entered
      //does not segfault when cd foo.txt is entered
      else if (strcmp(token[0],"cd")==0)
      {
        int track=2;
        uint16_t ClusterLow;

        //file is open
        if(val ==2)
        {
          int i;

          for (i=0; i<16; i++)
          {
            //compares input with directory name
            //this gets executed, when cd foo.txt is entered
            if(compare_Name( token[1], dir[i].DIR_Name))
            //if(strcmp(token[1],dir[i].DIR_Name)==0)
            {
              printf("inside\n");
              ClusterLow = dir[i].DIR_FirstClusterLow;
              fseek(fp, LBAToOffset( ClusterLow ), SEEK_SET);
              fread(dir, 16, sizeof( struct DirectoryEntry), fp);
              track = 3;
              break;
            }
          }
#if 0
          if(track == 2)
          {
             printf("Error. File not found\n");
          }
          else
          {
            if( ClusterLow == 0 ) ClusterLow = 2;
            int offset = LBAToOffset(ClusterLow);
            fseek(fp, offset, SEEK_SET);
            fread(dir, 16, sizeof(struct DirectoryEntry), fp);
          }
#endif
        }
        else if(strcmp(token[0],"ls")==0){
              int ko =0;
              for(ko=0;ko<16;ko++){
                fread(dir, 16, sizeof( struct DirectoryEntry), fp);
                int z = 0;
                for(z=0;z<16;z++){
                  printf("%c \n",dir[ko].DIR_Name);
                }
              }
        }
        //file is not open
        else
        {
          printf("Error. File needs to be opened first\n");
        }

      }

    }

    // free( working_root );

  }
  return 0;
}
