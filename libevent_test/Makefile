CLI=client
SER=server
SRC_SER=server.c
SRC_CLI=client.c

LIB=-L/usr/lib -levent

.PHONY:all
all:$(SER) $(CLI)

$(SER):$(SRC_SER)
	gcc -o $@ $^ $(LIB) -g
$(CLI):$(SRC_CLI)
	gcc -o $@ $^ $(LIB) -g

.PHONY:clean
clean:
	rm -rf $(CLI) $(SER)
