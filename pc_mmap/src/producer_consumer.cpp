#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "cb.h"
#include "mapper.h"
#include "reducer.h"

#define NUM_OF_CLI  4

using namespace std;

int num_records = 0;
int num_of_users = 0;
char file_name[15];

int
main (int argc, char **argv)
{

  buffer_array_t *buf_arr = NULL;
  int mpid = 0;
  int rpid = 0;
  int i = 0;
  pid_t child_pid;

  if (argc != NUM_OF_CLI) {

    cout << "ERR: Incorrect number of Command line arguments" << endl;
    cout << "Refer to README for correct input" << endl;
    return -1;
  }

  num_records = stoi(argv[1]);
  num_of_users = stoi(argv[2]);
  strcpy(file_name, argv[3]);

  buf_arr = buffer_array_init();
  if (!buf_arr) {
  
    return -1;
  }
  
  /*
   * creating mapper
   */
  mpid = fork();
  switch (mpid) {
  case -1:
  
      cout << "ERR: could not create mapper, errnp: " << errno << endl;
      break;
  case 0:
    /*
     * inside mapper
     */
    mapper_main(buf_arr);
    break;
  default:

    for (i = 0; i < num_of_users; i++) {
    
      rpid = fork();
      if (rpid < 0) {
      
        cout << "ERR: could not reducer " << i << " errnp: " << errno << endl;
        return -1;
      }
      if (rpid == 0) {
    
        reducer_main(buf_arr, i);
        break;
      }
    }
    break;
  }

  for (;;) {
 
    child_pid = wait(NULL);
    if (child_pid == -1) {
    
      if (errno == ECHILD) {
      
        exit(EXIT_SUCCESS);
      } else {
      
        cout << "ERR: wait" << endl;
        exit(-1);
      }
    }
  }

  return 0;
}
