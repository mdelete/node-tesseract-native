all: tesseract_native.node

node-tesseract-native.o: node-tesseract-native.cc
	g++ -Wall -fPIC -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -I/usr/local/node/include/node -c node-tesseract-native.cc -o node-tesseract-native.o

tesseract_native.node: node-tesseract-native.o
	g++ node-tesseract-native.o -o tesseract_native.node -shared -L/usr/local/lib -ltesseract -llept

clean:
	rm -f *.o tesseract_native.node

