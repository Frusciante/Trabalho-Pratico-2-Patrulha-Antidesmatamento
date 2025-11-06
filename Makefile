CC := gcc
CFLAGS := -g
LDFLAGS := -lpthread

EXE_CLIENT := client
EXE_SERVER := server

all: $(EXE_SERVER) $(EXE_CLIENT)

$(EXE_SERVER): server.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(EXE_CLIENT): client.c thread.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(EXE_CLIENT) $(EXE_SERVER)
