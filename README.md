# Multi-User Communicating Shells with Shared Messaging

This project implements a concurrent, GUI-based terminal emulator capable of running multiple shell instances in parallel. It combines standard shell functionality (process execution, piping, redirection) with an inter-process communication (IPC) system that allows distinct shell instances to broadcast messages to a shared buffer.

The application is built using **C**, **GTK** for the interface, and **POSIX Shared Memory** for IPC, strictly adhering to the **Model-View-Controller (MVC)** architectural pattern.

## Key Features

* **GUI Terminal Interface:** A fully functional terminal window built with GTK (using widgets like `GtkTextView` and `GtkEntry`).
* **Shell Command Execution:** Parses and executes standard shell commands (e.g., `ls`, `grep`, `cat`) by forking child processes and managing `stdin`/`stdout`/`stderr` redirection.
* **Inter-Process Chat:** A built-in messaging system allowing users to broadcast messages across all active shell instances using a custom protocol (e.g., `@msg`).
* **Shared Memory IPC:** Utilizes POSIX shared memory (`shm_open`, `mmap`) protected by semaphores to ensure thread-safe communication between independent processes.
* **MVC Architecture:** Clean separation of concerns between backend logic (Model), UI rendering (View), and input handling (Controller).

---

## 🏗 Architecture

The project is structured around the Model-View-Controller design pattern:

### 1. Model (Backend Logic)
The Model is responsible for data management and low-level system operations.
* **Process Management:** Handles `fork()` and `execvp()` calls to spawn child processes for shell commands. It manages pipes to capture output and tracks process PIDs.
* **Shared Memory Buffer:** Initializes and manages a memory-mapped region (`mmap`) that acts as a centralized bulletin board for messages.
* **State:** Maintains the state of the current session and the message history.

### 2. View (GTK Interface)
The View handles the graphical presentation.
* **Terminal Emulator:** Displays command output and accepts user input.
* **Message Panel:** A dedicated area (sidebar) that reads from the model to display broadcasted messages from other shell instances.

### 3. Controller (Logic Coordinator)
The Controller acts as the bridge between the user and the system.
* **Input Parsing:** Analyzes user input to distinguish between standard shell commands and chat messages.
    * *Command:* Calls `model_execute_command()`.
    * *Message:* Calls `model_send_message()`.
* **Polling:** Periodically checks the shared memory via the Model to update the View with new messages.

---

## 💻 Technical Implementation

### Shared Memory & Synchronization

To allow separate processes (shell windows) to communicate, the system uses a shared memory segment. To prevent race conditions (data corruption) when multiple shells try to write simultaneously, access is synchronized using POSIX semaphores.

**Data Structure:**
The shared memory is structured using a custom header to track the semaphore and file descriptor:

```c
typedef struct shmbuf {
    sem_t sem;         /** Semaphore to control read/write access */
    size_t cnt;        /** Number of bytes used in 'msgbuf' */
    int fd;            /** File descriptor associated with the memory */
    char msgbuf[];     /** The actual data buffer */
} ShmBuf;
```

## Process Execution
Commands are executed by creating a child process. The system uses `dup2()` to redirect the child's standard output (stdout) to a pipe, allowing the parent process (the GUI) to read the command's output and display it in the GTK window.

---

## 🛠 Installation & Build

### Prerequisites
* GCC Compiler
* Make
* GTK+ 3.0 Development Libraries

**On Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential libgtk-3-dev
```

### Building the Project
Clone the repository and run the makefile:
```bash
git clone <repository-url>
cd <repository-folder>
make
```

## 📖 Usage

1. **Start the Shell:** Run the compiled executable to open a new shell instance.
   ```bash
   ./shell_app
   ```
   *You can open multiple instances of the application to test communication.*

2. **Execute Commands:** Type standard Linux commands in the input bar:
   ```bash
   > ls -la
   > cat main.c | grep "include"
   ```

3. **Send Messages:** To send a message to all other open shell instances, prefix your input with `@msg`:
   ```bash
   > @msg Hello world from Shell 1!
   ```
   *The message will appear in the shared message panel of all active instances.*

---

## 📂 Project Structure

* `model.c` / `model.h`: Handles `shm_open`, `mmap`, `fork`, `exec`, and semaphore logic.
* `view.c` / `view.h`: Manages GTK window creation, widgets, and layout.
* `controller.c` / `controller.h`: Connects signal handlers and routes logic between Model and View.
* `main.c`: Entry point for the application.
* `Makefile`: Build configuration.
