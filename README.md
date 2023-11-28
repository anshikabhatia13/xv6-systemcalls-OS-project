# XV6 Operating System Project

This project is course project for Operating Systems course, done under Dr Jagpreet Singh at IIT Ropar, focusing on extending the XV6 Operating System. The project includes four parts, each addressing different aspects of system call tracing, process information retrieval, command history maintenance, and process statistics.

## Project Structure

- **Part One: System Call Tracing**
  - Modification of the XV6 kernel to print a line for each system call invocation, displaying the name of the system call and its return value.
  - Modification made to the `syscall()` function in `syscall.c`.

- **Part Two: ps System Call**
  - Addition of a new system call (`getprocinfo`) to retrieve information about processes in the system.
  - Creation of a user-level program (`ps.c`) to call the `getprocinfo` system call and print process information.
  - Shell integration for running the `ps` command.

- **Part Three: History of Commands**
  - Implementation of a command history feature in the XV6 kernel to store past shell commands.
  - Mechanisms for retrieving previous/next commands using the ↑/↓ keys.
  - Addition of a new system call (`history`) to retrieve command history.
  - Integration of a `history` command in the shell.

- **Part Four: Process Statistics**
  - Extension of the `proc` struct in XV6 to include fields for creation time, sleep time, ready time, and run time.
  - Addition of a new system call (`wait2`) to provide detailed information about terminated child processes.
  - Capture and update of process statistics during creation and clock ticks.

## Major Files Modified:

- `syscall.c`: System call tracing and `getprocinfo` system call.
- `ps.c`: User-level program for process information retrieval.
- `console.c`: Implementation of command history features.
- `proc.h`: Extension of the `proc` struct for process statistics.
- `syscall.h`: System call numbers and declarations.
- `defs.h`: Definitions related to system calls.
 For more details refer to the project report


## Usage

To run XV6 with the implemented features, follow the build and execution instructions provided with the XV6 source code.

Feel free to explore the code, and use it as a reference for learning about operating system concepts and XV6 kernel development.
