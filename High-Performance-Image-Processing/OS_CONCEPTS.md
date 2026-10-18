# OS Concepts Implemented in This Project

This project is a comprehensive demonstration of **Systems Programming** and **Operating System** concepts. Below is a breakdown of the specific OS mechanisms used in each component.

## 1. Concurrency & Parallelism

### Multi-Threading (pthreads)
- **File:** `src/server.c`
- **Concept:** The server uses `pthread_create` to spawn a new thread for every incoming client connection.
- **Why:** This allows the **Main Thread** to keep listening for new connections while **Worker Threads** process images in parallel.
- **Mechanism:** Threads are `detached` so the OS automatically reclaims their resources upon completion.

### Multi-Processing
- **File:** `run_demo.sh` & `src/client.c`
- **Concept:** The shell script launches multiple instances of the client program in the background (`&`).
- **Why:** The OS Scheduler manages these as separate, independent processes running simultaneously, competing for CPU time.

---

## 2. Synchronization (The "Traffic Control")

### Mutexes (Mutual Exclusion)
- **File:** `src/logging.c`, `src/server.c`
- **Concept:** Locks used to protect shared resources from **Race Conditions**.
- **Usage:**
  - `log_lock`: Ensures console output lines don't get mixed up.
  - `pool.lock`: Protects the `MemoryPool` so two threads don't grab the same buffer.

### Condition Variables
- **File:** `src/server.c` (`pool_alloc`)
- **Concept:** Implements **Wait/Notify logic**.
- **Usage:** If the pool is full, a thread sleeps (`pthread_cond_wait`) until another thread signals it (`pthread_cond_signal`). This avoids inefficient "busy waiting".

### File Locking (flock)
- **File:** `src/logging.c`
- **Concept:** Inter-Process Communication (IPC) synchronization.
- **Usage:** Prevents data corruption when the Server process and Client processes try to write to `application.log` at the exact same time.

---

## 3. Networking (IPC)

### TCP Sockets
- **File:** Server & Client
- **Concept:** The primary communication channel (Inter-Process Communication).
- **Usage:** Uses the Berkeley Sockets API (`socket`, `bind`, `listen`, `accept`, `connect`) to establish reliable byte streams between independent processes.

---

## 4. Memory Management

### Dynamic Memory Allocation
- **File:** Everywhere
- **Concept:** Using the Heap.
- **Usage:** `malloc` and `free` are used to manage memory for image buffers that are too large for the Stack.

### Custom Memory Pooling
- **File:** `src/server.c`
- **Concept:** Resource Management / Bounded Buffer.
- **Usage:** The server pre-allocates a fixed set of buffers (`POOL_SIZE`). This mimics how OS kernels manage fixed resources (like file descriptors) and prevents memory fragmentation.

---

## 5. Process Management & Signals

### Signal Handling
- **File:** `src/server.c`
- **Concept:** Intercepting OS interrupts.
- **Usage:** Catches `SIGINT` (Ctrl+C) to perform a **Graceful Shutdown** (cleaning up memory, closing sockets) instead of abruptly crashing.

### Process & Thread Identification
- **File:** `src/logging.c`
- **Concept:** Execution Contexts.
- **Usage:**
  - `pthread_self()`: Identifies individual threads within the server.
  - `getpid()`: Identifies the client process ID.

---

## 6. Resource Usage & Scheduling

### System Calls
- **File:** `src/server.c`
- **Concept:** Accounting.
- **Usage:** Uses `getrusage` to query the kernel for:
  - **User/System CPU Time:** How long the processor worked on your code vs. kernel code.
  - **Context Switches:** How many times the Scheduler paused your process to run something else (Voluntary vs Involuntary).
