file_transfer_protocol
======================

Assignment 1 for CS 437: Distributed System

## How to test

1. `cd` to your directory
2. `ssh <your account>@ugrad<#>.cs.jhu.edu`, `mkdir cs433_1` to create a folder
3. `exit` to log out.
4. `scp *.h *.c makefile <your account>@ugrad<#>.cs.jhu.edu:~/cs433_1` to copy files.
5. Follow step 2 again, `cd cs433_1`, `make` to build.
6. Now you can run `./rcv <loss rate>` and `./ncp <loss rate> <local file> <dest file name>@<dest host name>`.
