all: TCPClientReader TCPServerLibrarian
TCPClientReader: TCPClientReader.c DieWithError.c
	gcc TCPClientReader.c DieWithError.c -o TCPClientReader
TCPServerLibrarian: TCPServerLibrarian.h TCPServerLibrarian.c DieWithError.c
	gcc TCPServerLibrarian.c DieWithError.c HandleTCPClient.c \
	CreateTCPServerSocket.c AcceptTCPConnection.c \
	-o TCPServerLibrarian -lrt -pthread
