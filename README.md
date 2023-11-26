# My University Projects

These projects were completed during my time at University (2017 to 2020).

## Projects

### Operating Systems

#### Key Features
Utilizes various low-level mechanisms:
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
<p>The various synchronization mechanisms are used throughout the project to manage access to the shared resources, i.e shared memory, message queue, drones, etc.</p>

### Overview of Files
- <b>simulation_manager.c</b>:
It is the starting point. It initializes the message queue used for communication between the Central and the warehouses. Additionally, it manages the shared memory, which stores statistics about deliveries.
- <b>central.c:</b>
Responsible for managing the drones, where each drone is a separate thread. This file also creates a named pipe to receive orders from the user.
- <b>armazem.c:</b> (meaning "warehouse" in portuguese)
Uses the message queue to communicate with the drones. Writes stats to the shared memory, to be read by the simulation manager.