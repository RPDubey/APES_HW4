prob1.elf:prob1.c
	gcc -o $@ $^ -pthread -lrt

clean:
	rm *.o *.elf
