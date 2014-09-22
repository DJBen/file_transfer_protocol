file_transfer_protocol
======================

Assignment 1 for CS 437: Distributed Systems

## Completion Status

* Ability to transfer large files (tested 100K, 1M, 50M and 100M) with losses (tested ncp loss = [0, 1, 2, 5, 10, 20, 30] * rcv loss = [0, 1, 2, 5, 10, 20, 30].
* Glitches: There's a compiler warning because we use `unsigned char *` in the `strcpy` function and we don't know how to silence it.
* Not complete: t_ncp and t_rcv is transferring data but not correctly.

## How to run
1. `make`

## Source files we wrote / modified

* ncp.c
* rcv.c
* net_include.h
* queue.h queue.c
* message_dbg.h message_dbg.c
* makefile
* README.md
* t_ncp.c t_rcv.c

## How to test (this is to my partner)

1. `cd` to your directory
2. `ssh <your account>@ugrad<#>.cs.jhu.edu`, `mkdir cs433_1` to create a folder
3. `exit` to log out.
4. `scp *.h *.c makefile <your account>@ugrad<#>.cs.jhu.edu:~/cs433_1` to copy files.
5. Follow step 2 again, `cd cs433_1`, `make` to build.
6. Now you can run `./rcv <loss rate>` and `./ncp <loss rate> <local file> <dest file name>@<dest host name>`.
