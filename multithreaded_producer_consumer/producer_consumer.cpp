#include <iostream>
#include <string>
#include <pthread.h>
#include <fstream>
#include <unordered_map>
#include <queue>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <climits>

using namespace std;

#define NUM_OF_CLI  4     // Number of command line inputs
#define SUCCESS 0

/*
 * is_dummy: used to send dummy tuple to each 
 *           communication buffer at the end 
 *           of input tuples.
 * user_id: 4 characters user_id
 * topic: 15 characters topic
 * score: score assigned for a particular action
 */
typedef struct reducer_tuple_t_ {

  bool is_dummy;
  string user_id;
  string topic;
  int score;

} reducer_tuple_t;

/*
 * read: condition variable to send signal to reducer thread
 * write: condition variable to send signal to mapper thread
 * mutex: a mutex to lock the buffer (either by mapper or reducder thread)
 * q: a queue of type reducer_tuple_t used as a buffer 
 */
typedef struct buf_t_ {

  pthread_cond_t read;
  pthread_cond_t write;
  pthread_mutex_t mutex;
  queue<reducer_tuple_t> q;

} buf_t;

/*
 * user_mutex: a mutex to allow only one reducer thread 
 *             to print output tuples at a time
 */
pthread_mutex_t user_mutex;

/*
 * buffer_size: size of the communication buffer
 */
int     buffer_size;

/*
 * num_of_consumer_threads: number of consumer threads to be
 *                          spawned (each per individual user)
 */
int     num_of_consumer_threads;

/*
 * file_name: input file name
 */
string  file_name;

/*
 * input: a string to store the tuples from the input file
 */
string  input;

/*
 * user_id_to_thread_id_map: an unordered map to store 
 *                           the mapping of user_id to thread_id
 */
unordered_map<string, long> user_id_to_thread_id_map;

/*
 * thread_id_to_buffer_map: an unordered map to store the 
 *                          mapping of thread_id to buffer
 */
unordered_map<long, buf_t> thread_id_to_buffer_map;


/***************************
 *    UTILITY FUNCTIONS    *
 ***************************/


/*
 * Function to initialize the data structure 
 */
void
ds_init ()
{

  long i = 0;

  pthread_mutex_init(&user_mutex, NULL);
  for (i = 0; i < num_of_consumer_threads; i++) {
  
    buf_t buf;
    pthread_cond_init(&buf.read, NULL);
    pthread_cond_init(&buf.write, NULL);
    pthread_mutex_init(&buf.mutex, NULL);
    thread_id_to_buffer_map.insert(make_pair(i, buf));
  }

  return;
}

/*
 * Function to create the input string
 * which stores the tuples from input file
 * @ip: input file name
 */
void
create_input_string (string input_file)
{

  string line;

  ifstream myfile (input_file);

  if (!myfile.is_open ()) {
      cout << "Err: opening input file\n";
      return;
  }

  while (getline (myfile, line)) {
      input.append (line);
  }

  myfile.close();

  return;
}


/****************
 *    OUTPUT    *
 ****************/


void
print_reducer_op_for_user (string userid, unordered_map<string, int> map)
{
  unordered_map<string, int>:: iterator itr;

  for (itr = map.begin(); itr != map.end(); itr++) {
      cout << "(" << userid << "," << itr->first << "," << itr->second << ")" << endl;
  }
  cout << endl;

  return;
}


/****************
 *    MAPPER    *
 ****************/


/*
 * Function to process the input tuples from input file
 * It is called by mapper_main thread
 */
void
mapper_main_util ()
{

  int   len = 0;                    
  long  buffer_no = 0;              
  long  count = 0;

  len = input.length();
  for (int i = 0; i < len - 1; i++) {

    string userID = "";             // userID from the tuple
    char action;                    // action from the tuple
    string topic = "";              // topic from the tuple
    reducer_tuple_t tuple = {0};    // a tuple to store the processed i/p tuple

    /*
     * userID length is 4
     */
    userID = input.substr(i + 1, 4);
    i = i + 5;                        // incremented by 5 to skip ","
    tuple.user_id   = userID;
    
    /*
     * action lenght is 1
     */
    action = input.at(i + 1);
    i = i + 2;                        // incremented by 2 to skip ","
    /*
     * action == P  --> score = 50
     * action == L  --> score = 20
     * action == D  --> score = -10
     * action == C  --> score = 30
     * action == S  --> score = 40
     */
    if (action == 'P') {

      tuple.score = 50;
    } else if (action == 'L') {

      tuple.score = 20;
    } else if (action == 'D') {

      tuple.score = -10;
    } else if (action == 'C') {

      tuple.score = 30;
    } else if (action == 'S') {

      tuple.score = 40;
    } else {
      
      continue;
    }

    /*
     * topic lenght is 15
     */
    topic = input.substr (i + 1, 15);
    i = i + 17;                       // incremented by 17 to skip "),"
    tuple.topic = topic;
 
    /*
     * itr1: to iterate over user_id_to_thread_id_map
     */
    unordered_map<string, long> :: iterator itr1;
    /*
     * itr2: to iterate over thread_id_to_buffer_map
     */
    unordered_map<long, buf_t> :: iterator itr2;

    itr1 = user_id_to_thread_id_map.find(tuple.user_id);
    if (itr1 == user_id_to_thread_id_map.end()) {
    
      /*
       * if a new user_id is found than the next available buffer 
       * is assigned to it and the mapping of user_id to buffer_no
       * is inserted into the user_id_to_thread_id_map map
       */
      buffer_no = count;
      count++;
      user_id_to_thread_id_map.insert(make_pair(tuple.user_id, buffer_no));
    } else {

      /*
       * Not a new user id so finding the previously assigned buffer_no 
       * to it from user_id_to_thread_id_map map
       */
      buffer_no = itr1->second;
    }

    /*
     * getting the buffer_no from thread_id_to_buffer_map map
     */
    itr2 = thread_id_to_buffer_map.find(buffer_no);
    
    /*
     * acquiring the mutex to write to the buffer
     */
    pthread_mutex_lock (&itr2->second.mutex);

    /*
     * if the buffer is full than wait for reducer to read a slot and 
     * make one slot available
     */
    while (itr2->second.q.size() >= buffer_size) {
    
      pthread_cond_wait(&itr2->second.write, &itr2->second.mutex);
    }

    /*
     * push the tuple into the buffer
     */
    itr2->second.q.push(tuple);
    
    /*
     * signal the reducer thread that there is a tuple avaiable 
     * in the buffer for it to read
     */
    pthread_cond_signal(&itr2->second.read);

    /*
     * release the mutex for reducer thread to use the buffer
     */
    pthread_mutex_unlock(&itr2->second.mutex);
  }

  /*
   * create a dummy tuple to send to each buffer to signal 
   * no more tuples will be generated by the mapper thread
   */
  reducer_tuple_t tuple;
  tuple.is_dummy = true;
  tuple.user_id = "####";
  tuple.topic = "###############";
  tuple.score = INT_MIN;
  
  /*
   * push the dummy tuple in each buffer
   */
  unordered_map<long, buf_t> :: iterator itr;
  for (itr = thread_id_to_buffer_map.begin();
       itr != thread_id_to_buffer_map.end();
       itr++) {
    
    /*
     * acquiring the mutex to write to the buffer
     */
    pthread_mutex_lock (&itr->second.mutex);
    
    /*
     * if the buffer is full than wait for reducer to read a slot and 
     * make one slot available
     */
    while (itr->second.q.size() >= buffer_size) {
    
      pthread_cond_wait(&itr->second.write, &itr->second.mutex);
    }
    
    /*
     * push the tuple into the buffer
     */
    itr->second.q.push(tuple);
    
    /*
     * signal the reducer thread that there is a tuple avaiable 
     * in the buffer for it to read
     */
    pthread_cond_signal(&itr->second.read);
    
    /*
     * release the mutex for reducer thread to use the buffer
     */
    pthread_mutex_unlock(&itr->second.mutex);
  }

  return;
}

/*
 * main mapper function to spawn the mapper thread
 */
void*
mapper_main (void* thread_id)
{

  long tid = 0;
  tid = (long) thread_id;

  mapper_main_util();

  pthread_exit((void*) 0);
}


/****************
 *    REDUCER   *
 ****************/


/*
 * function to process the tuples produced 
 * by mapper thread
 */
void
reducer_main_util (long thread_id)
{

  string                      user_id;
  reducer_tuple_t             tuple;

  /*
   * a map to store the topic and total score
   */
  unordered_map<string, int>  map; 

  /*
   * itr: a iterator to iterate over the thread_id_to_buffer_map map
   */
  unordered_map<long, buf_t> :: iterator itr;

  itr = thread_id_to_buffer_map.find(thread_id);
  while (true) {
  
    /*
     * acquire the mutex to read from the buffer
     */
    pthread_mutex_lock(&itr->second.mutex);
    
    /*
     * wait if the buffer is empty
     */
    while (itr->second.q.empty()) {
    
      pthread_cond_wait(&itr->second.read, &itr->second.mutex);
    }

    /*
     * remove a tuple from the buffer
     */
    tuple = itr->second.q.front();
    itr->second.q.pop();
    
    /*
     * check if thee current tuple is last tuple 
     * or not
     */
    if (tuple.is_dummy) {
    
      break;
    } else {
    
      user_id = tuple.user_id;
    }

    /*
     * signal the mapper thread that a slot is available
     * for it to write to the buffer
     */
    pthread_cond_signal(&itr->second.write);

    /*
     * release the mutex so that the mapper thread can use 
     * to write to the buffer
     */
    pthread_mutex_unlock(&itr->second.mutex);

    /*
     * insert the topic and score in the map
     */
    unordered_map<string, int> :: iterator got; 
    got= map.find(tuple.topic);
    if (got == map.end()) {
      map.insert(make_pair(tuple.topic, tuple.score));
    } else {
      int total_score = got->second + tuple.score;
      map.erase(tuple.topic);
      map.insert(make_pair(tuple.topic, total_score));
    }
  }

  /*
   * acquire the mutex to print output tuple
   * for the user_id associated with the thread_id
   */
  pthread_mutex_lock(&user_mutex);

  /*
   * print the output tuples for the 
   * user_id
   */
  print_reducer_op_for_user(user_id, map);

  /*
   * release the mutex so that other threads 
   * can print the output tuples
   */
  pthread_mutex_unlock(&user_mutex);

  return;
}

/*
 * main reducer function to spawn
 * num_of_consumer_threads 
 */
void*
reducer_main (void* thread_id)
{

  long tid = 0;
  tid = (long) thread_id;

  reducer_main_util(tid);

  pthread_exit((void*) 0);
}


/**************************
 * Driver function (main) *
 **************************/


int
main (int argc, char ** argv)
{

  long  t = 0;
  int   rc = 0;
  void* status = NULL;

  if (argc != NUM_OF_CLI) {
  
    cout << "ERR: Incorrect number of Command line arguments" << endl;
    cout << "Refer to README for correct input" << endl;
    return -1;
  }

  /*
   * store the command line inputs
   */
  buffer_size = stoi(argv[1]);
  num_of_consumer_threads = stoi(argv[2]);
  file_name = argv[3];

  if (buffer_size == 0) {
  
    cout << "ERR: buffer size has to be atleast 1" << endl;
    return -1;
  }

  /*
   * create the input string
   */
  create_input_string (file_name);
  
  /*
   * initialize the data structures
   */
  ds_init();
 
  /*
   * mapper thread
   */
  pthread_t mapper_thread;
  
  /*
   * consumer threads
   */
  pthread_t consumer_threads[num_of_consumer_threads];

  /*
   * declaring the attribute 
   */
  pthread_attr_t attr;
  rc = pthread_attr_init(&attr);
  if (rc != SUCCESS) {
  
    cout << "ERR: initializing attribute with err_no: "
         << rc << endl;
    return -1;
  }

  /*
   * set the attribute joinable
   */
  rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  if (rc != SUCCESS) {
  
    cout << "ERR: setting attribute to joinable with err_no: " 
         << rc << endl;
    return -1;
  }

  /*
   * creating the mapper thread
   */
  rc = pthread_create(&mapper_thread, &attr, 
                      mapper_main, (void *)t);
  if (rc != SUCCESS) {
  
    cout << "ERR: creating mapper thread with err_no: " 
         << rc << endl;
    return -1;
  }
  
  /*
   * creating the consumer threads
   */
  for (t = 0; t < num_of_consumer_threads; t++) {
  
    rc = pthread_create(&consumer_threads[t], &attr, 
                        reducer_main, (void *)t);
    if (rc != SUCCESS) {
    
      cout << "ERR: creating reducer: " << t << " with err_no: " 
           << rc << endl;
      return -1;
    }
  }

  /*
   * destroying the attribute
   */
  rc = pthread_attr_destroy(&attr);
  if (rc != SUCCESS) {
  
    cout << "ERR: destroying attribute with err_no: " 
         << rc << endl;
    return -1;
  }

  /*
   * joining the mapper thread to main
   */
  rc = pthread_join (mapper_thread, &status);
  if (rc != SUCCESS) {
  
    cout << "ERR: joining mapper thread to main with err_no: " 
         << rc << endl;
    return -1;
  }

  /*
   * joining the consumer threads to main
   */
  for (t = 0; t < num_of_consumer_threads; t++) {
  
    rc = pthread_join(consumer_threads[t], &status);
    if (rc != SUCCESS) {
    
      cout << "ERR: joining reducer thread: " << t 
           << "to main with err_no: " << rc << endl;
      return -1;
    }
  }

  pthread_exit(NULL);
  return 0;
}
