In this assignment, we are simulating electronic fund transfer (EFT) between bank accounts. We will assume that there is just one bank and severalaccounts. The program will take an input file in the form:

AccountNo1 <space> initialBalance1 
AccountNo2 <space> initialBalance2 
..
AccountNoN <space> initialBalanceN
Transfer <space> accountNoFrom1 <space> accountNoTo1 <space> Amount1
Transfer <space> accountNoFrom1 <space> accountNoTo2 <space> Amount2
...
Transfer <space> accountNoFrom1 <space> accountNoTo1 <space> Amount1

which first lists the accounts in the system along with the initial balances and then lists the transfers between accounts. We are assuming that all transfers refer to existing accounts and all initial balances and the transfer amounts are nonnegative integers.

Taking the number of worker threads to be spawned from the command line.

How to execute the program
1. make
2. ./output input.txt no_of_worker_threads
