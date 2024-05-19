all: TCPClientReader TCPClientMonitor TCPServerLibrarian
TCPClientReader: TCPClientReader.c DieWithError.c
	gcc TCPClientReader.c DieWithError.c -o TCPClientReader
TCPClientMonitor: TCPClientMonitor.c DieWithError.c
	gcc TCPClientMonitor.c DieWithError.c -o TCPClientMonitor
TCPServerLibrarian: TCPServerLibrarian.h TCPServerLibrarian.c DieWithError.c
	gcc TCPServerLibrarian.c DieWithError.c HandleTCPClient.c HandleTCPMonitor.c \
	CreateTCPServerSocket.c AcceptTCPConnection.c \
	-o TCPServerLibrarian -lrt -pthread
