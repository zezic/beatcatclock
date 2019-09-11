bcc: main.o jackio.o
	g++ -g -o bcc main.o jackio.o -laubio -ljack

jackio.o: src/jackio.cpp include/jackio.hpp include/aubio_priv.hpp
	g++ -c src/jackio.cpp -laubio

main.o: src/main.cpp include/jackio.hpp
	g++ -c src/main.cpp

clean:
	rm *.o bcc
