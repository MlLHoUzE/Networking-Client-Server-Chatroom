#ifndef _Utilities_HG_
#define _Utilities_HG_

#include <string>

using Record = std::vector<std::string>;
using Records = std::vector<Record>;
static int selectCallback(void *data, int argc, char **argv, char **azColName)
{
	
	for (int i = 0; i < argc; i++)
	{
		std::cout << azColName[i] << " = " << argv[i] << std::endl;
	}

	Records* records = static_cast<Records*>(data);
	try {
		records->emplace_back(argv, argv + argc);
	}
	catch (...) {
		// abort select on failure, don't let exception propogate thru sqlite3 call-stack
		return 1;
	}

	//just for spacing
	std::cout << std::endl;

	return 0;
}

void eraseLeadingSpaces(std::string &message)
{
	for (int index = 0; index != message.length(); index++)
	{
		if (message[0] == ' ')
		{
			message.erase(0, 1);
		}
		else
		{
			break;
		}
	}
}

void customSend(SOCKET socket, std::string buffer, short length)
{
	/*int resultInt = send((SOCKET)socket, buffer.toCharArray(), length, 0);
	if (resultInt == SOCKET_ERROR)
	{
	std::cout << "send failed with error: " << WSAGetLastError() << std::endl;
	closesocket((SOCKET)socket);
	WSACleanup();
	}*/
	int resultInt = 0;
	int bytesLeftToSend = 0;
	const char* sendArray = buffer.c_str();
	bytesLeftToSend = length;
	while (bytesLeftToSend > 0)
	{
		resultInt = send(socket, sendArray, length, 0);
		if (resultInt < 0)
		{
			break;
		}

		bytesLeftToSend -= resultInt;
		sendArray += resultInt;
	}
}


#endif