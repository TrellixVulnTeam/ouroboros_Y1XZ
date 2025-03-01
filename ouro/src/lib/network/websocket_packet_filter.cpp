// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com


#include "websocket_packet_filter.h"
#include "websocket_protocol.h"

#include "network/bundle.h"
#include "network/channel.h"
#include "network/tcp_packet.h"
#include "network/network_interface.h"
#include "network/packet_receiver.h"

namespace Ouroboros { 
namespace Network
{

//-------------------------------------------------------------------------------------
WebSocketPacketFilter::WebSocketPacketFilter(Channel* pChannel):
	pFragmentDatasRemain_(0),
	fragmentDatasFlag_(FRAGMENT_MESSAGE_HREAD),
	msg_opcode_(0),
	msg_fin_(0),
	msg_masked_(0),
	msg_mask_(0),
	msg_length_field_(0),
	msg_payload_length_(0),
	msg_frameType_(websocket::WebSocketProtocol::ERROR_FRAME),
	pChannel_(pChannel),
	pTCPPacket_(NULL)
{
}

//-------------------------------------------------------------------------------------
WebSocketPacketFilter::~WebSocketPacketFilter()
{
	reset();
}

//-------------------------------------------------------------------------------------
void WebSocketPacketFilter::reset()
{
	msg_opcode_ = 0;
	msg_fin_ = 0;
	msg_masked_ = 0;
	msg_mask_ = 0;
	msg_length_field_ = 0;
	msg_payload_length_ = 0;
	pFragmentDatasRemain_ = 0;
	fragmentDatasFlag_ = FRAGMENT_MESSAGE_HREAD;

	TCPPacket::reclaimPoolObject(pTCPPacket_);
	pTCPPacket_ = NULL;
}

//-------------------------------------------------------------------------------------
Reason WebSocketPacketFilter::send(Channel * pChannel, PacketSender& sender, Packet * pPacket, int userarg)
{
	if(pPacket->encrypted())
		return PacketFilter::send(pChannel, sender, pPacket, userarg);

	Bundle* pBundle = pPacket->pBundle();
	TCPPacket* pRetTCPPacket = TCPPacket::createPoolObject(OBJECTPOOL_POINT);
	websocket::WebSocketProtocol::FrameType frameType = websocket::WebSocketProtocol::BINARY_FRAME;

	if (pBundle)
	{
		Bundle::Packets& packs = pBundle->packets();

		if (packs.size() > 1)
		{
			bool isEnd = packs.back() == pPacket;
			bool isBegin = packs.front() == pPacket;

			if (!isEnd && !isBegin)
			{
				frameType = websocket::WebSocketProtocol::NEXT_FRAME;
			}
			else
			{
				if (!isEnd)
				{
					frameType = websocket::WebSocketProtocol::INCOMPLETE_BINARY_FRAME;
				}
				else
				{
					frameType = websocket::WebSocketProtocol::END_FRAME;
				}
			}
		}
	}

	websocket::WebSocketProtocol::makeFrame(frameType, pPacket, pRetTCPPacket);

	int space = pPacket->length() - pRetTCPPacket->space();
	if(space > 0)
	{
		WARNING_MSG(fmt::format("WebSocketPacketFilter::send: no free space, buffer added:{}, total={}.\n",
			space, pRetTCPPacket->size()));

		pRetTCPPacket->data_resize(pRetTCPPacket->size() + space);
	}

	(*pRetTCPPacket).append(pPacket->data() + pPacket->rpos(), pPacket->length());
	pRetTCPPacket->swap(*(static_cast<Ouroboros::MemoryStream*>(pPacket)));
	TCPPacket::reclaimPoolObject(pRetTCPPacket);

	pPacket->encrypted(true);
	return PacketFilter::send(pChannel, sender, pPacket, userarg);
}

//-------------------------------------------------------------------------------------
Reason WebSocketPacketFilter::recv(Channel * pChannel, PacketReceiver & receiver, Packet * pPacket)
{
	while (pPacket->length() > 0)
	{
		if (fragmentDatasFlag_ == FRAGMENT_MESSAGE_HREAD)
		{
			if (pFragmentDatasRemain_ == 0)
			{
				OURO_ASSERT(pTCPPacket_ == NULL);

				size_t rpos = pPacket->rpos();

				reset();

				// If no cache has been created, try to parse the header directly. If the information is sufficiently parsed, continue to the next step.
				pFragmentDatasRemain_ = websocket::WebSocketProtocol::getFrame(pPacket, msg_opcode_, msg_fin_, msg_masked_,
					msg_mask_, msg_length_field_, msg_payload_length_, msg_frameType_);

				if (pFragmentDatasRemain_ > 0)
				{
					pPacket->rpos(rpos);
					pTCPPacket_ = TCPPacket::createPoolObject(OBJECTPOOL_POINT);
					pTCPPacket_->append(*(static_cast<MemoryStream*>(pPacket)));
					pPacket->done();
				}
				else
				{
					fragmentDatasFlag_ = FRAGMENT_MESSAGE_DATAS;
					pFragmentDatasRemain_ = (int32)msg_payload_length_;
				}
			}
			else
			{
				OURO_ASSERT(pTCPPacket_ != NULL);

				// If the length is greater than the remaining read length, then you can start parsing
				// Otherwise the package memory continues to be cached
				if ((int32)pPacket->length() >= pFragmentDatasRemain_)
				{
					size_t wpos = pPacket->wpos();
					size_t rpos = pPacket->rpos();

					pPacket->wpos(rpos + pFragmentDatasRemain_);

					// First add the required data to pTCPPacket_
					pTCPPacket_->append(*(static_cast<MemoryStream*>(pPacket)));

					// Restore the write location back
					pPacket->wpos(wpos);

					// discard data that has already been read
					pPacket->read_skip(pFragmentDatasRemain_);

					size_t buffer_rpos = pTCPPacket_->rpos();
					pFragmentDatasRemain_ = websocket::WebSocketProtocol::getFrame(pTCPPacket_, msg_opcode_, msg_fin_, msg_masked_,
						msg_mask_, msg_length_field_, msg_payload_length_, msg_frameType_);

					// If it is still greater than 0, it means that it needs to continue to receive the package.
					if (pFragmentDatasRemain_ > 0)
					{
						// Since there is no parsing at a time, we retire the data and try to parse again next time.
						pTCPPacket_->rpos(buffer_rpos);

						// If the current package has data and is greater than or equal to the data we need, continue to the next loop and parse immediately.
						if ((int32)pPacket->length() >= pFragmentDatasRemain_)
							continue;
					}
					else
					{
						// frame parsing is completed, the object is recycled
						TCPPacket::reclaimPoolObject(pTCPPacket_);
						pTCPPacket_ = NULL;

						// Is there data to carry? If not, do not enter data parsing
						if (msg_payload_length_ > 0)
						{
							fragmentDatasFlag_ = FRAGMENT_MESSAGE_DATAS;
							pFragmentDatasRemain_ = (int32)msg_payload_length_;
						}
					}
				}
				else
				{
					pTCPPacket_->append(*(static_cast<MemoryStream*>(pPacket)));
					pFragmentDatasRemain_ -= pPacket->length();

					pPacket->done();
				}
			}

			if (websocket::WebSocketProtocol::ERROR_FRAME == msg_frameType_)
			{
				ERROR_MSG(fmt::format("WebSocketPacketFilter::recv: frame error! addr={}!\n",
					pChannel_->c_str()));

				this->pChannel_->condemn("WebSocketPacketFilter::recv: frame error!");
				reset();

				TCPPacket::reclaimPoolObject(static_cast<TCPPacket*>(pPacket));
				return REASON_WEBSOCKET_ERROR;
			}
			else if (msg_frameType_ == websocket::WebSocketProtocol::TEXT_FRAME ||
				msg_frameType_ == websocket::WebSocketProtocol::INCOMPLETE_TEXT_FRAME ||
				msg_frameType_ == websocket::WebSocketProtocol::PONG_FRAME)
			{
				ERROR_MSG(fmt::format("WebSocketPacketFilter::recv: Does not support FRAME_TYPE({})! addr={}!\n",
					websocket::WebSocketProtocol::getFrameTypeName(msg_frameType_), pChannel_->c_str()));

				this->pChannel_->condemn("WebSocketPacketFilter::recv: Does not support FRAME_TYPE");
				reset();

				TCPPacket::reclaimPoolObject(static_cast<TCPPacket*>(pPacket));
				return REASON_WEBSOCKET_ERROR;
			}
			else if (msg_frameType_ == websocket::WebSocketProtocol::CLOSE_FRAME)
			{
				this->pChannel_->condemn("");
				reset();

				TCPPacket::reclaimPoolObject(static_cast<TCPPacket*>(pPacket));
				return REASON_SUCCESS;
			}
			else if (msg_frameType_ == websocket::WebSocketProtocol::INCOMPLETE_FRAME)
			{
				// continue to wait for subsequent content to arrive
			}
			else if (msg_frameType_ == websocket::WebSocketProtocol::PING_FRAME)
			{
				if (pFragmentDatasRemain_ <= 0)
				{
					Reason reason = onPing(pChannel, pPacket);
					if (reason != REASON_SUCCESS)
					{
						reset();

						TCPPacket::reclaimPoolObject(static_cast<TCPPacket*>(pPacket));
						return reason;
					}
				}

				continue;
			}
		}
		else
		{
			if (pFragmentDatasRemain_ <= 0)
			{
				ERROR_MSG(fmt::format("WebSocketPacketFilter::recv: pFragmentDatasRemain_ <= 0! addr={}!\n",
					pChannel_->c_str()));

				this->pChannel_->condemn("WebSocketPacketFilter::recv: pFragmentDatasRemain_ <= 0!");
				reset();

				TCPPacket::reclaimPoolObject(static_cast<TCPPacket*>(pPacket));
				return REASON_WEBSOCKET_ERROR;
			}

			if (pTCPPacket_ == NULL)
				pTCPPacket_ = TCPPacket::createPoolObject(OBJECTPOOL_POINT);

			if (pFragmentDatasRemain_ <= (int32)pPacket->length())
			{
				pTCPPacket_->append(pPacket->data() + pPacket->rpos(), pFragmentDatasRemain_);
				pPacket->read_skip((size_t)pFragmentDatasRemain_);
				pFragmentDatasRemain_ = 0;
			}
			else
			{
				pTCPPacket_->append(*(static_cast<MemoryStream*>(pPacket)));
				pFragmentDatasRemain_ -= pPacket->length();
				pPacket->done();
			}

			Reason reason = REASON_SUCCESS;

			if (msg_frameType_ == websocket::WebSocketProtocol::PING_FRAME)
			{
				// Continue to wait until the rest of the content arrives
				if (pFragmentDatasRemain_ > 0)
					continue;

				if (!websocket::WebSocketProtocol::decodingDatas(pTCPPacket_, msg_masked_, msg_mask_))
				{
					ERROR_MSG(fmt::format("WebSocketPacketFilter::recv: decoding-frame error! addr={}!\n",
						pChannel_->c_str()));

					this->pChannel_->condemn("WebSocketPacketFilter::recv: decoding-frame error!");
					reset();

					TCPPacket::reclaimPoolObject(static_cast<TCPPacket*>(pPacket));
					return REASON_WEBSOCKET_ERROR;
				}

				reason = onPing(pChannel, pTCPPacket_);
			}
			else
			{
				if (!websocket::WebSocketProtocol::decodingDatas(pTCPPacket_, msg_masked_, msg_mask_))
				{
					ERROR_MSG(fmt::format("WebSocketPacketFilter::recv: decoding-frame error! addr={}!\n",
						pChannel_->c_str()));

					this->pChannel_->condemn("WebSocketPacketFilter::recv: decoding-frame error!");
					reset();

					TCPPacket::reclaimPoolObject(static_cast<TCPPacket*>(pPacket));
					return REASON_WEBSOCKET_ERROR;
				}

				reason = PacketFilter::recv(pChannel, receiver, pTCPPacket_);
				OURO_ASSERT(reason == REASON_SUCCESS);

				// pTCPPacket_ does not need to be recycled here
				pTCPPacket_ = NULL;
			}

			if (pFragmentDatasRemain_ == 0)
				reset();

			if (reason != REASON_SUCCESS)
			{
				TCPPacket::reclaimPoolObject(static_cast<TCPPacket*>(pPacket));
				reset();
				return reason;
			}
		}
	}

	TCPPacket::reclaimPoolObject(static_cast<TCPPacket*>(pPacket));
	return REASON_SUCCESS;
}

//-------------------------------------------------------------------------------------
Reason WebSocketPacketFilter::onPing(Channel * pChannel, Packet* pPacket)
{
	OURO_ASSERT(pFragmentDatasRemain_ == 0);

	TCPPacket* pPongPacket = TCPPacket::createPoolObject(OBJECTPOOL_POINT);
	websocket::WebSocketProtocol::makeFrame(websocket::WebSocketProtocol::PONG_FRAME, pPacket, pPongPacket);

	if (msg_payload_length_ > 0)
	{
		pPongPacket->append(pPacket->data() + pPacket->rpos(), msg_payload_length_);
		pPacket->read_skip((size_t)msg_payload_length_);
	}

	int sendSize = pPongPacket->length();

	while (sendSize > 0)
	{
		int ret = pChannel->pEndPoint()->send(pPongPacket->data() + (pPongPacket->length() - sendSize), sendSize);
		if (ret <= 0)
		{
			ERROR_MSG(fmt::format("WebSocketPacketFilter::recv: send({}) pong-frame error! addr={}, sendSize={}\n",
				ret, pChannel_->c_str(), sendSize));

			break;
		}

		sendSize -= ret;
	}

	TCPPacket::reclaimPoolObject(pPongPacket);

	pFragmentDatasRemain_ = 0;
	fragmentDatasFlag_ = FRAGMENT_MESSAGE_HREAD;
	return REASON_SUCCESS;
}

//-------------------------------------------------------------------------------------
} 
}
