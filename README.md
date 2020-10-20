# TCP Webserver
This project is the second programming assignment for CSCI-4273 Network Systems (Fall 2020) at CU Boulder.
The premise is to create a basic HTTP webserver that could respond to multiple connections simultaneously.

## Usage
In the root directory of the project is a Makefile. There is really only one target (besides `clean`), so you can
compile the server by simply running `make`. The usage of the program is as follows:

```shell script
./server <PORT>
```

The server runs in an infinite loop until it receives the SIGINT signal via CTRL+C. The content it looks to serve should
be located at `./www`. Failure to create this directory with an "index.htm" or "index.html" file will cause the program
to immediately exit.

## Files of Importance
The following are explanations of the code files and their significance.

### server.c
The `server.c` is a C program that contains the main code for this project. It is thoroughly commented with adequately
named variables to assist in your understanding. Please reference the code for more information about this file. 

### Makefile
The Makefile includes one make target to compile the server. It also includes the `clean` target to get rid of compiled
files. Running `make` will produce the `./server` executable which is necessary to run the program.

### CMakeLists.txt
This file is analogous to the `Makefile` and is used for both user preference and compatibility with JetBrains IDEs. If
you do not recognize this file, ignore it and use the `Makefile` instead.

### httpStressTest.py
This file is a custom Python 3 script designed to stress test the server. Running `python3 httpStressTest.py --help`
will detail how to use this program if you so desire.

## Credits
This project was designed by Jacob Malcy at the University of Colorado Boulder during the Fall 2020 semester.
