CFLAGS=-O3 -Wall
motion: motion.o motion_main.o
	$(CC) -o $@ $^

backlight-timeout: motion.o timeout.o
	$(CC) -o $@ $^

motion.o: motion.h

timeout.o: motion.h

clean:
	rm *.o || true
	rm motion backlight-timeout || true
