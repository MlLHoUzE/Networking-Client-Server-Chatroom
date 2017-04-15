#ifndef _cChatroom_HG_
#define _cChatroom_HG_

#include <string>
#include <vector>
#include <map>

class cChatroom
{
public:
	cChatroom(std::string name);
	~cChatroom();

	void broadcastToRoom(std::string message, int sender);
	void broadcastJoinToRoom(int newMember);
	void broadcastLeaveRoom(int oldMember);
	void addMember(int memberSocket, std::string userName);
	void removeMember(int memberSocket);
	std::string getName();

private:
	std::string mName;
	std::vector<int> mMemberSockets;
	std::map<int, std::string> mUsers;
};

#endif