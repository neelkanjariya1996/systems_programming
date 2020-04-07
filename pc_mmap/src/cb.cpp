#include <iostream>
#include <bits/stdc++.h>
#include <errno.h>
#include <sys/mman.h>
#include <cstring>
#include <pthread.h>
#include "cb.h"

using namespace std;

static void
buffer_init (buffer_t *buf)
{

  pthread_mutexattr_t attrmutex;
  pthread_condattr_t attrcond;

  buf->front = -1;
  buf->rear = -1;
  buf->size = num_records;
  buf->num_of_elements = 0;

  pthread_condattr_init(&attrcond);
  pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&buf->read, &attrcond);
  pthread_cond_init(&buf->write, &attrcond);
  
  pthread_mutexattr_init(&attrmutex);
  pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&buf->mutex, &attrmutex);
}

buffer_array_t*
buffer_array_init ()
{

  int single_buf_size = 0;
  int buf_arr_size = 0;
  buffer_array_t *buf_arr = NULL;
  buffer_t *buf = NULL;
  pthread_mutexattr_t attrmutex;

  single_buf_size = sizeof(buffer_t) + (num_records * sizeof(record_t));
  buf_arr_size = sizeof(buffer_array_t) + (num_of_users * single_buf_size);

  buf_arr = (buffer_array_t *) mmap(NULL, buf_arr_size, PROT_READ | PROT_WRITE, 
                                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (buf_arr == MAP_FAILED) {
  
    cout << "ERR: could not create mapping " << errno << endl;
    return NULL;
  }

  for (int i = 0; i < num_of_users; i++) {
 
    buf = get_user_buffer(buf_arr, i);
    buffer_init(buf);
  }

  pthread_mutexattr_init(&attrmutex);
  pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&buf_arr->super_mutex, &attrmutex);
  
  return buf_arr;
}

void
buffer_array_cleanup (buffer_array_t *buf_arr)
{

  int size = sizeof(buffer_t);
  int mmap_size = (size + (num_records * sizeof(record_t))) * (num_of_users);
  
  munmap(buf_arr, mmap_size);
}


void
print_record (record_t record)
{

  cout << "user_id: " << record.user_id << " ";
  cout << "topic: " << record.topic << " ";
  cout << "score: " << record.score;
  cout << "is_dummy: " << record.is_dummy << endl;
}

static void
print_buffer (buffer_t* buf)
{

  if(is_buffer_empty(buf)) {

    cout << "Buffer is empty" << endl;
    return;
  }

  if (buf->rear >= buf->front) {

    for (int i = buf->front; i <= buf->rear; i++) {

      print_record(buf->arr[i]);
    }
    cout << endl;
  } else {

    for (int i = buf->front; i < buf->size; i++) {

      print_record(buf->arr[i]);
    }

    for (int i = 0; i <= buf->rear; i++) {

      print_record(buf->arr[i]);
    }

    cout << endl;
  }
}

void
print_buffer_array (buffer_array_t *buf_arr)
{

  int i = 0;
  buffer_t *buf = NULL;

  for (i = 0; i < num_of_users; i++) {
 
    buf = get_user_buffer(buf_arr, i);
    print_buffer(buf);
  }
}

bool
is_buffer_full (buffer_t* buf)
{

  return (buf->size == buf->num_of_elements);
}

bool
is_buffer_empty (buffer_t* buf)
{

  return (buf->num_of_elements == 0);
}

void
copy_record (record_t *src, record_t *dst)
{

  strcpy(dst->user_id, src->user_id);
  strcpy(dst->topic, src->topic);
  dst->score = src->score;
  dst->is_dummy = src->is_dummy;
}

void
enqueue (buffer_t* buf, record_t record)
{

  if (is_buffer_full(buf)) {

    cout << "Buffer is full" << endl;
    return;
  } else if (buf->front == -1) {

    buf->front = 0;
    buf->rear = 0;
  } else if ((buf->rear == (buf->size - 1)) && ((buf->front != 0))) {

    buf->rear = 0;
    copy_record(&record, &buf->arr[buf->rear]);
  } else {

    buf->rear++;
  }
  copy_record(&record, &buf->arr[buf->rear]);
  buf->num_of_elements++;
}

int
dequeue (buffer_t* buf, record_t* record)
{

  if (is_buffer_empty(buf)) {
  
    cout << "Buffer is empty" << endl;
    return FAILURE;
  }

  record_t tmp = buf->arr[buf->front];
  if (buf->front == buf->rear) {
  
    buf->front = -1;
    buf->rear = -1;
  } else if (buf->front == (buf->size - 1)) {
  
    buf->front = 0;
  } else {
  
    buf->front++;
  }
  buf->num_of_elements--;

  *record = tmp;

  return SUCCESS;
}
