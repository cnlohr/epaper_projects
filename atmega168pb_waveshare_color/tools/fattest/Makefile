all : fattest

fattest : fattest.c ../basicfat.c
	gcc -o $@ $^  -I. -I.. -DTESTSTRAP

clean :
	rm -rf *.o *~

