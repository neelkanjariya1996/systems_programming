#include <iostream>
#include <fstream>
#include <string>

using namespace std;

/*******************
 * Global storeage *
 *******************/

/*
 * input tuple string
 */
string input;

/*
 * Assuming input is of the format
 * "(4 char userID, 1 char action, 15 char topic)"
 */
/*
 * validate mapper input
 * example of valid input:
 *  (1111,S,aaaaaaaaaaaaaaa)
 */
/*
bool
validate_mapper_input (string line)
{
  int len = 0, i = 0;

  if (line.empty()) {
    cout << "Please enter a non empty input" << endl;
    return (false);
  }

  len = line.length();
  if (!((len == 24) ||
      (len == 25))) {
    cout << "Invalid input lenght" << endl;
    return (false);
  }

  // validate open braces
  i = 0;
  if (line.at(i++) != '(') {
    cout << "Did not find opening paranthesis" << endl;
    return (false);
  }

  // validate userid
  while (i < 5) {
    if (!isdigit(line.at(i++))) {
      cout << "Invalid userid" << endl;
      return(false);
    }
  }

  // validate comma
  if (line.at(i++) != ',') {
    cout << "Please enter a comma after user-id" << endl;
    return (false);
  }

  // validate action
  char c = line.at(i++);
  if ( !((c == 'P') ||
       (c == 'L') ||
       (c == 'D') ||
       (c == 'C') ||
       (c == 'S'))) {
    cout << "Please enter a valid action" << endl;
    return (false);
  }

  // validate comma
  if (line.at(i++) != ',') {
    cout << "Please enter a comma after action" << endl;
    return (false);
  }

  i += 15;

  if (line.at(i++) != ')') {
    cout << "Did not find closing paranthesis" << endl;
    return (false);
  }
  return (true);
}
*/

/*
 * Function to create string of the input file tuples
 *
 * @input input_file input file name
 */
void
create_input_string (const char *input_file)
{

  string line;

  ifstream myfile (input_file);

  if (!myfile.is_open ()) {
      cout << "Error: opening input file\n";
      return;
  }

  while (getline (myfile, line)) {
      input.append (line);
  }

  myfile.close();

  return;

}

/*
 * Functon to print tuples on standard output
 */
void
tuple_print ()
{
  int len = 0;

  len = input.length ();
  for (int i = 0; i < len - 1; i++) {

    string userID = "";                     // userID from the tuple
    char action;                            // action from the tuple
    string topic = "";                      // topic from the tuple

    /*
     * userID length is 4
     */
    userID = input.substr(i + 1, 4);
    i = i + 5;                        // incremented by 5 to skip ","

    /*
     * action lenght is 1
     */
    action = input.at(i + 1);
    i = i + 2;                        // incremented by 2 to skip ","

    /*
     * topic lenght is 15
     */
    topic = input.substr (i + 1, 15);
    i = i + 17;                       // incremented by 17 to skip "),"

    /*
     * action == P  --> score = 50
     * action == L  --> score = 20
     * action == D  --> score = -10
     * action == C  --> score = 30
     * action == S  --> score = 40
     */
    if (action == 'P') {
      cout << "(" << userID << "," << topic << ",50)" << endl;
    } else if (action == 'L') {
      cout << "(" << userID << "," << topic << ",20)" << endl;
    } else if (action == 'D') {
      cout << "(" << userID << "," << topic << ",-10)" << endl;
    } else if (action == 'C') {
      cout << "(" << userID << "," << topic << ",30)" << endl;
    } else if (action == 'S') {
      cout << "(" << userID << "," << topic << ",40)" << endl;
    } else {
      return;
    }
  }
}

/***************************
 * Driver  Function (main) *
 ***************************/
int
main ()
{

    create_input_string ("input.txt");

    tuple_print ();

    return 0;

}
