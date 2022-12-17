build:
	gcc main.c includes/*.c -o webserver

debug: 
	valgrind ./webserver
