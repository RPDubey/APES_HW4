prob1.elf:prob1.c
	gcc -g -o $@ $^ -pthread -lrt $(CFLAGS)

clean:
	rm *.o *.elf
