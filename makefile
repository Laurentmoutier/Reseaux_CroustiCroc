all:sender receiver

sender:
	gcc -lz -o sender sender.c

receiver:
	gcc -lz -o receiver receiver.c

clean:
	-rm -f sender receiver