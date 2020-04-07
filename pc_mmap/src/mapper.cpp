#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <climits>
#include "cb.h"

using namespace std;

string input;

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

void
mapper_main_util (buffer_array_t *buf_arr)
{

  int len = 0;
  int buffer_no = 0;
  int count = 0;
  int i = 0;
  buffer_t *buf = NULL;
  record_t dummy_record = {0};
  unordered_map <string, int> user_to_buffer_map;

  unordered_map <string, int> :: iterator itr;
  len = input.length();
  for (i = 0; i < len - 1; i++) {

    string user_id;                 // userID from the tuple
    char action;                    // action from the tuple
    string topic;                   // topic from the tuple
    record_t tuple = {0};           // a tuple to store the processed i/p tuple

    /*
     * input tuples are not dummy
     */
    tuple.is_dummy = false;

    /*
     * user_id length is 4
     */
    user_id = input.substr(i + 1, 4);
    i = i + 5;                        // incremented by 5 to skip ","
    strncpy(tuple.user_id, user_id.c_str(), 4);
    tuple.user_id[4] = '\0';

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
    strncpy(tuple.topic, topic.c_str(), 15);
    tuple.topic[15] = '\0';

    itr = user_to_buffer_map.find(user_id);
    if (itr == user_to_buffer_map.end()) {
    
      buffer_no = count;
      count++;
      user_to_buffer_map.insert(make_pair(user_id, buffer_no));
    } else {
    
      buffer_no = itr->second;
    }
    
    buf = get_user_buffer(buf_arr, buffer_no);

    pthread_mutex_lock (&buf->mutex);

    while (is_buffer_full(buf)) {

      pthread_cond_wait(&buf->write, &buf->mutex);
    }

    enqueue(buf, tuple);

    pthread_cond_signal(&buf->read);

    pthread_mutex_unlock(&buf->mutex);
  }
  
  /*
   * enqueuing dummy record to each buffer
   */
  dummy_record.is_dummy = true;
  strncpy(dummy_record.user_id, "####", 4);
  strncpy(dummy_record.topic, "###############", 15);
  dummy_record.score = INT_MIN;
  
  for (i = 0; i < num_of_users; i++) {
  
    buf = get_user_buffer(buf_arr, i);
    
    pthread_mutex_lock (&buf->mutex);

    while (is_buffer_full(buf)) {

      pthread_cond_wait(&buf->write, &buf->mutex);
    }

    enqueue(buf, dummy_record);

    pthread_cond_signal(&buf->read);

    pthread_mutex_unlock(&buf->mutex);
  }
}

void
mapper_main (buffer_array_t* buf_arr)
{
 
  create_input_string(file_name);
  mapper_main_util(buf_arr);
}
