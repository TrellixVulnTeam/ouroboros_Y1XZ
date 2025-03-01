// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com

#ifndef OURO_SELECT_POLLER_H
#define OURO_SELECT_POLLER_H

#include "event_poller.h"

namespace Ouroboros { 
namespace Network
{

#ifndef HAS_EPOLL

class SelectPoller : public EventPoller
{
public:
	SelectPoller();

protected:
	virtual bool doRegisterForRead(int fd);
	virtual bool doRegisterForWrite(int fd);

	virtual bool doDeregisterForRead(int fd);
	virtual bool doDeregisterForWrite(int fd);

	virtual int processPendingEvents(double maxWait);

private:
	void handleNotifications(int &countReady,
			fd_set &readFDs, fd_set &writeFDs);

	fd_set						fdReadSet_;
	fd_set						fdWriteSet_;

	// The last registered socket descriptor (read or write)
	int							fdLargest_;

	// Registered number of socket descriptors written
	int							fdWriteCount_;
};


#endif // HAS_EPOLL

}
}
#endif // OURO_SELECT_POLLER_H
