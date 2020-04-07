#include <iostream>
#include <string>
#include <unordered_map>
#include <stdlib.h>
#include <stdio.h>
#include "cb.h"

using namespace std;

void
print_reducer_op_for_user (string userid, unordered_map<string, int> map)
{
  unordered_map<string, int>:: iterator itr;

  for (itr = map.begin(); itr != map.end(); itr++) {
      cout << "(" << userid << "," << itr->first << "," << itr->second << ")" << endl;
  }
  cout << endl;
}

void
reducer_main_util (buffer_array_t *buf_arr, buffer_t *buf)
{

  int i = 0;
  record_t record;
  string user_id;
  string topic;
  int total_score = 0;
  unordered_map <string, int> map;

  unordered_map <string, int> :: iterator itr;
  while (true) {
 
    pthread_mutex_lock(&buf->mutex);

    while (is_buffer_empty(buf)) {
    
      pthread_cond_wait(&buf->read, &buf->mutex);
    }

    dequeue(buf, &record);
    if (record.is_dummy) {
   
      break;
    }

    pthread_cond_signal(&buf->write);

    pthread_mutex_unlock(&buf->mutex);

    user_id = string(record.user_id);
    topic = string(record.topic);
    itr = map.find(topic);
    if (itr == map.end()) {
    
      map.insert(make_pair(topic, record.score));
    } else {
    
      total_score = itr->second + record.score;
      map.erase(topic);
      map.insert(make_pair(topic, total_score));
    }
  }
  
  pthread_mutex_lock(&buf_arr->super_mutex);

  print_reducer_op_for_user(user_id, map);

  pthread_mutex_unlock(&buf_arr->super_mutex);

  return;
}

void
reducer_main (buffer_array_t *buf_arr, int user)
{

  buffer_t *buf = NULL;

  buf = get_user_buffer(buf_arr, user);
  
  reducer_main_util(buf_arr, buf);
  
  return;
}
