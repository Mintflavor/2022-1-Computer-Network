TARGET = myserver
myserver: server.c
	gcc server.c -o myserver
clean:
	$(RM) $(TARGET)
