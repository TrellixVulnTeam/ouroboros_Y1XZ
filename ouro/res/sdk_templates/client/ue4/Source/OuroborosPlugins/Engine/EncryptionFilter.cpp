#include "EncryptionFilter.h"
#include "MemoryStream.h"
#include "MessageReader.h"
#include "PacketSenderTCP.h"
#include "PacketSenderKCP.h"
#include "Engine/KBDebug.h"

namespace Ouroboros
{

EncryptionFilter::~EncryptionFilter()
{
}

BlowfishFilter::BlowfishFilter(int keySize) :
	isGood_(false),
	pPacket_(new MemoryStream()),
	pEncryptStream_(new MemoryStream()),
	packetLen_(0),
	padSize_(0),
	key_(),
	keySize_(0),
	pBlowFishKey_(NULL)
{
	unsigned char buf[20] = "";
	RAND_bytes(buf, 20);
	key_ = (char *)buf;
	keySize_ = key_.Len();

	init();
}

BlowfishFilter::BlowfishFilter(const FString & key) :
	isGood_(false),
	pPacket_(new MemoryStream()),
	pEncryptStream_(new MemoryStream()),
	packetLen_(0),
	padSize_(0),
	key_(key),
	keySize_(key_.Len()),
	pBlowFishKey_(NULL)
{
	init();
}

BlowfishFilter::~BlowfishFilter()
{
	OURO_SAFE_RELEASE(pPacket_);
	OURO_SAFE_RELEASE(pEncryptStream_);
	OURO_SAFE_RELEASE(pBlowFishKey_);
}

bool BlowfishFilter::init()
{
	pBlowFishKey_ = new BF_KEY;

	if (MIN_KEY_SIZE <= keySize_ && keySize_ <= MAX_KEY_SIZE)
	{

		BF_set_key(this->pBlowFishKey(), key_.Len(), reinterpret_cast<const unsigned char*>(TCHAR_TO_ANSI(*key_)));
		isGood_ = true;
	}
	else
	{
		ERROR_MSG("BlowfishFilter::init: invalid length %d", key_.Len());
		isGood_ = false;
	}

	return isGood_;
}

void BlowfishFilter::encrypt(MemoryStream *pMemoryStream)
{
	// BlowFish can only encrypt and decrypt 8 bytes of data at a time
	// less than 8 bytes, padding 0
	uint8 padSize = 0;

	if (pMemoryStream->length() % BLOCK_SIZE != 0)
	{
		padSize = BLOCK_SIZE - pMemoryStream->length() % BLOCK_SIZE;
		pMemoryStream->data_resize(pMemoryStream->size() + padSize);
		memset(pMemoryStream->data() + pMemoryStream->wpos(), 0, padSize);
		pMemoryStream->wpos(pMemoryStream->wpos() + padSize);
	}

	encrypt(pMemoryStream->data() + pMemoryStream->rpos(), pMemoryStream->length());

	uint16 packetLen = pMemoryStream->length() + 1;
	pEncryptStream_->writeUint16(packetLen);
	pEncryptStream_->writeUint8(padSize);
	pEncryptStream_->append(pMemoryStream->data() + pMemoryStream->rpos(), pMemoryStream->length());

	pMemoryStream->swap(*pEncryptStream_);
	pEncryptStream_->clear(false);
}

void BlowfishFilter::encrypt(uint8 *buf, MessageLengthEx len)
{
	if (len % BLOCK_SIZE != 0)
	{
		ERROR_MSG("BlowfishFilter::encrypt: Input length (%d) is not a multiple of block size ", len);
		return;
	}

	uint8* data = buf;
	uint64 prevBlock = 0;
	for (uint32 i = 0; i < len; i += BLOCK_SIZE)
	{
		if (prevBlock != 0)
		{
			uint64 oldValue = *(uint64*)(data + i);
			*(uint64*)(data + i) = *(uint64*)(data + i) ^ (prevBlock);
			prevBlock = oldValue;
		}
		else
		{
			prevBlock = *(uint64*)(data + i);
		}

		BF_ecb_encrypt(data + i, data + i, this->pBlowFishKey(), BF_ENCRYPT);
	}
}

void BlowfishFilter::encrypt(uint8 *buf, MessageLengthEx offset, MessageLengthEx len)
{
	encrypt(buf + offset, len);
}

void BlowfishFilter::decrypt(MemoryStream *pMemoryStream)
{
	decrypt(pMemoryStream->data() + pMemoryStream->rpos(), pMemoryStream->length());
}

void BlowfishFilter::decrypt(uint8 *buf, MessageLengthEx len)
{
	if (len % BLOCK_SIZE != 0)
	{
		ERROR_MSG("BlowfishFilter::decrypt: Input length (%d) is not a multiple of block size ", len);
		return;
	}

	uint8* data = buf;
	uint64 prevBlock = 0;
	for (uint32 i = 0; i < len; i += BLOCK_SIZE)
	{
		BF_ecb_encrypt(data + i, data + i, this->pBlowFishKey(), BF_DECRYPT);

		if (prevBlock != 0)
		{
			*(uint64*)(data + i) = *(uint64*)(data + i) ^ (prevBlock);
		}

		prevBlock = *(uint64*)(data + i);
	}
}

void BlowfishFilter::decrypt(uint8 *buf, MessageLengthEx offset, MessageLengthEx len)
{
	decrypt(buf + offset, len);
}

bool BlowfishFilter::send(PacketSenderBase* pPacketSender, MemoryStream *pPacket)
{
	if (!isGood_)
	{
		ERROR_MSG("BlowfishFilter::send: Dropping packet due to invalid filter");
		return false;
	}

	encrypt(pPacket);
	return pPacketSender->send(pPacket);;
}

bool BlowfishFilter::recv(MessageReader* pMessageReader, MemoryStream *pPacket)
{
	if (!isGood_)
	{
		ERROR_MSG("BlowfishFilter::recv: Dropping packet due to invalid filter");
		return false;
	}

	uint32 oldrpos = pPacket->rpos();
	uint32 len = pPacket->length();
	uint16 packeLen = pPacket->readUint16();

	if (0 == pPacket_->length() && len > MIN_PACKET_SIZE && packeLen - 1 == len - 3)
	{
		int padSize = pPacket->readUint8();
		decrypt(pPacket);

		if (pMessageReader)
		{
			pMessageReader->process(pPacket->data() + pPacket->rpos(), 0, pPacket->length() - padSize);
		}

		pPacket->clear(false);
		return true;
	}

	pPacket->rpos(oldrpos);
	pPacket_->append(pPacket->data() + pPacket->rpos(), pPacket->length());
	pPacket->clear(false);

	while (pPacket_->length() > 0)
	{
		uint32 currLen = 0;
		int oldwpos = 0;
		if (packetLen_ <= 0)
		{
			if (pPacket_->length() >= MIN_PACKET_SIZE)
			{
				(*pPacket_) >> packetLen_;
				(*pPacket_) >> padSize_;

				packetLen_ -= 1;

				if (pPacket_->length() > packetLen_)
				{
					currLen = (uint32)(pPacket_->rpos() + packetLen_);
					oldwpos = pPacket_->wpos();
					pPacket_->wpos(currLen);
				}
				else if (pPacket_->length() < packetLen_)
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			if (pPacket_->length() > packetLen_)
			{
				currLen = (uint32)(pPacket_->rpos() + packetLen_);
				oldwpos = pPacket_->wpos();
				pPacket_->wpos(currLen);
			}
			else if (pPacket_->length() < packetLen_)
			{
				return false;
			}
		}

		decrypt(pPacket_);
		pPacket_->wpos(pPacket_->wpos() - padSize_);

		if (pMessageReader)
		{
			pMessageReader->process(pPacket_->data() + pPacket_->rpos(), 0, pPacket_->length());
		}

		if (currLen > 0)
		{
			pPacket_->rpos(currLen);
			pPacket_->wpos(oldwpos);
		}
		else
		{
			pPacket_->clear(false);
		}

		packetLen_ = 0;
		padSize_ = 0;
	}
	return true;
}

}