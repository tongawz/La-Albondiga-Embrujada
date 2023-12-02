todo: reposteros cocineros mozos

mozos: mozos.o cocina.o
	g++ -Wall -g -o mozos mozos.o cocina.o 
mozos.o: mozos.c 
	g++ -Wall -g -c mozos.c 

cocineros: cocineros.o cocina.o
	g++ -Wall -g -o cocineros cocineros.o cocina.o 
cocineros.o: cocineros.c 
	g++ -Wall -g -c cocineros.c 

reposteros: reposteros.o cocina.o
	g++ -Wall -g -o reposteros reposteros.o cocina.o 
reposteros.o: reposteros.c 
	g++ -Wall -g -c reposteros.c 


cocina.o: cocina.c cocina.h
	g++ -Wall -g -c cocina.c
clean:
	rm -f *.o
	rm -f cocineros mozos reposteros