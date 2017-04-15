#include "cChatroom.h"
#include "winsock2.h"
#include "Buffer.h"
#include "eMessageID.h"
#include <iostream>
#include "Headers.pb.h"

cChatroom::cChatroom(std::string name)
{
	mName = name;
}

cChatroom::~cChatroom()
{

}

void cChatroom::broadcastToRoom(std::string message, int sender)
{
	headers::Container* protoContainer = new headers::Container();
	headers::Message* protoMessage = new headers::Message();
	int resultInt = 0;
	int bytesLeftToSend;
	//Buffer sendBuf(0);
	//int packetLength = 2 + 2  + 2 + 2 + this->mName.length() + 2 + message.length();
	//sendBuf.writeInt16BE(packetLength);	//totalpacketsize = packetLength + messageID + name + roomLength + roomName + messageLength + message
	//protoMessage->set_packetlength(packetLength);
	//sendBuf.writeInt16BE(MESSAGE_ID_MESSAGE);	//messageID
	protoContainer->set_id(headers::Container_requestID_ID_MESSAGE);
	//sendBuf.writeInt16BE(sender);	//socket of sender
	//protoMessage->set_username(sender);
	//sendBuf.writeStringBE(mName);	//the room name itself
	protoMessage->set_roomname(mName);
	//sendBuf.writeStringBE(message);
	protoMessage->set_message(message);
	
	protoMessage->set_username(this->mUsers.find(sender)->second);

	int packetLength = protoContainer->ByteSize() + 2;
	protoContainer->set_packetlength(packetLength);
	
	std::string serializedString;
	protoContainer->SerializeToString(&serializedString);
	const char* sendArray = serializedString.c_str();//sendBuf.toCharArray();

	bytesLeftToSend = packetLength;
	for (std::map<int, std::string>::iterator it = mUsers.begin(); it != mUsers.end(); it++)
	{
		while (bytesLeftToSend > 0)
		{
			if (mUsers.find(it->first)->first == sender)
			{
				break;
			}
			resultInt = send(mUsers.find(it->first)->first, sendArray, packetLength, 0);
			if (resultInt < 0)
			{
				break;
			}

			bytesLeftToSend -= resultInt;
			sendArray += resultInt;
		}
		std::cout << packetLength << " Sent to: " << mUsers.find(it->first)->first << std::endl;
		bytesLeftToSend = packetLength;
		sendArray = serializedString.c_str();//sendBuf.toCharArray();
		
	}
}

void cChatroom::broadcastJoinToRoom( int newMember)
{
	headers::Container* protoContainer = new headers::Container();
	headers::Message* protoMessage = new headers::Message();
	int resultInt = 0;
	int bytesLeftToSend;
	//Buffer sendBuf(0);
	std::string name = mUsers.find(newMember)->second;
	std::string message = name + " has joined the room.";
//	int packetLength = 2 + 2 + 2 + this->mName.length() + 2 + message.length();
	//sendBuf.writeInt16BE(packetLength);	//totalpacketsize = packetLength + messageID + roomLength + roomName + messageLength + message
	//protoMessage->set_packetlength(packetLength);
	//sendBuf.writeInt16BE(MESSAGE_ID_JOIN);	//messageID
	protoContainer->set_id(headers::Container_requestID_ID_JOIN);
	//sendBuf.writeStringBE(mName);	//the room name itself
	protoMessage->set_username("");
	protoMessage->set_roomname(mName);
	//sendBuf.writeStringBE(message);
	protoMessage->set_message(message);
	protoContainer->set_allocated_message(protoMessage);
	int packetLength = protoContainer->ByteSize() + 2;
	protoContainer->set_packetlength(packetLength);

	std::string serializedString;
	protoContainer->SerializeToString(&serializedString);

	const char* sendArray = serializedString.c_str();//sendBuf.toCharArray();
	
	bytesLeftToSend = packetLength;
	
	//for (int index = 0; index != mUsers.size(); index++)
	for (std::map<int, std::string>::iterator it = mUsers.begin(); it != mUsers.end(); it++)
	{
		while (bytesLeftToSend > 0)
		{
			if (mUsers.find(it->first)->first == newMember)
			{
				break;
			}
			resultInt = send(mUsers.find(it->first)->first, sendArray, packetLength, 0);
			if (resultInt < 0)
			{
				break;
			}
			
			bytesLeftToSend -= resultInt;
			sendArray += resultInt;
		}
		bytesLeftToSend = packetLength;
		sendArray = serializedString.c_str();//sendBuf.toCharArray();
	}
}

void cChatroom::broadcastLeaveRoom(int oldMember)
{
	headers::Container* protoContainer = new headers::Container();
	headers::Message* protoMessage = new headers::Message();
	int resultInt = 0;
	int bytesLeftToSend;
	//Buffer sendBuf(0);
	std::string name = mUsers.find(oldMember)->second;
	std::string message = name + " has joined the room.";
	//int packetLength = 2 + 2 + 2 + this->mName.length() + 2 + message.length();
	//sendBuf.writeInt16BE(packetLength);	//totalpacketsize = packetLength + messageID + roomLength + roomName + messageLength + message
	//protoMessage->set_packetlength(packetLength);
	//sendBuf.writeInt16BE(MESSAGE_ID_LEAVE);	//messageID
	protoContainer->set_id(headers::Container_requestID_ID_LEAVE);
	//sendBuf.writeStringBE(mName);	//the room name itself
	protoMessage->set_username("");
	protoMessage->set_roomname(mName);
	//sendBuf.writeStringBE(message);
	protoMessage->set_message(message);

	protoContainer->set_allocated_message(protoMessage);
	int packetLength = protoContainer->ByteSize() + 2;
	protoContainer->set_packetlength(packetLength);

	std::string serializedString;
	protoContainer->SerializeToString(&serializedString);

	const char* sendArray = serializedString.c_str();// sendBuf.toCharArray();

	bytesLeftToSend = packetLength;
	//for (int index = 0; index != mMemberSockets.size(); index++)
	for (std::map<int, std::string>::iterator it = mUsers.begin(); it != mUsers.end(); it++)
	{
		while (bytesLeftToSend > 0)
		{
			resultInt = send(mUsers.find(it->first)->first, sendArray, packetLength, 0);
			if (resultInt < 0)
			{
				break;
			}

			bytesLeftToSend -= resultInt;
			sendArray += resultInt;
		}
		bytesLeftToSend = packetLength;
		sendArray = serializedString.c_str();//sendBuf.toCharArray();
	}

	removeMember(oldMember);
}

void cChatroom::addMember(int memberSocket, std::string username)
{
	mMemberSockets.push_back(memberSocket);
	mUsers.insert(std::pair<int, std::string>(memberSocket, username));
}

void cChatroom::removeMember(int memberSocket)
{
	mUsers.erase(memberSocket);
	/*for (std::map<int, std::string>::iterator it = mUsers.begin(); it != mUsers.end(); it++)
	{
		if (mUsers.find(it->first)->first == memberSocket)
		{
			mUsers.erase()
			mMemberSockets.erase(mMemberSockets.begin() + index);
			break;
		}
	}
	mMemberSockets.shrink_to_fit();
	mUsers.*/
}

std::string cChatroom::getName()
{
	return mName;
}