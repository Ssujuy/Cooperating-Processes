
CC = gcc

CFLAGS = -Wall -pthread

OBJS = program.o

EXEC = program

ARGS = -f doc.txt -k 2 -n 2

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(CFLAGS)


clean:
	rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	./$(EXEC) $(ARGS)