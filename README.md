# Academia Portal — Multi-User Course Management System

Centralized client–server academic management system built in C using TCP sockets, pthreads, and file locking.

## Features
- Role-based access (Admin, Faculty, Student)
- Concurrent client handling using pthreads
- File-based storage with fcntl read/write locks
- Course enrollment, seat management, and authentication

## Tech Stack
- C
- TCP Sockets
- pthreads
- fcntl file locking

## Build
make

## Run Server
./academia_server

## Run Client
./academia_client
