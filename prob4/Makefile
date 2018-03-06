#prob2.elf:prob2.c
	#gcc -g -o $@ $^ -pthread -lrt $(CFLAGS)

#clean:
	#rm *.o *.elf

obj-m += prob3.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
