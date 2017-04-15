#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "sClientInfo.h"
#include "cChatroom.h"
#include <vector>
#include "Headers.pb.h"
#include "sha.h"
#include "sqlite3.h"
#include "Utilities.h"

//#include "Buffer.h"
//#include "eMessageID.h"

// for windows socket link
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

std::vector<sClientInfo*> g_vecClientInfo;
std::vector<cChatroom*> g_vecChatrooms;

void genSalt(std::string &salt)
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < 32; ++i) {
		int randIndex = rand() % (sizeof(alphanum) - 1);
		salt[i] = alphanum[randIndex];//rand() % (sizeof(alphanum) - 1)];
	}

	salt[32] = 0;
}

int main() 
{
	
	//Buffer buffer(0);

	//Step 1
	//Initial winsock
	WSADATA wsaData; //Create a Windows Socket Application Data Object

	int resultInt;

	resultInt = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (resultInt != 0)
	{
		std::cout << "WinSock Initalization failed" << std::endl;
		return 1;
	}

	//Step 2
	//Create a Socket
	struct addrinfo *result = NULL;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints)); //Fills a block of memory with zeros
	hints.ai_family = AF_INET; //Unspecified so either IPv4 or IPv6 address can be returned
	hints.ai_socktype = SOCK_STREAM; //Stream must be specified for TCP
	hints.ai_protocol = IPPROTO_TCP; //Protocol is TCP
	hints.ai_flags = AI_PASSIVE;


	resultInt = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (resultInt != 0)
	{
		std::cout << "Socket Initalization failed" << std::endl;
		WSACleanup(); //will nicely kill our WinSock instance
		return 1;
	}

	std::cout << "Step 1: WinSock Initalized" << std::endl;

	//Copy the result object pointer
	ptr = result;

	SOCKET listeningSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	if (listeningSocket == INVALID_SOCKET)
	{
		std::cout << "Socket Initalization failed" << std::endl;
		freeaddrinfo(result); //free memory allocated to provent memory leaks
		WSACleanup(); //will nicely kill our WinSock instance
		return 1;
	}



	/*NEW STUFF OCT 3RD */

	//We need to switch the socket to be non blocking
	// If iMode!=0, non-blocking mode is enabled.
	u_long iMode = 1;
	ioctlsocket(listeningSocket, FIONBIO, &iMode);

	/****/


	std::cout << "Step 2: Socket Created" << std::endl;

	//Step3 
	//Bind the Socket
	
	resultInt = bind(listeningSocket, result->ai_addr, (int)result->ai_addrlen);

	if (listeningSocket == INVALID_SOCKET)
	{
		std::cout << "Socket binding failed" << std::endl;
		freeaddrinfo(result); //free memory allocated to provent memory leaks
		closesocket(listeningSocket); //Close the socket
		WSACleanup(); //will nicely kill our WinSock instance
		return 1;
	}

	freeaddrinfo(result); //Once bind is called the address info is no longer needed so free the memory allocated

	std::cout << "Step 3: Socket Bound" << std::endl;

	//Step 4
	//Listen for a client connection

	if (listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "Socket listening failed" << std::endl;
		closesocket(listeningSocket); //Close the socket
		WSACleanup(); //will nicely kill our WinSock instance
		return 1;
	}

	//Create DB
	sqlite3 *database;
	char* SQLErrorMessage = 0;
	int returnCode;

	//attempt to open the database
	returnCode = sqlite3_open("Auth.db", &database);

	if (returnCode)
	{
		std::cout << "Can't open database: " << sqlite3_errmsg(database) << std::endl;
		return 0;
	}
	else
	{
		std::cout << "Database opened" << std::endl;
	}

	//create the two tables
	const char* sql = "CREATE TABLE auth(" \
		"userName VARCHAR(255),"\
		"salt CHAR(64),"\
		"hashedPass CHAR(64),"\
		"userID BIGINT);";

	returnCode = sqlite3_exec(database, sql, NULL, 0, &SQLErrorMessage);
	if (returnCode != SQLITE_OK)
	{
		std::cout << "SQL error: " << SQLErrorMessage << std::endl;
		sqlite3_free(SQLErrorMessage);
	}
	else
	{
		std::cout << "Table created successfully auth" << std::endl;
	}

	 sql = "CREATE TABLE user(" \
		"userName VARCHAR(255),"\
		"lastLogin CHAR(64),"\
		"creationDate CHAR(64));";

	returnCode = sqlite3_exec(database, sql, NULL, 0, &SQLErrorMessage);
	if (returnCode != SQLITE_OK)
	{
		std::cout << "SQL error: " << SQLErrorMessage << std::endl;
		sqlite3_free(SQLErrorMessage);
	}
	else
	{
		std::cout << "Table created successfully user" << std::endl;
	}

	std::cout << "Step 4: Listening on Socket" << std::endl;


	/*NEW STUFF OCT 3RD */

	//Set up file descriptors for select statement
	fd_set master;    //Master list
	fd_set read_fds;  //Temp list for select() statement

					  //Clear the master and temp sets
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	//Add the listener to the master set
	FD_SET(listeningSocket, &master);

	//Keep track of the biggest file descriptor
	int newfd;        //Newly accepted socket descriptor
	int fdmax = listeningSocket; //Maximum file descriptor number (to start it's the listener)
	std::cout << "fdmax " << fdmax << std::endl;

	//Set timeout time
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500 * 1000; // 500 ms

	bool firstMessage = true;
	int currentBytes = 0;
	//Main loop
	while (true) //infinite loop, this could be scary
	{

		//Select function checks to see if any socket has activity
		read_fds = master; // copy the master to a temp
		if (select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1)
		{
			std::cout << "Select Error" << std::endl;
			exit(4);
		}

		//Loop through existing connections looking for data to read
		for (int i = 0; i <= fdmax; i++)
		{
			//If no flag is raised keep looping
			if (!FD_ISSET(i, &read_fds))
			{
				continue;
			}

			//If the raised flag is on the listening socket accept a new connection then keep looping
			if (i == listeningSocket)
			{

				// handle new connections
				sockaddr_in client;
				socklen_t addrlen = sizeof sockaddr_storage; //this represents the client address size
				newfd = accept(listeningSocket, (struct sockaddr *)&client, &addrlen);
				std::cout << "Connected to: " << client.sin_addr.s_addr << std::endl;
				sClientInfo* temp = new sClientInfo;
				temp->mySocket = newfd;
				g_vecClientInfo.push_back(temp);
				std::cout << "newfd " << newfd << std::endl;
				if (newfd == -1)
				{
					std::cout << "Accept Error" << std::endl;
					continue;
				}

				FD_SET(newfd, &master); // add to master set

				//Keep track of max fd
				if (newfd > fdmax)
					fdmax = newfd;

				std::cout << "New connection on socket " << newfd << std::endl;
				continue;
			}

			/////////////////////////////////
			// Recieve an incoming message //
			/////////////////////////////////

			char recvbuf[DEFAULT_BUFLEN];
			int incomingMessageLength = 0;
			std::string recvString;
			std::string outString;
			
			//Recieve the message
			incomingMessageLength = recv(i, recvbuf, sizeof recvbuf, 0);
			currentBytes += incomingMessageLength;
		

			if (incomingMessageLength > 0)
			{
				std::cout << "Bytes received: " << incomingMessageLength << std::endl;
				headers::Container* protoContainer = new headers::Container();
				const headers::Message* protoMessage = new headers::Message();
				const headers::CreateAccount* protoCreateMessage = new headers::CreateAccount();
				const headers::AuthenticateLogin* protoAuthenticateMessage = new headers::AuthenticateLogin();
				const headers::AuthenticateLogin* protoLogin = new headers::AuthenticateLogin();
				
				recvString.append(recvbuf, incomingMessageLength);
				outString.append(recvbuf, incomingMessageLength);
				
				
				//buffer.clear();
				//buffer.loadBuffer(recvbuf, incomingMessageLength);
				protoContainer->ParseFromString(recvString);

				//packet length
				int packetLength = protoContainer->packetlength();//buffer.readInt16BE();
				while (currentBytes < packetLength)
				{
					resultInt = recv(i, recvbuf, sizeof recvbuf, 0);

					outString.append(recvbuf, resultInt);
					//buffer.loadBuffer(recvbuf, resultInt);
					currentBytes += resultInt;
				}
				protoContainer->ParseFromString(outString);
				currentBytes = 0;
				//messageID
				std::string name;
				std::string password;
				int messageID = protoContainer->id();//buffer.readInt16BE();
				//short messageLength;
				std::string message;
				std::string hashPass;
				std::string salt;
				std::string insertString;
				std::string sqlString;
				const char* data = "empty";
				salt.resize(32);
				Records records;
				Records userRecord;
				//int roomLength;
				std::string roomName;
				bool bRoomExists = false;
				cChatroom* curRoom;

				switch (messageID)
				{
				case headers::Container_requestID_ID_JOIN:	//packet format = packetLength-messageID-roomLength-roomName
					//message length
					//roomLength = buffer.readShort16BE();
					//string 				
					protoMessage = &protoContainer->message();
					roomName = protoMessage->roomname();//buffer.readStringBE(roomLength);
					name = protoMessage->username();
					message = protoMessage->message();
					bRoomExists = false;
					
					if (g_vecChatrooms.empty())	//if no rooms exist on server create this room
					{
						curRoom = new cChatroom(roomName);
						curRoom->addMember(i, name);
						g_vecChatrooms.push_back(curRoom);
					}
					else
					{
						for (int index = 0; index != g_vecChatrooms.size(); index++)
						{
							if (roomName == g_vecChatrooms[index]->getName())
							{	//we found a room with the same name
								bRoomExists = true;	
								curRoom = g_vecChatrooms[index];	//set the current room to the found matching room
								curRoom->addMember(i, name);
								break;
							}
						}
						if (!bRoomExists)	//if the room doesn't exist already
						{
							curRoom = new cChatroom(roomName);	//crete the new room
							curRoom->addMember(i, name);	//add joining socket to list of members
							g_vecChatrooms.push_back(curRoom);	//add the room to the list of all the rooms
						}

					}
					curRoom->broadcastJoinToRoom(i);	//broadcast the "name" (at this point the socket) that joined the room to everyone in the room
					break;
				case headers::Container_requestID_ID_LEAVE:
					//roomLength = buffer.readInt16BE();
					protoMessage = &protoContainer->message();
					roomName = protoMessage->roomname();//buffer.readStringBE(roomLength);
					name = protoMessage->username();
					message = protoMessage->message();
					bRoomExists = false;
					if (g_vecChatrooms.empty())
					{
						//no chatrooms on server 
						//send error message to client

					}
					else
					{
						for (int index = 0; index != g_vecChatrooms.size(); index++)
						{
							if (roomName == g_vecChatrooms[index]->getName())
							{
								bRoomExists = true;
								curRoom = g_vecChatrooms[index];
								curRoom->broadcastLeaveRoom(i);
							}
						}
						if (!bRoomExists)
						{
							//uhoh
						}
					}
					break;
				case headers::Container_requestID_ID_MESSAGE:	//format is packageLength-MessageID-roomLength-RoomName-messageLength-Message
					//roomLength = buffer.readInt16BE();
					protoMessage = &protoContainer->message();
					roomName = protoMessage->roomname();//buffer.readStringBE(roomLength);
					name = protoMessage->username();
					//messageLength = buffer.readInt16BE();
					message = protoMessage->message();//buffer.readStringBE(messageLength);
					bRoomExists = false;

					if (g_vecChatrooms.empty())	//something went horribly horribly wrong
					{
						//send error to client
						break;
					}
					else
					{
						for (int index = 0; index != g_vecChatrooms.size(); index++)
						{
							if (roomName == g_vecChatrooms[index]->getName())	//found the room
							{
								bRoomExists = true;	//room exists
								curRoom = g_vecChatrooms[index];	
								curRoom->broadcastToRoom(message, i);
								break;	//break out of loop we found what we are looking for
							}
						}
						if (!bRoomExists)
						{
							//uhoh
						}
					}
					break;
				case headers::Container_requestID_ID_CREATE:
					//create an ID
					//protoCreateMessage->ParseFromString(outString);
					protoCreateMessage = &protoContainer->create();
					name = protoCreateMessage->username();
					password = protoCreateMessage->password();
					//generate salt
					genSalt(salt);
					password.append(salt);
					hashPass = sha256(password);
					
					insertString = "INSERT INTO AUTH (userName, salt, hashedPass, userID) "\
						"VALUES ('" + name + "', '" + salt + "' , '" + hashPass + "', " + std::to_string(i) + ");";
					sql = insertString.c_str();
					returnCode = sqlite3_exec(database, sql, NULL, 0, &SQLErrorMessage);
					if (returnCode != SQLITE_OK)
					{
						std::cout << "SQL error: " << SQLErrorMessage << std::endl;
						sqlite3_free(SQLErrorMessage);
						headers::Container* containerOut = new headers::Container();
						headers::CreateAccountFailure* protoCreateFailMessage = new headers::CreateAccountFailure();
						containerOut->set_id(headers::Container_requestID_ID_CREATE_FAIL);
						protoCreateFailMessage->set_errorreason(headers::CreateAccountFailure_reason_INTERNAL_SERVER_ERROR);
						containerOut->set_allocated_createfail(protoCreateFailMessage);
						packetLength = containerOut->ByteSize() + 2;
						containerOut->set_packetlength(packetLength);
						std::string serializedString;
						containerOut->SerializeToString(&serializedString);
						customSend((SOCKET)i, serializedString, (short)packetLength);
						break;
					}
					else
					{
						std::cout << "INSERT executed successfully" << std::endl;
					}
					insertString = "INSERT INTO USER (userName, lastLogin, creationDate) "\
						"VALUES ('" + name + "', 'Test', 'Test2');";
					sql = insertString.c_str();
					returnCode = sqlite3_exec(database, sql, NULL, 0, &SQLErrorMessage);
					if (returnCode != SQLITE_OK)
					{
						std::cout << "SQL error: " << SQLErrorMessage << std::endl;
						sqlite3_free(SQLErrorMessage);
						headers::Container* containerOut = new headers::Container();
						headers::CreateAccountFailure* protoCreateFailMessage = new headers::CreateAccountFailure();
						containerOut->set_id(headers::Container_requestID_ID_CREATE_FAIL);
						protoCreateFailMessage->set_errorreason(headers::CreateAccountFailure_reason_INTERNAL_SERVER_ERROR);
						containerOut->set_allocated_createfail(protoCreateFailMessage);
						packetLength = containerOut->ByteSize() + 2;
						containerOut->set_packetlength(packetLength);
						std::string serializedString;
						containerOut->SerializeToString(&serializedString);
						customSend((SOCKET)i, serializedString, (short)packetLength);
						break;
					}
					else
					{
						std::cout << "INSERT executed successfully" << std::endl;
					}
					{
					headers::Container* containerOut = new headers::Container();
					headers::CreateAccountSuccess* protoCreateSuccessMessage = new headers::CreateAccountSuccess();
					containerOut->set_id(headers::Container_requestID_ID_CREATE_SUCCESS);
					protoCreateSuccessMessage->set_userid("1");
					containerOut->set_allocated_createsuccess(protoCreateSuccessMessage);
					packetLength = containerOut->ByteSize() + 2;
					containerOut->set_packetlength(packetLength);
					
						std::string serializedString;
						containerOut->SerializeToString(&serializedString);

						customSend((SOCKET)i, serializedString, (short)packetLength);
					}
					//TODO: send success or fail message back to client
					break;
				case headers::Container_requestID_ID_LOGIN:
					//check against authentication table
					protoLogin = &protoContainer->auth();
					name = protoLogin->username();
					password = protoLogin->password();
					
					sqlString = "SELECT * FROM auth WHERE userName LIKE '" + name + "';";
					sql = sqlString.c_str();
					returnCode = sqlite3_exec(database, sql, selectCallback, &records, &SQLErrorMessage);
					if (returnCode != SQLITE_OK)
					{
						std::cout << "SELECT didn't work" << std::endl;
						break;
					}
					else
					{
						std::string authHashPass = records[0][2];
						std::string mySalt = records[0][1];
						password.append(mySalt);
						std::string testPass = sha256(password);
						//int id = records[0][3];
						if (testPass == authHashPass)
						{
							std::cout << "COrrect" << std::endl;
							headers::Container* containerOut = new headers::Container();
							headers::AuthenticateSuccess* authSuccess = new headers::AuthenticateSuccess();
							containerOut->set_id(headers::Container_requestID_ID_AUTHENTICATE_SUCCESS);
							authSuccess->set_userid("1");
							sqlString = "SELECT * FROM user WHERE userName LIKE '" + name + "';";
							sql = sqlString.c_str();
							returnCode = sqlite3_exec(database, sql, selectCallback, &userRecord, &SQLErrorMessage);
							authSuccess->set_creationdate(userRecord[0][2]);
							containerOut->set_allocated_authsuccess(authSuccess);
							packetLength = containerOut->ByteSize() + 2;
							containerOut->set_packetlength(packetLength);

							std::string serializedString;
							containerOut->SerializeToString(&serializedString);

							customSend((SOCKET)i, serializedString, (short)packetLength);
							//auth successful
						}
						else
						{
							//invalid credentials
						}

						std::cout << "SELECT worked" << std::endl;
					}
					break;
				default:

					break;
				}
			}
			else if (incomingMessageLength == 0)
			{
				std::cout << "Connection closing..." << std::endl;
				closesocket(i); //CLose the socket
				FD_CLR(i, &master); // remove from master set

				continue;
			}
			else
			{
				std::cout << "recv failed: " << WSAGetLastError() << std::endl;
				closesocket(i);
				WSACleanup();

				//You probably don't want to stop the server based on this in the real world
				return 1;
			}
		}
	}

	/****/


	//Step 7
	//Disconnect and Cleanup

	// shutdown the send half of the connection since no more data will be sent
	resultInt = shutdown(listeningSocket, SD_SEND);
	if (resultInt == SOCKET_ERROR)
	{
		std::cout << "shutdown failed:" << WSAGetLastError << std::endl;
		closesocket(listeningSocket);
		WSACleanup();
		return 1;
	}

	//Final clean up
	closesocket(listeningSocket);
	WSACleanup(); 

	std::cout << "Step 7: Disconnect" << std::endl;

	//Keep the window open
	std::cout << "\nwaiting on exit";
	int tempInput;
	std::cin >> tempInput;

	return 0;
}