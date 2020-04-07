#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <string.h>
#include <fstream>
#include <unordered_map>
#include <queue>
#include <vector>
#include <bits/stdc++.h>

using namespace std;

#define NUM_OF_CLI 3
#define SUCCESS 0

int num_of_workers = 0;
string file_name;

sem_t global_sem;

typedef struct acc_bal_t_ {

  sem_t acc_sem;
  int bal;

} acc_bal_t;

typedef struct transaction_t_ {

  int from_acc;
  int to_acc;
  int transfer_amt;

} transaction_t;

typedef struct buf_t_ {

  queue<transaction_t> q;

} buf_t;

unordered_map <int, acc_bal_t> acc_map;
unordered_map <long, buf_t> thread_id_to_queue_map;
vector <int> input_order;

void
ds_init ()
{

  long i = 0;

  if (sem_init(&global_sem, 0, 1) == -1) {
  
    cout << "ERR: Global semaphore init" << endl;
    return;
  }

  for (i = 0; i < num_of_workers; i++) {

    buf_t buf;
    thread_id_to_queue_map.insert(make_pair(i, buf));
  }

  return;
}

bool
process_input_file_for_transfer_util (string line)
{

  string delim_1 = " ";
  string delim_2 = "\n";
  size_t found_1 = 0;
  size_t found_2 = 0;
  size_t found_3 = 0;
  size_t found_4 = 0;
  static long i = 0;
  long first = 0;
  transaction_t trans;
  unordered_map <long, buf_t> :: iterator itr;
    
  found_1 = line.find(delim_1);
  found_2 = line.find(delim_1, found_1 + 1);
  found_3 = line.find(delim_1, found_2 + 1);
  found_4 = line.find(delim_2, found_3 + 1);
  trans.from_acc = stoi(line.substr(found_1 + 1, found_2 - (found_1 + 1)));
  trans.to_acc = stoi(line.substr(found_2 + 1, found_3 - (found_2 + 1)));
  trans.transfer_amt = stoi(line.substr(found_3 + 1, found_4 - (found_3 + 1)));
  first = (i % num_of_workers);
  itr = thread_id_to_queue_map.find(first);
  if (itr == thread_id_to_queue_map.end()) {
  
    cout << "ERR: could not find thread id" << endl;
    return (false);
  }
  itr->second.q.push(trans);
  i++;

  return (true);
}

bool
process_input_file_for_acc_util (string line)
{

  string delim_1 = " ";
  string delim_2 = "\n";
  size_t found_1 = 0;
  size_t found_2 = 0;
  int acc_no = 0;
  acc_bal_t acc;
  
  found_1 = line.find(delim_1);
  found_2 = line.find(delim_2, found_1 + 1);
  acc_no = stoi(line.substr(0,found_1));
  acc.bal = stoi(line.substr(found_1 + 1, found_2 - (found_1 + 1)));
  if (sem_init(&acc.acc_sem, 0, 1) == -1) {
  
    cout << "ERR: sem init" << endl;
    return (false);
  }
  acc_map.insert(make_pair(acc_no, acc));
  input_order.push_back(acc_no);

  return (true);
}

bool
process_input_file (string input_file)
{

  string line;

  ifstream myfile(input_file);

  if (!myfile.is_open()) {

    cout << "ERR: opening input file" << endl;
    return (false);
  }

  while (getline(myfile, line)) {

    if (line.at(0) == 'T') {

      if (!(process_input_file_for_transfer_util(line))) {
      
        return (false);
      }
    } else {

      if (!(process_input_file_for_acc_util(line))) {
      
        return (false);
      }
    }
  }

  return (true);
}

void
worker_main_util (long thread_id)
{

  transaction_t tuple;
  acc_bal_t from;
  acc_bal_t to;
  bool from_sem;
  unordered_map<long, buf_t> :: iterator itr1;
  unordered_map<int, acc_bal_t> :: iterator itr2;
  unordered_map<int, acc_bal_t> :: iterator itr3;

  itr1 = thread_id_to_queue_map.find(thread_id);
  while (!(itr1->second.q.empty())) {

    tuple = itr1->second.q.front();
    if (sem_wait(&global_sem) == -1) {
    
      cout << "ERR: global semaphore wait" << endl;
      return;
    }
    itr2 = acc_map.find(tuple.from_acc);
    itr3 = acc_map.find(tuple.to_acc);
    from = itr2->second;
    to = itr3->second;
    if (sem_trywait(&(from.acc_sem)) == -1) {
    
      cout << "ERR: sem trywait from account" << endl;
      return;
    } else {
    
      from_sem = true;
      if (sem_trywait(&(to.acc_sem)) == -1) {
      
        cout << "ERR: sem trywait to account" << endl;
      } else {
     
        if (sem_post(&global_sem) == -1) {
        
          cout << "ERR: sem post global" << endl;
          return;
        }
        itr1->second.q.pop();
        itr2->second.bal -= tuple.transfer_amt;
        itr3->second.bal += tuple.transfer_amt;
        from_sem = false;
        if (sem_post(&(from.acc_sem)) == -1) {
        
          cout << "ERR: sem post from acc" << endl;
          return;
        }
        if (sem_post(&(to.acc_sem)) == -1) {
        
          cout << "ERR: sem post to acc" << endl;
          return;
        }
      }
    }

    if (from_sem) {
    
      if (sem_post(&(from.acc_sem)) == -1) {
        
        cout << "ERR: sem post from acc" << endl;
        return;
    }
  }
  }

  return;
}

void*
worker_main (void* thread_id)
{

  long tid = 0;
  tid = (long) thread_id;

  worker_main_util(tid);

  pthread_exit((void*) 0);
}

void
print_output ()
{

  unordered_map <int, acc_bal_t> :: iterator itr;
  int i = 0;

  for (i = 0; i < input_order.size(); i++) {
    itr = acc_map.find(input_order[i]);
    cout << itr->first << " " << itr->second.bal << endl;
  }

  return;
}

int
main (int argc, char **argv)
{

  long  t = 0;
  int   rc = 0;
  void* status = NULL;

  if (argc != NUM_OF_CLI) {
  
    cout << "ERR: Incorrect number of command line arguments" << endl;
    cout << "Refer README for for correct input format" << endl;
    return -1;
  }
  
  file_name = argv[1];
  num_of_workers = stoi(argv[2]);

  if (num_of_workers == 0) {
  
    cout << "ERR: Number of workers has to be atleast 1" << endl;
    return -1;
  }

  ds_init();

  if (!(process_input_file(file_name))) {
  
    return -1;
  }

  pthread_t worker_threads[num_of_workers];

  pthread_attr_t attr;
  rc = pthread_attr_init(&attr);
  if (rc != SUCCESS) {

    cout << "ERR: initializing attribute with err_no: "
         << rc << endl;
    return -1;
  }

  rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  if (rc != SUCCESS) {
  
    cout << "ERR: setting attribute to joinable with err_no: " 
         << rc << endl;
    return -1;
  }

  for (t = 0; t < num_of_workers; t++) {

    rc = pthread_create(&worker_threads[t], &attr,
                        worker_main, (void *)t);
    if (rc != SUCCESS) {

      cout << "ERR: creating reducer: " << t << " with err_no: "
           << rc << endl;
      return -1;
    }
  }

  rc = pthread_attr_destroy(&attr);
  if (rc != SUCCESS) {

    cout << "ERR: destroying attribute with err_no: "
         << rc << endl;
    return -1;
  }

  for (t = 0; t < num_of_workers; t++) {

    rc = pthread_join(worker_threads[t], &status);
    if (rc != SUCCESS) {

      cout << "ERR: joining reducer thread: " << t
           << "to main with err_no: " << rc << endl;
      return -1;
    }
  }

  print_output();

  pthread_exit(NULL);
  return 0;
}
