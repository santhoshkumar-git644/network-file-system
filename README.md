# Network File System (NFS)

A custom, distributed Network File System (NFS) implemented in C. This project simulates a highly scalable file system environment composed of three main components: a central **Naming Server**, multiple distributed **Storage Servers**, and end-user **Clients**.

## Architecture

1. **Naming Server (NM):** Acts as the central registry and directory service for the NFS. It orchestrates communication, maintains a hashmap mapping files to Storage Servers, enforces user access permissions, and load-balances requests.
2. **Storage Server (SS):** The servers that physically hold the file data. They process commands for reading, writing, and administrating files, executing directory tasks, and streaming data.
3. **Client:** The interface through which users connect. Clients initially contact the NM to get authorization and the location of a file, then communicate directly with the designated SS for high-throughput operations.

## Features Supported

* **Core File Operations:** Create, Read, Write, Delete, and Info.
* **Concurrency:** Fine-grained concurrency controls with POSIX read-write locks (`pthread_rwlock`). Concurrent reads are fully supported. Writes can optionally be asynchronous.
* **Directory Operations:** Creation of directories (`MKDIR`) and listing contents (`LSDIR`).
* **Advanced File Interaction:**
  * **UNDO:** Revert the most recent file changes.
  * **STREAM:** Stream large files in chunks from the Storage Server to the Client without overwhelming memory.
  * **EXEC:** Trigger remote execution of executable files on the Storage Server.
* **High Availability & Reliability:**
  * **Replication:** Asynchronous replication triggered on file creation/write to duplicate files across Storage Servers for fault tolerance.
  * **LRU Caching:** The Naming Server uses an LRU cache to speed up lookups for frequently accessed files.
* **User Security & Permissions:**
  * User authentication (`ADDUSER`, `LOGIN`).
  * File-level authorization (The creator is implicitly granted access, and can use `GRANT` to share access).
* **Naming Server Search:** Allows substring-based searching of files and directories across all active Storage Servers (`SEARCH`).

## Compilation

Standard GNU Make or `gcc` can be used to compile the components (assumes a POSIX/MinGW environment). 
Example manual compilation:
```bash
# Naming Server
gcc -o nm src/naming_server/*.c -Iinclude -lpthread

# Storage Server
gcc -o ss src/storage_server/*.c -Iinclude -lpthread

# Client
gcc -o client src/client/*.c -Iinclude
```

## Running the Architecture

1. Start the **Naming Server**.
2. Start one or more **Storage Servers**. (They will automatically register with the NM).
3. Connect a **Client** instance and start executing commands!

## Supported Client Commands
- `ADDUSER <username>`
- `LOGIN <username>`
- `CREATE <filename>`
- `READ <filename>`
- `WRITE <filename> <sentence_number> [SYNC|ASYNC]`
- `DELETE <filename>`
- `UNDO <filename>`
- `INFO <filename>`
- `STREAM <filename>`
- `EXEC <filename>`
- `MKDIR <dirname>`
- `LSDIR <dirname>`
- `SEARCH <substring>`
- `GRANT <filename> <username>`
