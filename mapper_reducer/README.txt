This assignment is divided into three programs

1. Mapper program(mapper.cpp):
    Mapper takes input tuples (expected as below format) from a file in the form of (userid,action,topic) and converts them into output tuple in the form of (userid,topic,score). This program parses the input string and converts it into output tuples as per rules mentioned in the assignment using standard string parsing logic.
Input Format: "(4 char userID, 1 char action, 15 char topic)"

2. Reducer program(reducer.cpp):
    Reducer takes input in the form of (userid, topic, score) and converts them into output tuple into the form of (userid, topic, totalscore). This program keeps an unordered map to keep a tally of total score per topic for a user.

3. combiner program(combiner.cpp):
    Combiner combines the above two programs with effective use of pipe, fork, exec and dup2 system calls. in effect it takes the input file and outputs the total score per topic for each user.

Input file is stored as input.txt in the project directory.

Running Mapper:
1. make
2. ./mapper input.txt

Running Reducer:
1. make
2. ./reducer

Running Combiner:
1. make
2. ./combiner input.txt

For Testing:
1. run ./run.sh
