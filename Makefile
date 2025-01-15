motion: motion.o motion_main.o
	$(CC) -o $@ $^

backlight-timeout: timeout.o
	$(CC) -o $@ $^
