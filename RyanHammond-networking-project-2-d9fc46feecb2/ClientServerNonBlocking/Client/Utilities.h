#ifndef _Utilities_HG_
#define _Utilities_HG_

#include <string>

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

void customSend(LPVOID socket, Buffer buffer, short length)
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
	const char* sendArray = buffer.toCharArray();
	bytesLeftToSend = buffer.readInt16BE();
	while (bytesLeftToSend > 0)
	{
		resultInt = send((SOCKET)socket, sendArray, length, 0);
		if (resultInt < 0)
		{
			break;
		}

		bytesLeftToSend -= resultInt;
		sendArray += resultInt;
	}
}

void customSend(LPVOID socket, std::string buffer, short length)
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
		resultInt = send((SOCKET)socket, sendArray, length, 0);
		if (resultInt < 0)
		{
			break;
		}

		bytesLeftToSend -= resultInt;
		sendArray += resultInt;
	}
}


#endif