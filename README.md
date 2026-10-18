# OS Project: Multi-Threaded Image Processing System

## 1. Overview
This project demonstrates a high-performance **client-server architecture** for image processing, built entirely in C. It showcases advanced Operating System concepts including:

*   **Multi-Threading (pthreads)**: The server handles multiple clients simultaneously.
*   **Synchronization**: Uses Mutexes and Condition Variables to manage a thread-safe Memory Pool.
*   **Process Coordination**: Implements File Locking (`flock`) for safe shared logging between processes.
*   **Signal Handling**: Graceful shutdown mechanism using `SIGINT`.
*   **Networking**: TCP Sockets for reliable image data transfer.

---

## 2. Prerequisites
*   GCC Compiler
*   Make

---

## 3. Building the Project
We use a **Makefile** to automate dependency management and compilation.

1.  Open your terminal in the project folder.
2.  Run the build command:
    ```bash
    make
    ```

**What happens?**
*   The system checks for required header files (`stb_image.h`, etc.) in `src/`. If missing, it downloads them automatically.
*   It compiles the `server` (linked with `-pthread`).
*   It compiles the `client` (linked with `-lm`).
*   It prepares the `images/` folder (if empty, you should add images there).

---

## 4. How to Run (Step-by-Step)

### Step 1: Start the Server
The server manages the image processing logic.
```bash
./server
```
*   *Tip*: You can run this in a separate terminal tab, or run it in the background with `./server &`.

### Step 2: Prepare an Image
You can use **any standard image** (JPG, PNG, BMP, etc.).
*   Place your images in the **`images/`** folder. We recommend putting multiple images here to test the queue.

### Step 3: Run the Client
Run the client executable, passing your image filename as an argument:
```bash
./client images/my_photo.jpg
```

**What happens next?**
1.  The client loads your image and resizes it to 512x512.
2.  It connects to the server.
3.  The server inverts the colors (negative effect).
4.  The server sends the processed image back.
5.  The client saves the result as **`output/output_<id>.png`**.

---

## 5. Demonstrating Concurrency (Queue System)
The server is configured with a **Memory Pool Size of 1** and a **3-second artificial delay** ("Cool Down") to demonstrate queueing logic.

The easiest way to see this is using the `run_demo.sh` script (see Section 9). But to do it manually:

1.  Start the server.
2.  Open 3 separate terminal tabs.
3.  Run the client in all 3 tabs as fast as possible:
    ```bash
    ./client images/image1.jpg  # Tab 1
    ./client images/image2.jpg  # Tab 2
    ./client images/image3.jpg  # Tab 3
    ```

**Expected Result (Check `application.log`):**
*   **Client 1** starts immediately.
*   **Clients 2 & 3** trigger the log: `[SERVER] [INFO] Pool full. Waiting for a slot...`
*   After 3 seconds (when Client 1 finishes), Client 2 automatically wakes up.
*   After another 3 seconds, Client 3 wakes up.


---

## 6. Monitoring & Logging
All actions are logged to both the **console** and a shared file: **`application.log`**.
To view the logs in real-time:
```bash
tail -f application.log
```
*   Logs are tagged with `[SERVER]` or `[CLIENT]` to identify the source.
*   Timestamps prevent confusion during parallel execution.

---

## 7. Graceful Shutdown & Resource Reporting
To stop the server safely:
1.  Go to the terminal running the server.
2.  Press **`Ctrl+C`**.
3.  The server will catch the signal, log a shutdown message, and **dump a Resource Usage Report** (CPU time, Context Switches) and the recent log history to the console.

---

## 8. Cleanup
To reset the project (delete executables, logs, and `output/` folder):
```bash
make clean
```
## 9. Automated Concurrency Demo
We provide a smart helper script to launch multiple clients simultaneously.

**Usage:**
```bash
# Automatically finds and processes all images in the 'images/' folder
./run_demo.sh

# Or run specific images
./run_demo.sh images/my_photo.png
```

This script will:
1.  Create an `output/` directory if it doesn't exist.
2.  Spawn background processes for each image found.
3.  Process them in parallel (subject to the Server's Queue).
4.  Save results to `output/output_<id>.png`.

