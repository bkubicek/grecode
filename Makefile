OPTS= -g 

all: grecode

grecode: gdecoder.o main.o
	${CXX} ${OPTS} gdecoder.o main.o -o grecode
	
main.o: main.cpp gdecoder.h
	${CXX} ${OPTS} -c main.cpp -o main.o

gdecoder.o: gdecoder.cpp gdecoder.h
	${CXX} ${OPTS} -c gdecoder.cpp -o gdecoder.o

clean:
	rm *.o
	rm grecode

test:
	#./grecode -xflip tests/testcode.ngc -o tests/out.ngc
	#./grecode -xflip tests/maxboard_cut.ngc  -o tests/testcut.ngc
	#./grecode -align cmin cmin tests/maxboard_cut.ngc  -o tests/testalign.ngc
	#cat tests/testcode.ngc |./grecode -xflip >tests/out2.ngc
	#./grecode tests/max232_kombi.ngc    -rot 45 -o tests/test.ngc -g tests/test.xy
	#./grecode tests/arctest.ngc -killn -rot 180 -o tests/test.ngc -g tests/test.xy
	./grecode tests/abstest.ngc -knive 0.1 -o tests/test.ngc -g tests/test.xy
