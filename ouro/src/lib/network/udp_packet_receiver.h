// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com

#ifndef OURO_NETWORKUDPPACKET_RECEIVER_H
#define OURO_NETWORKUDPPACKET_RECEIVER_H

#include "common/common.h"
#include "common/timer.h"
#include "common/objectpool.h"
#include "helper/debug_helper.h"
#include "network/common.h"
#include "network/interfaces.h"
#include "network/udp_packet.h"
#include "network/packet_receiver.h"

namespace Ouroboros { 
namespace Network
{
class Socket;
class Channel;
class Address;
class NetworkInterface;
class EventDispatcher;

class UDPPacketReceiver : public PacketReceiver
{
public:
	typedef OUROShared_ptr< SmartPoolObject< UDPPacketReceiver > > SmartPoolObjectPtr;
	static SmartPoolObjectPtr createSmartPoolObj(const std::string& logPoint);
	static ObjectPool<UDPPacketReceiver>& ObjPool();
	static UDPPacketReceiver* createPoolObject(const std::string& logPoint);
	static void reclaimPoolObject(UDPPacketReceiver* obj);
	static void destroyObjPool();

	UDPPacketReceiver():PacketReceiver(){}
	UDPPacketReceiver(EndPoint & endpoint, NetworkInterface & networkInterface);
	virtual ~UDPPacketReceiver();

	Reason processFilteredPacket(Channel* pChannel, Packet * pPacket);
	
	virtual PacketReceiver::PACKET_RECEIVER_TYPE type() const
	{
		return UDP_PACKET_RECEIVER;
	}

	virtual ProtocolSubType protocolSubType() const {
		return SUB_PROTOCOL_UDP;
	}

	virtual bool processRecv(UDPPacket* pReceiveWindow);
	virtual bool processRecv(bool expectingPacket);

	virtual Channel* findChannel(const Address& addr);

protected:
	PacketReceiver::RecvState checkSocketErrors(int len, bool expectingPacket);

protected:

};

}
}

#ifdef CODE_INLINE
#include "udp_packet_receiver.inl"
#endif
#endif // OURO_NETWORKUDPPACKET_RECEIVER_H
