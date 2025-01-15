motion: motion.o motion_main.o
	$(CC) -o $@ $^

backlight-timeout: motion.o timeout.o
	$(CC) -o $@ $^

clean:
	rm *.o
	rm motion backlight-timeout
