# 🗨️ TCP Chat Application in C

A simple **multi-client chat application** built in **C** using **TCP sockets** and `select()`.  
- Each client chooses a **username** when connecting.  
- Messages are broadcast to all other connected clients.  
- Supports multiple users chatting simultaneously.  
- Type `EXIT` to disconnect.  

---

## 📂 Project Structure

```
chat-app/
├── server.c   # Chat server
├── client.c   # Chat client
└── README.md  # Documentation
```

---

## ⚙️ Build Instructions

Compile both server and client:

```bash
gcc server.c -o server
gcc client.c -o client
```

---

## ▶️ Usage

### 1. Start the Server
Run the server first:

```bash
./server
```

Server will listen on port `8080`:

```
Chat server started on port 8080...
Waiting for client........
```

---

### 2. Start Clients
In separate terminals, run:

```bash
./client
```

You’ll be prompted to enter your name:

```
Connected to chat server at 127.0.0.1:8080
Enter your name: Harsh
Welcome, Harsh! Type messages below.
```

---

### 3. Chat!
Example session:

**Terminal 1 (Harsh):**
```
Enter your name: Harsh
Welcome, Harsh! Type messages below.
hello everyone
```

**Terminal 2 (Raj):**
```
Enter your name: Raj
Welcome, Raj! Type messages below.
Harsh: hello everyone
hi Harsh!
```

---

## 🛑 Exit
To disconnect, simply type:

```
EXIT
```

The server will close your connection gracefully.

---

## 📌 Notes
- Default port: **8080** (can be changed in `#define PORT` in both files).  
- Supports up to **8 concurrent clients** (change `MAX_CLIENT` to allow more).  
- Works on Linux/Unix systems.  
