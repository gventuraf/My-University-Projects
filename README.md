# My University Projects

These are some of the projects I made during Univrsity (2017 to 2020).

## Projects:

### Operating Systems:

**Makes use of various low-level mechanisms:**
- fork()
- threads
- mutexes
- conditional variables
- signals
- shared memory
- semaphores
- message queues (SystemV)
- pipes (POSIX).

<br>
**Project Overview**
Drones make deliveries to warehouses. This software manages the whole process by choosing which drone makes each delivery, based on distance.
<br>
<ins>simulation_manager.c:</ins> It is the starting point. It initializes the message queue used for communication between the Central and the warehouses. Additionally, it manages the shared memory, which stores statistics about deliveries.
<br>
<ins>central.c:</ins> Responsible for managing the drones, where each drone is a separate thread. This file also creates a named pipe to receive orders from the user.
<br>
<ins>armazem.c:</ins> This means "warehouse" in portuguese. Uses the message queue to communicate with the drones. Writes stats to the shared memory, to be read by the simulation manager.
<br>
The various synchronization mechanisms are used throughout the project to manage access to the shared resources, i.e shared memory, message queue, drones, etc.