# Multithreaded HTTP Server

A simple multithreaded HTTP server written in C that supports **GET, POST, and DELETE** requests.  
It handles multiple client connections using **POSIX threads (pthreads)**.

## Features
- Supports **GET** requests to serve files.
- Supports **POST** requests to save data to a file.
- Supports **DELETE** requests to remove files.
- Logs client requests with timestamps and status codes.
- Uses **multi-threading** to handle multiple clients concurrently.

## Compilation  
Use `gcc` with the `-pthread` flag:  
```sh
gcc server.c -o server -pthread
