
Obj:
A simple yet functional web server implemented in C that uses low-level system calls and the TCP/IP stack to handle basic HTTP GET requests. This project demonstrates a solid understanding of network protocols, socket programming, and server-side architecture.


# Table of Contents

- [Project Overview]
- [Features]
- [Technologies Used]
- [Learning Objectives]
- [How to Build]
- [How to Run]
- [Project Structure]
- [Future Improvements]



## Project Overview
This project is part of a Computer Networks course at the University of North Texas (CSCE 3530). The objective is to build a functional web server that:

- Accepts TCP connections
- Handles HTTP GET requests
- Serves static HTML files from a local directory
- Responds with HTTP 200 OK or HTTP 404 Not Found

## Features
- Basic HTTP/1.0 GET support
- Static file serving
- Graceful error handling (404 page)
- Logs requests to terminal
- Clean, modular C code

## Technologies Used

- C Programming Language
- TCP/IP Socket Programming (BSD Sockets)
- POSIX system calls (open, read, write, close)
- Makefile for build automation

## Learning Objectives
- Understand how web servers work under the hood
- Practice socket programming and server concurrency
- Learn the anatomy of HTTP requests and responses
- Build and test a real networking application

##  How to Build

Prerequisites

- Linux/macOS (WSL works too)
- GCC Compiler

### To Compile:

gcc -o webserver webserver.c

## How to Run

./webserver 
Port#: <PORT>

Then in your browser, go to:

http://localhost:<PORT>/index.html

or for UNT network Users:
http://cell04-cse.eng.unt.edu:<PORT>/

Make sure index.html is in the same directory as the server.

## Project Structure

webserver/
├── webserver.c        # Main server source code
├── 404.html           # Error page if file not found
├── index.html         # Test file served by the server
├── README.md          # This file
└── Makefile           #  Build automation

## Demo

Coming soon


## Future Improvements

- Add support for HTTP/1.1
- Implement persistent connections
- MIME type detection
- Multithreading (one thread per connection)
- Logging to file




