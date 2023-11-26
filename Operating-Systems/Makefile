CC	= gcc
CFLAGS	= -D_REENTRANT -lpthread -Wall -o
LDFLAGS = -lm

prog01: prog02 prog03
	$(CC) simulation_manager.c $(CFLAGS) sim.out $(LDFLAGS)

prog02:
	$(CC) central.c shared_functions.c drone_movement.c $(CFLAGS) central.out $(LDFLAGS)

prog03:
	$(CC) armazem.c shared_functions.c $(CFLAGS) armazem.out $(LDFLAGS)
