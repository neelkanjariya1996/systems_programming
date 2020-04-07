#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

void
combiner (int argc, char *argv[])
{
  int ret = 0;        /* ret value */
  int rpid = 0;       /* reducer */
  int mpid = 0;       /* mapper */
  int pfds[2] = {0};  /* pipe fds */
  int status = 0;

  /*
   * Initialize the pipe
   */
  if (pipe(pfds) < 0) {
    cout << "Could not create pipe errno: " << errno << endl;
    goto error;
  }

  /*
   * fork to create the reducer process
   */
  rpid = fork();
  if (rpid < 0) {
    cout << "Could not fork reducer errno " << errno << endl;
    goto error;
  }

  if (rpid == 0) {

    /*
     * this will execute for reducer process
     */

    /*
     * dup2 stdin to pipe read fd
     */
    ret = dup2(pfds[0], STDIN_FILENO);
    if (ret < 0) {
      cout << "Could not dup stdin to pipe read fd errno " << errno << endl;
      goto error;
    }

    close(pfds[1]);

    /*
     * exec to the reducer
     */
    ret = execl("./reducer", "./reducer", NULL);
    if (ret < 0) {
      cout << "Could not exec reducer errno " << errno << endl;
      goto error;
    }
  }

  if (rpid > 0) {

    /*
     * this will be excuted for combiner process
     * at this point we should have spawned the reducer and dup2 its stdin to pipe read fd
     * now fork and create the mapper
     */
    mpid = fork();
    if (mpid < 0) {
      cout << "Could not fork mapper" << errno << endl;
      goto error;
    }

    if (mpid == 0) {

      /*
       * this will execute in mapper process
       */

      /*
       * dup stdout to pipe write fd
       */
      ret = dup2(pfds[1], STDOUT_FILENO);
      if (ret < 0 ) {
        cout << "Could not dup stdout to pipe write fd errno " << errno << endl;
        goto error;
      }

      close(pfds[0]);

      /*
       * exec to the mapper
       */
      ret = execl("./mapper", "./mapper", argv[1], NULL);
      if (ret < 0) {
        cout << "Could not exec mapper errno " << errno << endl;
        goto error;
      }
    }

    if (mpid > 0) {

      /*
       * this will be executed in combiner process
       */
      close(pfds[0]);
      close(pfds[1]);
      wait(NULL);
    }
  }

  wait(NULL);
error:
  close(pfds[0]);
  close(pfds[1]);
  return;
}

/*
 * driver program for combiner
 */
int
main (int argc, char *argv[])
{
  if (argc != 2) {
    cout << "Invalid arguments, please see readme for instructions" << endl;
  }

  combiner(argc, argv);
  return (0);
}
