all:
	gcc -Wall -o syn_thistogram syn_thistogram.c -lpthread
	gcc -Wall -o syn_phistogram syn_phistogram.c -lrt -lpthread
