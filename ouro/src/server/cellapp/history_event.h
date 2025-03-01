// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com

#ifndef OURO_HISTORY_EVENT_H
#define OURO_HISTORY_EVENT_H

#include "helper/debug_helper.h"
#include "common/common.h"	


namespace Ouroboros{
namespace Network{
	class MessageHandler;
}

typedef uint32 HistoryEventID;

/**
	Describe a historical event
*/
class HistoryEvent
{
public:
	HistoryEvent(HistoryEventID id, const Network::MessageHandler& msgHandler, uint32 msglen);
	virtual ~HistoryEvent();

	HistoryEventID id() const{ return id_; }
	uint32 msglen() const { return msglen_; }

	void addMsgLen(uint32 v){ msglen_ += v; }

protected:
	HistoryEventID id_;
	uint32 msglen_;
	const Network::MessageHandler& msgHandler_;
};

/**
	Manage all historical events
*/
class EventHistory
{
public:
	EventHistory();
	virtual ~EventHistory();

	bool add(HistoryEvent* phe);
	bool remove(HistoryEvent* phe);
protected:
	std::map<HistoryEventID, HistoryEvent*> events_;
};

}
#endif
