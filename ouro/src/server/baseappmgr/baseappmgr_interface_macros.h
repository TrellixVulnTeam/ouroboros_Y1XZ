// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com


#if defined(DEFINE_IN_INTERFACE)
	#undef OURO_BASEAPPMG_INTERFACE_MACRO_H
#endif


#ifndef OURO_BASEAPPMG_INTERFACE_MACRO_H
#define OURO_BASEAPPMG_INTERFACE_MACRO_H

// common include	
#include "network/interface_defs.h"
// windows include	
#if OURO_PLATFORM == PLATFORM_WIN32
#else
// linux include
#endif
	
namespace Ouroboros{

/**
	Baseappmgr message macro, the parameter is stream, you need to unlock it yourself.
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
	#undef BASEAPPMGR_MESSAGE_HANDLER_STREAM
#endif

#if defined(DEFINE_IN_INTERFACE)
#if defined(BASEAPPMGR)
#define BASEAPPMGR_MESSAGE_HANDLER_STREAM(NAME)									\
	void NAME##BaseappmgrMessagehandler_stream::handle(Network::Channel* pChannel,\
													Ouroboros::MemoryStream& s)	\
	{																			\
			Ouroboros::Baseappmgr::getSingleton().NAME(pChannel, s);				\
	}																			\

#else
#define BASEAPPMGR_MESSAGE_HANDLER_STREAM(NAME)									\
	void NAME##BaseappmgrMessagehandler_stream::handle(Network::Channel* pChannel,\
													Ouroboros::MemoryStream& s)	\
	{																			\
	}																			\
		
#endif
#else
#define BASEAPPMGR_MESSAGE_HANDLER_STREAM(NAME)									\
	class NAME##BaseappmgrMessagehandler_stream : public Network::MessageHandler\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
							Ouroboros::MemoryStream& s);							\
	};																			\

#endif

#define BASEAPPMGR_MESSAGE_DECLARE_STREAM(NAME, MSG_LENGTH)						\
	BASEAPPMGR_MESSAGE_HANDLER_STREAM(NAME)										\
	NETWORK_MESSAGE_DECLARE_STREAM(Baseappmgr, NAME,							\
				NAME##BaseappmgrMessagehandler_stream, MSG_LENGTH)				\
																				\

/**
	Baseappmgr message macro, only zero parameter message
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
	#undef BASEAPPMGR_MESSAGE_HANDLER_ARGS0
#endif

#if defined(DEFINE_IN_INTERFACE)
#if defined(BASEAPPMGR)
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS0(NAME)									\
	void NAME##BaseappmgrMessagehandler0::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
			Ouroboros::Baseappmgr::getSingleton().NAME(pChannel);				\
	}																			\

#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS0(NAME)									\
	void NAME##BaseappmgrMessagehandler0::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
	}																			\
		
#endif
#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS0(NAME)									\
	class NAME##BaseappmgrMessagehandler0 : public Network::MessageHandler		\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
												Ouroboros::MemoryStream& s);		\
	};																			\

#endif

#define BASEAPPMGR_MESSAGE_DECLARE_ARGS0(NAME, MSG_LENGTH)						\
	BASEAPPMGR_MESSAGE_HANDLER_ARGS0(NAME)										\
	NETWORK_MESSAGE_DECLARE_ARGS0(Baseappmgr, NAME,								\
				NAME##BaseappmgrMessagehandler0, MSG_LENGTH)					\
																				\

/**
	Baseappmgr message macro, message with only one parameter
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
	#undef BASEAPPMGR_MESSAGE_HANDLER_ARGS1
#endif

#if defined(DEFINE_IN_INTERFACE)
#if defined(BASEAPPMGR)
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS1(NAME, ARG_TYPE1, ARG_NAME1)			\
	void NAME##BaseappmgrMessagehandler1::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
			ARG_TYPE1 ARG_NAME1;												\
			s >> ARG_NAME1;														\
			Ouroboros::Baseappmgr::getSingleton().NAME(pChannel, ARG_NAME1);		\
	}																			\

#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS1(NAME, ARG_TYPE1, ARG_NAME1)			\
	void NAME##BaseappmgrMessagehandler1::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
	}																			\
		
#endif
#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS1(NAME, ARG_TYPE1, ARG_NAME1)			\
	class NAME##BaseappmgrMessagehandler1 : public Network::MessageHandler		\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
							Ouroboros::MemoryStream& s);							\
	};																			\

#endif

#define BASEAPPMGR_MESSAGE_DECLARE_ARGS1(NAME, MSG_LENGTH, ARG_TYPE1, ARG_NAME1)\
	BASEAPPMGR_MESSAGE_HANDLER_ARGS1(NAME, ARG_TYPE1, ARG_NAME1)				\
	NETWORK_MESSAGE_DECLARE_ARGS1(Baseappmgr, NAME,								\
				NAME##BaseappmgrMessagehandler1, MSG_LENGTH, ARG_TYPE1, ARG_NAME1)\
																				\

/**
	Baseappmgr message macro, message with only two parameters
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
	#undef BASEAPPMGR_MESSAGE_HANDLER_ARGS2
#endif

#if defined(DEFINE_IN_INTERFACE)
#if defined(BASEAPPMGR)
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS2(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2)				\
	void NAME##BaseappmgrMessagehandler2::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
			ARG_TYPE1 ARG_NAME1;												\
			s >> ARG_NAME1;														\
			ARG_TYPE2 ARG_NAME2;												\
			s >> ARG_NAME2;														\
			Ouroboros::Baseappmgr::getSingleton().NAME(pChannel,					\
													ARG_NAME1, ARG_NAME2);		\
	}																			\

#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS2(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2)				\
	void NAME##BaseappmgrMessagehandler2::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
	}																			\
		
#endif
#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS2(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2)				\
	class NAME##BaseappmgrMessagehandler2 : public Network::MessageHandler		\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
							Ouroboros::MemoryStream& s);							\
	};																			\

#endif

#define BASEAPPMGR_MESSAGE_DECLARE_ARGS2(NAME, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2)				\
	BASEAPPMGR_MESSAGE_HANDLER_ARGS2(NAME, ARG_TYPE1, ARG_NAME1, 				\
											ARG_TYPE2, ARG_NAME2)				\
	NETWORK_MESSAGE_DECLARE_ARGS2(Baseappmgr, NAME,								\
				NAME##BaseappmgrMessagehandler2, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2)				\


/**
	Baseappmgr message macro, message with only three parameters
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
	#undef BASEAPPMGR_MESSAGE_HANDLER_ARGS3
#endif

#if defined(DEFINE_IN_INTERFACE)
#if defined(BASEAPPMGR)
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS3(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3)				\
	void NAME##BaseappmgrMessagehandler3::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
			ARG_TYPE1 ARG_NAME1;												\
			s >> ARG_NAME1;														\
			ARG_TYPE2 ARG_NAME2;												\
			s >> ARG_NAME2;														\
			ARG_TYPE3 ARG_NAME3;												\
			s >> ARG_NAME3;														\
			Ouroboros::Baseappmgr::getSingleton().NAME(pChannel,					\
				ARG_NAME1, ARG_NAME2, 											\
				ARG_NAME3);														\
	}																			\

#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS3(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3)				\
	void NAME##BaseappmgrMessagehandler3::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
	}																			\
		
#endif
#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS3(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3)				\
	class NAME##BaseappmgrMessagehandler3 : public Network::MessageHandler		\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
												Ouroboros::MemoryStream& s);		\
	};																			\

#endif

#define BASEAPPMGR_MESSAGE_DECLARE_ARGS3(NAME, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3)				\
	BASEAPPMGR_MESSAGE_HANDLER_ARGS3(NAME, ARG_TYPE1, ARG_NAME1, 				\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3)				\
	NETWORK_MESSAGE_DECLARE_ARGS3(Baseappmgr, NAME,								\
				NAME##BaseappmgrMessagehandler3, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3)				\

/**
	Baseappmgr message macro, message with only four parameters
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
	#undef BASEAPPMGR_MESSAGE_HANDLER_ARGS4
#endif

#if defined(DEFINE_IN_INTERFACE)
#if defined(BASEAPPMGR)
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS4(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4)				\
	void NAME##BaseappmgrMessagehandler4::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
			ARG_TYPE1 ARG_NAME1;												\
			s >> ARG_NAME1;														\
			ARG_TYPE2 ARG_NAME2;												\
			s >> ARG_NAME2;														\
			ARG_TYPE3 ARG_NAME3;												\
			s >> ARG_NAME3;														\
			ARG_TYPE4 ARG_NAME4;												\
			s >> ARG_NAME4;														\
			Ouroboros::Baseappmgr::getSingleton().NAME(pChannel,					\
														ARG_NAME1, ARG_NAME2, 	\
				ARG_NAME3, ARG_NAME4);											\
	}																			\

#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS4(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4)				\
	void NAME##BaseappmgrMessagehandler4::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
	}																			\
		
#endif
#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS4(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4)				\
	class NAME##BaseappmgrMessagehandler4 : public Network::MessageHandler		\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
							Ouroboros::MemoryStream& s);							\
	};																			\

#endif

#define BASEAPPMGR_MESSAGE_DECLARE_ARGS4(NAME, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4)				\
	BASEAPPMGR_MESSAGE_HANDLER_ARGS4(NAME, ARG_TYPE1, ARG_NAME1, 				\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4)				\
	NETWORK_MESSAGE_DECLARE_ARGS4(Baseappmgr, NAME,								\
				NAME##BaseappmgrMessagehandler4, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4)				\

/**
	Baseappmgr message macro, message with only five parameters
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
	#undef BASEAPPMGR_MESSAGE_HANDLER_ARGS5
#endif

#if defined(DEFINE_IN_INTERFACE)
#if defined(BASEAPPMGR)
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS5(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5)				\
	void NAME##BaseappmgrMessagehandler5::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
			ARG_TYPE1 ARG_NAME1;												\
			s >> ARG_NAME1;														\
			ARG_TYPE2 ARG_NAME2;												\
			s >> ARG_NAME2;														\
			ARG_TYPE3 ARG_NAME3;												\
			s >> ARG_NAME3;														\
			ARG_TYPE4 ARG_NAME4;												\
			s >> ARG_NAME4;														\
			ARG_TYPE5 ARG_NAME5;												\
			s >> ARG_NAME5;														\
			Ouroboros::Baseappmgr::getSingleton().NAME(pChannel,					\
														ARG_NAME1, ARG_NAME2, 	\
				ARG_NAME3, ARG_NAME4, ARG_NAME5);								\
	}																			\

#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS5(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5)				\
	void NAME##BaseappmgrMessagehandler5::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
	}																			\
		
#endif
#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS5(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5)				\
	class NAME##BaseappmgrMessagehandler5 : public Network::MessageHandler		\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
							Ouroboros::MemoryStream& s);							\
	};																			\

#endif

#define BASEAPPMGR_MESSAGE_DECLARE_ARGS5(NAME, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5)				\
	BASEAPPMGR_MESSAGE_HANDLER_ARGS5(NAME, ARG_TYPE1, ARG_NAME1, 				\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5)				\
	NETWORK_MESSAGE_DECLARE_ARGS5(Baseappmgr, NAME,								\
				NAME##BaseappmgrMessagehandler5, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5)				\


/**
	Baseappmgr message macro, message with only six parameters
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
	#undef BASEAPPMGR_MESSAGE_HANDLER_ARGS6
#endif

#if defined(DEFINE_IN_INTERFACE)
#if defined(BASEAPPMGR)
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS6(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6)				\
	void NAME##BaseappmgrMessagehandler6::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
			ARG_TYPE1 ARG_NAME1;												\
			s >> ARG_NAME1;														\
			ARG_TYPE2 ARG_NAME2;												\
			s >> ARG_NAME2;														\
			ARG_TYPE3 ARG_NAME3;												\
			s >> ARG_NAME3;														\
			ARG_TYPE4 ARG_NAME4;												\
			s >> ARG_NAME4;														\
			ARG_TYPE5 ARG_NAME5;												\
			s >> ARG_NAME5;														\
			ARG_TYPE6 ARG_NAME6;												\
			s >> ARG_NAME6;														\
			Ouroboros::Baseappmgr::getSingleton().NAME(pChannel,					\
														ARG_NAME1, ARG_NAME2, 	\
				ARG_NAME3, ARG_NAME4, ARG_NAME5, ARG_NAME6);					\
	}																			\

#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS6(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6)				\
	void NAME##BaseappmgrMessagehandler6::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
	}																			\
		
#endif
#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS6(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6)				\
	class NAME##BaseappmgrMessagehandler6 : public Network::MessageHandler		\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
							Ouroboros::MemoryStream& s);							\
	};																			\

#endif

#define BASEAPPMGR_MESSAGE_DECLARE_ARGS6(NAME, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6)				\
	BASEAPPMGR_MESSAGE_HANDLER_ARGS6(NAME, ARG_TYPE1, ARG_NAME1, 				\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6)				\
	NETWORK_MESSAGE_DECLARE_ARGS6(Baseappmgr, NAME,								\
				NAME##BaseappmgrMessagehandler6, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6)				\


/**
	Baseappmgr message macro, message with only seven parameters
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
	#undef BASEAPPMGR_MESSAGE_HANDLER_ARGS7
#endif

#if defined(DEFINE_IN_INTERFACE)
#if defined(BASEAPPMGR)
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS7(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7)				\
	void NAME##BaseappmgrMessagehandler7::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
			ARG_TYPE1 ARG_NAME1;												\
			s >> ARG_NAME1;														\
			ARG_TYPE2 ARG_NAME2;												\
			s >> ARG_NAME2;														\
			ARG_TYPE3 ARG_NAME3;												\
			s >> ARG_NAME3;														\
			ARG_TYPE4 ARG_NAME4;												\
			s >> ARG_NAME4;														\
			ARG_TYPE5 ARG_NAME5;												\
			s >> ARG_NAME5;														\
			ARG_TYPE6 ARG_NAME6;												\
			s >> ARG_NAME6;														\
			ARG_TYPE7 ARG_NAME7;												\
			s >> ARG_NAME7;														\
			Ouroboros::Baseappmgr::getSingleton().NAME(pChannel,					\
														ARG_NAME1, ARG_NAME2, 	\
				ARG_NAME3, ARG_NAME4, ARG_NAME5, ARG_NAME6, ARG_NAME7);			\
	}																			\

#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS7(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7)				\
	void NAME##BaseappmgrMessagehandler7::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
	}																			\
		
#endif
#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS7(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7)				\
	class NAME##BaseappmgrMessagehandler7 : public Network::MessageHandler		\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
							Ouroboros::MemoryStream& s);							\
	};																			\

#endif

#define BASEAPPMGR_MESSAGE_DECLARE_ARGS7(NAME, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7)				\
	BASEAPPMGR_MESSAGE_HANDLER_ARGS7(NAME, ARG_TYPE1, ARG_NAME1, 				\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7)				\
	NETWORK_MESSAGE_DECLARE_ARGS7(Baseappmgr, NAME,								\
				NAME##BaseappmgrMessagehandler7, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7)				\



/**
	Baseappmgr message macro, message with only eight parameters
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
	#undef BASEAPPMGR_MESSAGE_HANDLER_ARGS8
#endif

#if defined(DEFINE_IN_INTERFACE)
#if defined(BASEAPPMGR)
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS8(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8)				\
	void NAME##BaseappmgrMessagehandler8::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
			ARG_TYPE1 ARG_NAME1;												\
			s >> ARG_NAME1;														\
			ARG_TYPE2 ARG_NAME2;												\
			s >> ARG_NAME2;														\
			ARG_TYPE3 ARG_NAME3;												\
			s >> ARG_NAME3;														\
			ARG_TYPE4 ARG_NAME4;												\
			s >> ARG_NAME4;														\
			ARG_TYPE5 ARG_NAME5;												\
			s >> ARG_NAME5;														\
			ARG_TYPE6 ARG_NAME6;												\
			s >> ARG_NAME6;														\
			ARG_TYPE7 ARG_NAME7;												\
			s >> ARG_NAME7;														\
			ARG_TYPE8 ARG_NAME8;												\
			s >> ARG_NAME8;														\
			Ouroboros::Baseappmgr::getSingleton().NAME(pChannel,					\
										ARG_NAME1, ARG_NAME2, ARG_NAME3, 		\
										ARG_NAME4, ARG_NAME5, ARG_NAME6,		\
										ARG_NAME7, ARG_NAME8);					\
	}																			\

#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS8(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8)				\
	void NAME##BaseappmgrMessagehandler8::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
	}																			\
		
#endif
#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS8(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8)				\
	class NAME##BaseappmgrMessagehandler8 : public Network::MessageHandler		\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
							Ouroboros::MemoryStream& s);							\
	};																			\

#endif

#define BASEAPPMGR_MESSAGE_DECLARE_ARGS8(NAME, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8)				\
	BASEAPPMGR_MESSAGE_HANDLER_ARGS8(NAME, ARG_TYPE1, ARG_NAME1, 				\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8)				\
	NETWORK_MESSAGE_DECLARE_ARGS8(Baseappmgr, NAME,								\
				NAME##BaseappmgrMessagehandler8, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,	\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8)				\

/**
	Baseappmgr message macro, message with only nine parameters
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
	#undef BASEAPPMGR_MESSAGE_HANDLER_ARGS9
#endif

#if defined(DEFINE_IN_INTERFACE)
#if defined(BASEAPPMGR)
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS9(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9)				\
	void NAME##BaseappmgrMessagehandler9::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
			ARG_TYPE1 ARG_NAME1;												\
			s >> ARG_NAME1;														\
			ARG_TYPE2 ARG_NAME2;												\
			s >> ARG_NAME2;														\
			ARG_TYPE3 ARG_NAME3;												\
			s >> ARG_NAME3;														\
			ARG_TYPE4 ARG_NAME4;												\
			s >> ARG_NAME4;														\
			ARG_TYPE5 ARG_NAME5;												\
			s >> ARG_NAME5;														\
			ARG_TYPE6 ARG_NAME6;												\
			s >> ARG_NAME6;														\
			ARG_TYPE7 ARG_NAME7;												\
			s >> ARG_NAME7;														\
			ARG_TYPE8 ARG_NAME8;												\
			s >> ARG_NAME8;														\
			ARG_TYPE9 ARG_NAME9;												\
			s >> ARG_NAME9;														\
			Ouroboros::Baseappmgr::getSingleton().NAME(pChannel,					\
										ARG_NAME1, ARG_NAME2, ARG_NAME3, 		\
										ARG_NAME4, ARG_NAME5, ARG_NAME6,		\
										ARG_NAME7, ARG_NAME8, ARG_NAME9);		\
	}																			\

#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS9(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9)				\
	void NAME##BaseappmgrMessagehandler9::handle(Network::Channel* pChannel,	\
												Ouroboros::MemoryStream& s)		\
	{																			\
	}																			\
		
#endif
#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS9(NAME, ARG_TYPE1, ARG_NAME1,			\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9)				\
	class NAME##BaseappmgrMessagehandler9 : public Network::MessageHandler		\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
							Ouroboros::MemoryStream& s);							\
	};																			\

#endif

#define BASEAPPMGR_MESSAGE_DECLARE_ARGS9(NAME, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9)				\
	BASEAPPMGR_MESSAGE_HANDLER_ARGS9(NAME, ARG_TYPE1, ARG_NAME1, 				\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9)				\
	NETWORK_MESSAGE_DECLARE_ARGS9(Baseappmgr, NAME,								\
				NAME##BaseappmgrMessagehandler9, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,	\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9)				\

/**
	Baseappmgr message macro, message with only ten parameters
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
	#undef BASEAPPMGR_MESSAGE_HANDLER_ARGS10
#endif

#if defined(DEFINE_IN_INTERFACE)
#if defined(BASEAPPMGR)
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS10(NAME, ARG_TYPE1, ARG_NAME1,				\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9,				\
											ARG_TYPE10, ARG_NAME10)				\
	void NAME##BaseappmgrMessagehandler10::handle(Network::Channel* pChannel,		\
												Ouroboros::MemoryStream& s)		\
	{																			\
			ARG_TYPE1 ARG_NAME1;												\
			s >> ARG_NAME1;														\
			ARG_TYPE2 ARG_NAME2;												\
			s >> ARG_NAME2;														\
			ARG_TYPE3 ARG_NAME3;												\
			s >> ARG_NAME3;														\
			ARG_TYPE4 ARG_NAME4;												\
			s >> ARG_NAME4;														\
			ARG_TYPE5 ARG_NAME5;												\
			s >> ARG_NAME5;														\
			ARG_TYPE6 ARG_NAME6;												\
			s >> ARG_NAME6;														\
			ARG_TYPE7 ARG_NAME7;												\
			s >> ARG_NAME7;														\
			ARG_TYPE8 ARG_NAME8;												\
			s >> ARG_NAME8;														\
			ARG_TYPE9 ARG_NAME9;												\
			s >> ARG_NAME9;														\
			ARG_TYPE10 ARG_NAME10;												\
			s >> ARG_NAME10;													\
			Ouroboros::Baseappmgr::getSingleton().NAME(pChannel,					\
										ARG_NAME1, ARG_NAME2, ARG_NAME3, 		\
										ARG_NAME4, ARG_NAME5, ARG_NAME6,		\
										ARG_NAME7, ARG_NAME8, ARG_NAME9,		\
										ARG_NAME10);							\
	}																			\

#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS10(NAME, ARG_TYPE1, ARG_NAME1,				\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9,				\
											ARG_TYPE10, ARG_NAME10)				\
	void NAME##BaseappmgrMessagehandler10::handle(Network::Channel* pChannel,		\
												Ouroboros::MemoryStream& s)		\
	{																			\
	}																			\
		
#endif
#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS10(NAME, ARG_TYPE1, ARG_NAME1,				\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9,				\
											ARG_TYPE10, ARG_NAME10)				\
	class NAME##BaseappmgrMessagehandler10 : public Network::MessageHandler		\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
							Ouroboros::MemoryStream& s);							\
	};																			\

#endif

#define BASEAPPMGR_MESSAGE_DECLARE_ARGS10(NAME, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,	\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9,				\
											ARG_TYPE10, ARG_NAME10)				\
	BASEAPPMGR_MESSAGE_HANDLER_ARGS10(NAME, ARG_TYPE1, ARG_NAME1, 					\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9,				\
											ARG_TYPE10, ARG_NAME10)				\
	NETWORK_MESSAGE_DECLARE_ARGS10(Baseappmgr, NAME,								\
				NAME##BaseappmgrMessagehandler10, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9,				\
											ARG_TYPE10, ARG_NAME10)				\


/**
	Baseappmgr message macro, message with only one parameter
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
	#undef BASEAPPMGR_MESSAGE_HANDLER_ARGS11
#endif

#if defined(DEFINE_IN_INTERFACE)
#if defined(BASEAPPMGR)
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS11(NAME, ARG_TYPE1, ARG_NAME1,				\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9,				\
											ARG_TYPE10, ARG_NAME10,				\
											ARG_TYPE11, ARG_NAME11)				\
	void NAME##BaseappmgrMessagehandler11::handle(Network::Channel* pChannel,		\
												Ouroboros::MemoryStream& s)		\
	{																			\
			ARG_TYPE1 ARG_NAME1;												\
			s >> ARG_NAME1;														\
			ARG_TYPE2 ARG_NAME2;												\
			s >> ARG_NAME2;														\
			ARG_TYPE3 ARG_NAME3;												\
			s >> ARG_NAME3;														\
			ARG_TYPE4 ARG_NAME4;												\
			s >> ARG_NAME4;														\
			ARG_TYPE5 ARG_NAME5;												\
			s >> ARG_NAME5;														\
			ARG_TYPE6 ARG_NAME6;												\
			s >> ARG_NAME6;														\
			ARG_TYPE7 ARG_NAME7;												\
			s >> ARG_NAME7;														\
			ARG_TYPE8 ARG_NAME8;												\
			s >> ARG_NAME8;														\
			ARG_TYPE9 ARG_NAME9;												\
			s >> ARG_NAME9;														\
			ARG_TYPE10 ARG_NAME10;												\
			s >> ARG_NAME10;													\
			ARG_TYPE11 ARG_NAME11;												\
			s >> ARG_NAME11;													\
			Ouroboros::Baseappmgr::getSingleton().NAME(pChannel,					\
										ARG_NAME1, ARG_NAME2, ARG_NAME3, 		\
										ARG_NAME4, ARG_NAME5, ARG_NAME6,		\
										ARG_NAME7, ARG_NAME8, ARG_NAME9,		\
										ARG_NAME10, ARG_NAME11);				\
	}																			\

#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS11(NAME, ARG_TYPE1, ARG_NAME1,				\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9,				\
											ARG_TYPE10, ARG_NAME10,				\
											ARG_TYPE11, ARG_NAME11)				\
	void NAME##BaseappmgrMessagehandler11::handle(Network::Channel* pChannel,		\
												Ouroboros::MemoryStream& s)		\
	{																			\
	}																			\
		
#endif
#else
#define BASEAPPMGR_MESSAGE_HANDLER_ARGS11(NAME, ARG_TYPE1, ARG_NAME1,				\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9,				\
											ARG_TYPE10, ARG_NAME10,				\
											ARG_TYPE11, ARG_NAME11)				\
	class NAME##BaseappmgrMessagehandler11 : public Network::MessageHandler		\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
							Ouroboros::MemoryStream& s);							\
	};																			\

#endif

#define BASEAPPMGR_MESSAGE_DECLARE_ARGS11(NAME, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,	\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9,				\
											ARG_TYPE10, ARG_NAME10,				\
											ARG_TYPE11, ARG_NAME11)				\
	BASEAPPMGR_MESSAGE_HANDLER_ARGS11(NAME, ARG_TYPE1, ARG_NAME1, 					\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9,				\
											ARG_TYPE10, ARG_NAME10,				\
											ARG_TYPE11, ARG_NAME11)				\
	NETWORK_MESSAGE_DECLARE_ARGS11(Baseappmgr, NAME,								\
				NAME##BaseappmgrMessagehandler11, MSG_LENGTH, ARG_TYPE1, ARG_NAME1,\
											ARG_TYPE2, ARG_NAME2,				\
											ARG_TYPE3, ARG_NAME3,				\
											ARG_TYPE4, ARG_NAME4,				\
											ARG_TYPE5, ARG_NAME5,				\
											ARG_TYPE6, ARG_NAME6,				\
											ARG_TYPE7, ARG_NAME7,				\
											ARG_TYPE8, ARG_NAME8,				\
											ARG_TYPE9, ARG_NAME9,				\
											ARG_TYPE10, ARG_NAME10,				\
											ARG_TYPE11, ARG_NAME11)				\


}
#endif
