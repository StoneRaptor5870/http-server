# C HTTP Server

A lightweight, modular HTTP server implementation written in C using TCP sockets. This server demonstrates fundamental web server concepts including socket programming, HTTP protocol handling, request parsing, and response generation.

## Features

- **TCP Socket Programming**: Built on standard POSIX socket APIs
- **HTTP/1.1 Protocol Support**: Handles basic HTTP requests and responses
- **Modular Architecture**: Clean separation of concerns across multiple modules
- **Request Parsing**: Parses HTTP method, path, version, headers, and body
- **Routing System**: Simple path-based routing with multiple endpoints
- **Graceful Shutdown**: Signal handling for clean server termination
- **Configurable Port**: Command-line port specification
- **Connection Management**: Handles multiple sequential client connections

## Project Structure

```
├── src/
│   ├── main.c          # Entry point and main server loop
│   ├── server.c        # TCP server socket operations
│   ├── request.c       # HTTP request parsing
│   ├── response.c      # HTTP response generation
│   └── handler.c       # Request routing and handlers
├── include/
│   ├── config.h        # Configuration constants
│   ├── server.h        # Server function declarations
│   ├── request.h       # Request structure and functions
│   ├── response.h      # Response structure and functions
│   └── handler.h       # Handler function declarations
└── README.md
```

## Compilation

### Prerequisites

- GCC compiler
- POSIX-compatible system (Linux, macOS, WSL)
- Standard C library

### Build Instructions

```bash
# Create build directory
mkdir -p bin

# Compile the server
gcc -Wall -Wextra -std=c99 -o bin/httpserver \
    src/main.c \
    src/server.c \
    src/request.c \
    src/response.c \
    src/handler.c \
    -I./include
```

Or create a simple Makefile:

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
SRCDIR = src
INCDIR = include
BINDIR = bin
SOURCES = $(wildcard $(SRCDIR)/*.c)
TARGET = $(BINDIR)/httpserver

$(TARGET): $(SOURCES) | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -o $@ $^

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(BINDIR)

.PHONY: clean
```

## Usage

### Starting the Server

```bash
# Run on default port (8080)
./bin/httpserver

# Run on custom port
./bin/httpserver 3000
```

### Accessing the Server

Once running, the server can be accessed via:

- **Home Page**: `http://localhost:8080/` or `http://localhost:8080/index.html`
- **About Page**: `http://localhost:8080/about`
- **404 Error**: Any other path (e.g., `http://localhost:8080/nonexistent`)

### Stopping the Server

Press `Ctrl+C` to gracefully shutdown the server.

## API Endpoints

| Method | Path          | Description                       |
| ------ | ------------- | --------------------------------- |
| GET    | `/`           | Home page with server information |
| GET    | `/index.html` | Same as home page                 |
| GET    | `/about`      | About page with server details    |
| GET    | `/*`          | 404 Not Found for all other paths |

## Configuration

Default settings can be modified in `include/config.h`:

```c
#define DEFAULT_PORT 8080
#define MAX_CONNECTIONS 10
#define BUFFER_SIZE 4096
#define MAX_RESPONSE_SIZE 8192
```

## Architecture Overview

### Core Components

1. **Server Module** (`server.c`): Handles TCP socket creation, binding, listening, and connection acceptance
2. **Request Module** (`request.c`): Parses incoming HTTP requests into structured data
3. **Response Module** (`response.c`): Builds HTTP responses with proper headers and status codes
4. **Handler Module** (`handler.c`): Routes requests to appropriate handlers and generates content
5. **Main Module** (`main.c`): Orchestrates the server lifecycle and request processing loop

### Request Flow

1. Server accepts incoming TCP connection
2. Raw HTTP request is read from socket
3. Request is parsed into structured format
4. Request is routed to appropriate handler
5. Handler generates HTTP response
6. Response is sent back to client
7. Connection is closed

## Example Output

```
Starting HTTP server on port 8080...
Server listening on http://localhost:8080
Press Ctrl+C to stop the server

New connection from 127.0.0.1:54321
Received request:
GET / HTTP/1.1
Host: localhost:8080
User-Agent: Mozilla/5.0...

Method: GET
Path: /
Version: HTTP/1.1
Content-Length: 0
Response sent

^C
Shutting down server...
```

## Limitations

- **Single-threaded**: Handles one request at a time
- **Basic HTTP**: Limited HTTP/1.1 feature support
- **No HTTPS**: Plain text communication only
- **No file serving**: Serves only hardcoded HTML responses
- **Simple routing**: Basic string matching for paths
- **No persistent connections**: Closes connection after each request

## Potential Enhancements

- Multi-threading for concurrent request handling
- File serving capabilities
- CGI support
- SSL/TLS encryption
- More HTTP methods (POST, PUT, DELETE)
- Header parsing and handling
- Cookie support
- Logging system
- Configuration file support

## Version History

- v1.0: Initial implementation with basic http server functionality (GET request)
- v1.1: CRUD routes ( GET, PUT, PATCH, POST, DELETE)
- v1.2: Static file serving (HTML, CSS, JS)
- v1.3: Implementation of URL and Query Params.
- v1.4: Dynamic Query Params and Database filter
- v1.5: Multithreading and Threadpool