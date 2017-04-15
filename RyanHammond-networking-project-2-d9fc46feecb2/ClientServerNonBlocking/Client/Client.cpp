#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include "eMessageID.h"
#include "Headers.pb.h"

#include "Buffer.h"
#include "Utilities.h"


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

bool Connected;
std::vector<std::string> g_vecRoomNames;
std::string userName;

void sendingThread(LPVOID ConnectSocket)
{
	//Buffer buffer(0);
	std::cout << "c: to create an account" << std::endl;
	std::cout << "a: to authenticate an account" << std::endl;
	std::cout << "j: 'roomName' to join a room" << std::endl;
	std::cout << "l: 'roomName' to leave a room" << std::endl;
	std::cout << "[roomName] 'your message' to send a message to a room you are in" << std::endl;

	while (Connected)
	{
		//Ask the user to input a message
		std::string message;
		std::getline(std::cin, message);
		short messageLength = (short)message.length();
		eraseLeadingSpaces(message);
		//The Quit / Exit command
		if (message == "q")
		{
			Connected = false;

			// shutdown the connection since no more data will be sent
			int resultInt = shutdown((SOCKET)ConnectSocket, SD_SEND);
			if (resultInt == SOCKET_ERROR)
			{
				std::cout << "shutdown failed with error: " << WSAGetLastError() << std::endl;
				closesocket((SOCKET)ConnectSocket);
				WSACleanup();
			}

			break;
		}
		else if (message[0] == 'j' || message[0] == 'J' && message[1] == ':')
		{
			message.erase(0, 2);
			
			headers::Container* m_container = new headers::Container();
			eraseLeadingSpaces(message);
			
			std::string curRoom;
			bool bRoomExists = false;
			
			if (g_vecRoomNames.empty())
			{
				curRoom = message;
				g_vecRoomNames.push_back(curRoom);

				//int packetLength = 2 + 2 + (short)message.length();
				//m_message->set_packetlength(packetLength);
				//buffer.writeInt16BE(packetLength);	//packetsize
				m_container->set_id(headers::Container_requestID_ID_JOIN);
				//buffer.writeInt16BE(-22222);	//message ID
				headers::Message* m_message = new headers::Message();
				m_message->set_username(userName);
				m_message->set_roomname(message);
				m_message->set_message("");
				m_container->set_allocated_message(m_message);
				//buffer.writeStringBE(message);
				int packetLength = m_container->ByteSize() + 2;
				m_container->set_packetlength(packetLength);
				
				std::string serializedString;
				m_container->SerializeToString(&serializedString);
				std::cout << "You have joined the room: " << message << std::endl;
				//customSend(ConnectSocket, buffer, packetLength);
				customSend(ConnectSocket, serializedString, packetLength);
			}
			else
			{
				for (int index = 0; index != g_vecRoomNames.size(); index++)
				{
					if (message == g_vecRoomNames[index])
					{
						bRoomExists = true;
						std::cout << "Already in room " << message << std::endl;
						break;
					}
				}
				if (!bRoomExists)
				{
					curRoom = message;
					g_vecRoomNames.push_back(curRoom);
					headers::Message* m_message = new headers::Message();
					//int packetLength = 2 + 2 + 2 + (short)message.length();
					//buffer.writeInt16BE(packetLength);	//packetsize7
					//m_message->set_packetlength(packetLength);
					//buffer.writeInt16BE(-22222);	//message ID
					m_container->set_id(headers::Container_requestID_ID_JOIN);
					//buffer.writeStringBE(message);
					m_message->set_username(userName);
					m_message->set_roomname(message);
					m_message->set_message("");
					m_container->set_allocated_message(m_message);
					int packetLength = m_container->ByteSize() + 2;
					m_container->set_packetlength(packetLength);
					std::string serializedString;
					m_container->SerializeToString(&serializedString);
					std::cout << "You have joined the room: " << message << std::endl;
					//customSend(ConnectSocket, buffer, packetLength);
					customSend(ConnectSocket, serializedString, packetLength);

				}
			}
		}//end of Join
		else if (message[0] == 'l' || message[0] == 'L' && message[1] == ':')	//if leave command is read
		{
			message.erase(0, 2);	//erase the command

			eraseLeadingSpaces(message);
			headers::Container *m_container = new headers::Container();

			std::string curRoom;
			bool bRoomExists = false;
			if (g_vecRoomNames.empty())
			{
				std::cout << "You are not part of any rooms.  Use command 'j: 'roomName' to join a room" << std::endl;
			}
			else
			{
				for (int index = 0; index != g_vecRoomNames.size(); index++)
				{
					if (message == g_vecRoomNames[index])
					{
						bRoomExists = true;
						//send leave message
						std::cout << "You have left the room: " << message << std::endl;
						//int packetLength = 2 + 2 + 2 +(short)message.length();
						headers::Message* m_message = new headers::Message();
						//buffer.writeInt16BE(packetLength);
						//m_message->set_packetlength(packetLength);
						//buffer.writeInt16BE(MESSAGE_ID_LEAVE);
						m_container->set_id(headers::Container_requestID_ID_LEAVE);
						//buffer.writeStringBE(message);
						m_message->set_username(userName);
						m_message->set_roomname(message);
						m_message->set_message("");
						m_container->set_allocated_message(m_message);
						int packetLength = m_container->ByteSize() + 2;
						m_container->set_packetlength(packetLength);
						std::string serializedString;
						m_container->SerializeToString(&serializedString);
						//customSend(ConnectSocket, buffer, packetLength);
						customSend(ConnectSocket, serializedString, packetLength);
						
						g_vecRoomNames.erase(g_vecRoomNames.begin() + index);
						g_vecRoomNames.shrink_to_fit();
						//remove from list
						break;
					}
				}
				if (!bRoomExists)
				{
					std::cout << "room " << message << " not found!" << std::endl;
				}
			}
		}//end of Leave
		else if (message[0] == '[')
		{
			headers::Container *m_container = new headers::Container();
			bool bRoomFound = false;
			bool bRoomSpecified = false;
			message.erase(0, 1);
			std::string roomName;
			for (int index = 0; index != message.length(); index++)
			{
				if (message[index] == ']')
				{
					bRoomSpecified = true;
					message.erase(index, 1);
					roomName.resize(index);
					break;
				}
				
			}
			if (!bRoomSpecified)	//if no room given
			{
				std::cout << "please specify a room using the syntax ['roomName']." << std::endl;
				continue;
			}

			//separate room name from rest of message
			for (int index = 0; index != roomName.size(); index++)
			{
				roomName[index] = message[0];
				message.erase(0, 1);
			}

			if (g_vecRoomNames.empty())	//if not part of any rooms
			{
				std::cout << "You have not joined any rooms yet.  Do so by typing 'j:' followed by the desired room name." << std::endl;
			}
			else
			{
				eraseLeadingSpaces(message);
				for (int index = 0; index != g_vecRoomNames.size(); index++)
				{
					if (roomName == g_vecRoomNames[index])
					{
						//we are part of that room
						//send message to server
						bRoomFound = true;
						headers::Message* m_message = new headers::Message();
						//int packetLength = 2//packetLength
						//	+ 2//messageID
						//	+ 2//room length
						//	+ (short)roomName.length()	//room name
						//	+ 2 //message length
						//	+ (short)message.length();	//message
						//buffer.writeInt16BE(packetLength);
						//m_message->set_packetlength(packetLength);
						//buffer.writeInt16BE(MESSAGE_ID_MESSAGE);
						m_container->set_id(headers::Container_requestID_ID_MESSAGE);
						//buffer.writeStringBE(roomName);
						m_message->set_username(userName);
						m_message->set_roomname(roomName);
						//buffer.writeStringBE(message);
						m_message->set_message(message);
						int packetLength = m_container->ByteSize() + 2;
						m_container->set_packetlength(packetLength);
						std::string serializedString;
						m_container->SerializeToString(&serializedString);
						customSend(ConnectSocket, serializedString, packetLength);
						//customSend(ConnectSocket, buffer, packetLength);
					}
				}
				if (!bRoomFound)
				{
					std::cout << "Couldn't find room: " << roomName << std::endl;
					
				}
			}


		}//end of message
		else if (message[0] == 'c' || message[0] == 'C' && message[1] == ':')
		{
			headers::Container* m_container = new headers::Container();
			headers::CreateAccount* m_message = new headers::CreateAccount();
			std::cout << "Enter userName: ";
			//std::string userName;
			std::getline(std::cin, userName);
			m_message->set_username(userName);
			std::cout << std::endl << "Enter password: ";
			std::string password;
			std::getline(std::cin, password);
			m_message->set_password(password);
			m_container->set_id(headers::Container_requestID_ID_CREATE);
			m_container->set_allocated_create(m_message);

			int packetLength = m_container->ByteSize() + 2;
			m_container->set_packetlength(packetLength);

			std::string serializedString;
			m_container->SerializeToString(&serializedString);
			customSend(ConnectSocket, serializedString, packetLength);
			
		}
		else if (message[0] == 'a' || message[0] == 'A' && message[1] == ':')
		{
			headers::Container* m_container = new headers::Container();
			headers::AuthenticateLogin* m_message = new headers::AuthenticateLogin();
			std::cout << "Enter userName: ";
			std::string userName;
			std::getline(std::cin, userName);
			m_message->set_username(userName);
			std::cout << std::endl << "Enter password: ";
			std::string password;
			std::getline(std::cin, password);
			m_message->set_password(password);
			m_container->set_id(headers::Container_requestID_ID_LOGIN);
			m_container->set_allocated_auth(m_message);

			int packetLength = m_container->ByteSize() + 2;
			m_container->set_packetlength(packetLength);

			std::string serializedString;
			m_container->SerializeToString(&serializedString);
			customSend(ConnectSocket, serializedString, packetLength);

		}

		//buffer.clear();
	}
}

int __cdecl main(int argc, char **argv) 
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints;

	Buffer buffer(0);

    char recvbuf[DEFAULT_BUFLEN];
    int resultInt;
    int recvbuflen = DEFAULT_BUFLEN;

	//Step 1
    // Initialize Winsock
    resultInt = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (resultInt != 0) 
	{
		std::cout << "WinSock Initalization failed" << std::endl;
		return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    resultInt = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
    if ( resultInt != 0 ) {
        std::cout << "Socket Initalization failed" << std::endl;
		WSACleanup();
        return 1;
    }

	//std::cout << "Step 1: WinSock Initalized" << std::endl;

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) 
	{

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (ConnectSocket == INVALID_SOCKET) 
		{
            std::cout << "Socket failed with error: " << WSAGetLastError() <<std::endl;
            WSACleanup();
            return 1;
        }

        // Connect to server.
        resultInt = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);

        if (resultInt == SOCKET_ERROR) 
		{
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

	//This is a global variable used in the thread
	Connected = true;

	//std::cout << "Step 2: Socket Created" << std::endl; //Put this here so we only see the message once althought we may try multiple sockets
	//std::cout << "Step 3: Connected to the server" << std::endl;

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) 
	{
		std::cout << "Unable to connect to server!" << std::endl;
        WSACleanup();
        return 1;
    }

	//Start a thread that takes keyboard input and sends
	std::thread sendThread(sendingThread, (LPVOID)ConnectSocket); //LPVOID is a pointer to any data type. we can use it to pass in whatever data we need.

    // Receive until the peer closes the connection
	int currentBytes = 0;
    do 
	{
		std::string recvString;
        resultInt = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		currentBytes += resultInt;
		if (resultInt > 0)
		{
			//std::cout << "Bytes received: " << resultInt << std::endl;
			//buffer.clear();
			//buffer.loadBuffer(recvbuf, resultInt);
			recvString.append(recvbuf);
			std::string outString;
			outString.append(recvbuf);
			//packet length
			headers::Container* protoContainer = new headers::Container();
			//headers::CreateAccountSuccess* protoCreateSuccessMessage;
			//headers::CreateAccountFailure* protoCreateFailMessage;
			//headers::AuthenticateSuccess* protoAuthSuccessMessage;
			//headers::AuthenticateFailure* protoAuthFailMessage;
			protoContainer->ParseFromString(recvString);
			int packetLength = protoContainer->packetlength();
			//int packetLength = buffer.readInt16BE();
			while (currentBytes < packetLength)
			{
				resultInt = recv(ConnectSocket, recvbuf, recvbuflen, 0);
				
				outString.append(recvbuf);
				//buffer.loadBuffer(recvbuf, resultInt);
				currentBytes += resultInt;
			}
			currentBytes = 0;
			protoContainer->ParseFromString(outString);
			//messageID
			int nameLength;
			int errReason;
			std::string name;
			int messageID = protoContainer->id();//buffer.readInt16BE();
			int roomLength;
			std::string roomName;
			int messageLength;
			std::string message;
			std::string creationDate;
			std::string userNameIn;
			const headers::Message* m_message = new headers::Message();
			const headers::CreateAccountSuccess* m_create = new headers::CreateAccountSuccess();
			const headers::CreateAccountFailure* m_fail = new headers::CreateAccountFailure();
			const headers::AuthenticateFailure* m_authFail = new headers::AuthenticateFailure();
			const headers::AuthenticateSuccess* m_authSuccess = new headers::AuthenticateSuccess();
			switch (messageID)
			{
			case headers::Container_requestID_ID_JOIN: //packetLength + messageID + roomLength + roomName + messageLength + message
				//handle join function
				
				m_message = &protoContainer->message();
				//roomLength = protoMessage.roomlength;//buffer.readInt16BE();
				userNameIn = m_message->username();
				roomName = m_message->roomname();//buffer.readStringBE(roomLength);
				//messageLength = protoMessage.messagelength;//buffer.readInt16BE();
				message = m_message->message();//buffer.readStringBE(messageLength);

				std::cout << "[" << roomName << "]   " << message << std::endl;

				break;
			case headers::Container_requestID_ID_MESSAGE:	//packetLength-MessageID-Name-roomLength-RoomName-MessageLength-Message
				//const headers::Message* m_message = new headers::Message();
				m_message = &protoContainer->message();
				name = m_message->username();//buffer.readInt16BE();
				//roomLength = protoMessage.roomlength;//buffer.readInt16BE();
				roomName = m_message->roomname();//buffer.readStringBE(roomLength);
				//messageLength = protoMessage.messagelength;//buffer.readInt16BE();
				message = m_message->message();//buffer.readStringBE(messageLength);
				std::cout << "[" << roomName << "] " << name << ": " << message << std::endl;
				break;
			case headers::Container_requestID_ID_LEAVE:
				//const headers::Message* m_message = new headers::Message();
				m_message = &protoContainer->message();

				name = m_message->username();
				//roomLength = protoMessage.roomlength;//buffer.readInt16BE();
				roomName = m_message->roomname();//buffer.readStringBE(roomLength);
				//messageLength = protoMessage.messagelength;//buffer.readInt16BE();
				message = m_message->message();//buffer.readStringBE(messageLength);

				std::cout << "[" << roomName << "]   " << message << std::endl;
				break;
			case headers::Container_requestID_ID_CREATE_SUCCESS:
				
				m_create = &protoContainer->createsuccess();
				name = m_create->userid();
				std::cout << name << " created successfully." << std::endl;
				break;
			case headers::Container_requestID_ID_CREATE_FAIL:
				
				m_fail = &protoContainer->createfail();
				errReason = m_fail->errorreason();
				switch (errReason)
				{
				case headers::CreateAccountFailure_reason_ACCOUNT_ALREADY_EXISTS:
					std::cout << "Error: Account Already Exists" << std::endl;
					break;
				case headers::CreateAccountFailure_reason_INTERNAL_SERVER_ERROR:
					std::cout << "Error: Internal Server Error.  Try Again." << std::endl;
					break;
				default:
					std::cout << "Error: Internal Server Error.  Try Again." << std::endl;
					break;
				}
				break;
			case headers::Container_requestID_ID_AUTHENTICATE_FAILURE:
				
				m_authFail = &protoContainer->authfail();
				errReason = m_authFail->errorreason();
				switch (errReason)
				{
				case headers::AuthenticateFailure_reason_INVALID_CREDENTIALS:
					std::cout << "Error: Invalid username/password." << std::endl;
					break;
				default:
					std::cout << "Error: Internal Server Error.  Try Again." << std::endl;
					break;
				}
				break;
			case headers::Container_requestID_ID_AUTHENTICATE_SUCCESS:
				
				m_authSuccess = &protoContainer->authsuccess();
				name = m_authSuccess->userid();
				creationDate = m_authSuccess->creationdate();
				std::cout << "Login Successful. " << userName << " was created on " << creationDate << std::endl;
				break;
			default:
				//do something?
				std::cout << "why?" << std::endl;
				break;
			}
		}
        else if ( resultInt == 0 )
            std::cout << "Connection closed" << std::endl;

        else
			std::cout << "recv failed with error: " << WSAGetLastError() << std::endl;

    } while( resultInt > 0 );

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

	std::cout << "Step 5: Disconnect" << std::endl;

	//Keep the window open
	std::cout << "\nwaiting on exit";
	int tempInput;
	std::cin >> tempInput;

    return 0;
}
