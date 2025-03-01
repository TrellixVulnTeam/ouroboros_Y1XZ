
#include "Bundle.h"
#include "MemoryStream.h"
#include "Messages.h"
#include "NetworkInterfaceBase.h"
#include "KBDebug.h"
#include "ObjectPool.h"

namespace Ouroboros
{

static ObjectPool<Bundle> _g_bundlePool;

Bundle::Bundle():
	pCurrPacket_(NULL),
	streams_(),
	numMessage_(0),
	messageLength_(0),
	pMsgtype_(NULL),
	curMsgStreamIndex_(0)
{
	pCurrPacket_ = MemoryStream::createObject();
}

Bundle::~Bundle()
{
	if (pCurrPacket_)
	{
		delete pCurrPacket_;
		pCurrPacket_ = NULL;
	}

	for (int i = 0; i < streams_.Num(); ++i)
	{
		delete streams_[i];
	}

	streams_.Empty();
}

Bundle* Bundle::createObject()
{
	return _g_bundlePool.createObject();
}

void Bundle::reclaimObject(Bundle* obj)
{
	obj->clear();
	_g_bundlePool.reclaimObject(obj);
}

void Bundle::newMessage(Message* pMsg)
{
	fini(false);

	pMsgtype_ = pMsg;
	numMessage_ += 1;

	(*this) << pMsgtype_->id;

	if (pMsgtype_->msglen == -1)
	{
		uint16 lengseat = 0;
		(*this) << lengseat;
		messageLength_ = 0;
	}

	curMsgStreamIndex_ = 0;
}

void Bundle::fini(bool issend)
{
	if (numMessage_ > 0)
	{
		writeMsgLength();

		streams_.Add(pCurrPacket_);
		pCurrPacket_ = MemoryStream::createObject();
	}

	if (issend)
	{
		numMessage_ = 0;
		pMsgtype_ = NULL;
	}

	curMsgStreamIndex_ = 0;
}

void Bundle::send(NetworkInterfaceBase* pNetworkInterface)
{
	fini(true);

	if (pNetworkInterface->valid())
	{
		for (int i = 0; i<streams_.Num(); ++i)
		{
			MemoryStream* stream = streams_[i];
			pNetworkInterface->send(stream);
		}
	}
	else
	{
		ERROR_MSG("Bundle::send(): networkInterface invalid!");
	}

	// We believe that if the send is complete, it is considered that the bundle is no longer used.
	// So we will put it back into the object pool to reduce the consumption of garbage collection.
	// If you need to continue using it, you should restart Bundle.createObject(),
	// If you use it directly without re-creating createObject(), you may have an inexplicable problem.
	// Only use this note to alert the user.
	Bundle::reclaimObject(this);
}

void Bundle::writeMsgLength()
{
	if (pMsgtype_->msglen != -1)
		return;

	MemoryStream* writePacket = pCurrPacket_;

	if (curMsgStreamIndex_ > 0)
	{
		writePacket = streams_[streams_.Num() - curMsgStreamIndex_];
	}

	uint8* data = writePacket->data();
	data[2] = (uint8)(messageLength_ & 0xff);
	data[3] = (uint8)(messageLength_ >> 8 & 0xff);
}

void Bundle::checkStream(uint32 v)
{
	if (v > pCurrPacket_->space())
	{
		streams_.Add(pCurrPacket_);
		pCurrPacket_ = MemoryStream::createObject();
		++curMsgStreamIndex_;
	}

	messageLength_ += v;
}

void Bundle::clear()
{
	// Put unused MemoryStream back into the buffer pool to reduce garbage collection
	for (int i = 0; i < streams_.Num(); ++i)
	{
		if(pCurrPacket_ != streams_[i])
			MemoryStream::reclaimObject(streams_[i]);
	}
	
	streams_.Empty();

	if (pCurrPacket_)
		pCurrPacket_->clear(false);
	else
		pCurrPacket_ = MemoryStream::createObject();

	numMessage_ = 0;
	messageLength_ = 0;
	pMsgtype_ = NULL;
	curMsgStreamIndex_ = 0;
}

Bundle &Bundle::operator<<(uint8 value)
{
	checkStream(sizeof(uint8));
	(*pCurrPacket_) << value;
	return *this;
}

Bundle &Bundle::operator<<(uint16 value)
{
	checkStream(sizeof(uint16));
	(*pCurrPacket_) << value;
	return *this;
}

Bundle &Bundle::operator<<(uint32 value)
{
	checkStream(sizeof(uint32));
	(*pCurrPacket_) << value;
	return *this;
}

Bundle &Bundle::operator<<(uint64 value)
{
	checkStream(sizeof(uint64));
	(*pCurrPacket_) << value;
	return *this;
}

Bundle &Bundle::operator<<(int8 value)
{
	checkStream(sizeof(int8));
	(*pCurrPacket_) << value;
	return *this;
}

Bundle &Bundle::operator<<(int16 value)
{
	checkStream(sizeof(int16));
	(*pCurrPacket_) << value;
	return *this;
}

Bundle &Bundle::operator<<(int32 value)
{
	checkStream(sizeof(int32));
	(*pCurrPacket_) << value;
	return *this;
}

Bundle &Bundle::operator<<(int64 value)
{
	checkStream(sizeof(int64));
	(*pCurrPacket_) << value;
	return *this;
}

Bundle &Bundle::operator<<(float value)
{
	checkStream(sizeof(float));
	(*pCurrPacket_) << value;
	return *this;
}

Bundle &Bundle::operator<<(double value)
{
	checkStream(sizeof(double));
	(*pCurrPacket_) << value;
	return *this;
}

Bundle &Bundle::operator<<(bool value)
{
	checkStream(sizeof(int8));
	(*pCurrPacket_) << value;
	return *this;
}

Bundle &Bundle::operator<<(const FString &value)
{
	uint32 len = value.Len();

	// +1 is the 0 position of the end of the string
	checkStream(len + 1);
	(*pCurrPacket_) << value;
	return *this;
}

Bundle &Bundle::operator<<(const char *str)
{
	// +1 is the 0 position of the end of the string
	uint32 len = (uint32)strlen(str) + 1; 

	checkStream(len);
	(*pCurrPacket_) << str;
	return *this;
}

void Bundle::appendBlob(const TArray<uint8>& datas)
{
	uint32 len = (uint32)datas.Num() + 4/*len size*/;

	checkStream(len);

	(*pCurrPacket_).appendBlob(datas);
}

void Bundle::appendUTF8String(const FString& str)
{
	FTCHARToUTF8 EchoStrUtf8(*str);
	uint32 len = (uint32)EchoStrUtf8.Length() + 4/*len size*/;

	checkStream(len);

	(*pCurrPacket_).appendUTF8String(str);
}

void Bundle::writeVector2(const FVector2D& v)
{
	checkStream(8);
	(*pCurrPacket_).writeVector2(v);
}

void Bundle::writeVector3(const FVector& v)
{
	checkStream(12);
	(*pCurrPacket_).writeVector3(v);
}

void Bundle::writeVector4(const FVector4& v)
{
	checkStream(16);
	(*pCurrPacket_).writeVector4(v);
}

}