# My University Projects

These projects were completed during my time at University (2017 to 2020).

## [Operating Systems](Operating-Systems/)

### Utilizes various low-level mechanisms:
- `fork()`
- threads
- mutexes
- conditional variables
- signals
- shared memory
- semaphores
- message queues (SystemV)
- pipes (POSIX)

### Project Overview
Drones make deliveries to warehouses. This software manages the whole process by choosing which drone makes each delivery, based on distance.

### Overview of Files
- <b>simulation_manager.c:</b>
It is the starting point. It initializes the message queue used for communication between the Central and the warehouses. Additionally, it manages the shared memory, which stores statistics about deliveries.
- <b>central.c:</b>
Responsible for managing the drones, where each drone is a separate thread. This file also creates a named pipe to receive orders from the user.
- <b>armazem.c:</b> (meaning "warehouse" in portuguese)
Uses the message queue to communicate with the drones. Writes stats to the shared memory, to be read by the simulation manager.
<p>The various synchronization mechanisms are used throughout the project to manage access to the shared resources, i.e shared memory, message queue, drones, etc.</p>


## [Introduction to Networking](Networking-Introduction/)

This project focuses on implementing networking functionalities using C sockets.
<p>There is a client app and a server app. The server can handle multiple clients simultaneously</p>

### Key Features

- <b>Client-Server Communication:</b> The client app interacts with the server application through established sockets, facilitating data exchange.
- <b>Multithreaded Server:</b> Utilizes threads to handle concurrent requests, enhancing the server's efficiency in managing multiple incoming connections.
- <b>Synchronization Mechanisms:</b> Implements mutexes and conditional variables to maintain synchronization and ensure data integrity in a multi-request environment.

## [A Simple Game in JavaScript](Game-JavaScript/)

### How to Play

1. <b>Launch the game:</b> Open the file html/index.html using Google Chrome (other browsers may not support all features).
2. <b>Browser Flags:</b> Launch Chrome with the following flags
    - ´--allow-file-access-from-files´
    - ´--autoplay-policy=no-user-gesture-required´