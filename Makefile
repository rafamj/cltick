CC = g++
CFLAGS =  -O3 -Wall
OBJECTS = cltick.o



modular: $(OBJECTS)
	 $(CC) $(OBJECTS) $(CFLAGS) -o cltick -lasound 

.cpp.o:
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm *.o cltick 
