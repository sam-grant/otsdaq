#include "otsdaq/NetworkUtilities/TCPReceiverSocket.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace ots;

//==============================================================================
TCPReceiverSocket::TCPReceiverSocket(int socketId) : TCPSocket(socketId) { }

//==============================================================================
TCPReceiverSocket::~TCPReceiverSocket(void) {}

//==============================================================================
std::string TCPReceiverSocket::receivePacket(std::chrono::milliseconds timeout)
{
	std::string retVal = "";
	auto start = std::chrono::steady_clock::now();
	
        size_t received_bytes = 0;
	uint32_t message_size;
	while(received_bytes < 4 && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start) < timeout) {
	  int this_received_bytes = receive(reinterpret_cast<char*>(&message_size) + received_bytes, 4 - received_bytes);
	  if(this_received_bytes < 0){
	    if(errno == EAGAIN) continue; 
	    return retVal;
	  }
	  received_bytes += this_received_bytes;
	}

	
	if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start) >= timeout) {
	  // std::cout << __PRETTY_FUNCTION__ << " timeout while receiving message size, returning null (received " << static_cast<int>(received_bytes) << " bytes)" << std::endl;
	  return retVal;
	}
	

	message_size = ntohl(message_size);
	// std::cout << "Received message size in header: " << message_size << std::endl;
	message_size -= 4; // Message size in header is inclusive, remove header size
	std::vector<char> buffer(message_size);
	received_bytes = 0;
	while(received_bytes < message_size && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start) < timeout) {
	  int this_received_bytes = receive(&buffer[received_bytes], message_size - received_bytes);
	  // std::cout << "Message receive returned " << this_received_bytes << std::endl;
	  if(this_received_bytes < 0){
	    if(errno == EAGAIN) continue; 
	    return retVal;
	  }
	  received_bytes += this_received_bytes;
	}
	
	retVal = std::string(buffer.begin(), buffer.end());

	if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start) >= timeout) {
	  std::cout << __PRETTY_FUNCTION__ << " timeout while receiving message of size " << message_size << ", returning " << retVal << std::endl;
	}

	return retVal;
}

//==============================================================================
int TCPReceiverSocket::receive(char* buffer, std::size_t bufferSize, int /*timeoutMicroSeconds*/)
{
	//std::cout << __PRETTY_FUNCTION__ << "Receiving Message for socket: " << getSocketId() << std::endl;
	if(getSocketId() == 0)
	{
		throw std::logic_error("Bad socket object (this object was moved)");
	}
	// std::cout << __PRETTY_FUNCTION__ << "WAITING: " << std::endl;
	int dataRead = ::read(getSocketId(), buffer, bufferSize);
	// std::cout << __PRETTY_FUNCTION__ << "Message-" << buffer << "- Error? " << (dataRead == static_cast<std::size_t>(-1)) << std::endl;
	if(dataRead < 0)
	{
		std::stringstream error;
		switch(errno)
		{
		case EBADF:
			error << "Socket file descriptor " << getSocketId() << " is not a valid file descriptor or is not open for reading...Errno: " << errno;
			break;
		case EFAULT:
			error << "Buffer is outside your accessible address space...Errno: " << errno;
			break;
		case ENXIO:
		{
			// Fatal error. Programming bug
			error << "Read critical error caused by a programming bug...Errno: " << errno;
			throw std::domain_error(error.str());
		}
		case EINTR:
			// TODO: Check for user interrupt flags.
			//       Beyond the scope of this project
			//       so continue normal operations.
			error << "The call was interrupted by a signal before any data was "
			         "read...Errno: "
			      << errno;
			break;
		case EAGAIN:
		{
			// recv is non blocking so this error is issued every time there are no messages to read
			// std::cout << __PRETTY_FUNCTION__ << "Couldn't read any data: " << dataRead << std::endl;
			// std::this_thread::sleep_for (std::chrono::seconds(1));
			return dataRead;
		}
		case ENOTCONN:
		{
			// Connection broken.
			// Return the data we have available and exit
			// as if the connection was closed correctly.
			return dataRead;
		}
		default:
		{
			error << "Read: returned -1...Errno: " << errno;
		}
		}
		throw std::runtime_error(error.str());
	}
	else if(dataRead == static_cast<std::size_t>(0))
	{
		std::cout << __PRETTY_FUNCTION__ << "Connection closed!" << std::endl;
		throw std::runtime_error("Connection closed");
	}
	// std::cout << __PRETTY_FUNCTION__ << "Message: " << buffer << " -> is error free! Socket: " << getSocketId() << std::endl;
	return dataRead;
}

//==============================================================================
void TCPReceiverSocket::setReceiveTimeout(unsigned int timeoutSeconds, unsigned int timeoutMicroSeconds)
{
	struct timeval tv;
	tv.tv_sec  = timeoutSeconds;
	tv.tv_usec = timeoutMicroSeconds;
	setsockopt(getSocketId(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}
