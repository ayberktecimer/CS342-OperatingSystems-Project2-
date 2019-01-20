# CS342-OperatingSystems-Project2-
## Part A
In this part, The same application in project 1 is developed, but this time shared memory is used, instead of intermediate files, to pass information from child processes to the parent process. Hence, again a multi-process application that generates a value histogram for the values sitting in a set of input ascii text files, one value per line is developed. Values can be an integers or real numbers.
### To run Part A
```
$ make
$ Format ./synphistogram minvalue maxvalue bincount N file1 ... fileN outfile
$ Example ./synphistogram 1 100 5 3 file1 file2 file3 outfile
```
## Part B
In this part, a similar application as in part A is developed. But this time, for each input file, there is a separate worker thread reading the input file. This time, a worker thread is not build a histogram, but passes the values that it reads from its input file to the main thread. The main thread builds a combined histogram. To pass values form worker threads to the main thread, a single shared linked list is used.

### To run Part B
```
$ make
$ Format ./synthistogram minvalue maxvalue bincount N file1 ... fileN outfile B
$ Example ./synthistogram 1 100 5 3 file1 file2 file3 outfile 10
```
Team Members:
Ayberk Tecimer
Emre Sülün
