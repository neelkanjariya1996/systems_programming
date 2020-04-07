#include <iostream>
#include <unordered_map>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

using namespace std;
#define MAX_LEN 25

/*
 * print the reducer o/p for a user
 *
 * @param useriid  The userid
 * @param map      Map of topic/totalscore for the user
 *
 * @return void
 */
void
print_reducer_op_for_user (string userid, unordered_map<string, int> map)
{
  unordered_map<string, int>:: iterator itr;

  for (itr = map.begin(); itr != map.end(); itr++) {
      cout << "(" << userid << "," << itr->first << "," << itr->second << ")" << endl;
  }
}

/*
 * validate the input passed to the reducer program
 * valid input is of the form: (userid(4-digit), topic(15-chars), score([-]2-digit))
 *
 * example of a valid input:
 *  (1111,aaaaaaaaaaaaaaa,50)
 *  (1111,aaaaaaaaaaaaaaa,-10)
 * @param line input from stdin
 * @return true if proper input provided, false otherwise
 */
bool
validate_reducer_input (string line)
{
  int len = 0, i = 0;

  if (line.empty()) {
    cout << "Please enter a non empty input" << endl;
    return (false);
  }

  len = line.length();
  if (!(len == 25 || len == 26)) {
    cout << "Invalid input lenght" << endl;
    return (false);
  }

  /*
   * validate open braces
   */
  i = 0;
  if (line.at(i++) != '(') {
    cout << "Did not find opening paranthesis" << endl;
    return (false);
  }

  /*
   * validate userid
   */
  while (i < 5) {
    if (!isdigit(line.at(i++))) {
      cout << "Invalid userid" << endl;
      return(false);
    }
  }

  if (line.at(i++) != ',') {
    cout << "Please enter a comma after user-id" << endl;
    return (false);
  }
  i += 15;

  if (line.at(i++) != ',') {
    cout << "Please enter a comma after topic" << endl;
    return (false);
  }

  if (len == 25) {
    while (i < 24) {
      if (!isdigit(line.at(i++))) {
        cout << "please enter a valid positive score" << endl;
        return(false);
      }
    }
  } else if (len == 26) {
    if (line.at(i++) != '-') {
      cout << "please enter a negetive sign for the score" << endl;
      return (false);
    }
    while (i < 25) {
      if (!isdigit(line.at(i++))) {
        cout << "please enter a valid negetive score" << endl;
        return(false);
      }
    }
  }

  if (line.at(i++) != ')') {
    cout << "Did not find closing paranthesis" << endl;
    return (false);
  }

  return (true);
}

/*
 * reducer program
 *  Reducer takes input from stdin
 *  Parses the input
 *  Calculates total score per topic for a user
 *  Prints the result on stdout
 *
 * @return void
 */
void
reducer ()
{
  unordered_map<string, int>  map;

  string line;            /* input line from stdin */
  string prev_user = "";  /* keep a track of the current user for o/p purposes */

  while (getline(cin, line)) {

    int i = 0;               /* pointer into the input line for parsing */
    string userid;           /* input userid */
    string topic;            /* input topic */
    string score;            /* input score */

    if (!validate_reducer_input(line)) {
      continue;
    }

    userid = line.substr(i + 1,  4);
    i += 5;

    topic = line.substr(i + 1, 15);
    i += 16;

    if (line.at(i + 1) == '-') {
      score = line.substr(i + 1, 3);
    } else {
      score = line.substr(i + 1, 2);
    }

    if (userid.compare(prev_user) != 0) {

      if (!prev_user.empty()) {
        print_reducer_op_for_user(prev_user, map);
      }

      prev_user = userid;
      map.clear();
    }

    unordered_map<string, int>::const_iterator got = map.find(topic);
    if (got == map.end()) {
      map.insert(make_pair(topic, atoi(score.c_str())));
    } else {
      int total_score = got->second + atoi(score.c_str());
      map.erase(topic);
      map.insert(make_pair(topic, total_score));
    }
  }

  print_reducer_op_for_user(prev_user, map);

  return;
}

/*
 * driver program for reducer
 */
int
main ()
{
  reducer();

  return (0);
}
