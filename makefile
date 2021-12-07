CC = gcc
CFLAG = -g
TAR1 = oss
TAR2 = user
M_OBJ = oss.o
U_OBJ = user.o
OBJ1 = shmem.h

all: $(TAR1) $(TAR2)

$(TAR1): $(M_OBJ) $(OBJ1)
	$(CC) $(CFLAG) -lpthread -lrt -o $(TAR1) $(M_OBJ) -lm

$(TAR2): $(U_OBJ) $(OBJ1)
	$(CC) $(CFLAG) -lpthread -lrt -o $(TAR2) $(U_OBJ) -lm

$(M_OBJ): oss.c
	$(CC) $(CFLAG) -c oss.c

$(U_OBJ): user.c
	$(CC) $(CFLAG) -c user.c

clean:
	rm -rf *.o *.log $(TAR1) $(TAR2)
