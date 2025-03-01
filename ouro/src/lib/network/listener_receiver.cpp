// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com


#include "listener_receiver.h"
#ifndef CODE_INLINE
#include "listener_receiver.inl"
#endif

#include "network/address.h"
#include "network/bundle.h"
#include "network/endpoint.h"
#include "network/event_dispatcher.h"
#include "network/network_interface.h"
#include "network/packet_receiver.h"
#include "network/error_reporter.h"

namespace Ouroboros { 
namespace Network
{
//-------------------------------------------------------------------------------------
ListenerReceiver::ListenerReceiver(EndPoint & endpoint,
								   Channel::Traits traits, 
									NetworkInterface & networkInterface	) :
	endpoint_(endpoint),
	traits_(traits),
	networkInterface_(networkInterface)
{
}

//-------------------------------------------------------------------------------------
ListenerReceiver::~ListenerReceiver()
{
}

//-------------------------------------------------------------------------------------
EventDispatcher & ListenerReceiver::dispatcher()
{
	return networkInterface_.dispatcher();
}

//-------------------------------------------------------------------------------------
}
}
