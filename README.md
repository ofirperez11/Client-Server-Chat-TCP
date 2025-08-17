# Client-Server Chat over TCP (C++)

A multi-client chat application implemented in **C++** using **TCP sockets**.  
The project includes both **server** and **client** implementations, supporting concurrent communication with multiple clients via threads.

---

## 📌 Features
- Multi-threaded server supporting multiple clients.
- Each client registers with a unique **name** when connecting.
- Clients can:
  - Send messages directly to another client by specifying their name.
  - Disconnect gracefully using the `exit` command.
- Thread-safe access to client connections using `std::mutex`.

---
