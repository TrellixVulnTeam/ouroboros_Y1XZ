// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com


#include "dbmgr.h"
#include "dbmgr_interface.h"
#include "dbtasks.h"
#include "profile.h"
#include "interfaces_handler.h"
#include "sync_app_datas_handler.h"
#include "update_dblog_handler.h"
#include "db_mysql/ouro_table_mysql.h"
#include "network/common.h"
#include "network/tcp_packet.h"
#include "network/udp_packet.h"
#include "network/message_handler.h"
#include "thread/threadpool.h"
#include "server/components.h"
#include "server/telnet_server.h"
#include "db_interface/db_interface.h"
#include "db_mysql/db_interface_mysql.h"
#include "entitydef/scriptdef_module.h"
#include "entitydef/py_entitydef.h"

#include "baseapp/baseapp_interface.h"
#include "cellapp/cellapp_interface.h"
#include "baseappmgr/baseappmgr_interface.h"
#include "cellappmgr/cellappmgr_interface.h"
#include "loginapp/loginapp_interface.h"

namespace Ouroboros{

ServerConfig g_serverConfig;
OURO_SINGLETON_INIT(Dbmgr);

//-------------------------------------------------------------------------------------
Dbmgr::Dbmgr(Network::EventDispatcher& dispatcher, 
			 Network::NetworkInterface& ninterface, 
			 COMPONENT_TYPE componentType,
			 COMPONENT_ID componentID):
	PythonApp(dispatcher, ninterface, componentType, componentID),
	loopCheckTimerHandle_(),
	mainProcessTimer_(),
	idServer_(1, 1024),
	pGlobalData_(NULL),
	pBaseAppData_(NULL),
	pCellAppData_(NULL),
	bufferedDBTasksMaps_(),
	numWrittenEntity_(0),
	numRemovedEntity_(0),
	numQueryEntity_(0),
	numExecuteRawDatabaseCommand_(0),
	numCreatedAccount_(0),
	pInterfacesHandlers_(),
	pSyncAppDatasHandler_(NULL),
	pUpdateDBServerLogHandler_(NULL),
	pTelnetServer_(NULL),
	loseBaseappts_()
{
	Ouroboros::Network::MessageHandlers::pMainMessageHandlers = &DbmgrInterface::messageHandlers;
}

//-------------------------------------------------------------------------------------
Dbmgr::~Dbmgr()
{
	loopCheckTimerHandle_.cancel();
	mainProcessTimer_.cancel();
	Ouroboros::sleep(300);

	for (std::vector<InterfacesHandler*>::iterator iter = pInterfacesHandlers_.begin(); iter != pInterfacesHandlers_.end(); ++iter)
	{
		SAFE_RELEASE((*iter));
	}
}

//-------------------------------------------------------------------------------------
ShutdownHandler::CAN_SHUTDOWN_STATE Dbmgr::canShutdown()
{
	if (getEntryScript().get() && PyObject_HasAttrString(getEntryScript().get(), "onReadyForShutDown") > 0)
	{
		// All scripts are loaded
		PyObject* pyResult = PyObject_CallMethod(getEntryScript().get(),
			const_cast<char*>("onReadyForShutDown"),
			const_cast<char*>(""));

		if (pyResult != NULL)
		{
			bool isReady = (pyResult == Py_True);
			Py_DECREF(pyResult);

			if (!isReady)
				return ShutdownHandler::CAN_SHUTDOWN_STATE_USER_FALSE;
		}
		else
		{
			SCRIPT_ERROR_CHECK();
			return ShutdownHandler::CAN_SHUTDOWN_STATE_USER_FALSE;
		}
	}

	OUROUnordered_map<std::string, Buffered_DBTasks>::iterator bditer = bufferedDBTasksMaps_.begin();
	for (; bditer != bufferedDBTasksMaps_.end(); ++bditer)
	{
		if (bditer->second.size() > 0)
		{
			thread::ThreadPool* pThreadPool = DBUtil::pThreadPool(bditer->first);
			OURO_ASSERT(pThreadPool);

			INFO_MSG(fmt::format("Dbmgr::canShutdown(): Wait for the task to complete, dbInterface={}, tasks{}=[{}], threads={}/{}, threadpoolDestroyed={}!\n",
				bditer->first, bditer->second.size(), bditer->second.getTasksinfos(), (pThreadPool->currentThreadCount() - pThreadPool->currentFreeThreadCount()),
				pThreadPool->currentThreadCount(), pThreadPool->isDestroyed()));

			return ShutdownHandler::CAN_SHUTDOWN_STATE_FALSE;
		}
	}

	Components::COMPONENTS& cellapp_components = Components::getSingleton().getComponents(CELLAPP_TYPE);
	if (cellapp_components.size() > 0)
	{
		std::string s;
		for (size_t i = 0; i<cellapp_components.size(); ++i)
		{
			s += fmt::format("{}, ", cellapp_components[i].cid);
		}

		INFO_MSG(fmt::format("Dbmgr::canShutdown(): Waiting for cellapp[{}] destruction!\n",
			s));

		return ShutdownHandler::CAN_SHUTDOWN_STATE_FALSE;
	}

	Components::COMPONENTS& baseapp_components = Components::getSingleton().getComponents(BASEAPP_TYPE);
	if (baseapp_components.size() > 0)
	{
		std::string s;
		for (size_t i = 0; i<baseapp_components.size(); ++i)
		{
			s += fmt::format("{}, ", baseapp_components[i].cid);
		}

		INFO_MSG(fmt::format("Dbmgr::canShutdown(): Waiting for baseapp[{}] destruction!\n",
			s));

		return ShutdownHandler::CAN_SHUTDOWN_STATE_FALSE;
	}

	return ShutdownHandler::CAN_SHUTDOWN_STATE_TRUE;
}

//-------------------------------------------------------------------------------------	
void Dbmgr::onShutdownBegin()
{
	PythonApp::onShutdownBegin();

	// notification script
	SCOPED_PROFILE(SCRIPTCALL_PROFILE);
	SCRIPT_OBJECT_CALL_ARGS0(getEntryScript().get(), const_cast<char*>("onDBMgrShutDown"), false);
}

//-------------------------------------------------------------------------------------	
void Dbmgr::onShutdownEnd()
{
	PythonApp::onShutdownEnd();
}

//-------------------------------------------------------------------------------------
bool Dbmgr::initializeWatcher()
{
	WATCH_OBJECT("numWrittenEntity", numWrittenEntity_);
	WATCH_OBJECT("numRemovedEntity", numRemovedEntity_);
	WATCH_OBJECT("numQueryEntity", numQueryEntity_);
	WATCH_OBJECT("numExecuteRawDatabaseCommand", numExecuteRawDatabaseCommand_);
	WATCH_OBJECT("numCreatedAccount", numCreatedAccount_);

	OUROUnordered_map<std::string, Buffered_DBTasks>::iterator bditer = bufferedDBTasksMaps_.begin();
	for (; bditer != bufferedDBTasksMaps_.end(); ++bditer)
	{
		WATCH_OBJECT(fmt::format("DBThreadPool/{}/dbid_tasksSize", bditer->first).c_str(), &bditer->second, &Buffered_DBTasks::dbid_tasksSize);
		WATCH_OBJECT(fmt::format("DBThreadPool/{}/entityid_tasksSize", bditer->first).c_str(), &bditer->second, &Buffered_DBTasks::entityid_tasksSize);
		WATCH_OBJECT(fmt::format("DBThreadPool/{}/printBuffered_dbid", bditer->first).c_str(), &bditer->second, &Buffered_DBTasks::printBuffered_dbid);
		WATCH_OBJECT(fmt::format("DBThreadPool/{}/printBuffered_entityID", bditer->first).c_str(), &bditer->second, &Buffered_DBTasks::printBuffered_entityID);
	}

	return ServerApp::initializeWatcher() && DBUtil::initializeWatcher();
}

//-------------------------------------------------------------------------------------
bool Dbmgr::run()
{
	return PythonApp::run();
}

//-------------------------------------------------------------------------------------
void Dbmgr::handleTimeout(TimerHandle handle, void * arg)
{
	PythonApp::handleTimeout(handle, arg);

	switch (reinterpret_cast<uintptr>(arg))
	{
		case TIMEOUT_TICK:
			this->handleMainTick();
			break;
		case TIMEOUT_CHECK_STATUS:
			this->handleCheckStatusTick();
			break;
		default:
			break;
	}
}

//-------------------------------------------------------------------------------------
void Dbmgr::handleMainTick()
{
	AUTO_SCOPED_PROFILE("mainTick");
	
	 // time_t t = ::time(NULL);
	 // static int ouroTime = 0;
	 // DEBUG_MSG(fmt::format("Dbmgr::handleGameTick[{}]:{}\n", t, ++ouroTime));
	
	threadPool_.onMainThreadTick();
	DBUtil::handleMainTick();
	networkInterface().processChannels(&DbmgrInterface::messageHandlers);
}

//-------------------------------------------------------------------------------------
void Dbmgr::handleCheckStatusTick()
{
	// Check the missing component process, if still can not be found within a period of time, you need to clean up the entitylog in the database
	if (loseBaseappts_.size() > 0)
	{
		std::map<COMPONENT_ID, uint64>::iterator iter = loseBaseappts_.begin();
		for (; iter != loseBaseappts_.end();)
		{
			if (timestamp() > iter->second)
			{
				Components::ComponentInfos* cinfo = Components::getSingleton().findComponent(iter->first);
				if (!cinfo)
				{
					ENGINE_COMPONENT_INFO& dbcfg = g_ouroSrvConfig.getDBMgr();
					std::vector<DBInterfaceInfo>::iterator dbinfo_iter = dbcfg.dbInterfaceInfos.begin();
					for (; dbinfo_iter != dbcfg.dbInterfaceInfos.end(); ++dbinfo_iter)
					{
						std::string dbInterfaceName = dbinfo_iter->name;

						DBUtil::pThreadPool(dbInterfaceName)->
							addTask(new DBTaskEraseBaseappEntityLog(iter->first));
					}
				}

				loseBaseappts_.erase(iter++);
			}
			else
			{
				++iter;
			}
		}
	}
}

//-------------------------------------------------------------------------------------
bool Dbmgr::initializeBegin()
{
	idServer_.set_range_step(g_ouroSrvConfig.getDBMgr().ids_increasing_range);
	return true;
}

//-------------------------------------------------------------------------------------
bool Dbmgr::inInitialize()
{
	// Initialize all extension modules
	// assets/scripts/
	if (!PythonApp::inInitialize())
		return false;

	std::vector<PyTypeObject*>	scriptBaseTypes;
	if(!EntityDef::initialize(scriptBaseTypes, componentType_)){
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool Dbmgr::initializeEnd()
{
	PythonApp::initializeEnd();

	// Add a timer, check some status every second
	loopCheckTimerHandle_ = this->dispatcher().addTimer(1000000, this,
							reinterpret_cast<void *>(TIMEOUT_CHECK_STATUS));

	mainProcessTimer_ = this->dispatcher().addTimer(1000000 / 50, this,
							reinterpret_cast<void *>(TIMEOUT_TICK));

	// Add globalData, baseAppData, cellAppData support
	pGlobalData_ = new GlobalDataServer(GlobalDataServer::GLOBAL_DATA);
	pBaseAppData_ = new GlobalDataServer(GlobalDataServer::BASEAPP_DATA);
	pCellAppData_ = new GlobalDataServer(GlobalDataServer::CELLAPP_DATA);
	pGlobalData_->addConcernComponentType(CELLAPP_TYPE);
	pGlobalData_->addConcernComponentType(BASEAPP_TYPE);
	pBaseAppData_->addConcernComponentType(BASEAPP_TYPE);
	pCellAppData_->addConcernComponentType(CELLAPP_TYPE);

	INFO_MSG(fmt::format("Dbmgr::initializeEnd: digest({})\n", 
		EntityDef::md5().getDigestStr()));
	
	SCOPED_PROFILE(SCRIPTCALL_PROFILE);

	// All scripts are loaded
	PyObject* pyResult = PyObject_CallMethod(getEntryScript().get(), 
										const_cast<char*>("onDBMgrReady"), 
										const_cast<char*>(""));

	if(pyResult != NULL)
		Py_DECREF(pyResult);
	else
		SCRIPT_ERROR_CHECK();

	pTelnetServer_ = new TelnetServer(&this->dispatcher(), &this->networkInterface());
	pTelnetServer_->pScript(&this->getScript());

	bool ret = pTelnetServer_->start(g_ouroSrvConfig.getDBMgr().telnet_passwd,
		g_ouroSrvConfig.getDBMgr().telnet_deflayer,
		g_ouroSrvConfig.getDBMgr().telnet_port);

	Components::getSingleton().extraData4(pTelnetServer_->port());
	
	return ret && initInterfacesHandler() && initDB();
}

//-------------------------------------------------------------------------------------
bool Dbmgr::installPyModules()
{
	return PythonApp::installPyModules() && script::entitydef::installModule("EntityDef");
}

//-------------------------------------------------------------------------------------
bool Dbmgr::uninstallPyModules()
{
	return script::entitydef::uninstallModule() && PythonApp::uninstallPyModules();
}

//-------------------------------------------------------------------------------------		
void Dbmgr::onInstallPyModules()
{
	PyObject * module = getScript().getModule();

	for (int i = 0; i < SERVER_ERR_MAX; i++)
	{
		if(PyModule_AddIntConstant(module, SERVER_ERR_STR[i], i))
		{
			ERROR_MSG( fmt::format("Dbmgr::onInstallPyModules: Unable to set Ouroboros.{}.\n", SERVER_ERR_STR[i]));
		}
	}

	APPEND_SCRIPT_MODULE_METHOD(module,		executeRawDatabaseCommand,		__py_executeRawDatabaseCommand,		METH_VARARGS,	0);
}

//-------------------------------------------------------------------------------------		
bool Dbmgr::initInterfacesHandler()
{
	std::vector< Network::Address > addresses = g_ouroSrvConfig.interfacesAddrs();
	std::string type = addresses.size() == 0 ? "dbmgr" : "interfaces";

	if (type == "dbmgr")
	{
		InterfacesHandler* pInterfacesHandler = InterfacesHandlerFactory::create(type);

		INFO_MSG(fmt::format("Dbmgr::initInterfacesHandler: interfaces addr({}), accountType:({}), chargeType:({}).\n",
			Network::Address::NONE.c_str(),
			type,
			type));

		if (!pInterfacesHandler->initialize())
			return false;

		pInterfacesHandlers_.push_back(pInterfacesHandler);
	}
	else
	{
		std::vector< Network::Address >::iterator iter = addresses.begin();
		for (; iter != addresses.end(); ++iter)
		{
			InterfacesHandler* pInterfacesHandler = InterfacesHandlerFactory::create(type);

			const Network::Address& addr = (*iter);

			INFO_MSG(fmt::format("Dbmgr::initInterfacesHandler: interfaces addr({}), accountType:({}), chargeType:({}).\n",
				addr.c_str(),
				type,
				type));

			((InterfacesHandler_Interfaces*)pInterfacesHandler)->setAddr(addr);

			if (!pInterfacesHandler->initialize())
				return false;

			pInterfacesHandlers_.push_back(pInterfacesHandler);
		}
	}

	return pInterfacesHandlers_.size() > 0;
}

//-------------------------------------------------------------------------------------		
bool Dbmgr::initDB()
{
	ScriptDefModule* pModule = EntityDef::findScriptModule(DBUtil::accountScriptName());
	if(pModule == NULL)
	{
		ERROR_MSG(fmt::format("Dbmgr::initDB(): not found account script[{}]!\n", 
			DBUtil::accountScriptName()));

		return false;
	}

	ENGINE_COMPONENT_INFO& dbcfg = g_ouroSrvConfig.getDBMgr();
	if (dbcfg.dbInterfaceInfos.size() == 0)
	{
		ERROR_MSG(fmt::format("DBUtil::initialize: not found dbInterface! (ouroboros[_defs].xml->dbmgr->databaseInterfaces)\n"));
		return false;
	}

	if (!DBUtil::initialize())
	{
		ERROR_MSG("Dbmgr::initDB(): can't initialize dbInterface!\n");
		return false;
	}

	bool hasDefaultInterface = false;

	std::vector<DBInterfaceInfo>::iterator dbinfo_iter = dbcfg.dbInterfaceInfos.begin();
	for (; dbinfo_iter != dbcfg.dbInterfaceInfos.end(); ++dbinfo_iter)
	{
		Buffered_DBTasks buffered_DBTasks;
		bufferedDBTasksMaps_.insert(std::make_pair((*dbinfo_iter).name, buffered_DBTasks));
		BUFFERED_DBTASKS_MAP::iterator buffered_DBTasks_iter = bufferedDBTasksMaps_.find((*dbinfo_iter).name);
		buffered_DBTasks_iter->second.dbInterfaceName((*dbinfo_iter).name);
	}

	for (dbinfo_iter = dbcfg.dbInterfaceInfos.begin(); dbinfo_iter != dbcfg.dbInterfaceInfos.end(); ++dbinfo_iter)
	{
		DBInterface* pDBInterface = DBUtil::createInterface((*dbinfo_iter).name);
		if(pDBInterface == NULL)
		{
			ERROR_MSG("Dbmgr::initDB(): can't create dbInterface!\n");
			return false;
		}

		bool ret = DBUtil::initInterface(pDBInterface);
		pDBInterface->detach();
		SAFE_RELEASE(pDBInterface);

		if(!ret)
			return false;

		if (std::string("default") == (*dbinfo_iter).name)
			hasDefaultInterface = true;
	}

	if (!hasDefaultInterface)
	{
		ERROR_MSG("Dbmgr::initDB(): \"default\" dbInterface was not found! (ouroboros[_defs].xml->dbmgr->databaseInterfaces)\n");
		return false;
	}

	if(pUpdateDBServerLogHandler_ == NULL)
		pUpdateDBServerLogHandler_ = new UpdateDBServerLogHandler();

	return true;
}

//-------------------------------------------------------------------------------------
void Dbmgr::finalise()
{
	SAFE_RELEASE(pUpdateDBServerLogHandler_);
	
	SAFE_RELEASE(pGlobalData_);
	SAFE_RELEASE(pBaseAppData_);
	SAFE_RELEASE(pCellAppData_);

	if (pTelnetServer_)
	{
		pTelnetServer_->stop();
		SAFE_RELEASE(pTelnetServer_);
	}

	DBUtil::finalise();
	PythonApp::finalise();
}

//-------------------------------------------------------------------------------------
InterfacesHandler* Dbmgr::findBestInterfacesHandler()
{
	if (pInterfacesHandlers_.size() == 0)
		return NULL;

	static size_t i = 0;

	return pInterfacesHandlers_[i++ % pInterfacesHandlers_.size()];
}

//-------------------------------------------------------------------------------------
void Dbmgr::onReqAllocEntityID(Network::Channel* pChannel, COMPONENT_ORDER componentType, COMPONENT_ID componentID)
{
	Ouroboros::COMPONENT_TYPE ct = static_cast<Ouroboros::COMPONENT_TYPE>(componentType);

	// Get an id segment and transfer it to IDClient
	std::pair<ENTITY_ID, ENTITY_ID> idRange = idServer_.allocRange();
	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);

	if(ct == BASEAPP_TYPE)
		(*pBundle).newMessage(BaseappInterface::onReqAllocEntityID);
	else	
		(*pBundle).newMessage(CellappInterface::onReqAllocEntityID);

	(*pBundle) << idRange.first;
	(*pBundle) << idRange.second;
	pChannel->send(pBundle);
}

//-------------------------------------------------------------------------------------
void Dbmgr::onRegisterNewApp(Network::Channel* pChannel, int32 uid, std::string& username, 
						COMPONENT_TYPE componentType, COMPONENT_ID componentID, COMPONENT_ORDER globalorderID, COMPONENT_ORDER grouporderID,
						uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport, std::string& extaddrEx)
{
	if(pChannel->isExternal())
		return;

	ServerApp::onRegisterNewApp(pChannel, uid, username, componentType, componentID, globalorderID, grouporderID,
						intaddr, intport, extaddr, extport, extaddrEx);

	Ouroboros::COMPONENT_TYPE tcomponentType = (Ouroboros::COMPONENT_TYPE)componentType;
	
	COMPONENT_ORDER startGroupOrder = 1;
	COMPONENT_ORDER startGlobalOrder = Components::getSingleton().getGlobalOrderLog()[getUserUID()];

	if(grouporderID > 0)
		startGroupOrder = grouporderID;

	if(globalorderID > 0)
		startGlobalOrder = globalorderID;

	if(pSyncAppDatasHandler_ == NULL)
		pSyncAppDatasHandler_ = new SyncAppDatasHandler(this->networkInterface());

	// Next step:
	// If you are connected to dbmgr, you need to wait to receive the app initial information.
	// For example: the initial assignment of the entityID section and the order in which the app is launched (whether the first baseapp is started)
	if(tcomponentType == BASEAPP_TYPE || 
		tcomponentType == CELLAPP_TYPE || 
		tcomponentType == LOGINAPP_TYPE)
	{
		switch(tcomponentType)
		{
		case BASEAPP_TYPE:
			{
				if(grouporderID <= 0)
					startGroupOrder = Components::getSingleton().getBaseappGroupOrderLog()[getUserUID()];
			}
			break;
		case CELLAPP_TYPE:
			{
				if(grouporderID <= 0)
					startGroupOrder = Components::getSingleton().getCellappGroupOrderLog()[getUserUID()];
			}
			break;
		case LOGINAPP_TYPE:
			if(grouporderID <= 0)
				startGroupOrder = Components::getSingleton().getLoginappGroupOrderLog()[getUserUID()];

			break;
		default:
			break;
		}
	}

	pSyncAppDatasHandler_->pushApp(componentID, startGroupOrder, startGlobalOrder);

	// Register yourself to all other baseapp and cellapp if it is baseapp or cellapp
	if(tcomponentType == BASEAPP_TYPE || 
		tcomponentType == CELLAPP_TYPE)
	{
		Ouroboros::COMPONENT_TYPE broadcastCpTypes[2] = {BASEAPP_TYPE, CELLAPP_TYPE};
		for(int idx = 0; idx < 2; ++idx)
		{
			Components::COMPONENTS& cts = Components::getSingleton().getComponents(broadcastCpTypes[idx]);
			Components::COMPONENTS::iterator fiter = cts.begin();
			for(; fiter != cts.end(); ++fiter)
			{
				if((*fiter).cid == componentID)
					continue;

				Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
				ENTITTAPP_COMMON_NETWORK_MESSAGE(broadcastCpTypes[idx], (*pBundle), onGetEntityAppFromDbmgr);
				
				if(tcomponentType == BASEAPP_TYPE)
				{
					BaseappInterface::onGetEntityAppFromDbmgrArgs11::staticAddToBundle((*pBundle), 
						uid, username, componentType, componentID, startGlobalOrder, startGroupOrder,
							intaddr, intport, extaddr, extport, g_ouroSrvConfig.getConfig().externalAddress);
				}
				else
				{
					CellappInterface::onGetEntityAppFromDbmgrArgs11::staticAddToBundle((*pBundle), 
						uid, username, componentType, componentID, startGlobalOrder, startGroupOrder,
							intaddr, intport, extaddr, extport, g_ouroSrvConfig.getConfig().externalAddress);
				}
				
				OURO_ASSERT((*fiter).pChannel != NULL);
				(*fiter).pChannel->send(pBundle);
			}
		}
	}
}

//-------------------------------------------------------------------------------------
void Dbmgr::onGlobalDataClientLogon(Network::Channel* pChannel, COMPONENT_TYPE componentType)
{
	if(BASEAPP_TYPE == componentType)
	{
		pBaseAppData_->onGlobalDataClientLogon(pChannel, componentType);
		pGlobalData_->onGlobalDataClientLogon(pChannel, componentType);
	}
	else if(CELLAPP_TYPE == componentType)
	{
		pGlobalData_->onGlobalDataClientLogon(pChannel, componentType);
		pCellAppData_->onGlobalDataClientLogon(pChannel, componentType);
	}
	else
	{
		ERROR_MSG(fmt::format("Dbmgr::onGlobalDataClientLogon: nonsupport {}!\n",
			COMPONENT_NAME_EX(componentType)));
	}
}

//-------------------------------------------------------------------------------------
void Dbmgr::onBroadcastGlobalDataChanged(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	uint8 dataType;
	std::string key, value;
	bool isDelete;
	COMPONENT_TYPE componentType;
	
	s >> dataType;
	s >> isDelete;

	s.readBlob(key);

	if(!isDelete)
	{
		s.readBlob(value);
	}

	s >> componentType;

	switch(dataType)
	{
	case GlobalDataServer::GLOBAL_DATA:
		if(isDelete)
			pGlobalData_->del(pChannel, componentType, key);
		else
			pGlobalData_->write(pChannel, componentType, key, value);
		break;
	case GlobalDataServer::BASEAPP_DATA:
		if(isDelete)
			pBaseAppData_->del(pChannel, componentType, key);
		else
			pBaseAppData_->write(pChannel, componentType, key, value);
		break;
	case GlobalDataServer::CELLAPP_DATA:
		if(isDelete)
			pCellAppData_->del(pChannel, componentType, key);
		else
			pCellAppData_->write(pChannel, componentType, key, value);
		break;
	default:
		OURO_ASSERT(false && "dataType error!\n");
		break;
	};
}

//-------------------------------------------------------------------------------------
void Dbmgr::reqCreateAccount(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	std::string registerName, password, datas;
	uint8 uatype = 0;

	s >> registerName >> password >> uatype;
	s.readBlob(datas);

	if(registerName.size() == 0)
	{
		ERROR_MSG("Dbmgr::reqCreateAccount: registerName is empty.\n");
		return;
	}

	findBestInterfacesHandler()->createAccount(pChannel, registerName, password, datas, ACCOUNT_TYPE(uatype));
	numCreatedAccount_++;
}

//-------------------------------------------------------------------------------------
void Dbmgr::onCreateAccountCBFromInterfaces(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	findBestInterfacesHandler()->onCreateAccountCB(s);
}

//-------------------------------------------------------------------------------------
void Dbmgr::onAccountLogin(Network::Channel* pChannel, Ouroboros::MemoryStream& s) 
{
	std::string loginName, password, datas;
	s >> loginName >> password;
	s.readBlob(datas);

	if(loginName.size() == 0)
	{
		ERROR_MSG("Dbmgr::onAccountLogin: loginName is empty.\n");
		return;
	}

	findBestInterfacesHandler()->loginAccount(pChannel, loginName, password, datas);
}

//-------------------------------------------------------------------------------------
void Dbmgr::onLoginAccountCBBFromInterfaces(Network::Channel* pChannel, Ouroboros::MemoryStream& s) 
{
	findBestInterfacesHandler()->onLoginAccountCB(s);
}

//-------------------------------------------------------------------------------------
void Dbmgr::queryAccount(Network::Channel* pChannel, 
						 std::string& accountName, 
						 std::string& password,
						 bool needCheckPassword,
						 COMPONENT_ID componentID,
						 ENTITY_ID entityID,
						 DBID entityDBID, 
						 uint32 ip, 
						 uint16 port)
{
	if(accountName.size() == 0)
	{
		ERROR_MSG("Dbmgr::queryAccount: accountName is empty.\n");
		return;
	}

	Buffered_DBTasks* pBuffered_DBTasks = 
		findBufferedDBTask(Dbmgr::getSingleton().selectAccountDBInterfaceName(accountName));

	if (!pBuffered_DBTasks)
	{
		ERROR_MSG(fmt::format("Dbmgr::queryAccount: not found dbInterface({})!\n", 
			Dbmgr::getSingleton().selectAccountDBInterfaceName(accountName)));
		return;
	}

	pBuffered_DBTasks->addTask(new DBTaskQueryAccount(pChannel->addr(), accountName, password, needCheckPassword,
		componentID, entityID, entityDBID, ip, port));

	numQueryEntity_++;
}

//-------------------------------------------------------------------------------------
void Dbmgr::onAccountOnline(Network::Channel* pChannel, 
							std::string& accountName, 
							COMPONENT_ID componentID, 
							ENTITY_ID entityID)
{
	// bufferedDBTasks_.addTask(new DBTaskAccountOnline(pChannel->addr(), 
	//	accountName, componentID, entityID));
}

//-------------------------------------------------------------------------------------
void Dbmgr::onEntityOffline(Network::Channel* pChannel, DBID dbid, ENTITY_SCRIPT_UID sid, uint16 dbInterfaceIndex)
{
	Buffered_DBTasks* pBuffered_DBTasks = findBufferedDBTask(g_ouroSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex));
	if (!pBuffered_DBTasks)
	{
		ERROR_MSG(fmt::format("Dbmgr::onEntityOffline: not found dbInterfaceIndex({})!\n", dbInterfaceIndex));
		return;
	}

	pBuffered_DBTasks->addTask(new DBTaskEntityOffline(pChannel->addr(), dbid, sid));
}

//-------------------------------------------------------------------------------------
void Dbmgr::executeRawDatabaseCommand(Network::Channel* pChannel, 
									  Ouroboros::MemoryStream& s)
{
	ENTITY_ID entityID = -1;
	s >> entityID;

	uint16 dbInterfaceIndex = 0;
	s >> dbInterfaceIndex;

	std::string dbInterfaceName = g_ouroSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex);
	if (dbInterfaceName.size() == 0)
	{
		ERROR_MSG(fmt::format("Dbmgr::executeRawDatabaseCommand: not found dbInterface({})!\n", dbInterfaceName));
		s.done();
		return;
	}

	if (entityID == -1)
	{
		thread::ThreadPool* pThreadPool = DBUtil::pThreadPool(dbInterfaceName);
		if (!pThreadPool)
		{
			ERROR_MSG(fmt::format("Dbmgr::executeRawDatabaseCommand: not found pThreadPool(dbInterface={})!\n", dbInterfaceName));
			s.done();
			return;
		}

		pThreadPool->addTask(new DBTaskExecuteRawDatabaseCommand(pChannel ? pChannel->addr() : Network::Address::NONE, s));
	}
	else
	{
		Buffered_DBTasks* pBuffered_DBTasks = findBufferedDBTask(dbInterfaceName);
		if (!pBuffered_DBTasks)
		{
			ERROR_MSG(fmt::format("Dbmgr::executeRawDatabaseCommand: not found pBuffered_DBTasks(dbInterface={})!\n", dbInterfaceName));
			s.done();
			return;
		}

		pBuffered_DBTasks->addTask(new DBTaskExecuteRawDatabaseCommandByEntity(pChannel ? pChannel->addr() : Network::Address::NONE, s, entityID));
	}

	s.done();

	++numExecuteRawDatabaseCommand_;
}

//-------------------------------------------------------------------------------------
PyObject* Dbmgr::__py_executeRawDatabaseCommand(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	PyObject* pycallback = NULL;
	PyObject* pyDBInterfaceName = NULL;
	int ret = -1;
	ENTITY_ID eid = -1;

	char* data = NULL;
	Py_ssize_t size;

	if (argCount == 4)
		ret = PyArg_ParseTuple(args, "s#|O|i|O", &data, &size, &pycallback, &eid, &pyDBInterfaceName);
	else if (argCount == 3)
		ret = PyArg_ParseTuple(args, "s#|O|i", &data, &size, &pycallback, &eid);
	else if (argCount == 2)
		ret = PyArg_ParseTuple(args, "s#|O", &data, &size, &pycallback);
	else if (argCount == 1)
		ret = PyArg_ParseTuple(args, "s#", &data, &size);

	if (ret == -1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::executeRawDatabaseCommand: args error!");
		PyErr_PrintEx(0);
		S_Return;
	}

	std::string dbInterfaceName = "default";
	if (pyDBInterfaceName)
	{
		dbInterfaceName = PyUnicode_AsUTF8AndSize(pyDBInterfaceName, NULL);

		if (!g_ouroSrvConfig.dbInterface(dbInterfaceName))
		{
			PyErr_Format(PyExc_TypeError, "Ouroboros::executeRawDatabaseCommand: args4, incorrect dbInterfaceName(%s)!",
				dbInterfaceName.c_str());

			PyErr_PrintEx(0);
			S_Return;
		}
	}

	Dbmgr::getSingleton().executeRawDatabaseCommand(data, (uint32)size, pycallback, eid, dbInterfaceName);
	S_Return;
}

//-------------------------------------------------------------------------------------
void Dbmgr::executeRawDatabaseCommand(const char* datas, uint32 size, PyObject* pycallback, ENTITY_ID eid, const std::string& dbInterfaceName)
{
	if (datas == NULL)
	{
		ERROR_MSG("Ouroboros::executeRawDatabaseCommand: execute error!\n");
		return;
	}

	int dbInterfaceIndex = g_ouroSrvConfig.dbInterfaceName2dbInterfaceIndex(dbInterfaceName);
	if (dbInterfaceIndex < 0)
	{
		ERROR_MSG(fmt::format("Ouroboros::executeRawDatabaseCommand: not found dbInterface({})!\n",
			dbInterfaceName));

		return;
	}

	//INFO_MSG(fmt::format("Ouroboros::executeRawDatabaseCommand{}:{}.\n", (eid > 0 ? fmt::format("(entityID={})", eid) : ""), datas));

	MemoryStream* pMemoryStream = MemoryStream::createPoolObject(OBJECTPOOL_POINT);
	(*pMemoryStream) << eid;
	(*pMemoryStream) << (uint16)dbInterfaceIndex;
	(*pMemoryStream) << componentID_ << componentType_;

	CALLBACK_ID callbackID = 0;

	if (pycallback && PyCallable_Check(pycallback))
		callbackID = callbackMgr().save(pycallback);

	(*pMemoryStream) << callbackID;
	(*pMemoryStream) << size;
	(*pMemoryStream).append(datas, size);
	executeRawDatabaseCommand(NULL, *pMemoryStream);
	MemoryStream::reclaimPoolObject(pMemoryStream);
}

//-------------------------------------------------------------------------------------
void Dbmgr::onExecuteRawDatabaseCommandCB(Ouroboros::MemoryStream& s)
{
	std::string err;
	CALLBACK_ID callbackID = 0;
	uint32 nrows = 0;
	uint32 nfields = 0;
	uint64 affectedRows = 0;
	uint64 lastInsertID = 0;

	PyObject* pResultSet = NULL;
	PyObject* pAffectedRows = NULL;
	PyObject* pLastInsertID = NULL;
	PyObject* pErrorMsg = NULL;

	s >> callbackID;
	s >> err;

	if (err.size() <= 0)
	{
		s >> nfields;

		pErrorMsg = Py_None;
		Py_INCREF(pErrorMsg);

		if (nfields > 0)
		{
			pAffectedRows = Py_None;
			Py_INCREF(pAffectedRows);

			pLastInsertID = Py_None;
			Py_INCREF(pLastInsertID);

			s >> nrows;

			pResultSet = PyList_New(nrows);
			for (uint32 i = 0; i < nrows; ++i)
			{
				PyObject* pRow = PyList_New(nfields);
				for (uint32 j = 0; j < nfields; ++j)
				{
					std::string cell;
					s.readBlob(cell);

					PyObject* pCell = NULL;

					if (cell == "OURO_QUERY_DB_NULL")
					{
						Py_INCREF(Py_None);
						pCell = Py_None;
					}
					else
					{
						pCell = PyBytes_FromStringAndSize(cell.data(), cell.length());
					}

					PyList_SET_ITEM(pRow, j, pCell);
				}

				PyList_SET_ITEM(pResultSet, i, pRow);
			}
		}
		else
		{
			pResultSet = Py_None;
			Py_INCREF(pResultSet);

			pErrorMsg = Py_None;
			Py_INCREF(pErrorMsg);

			s >> affectedRows;

			pAffectedRows = PyLong_FromUnsignedLongLong(affectedRows);

			s >> lastInsertID;
			pLastInsertID = PyLong_FromUnsignedLongLong(lastInsertID);
		}
	}
	else
	{
		pResultSet = Py_None;
		Py_INCREF(pResultSet);

		pErrorMsg = PyUnicode_FromString(err.c_str());

		pAffectedRows = Py_None;
		Py_INCREF(pAffectedRows);

		pLastInsertID = Py_None;
		Py_INCREF(pLastInsertID);
	}

	s.done();

	//DEBUG_MSG(fmt::format("Cellapp::onExecuteRawDatabaseCommandCB: nrows={}, nfields={}, err={}.\n", 
	//	nrows, nfields, err.c_str()));

	if (callbackID > 0)
	{
		SCOPED_PROFILE(SCRIPTCALL_PROFILE);

		PyObjectPtr pyfunc = pyCallbackMgr_.take(callbackID);
		if (pyfunc != NULL)
		{
			PyObject* pyResult = PyObject_CallFunction(pyfunc.get(),
				const_cast<char*>("OOOO"),
				pResultSet, pAffectedRows, pLastInsertID, pErrorMsg);

			if (pyResult != NULL)
				Py_DECREF(pyResult);
			else
				SCRIPT_ERROR_CHECK();
		}
		else
		{
			ERROR_MSG(fmt::format("Cellapp::onExecuteRawDatabaseCommandCB: not found callback:{}.\n",
				callbackID));
		}
	}

	Py_XDECREF(pResultSet);
	Py_XDECREF(pAffectedRows);
	Py_XDECREF(pLastInsertID);
	Py_XDECREF(pErrorMsg);
}

//-------------------------------------------------------------------------------------
void Dbmgr::writeEntity(Network::Channel* pChannel, 
						Ouroboros::MemoryStream& s)
{
	ENTITY_ID eid;
	DBID entityDBID;
	COMPONENT_ID componentID;
	uint16 dbInterfaceIndex;

	s >> componentID >> eid >> entityDBID >> dbInterfaceIndex;

	Buffered_DBTasks* pBuffered_DBTasks = findBufferedDBTask(g_ouroSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex));
	if (!pBuffered_DBTasks)
	{
		ERROR_MSG(fmt::format("Dbmgr::writeEntity: not found dbInterfaceIndex({})!\n", dbInterfaceIndex));
		s.done();
		return;
	}

	pBuffered_DBTasks->addTask(new DBTaskWriteEntity(pChannel->addr(), componentID, eid, entityDBID, s));
	s.done();

	++numWrittenEntity_;
}

//-------------------------------------------------------------------------------------
void Dbmgr::removeEntity(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	ENTITY_ID eid;
	DBID entityDBID;
	COMPONENT_ID componentID;
	uint16 dbInterfaceIndex;

	s >> dbInterfaceIndex >> componentID >> eid >> entityDBID;
	OURO_ASSERT(entityDBID > 0);

	Buffered_DBTasks* pBuffered_DBTasks = findBufferedDBTask(g_ouroSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex));
	if (!pBuffered_DBTasks)
	{
		ERROR_MSG(fmt::format("Dbmgr::removeEntity: not found dbInterfaceIndex({})!\n", dbInterfaceIndex));
		s.done();
		return;
	}

	pBuffered_DBTasks->addTask(new DBTaskRemoveEntity(pChannel->addr(),
		componentID, eid, entityDBID, s));

	s.done();

	++numRemovedEntity_;
}

//-------------------------------------------------------------------------------------
void Dbmgr::entityAutoLoad(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	COMPONENT_ID componentID;
	ENTITY_SCRIPT_UID entityType;
	ENTITY_ID start;
	ENTITY_ID end;
	uint16 dbInterfaceIndex = 0;

	s >> dbInterfaceIndex >> componentID >> entityType >> start >> end;

	DBUtil::pThreadPool(g_ouroSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex))->
		addTask(new DBTaskEntityAutoLoad(pChannel->addr(), componentID, entityType, start, end));
}

//-------------------------------------------------------------------------------------
void Dbmgr::deleteEntityByDBID(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	COMPONENT_ID componentID;
	ENTITY_SCRIPT_UID sid;
	CALLBACK_ID callbackID = 0;
	DBID entityDBID;
	uint16 dbInterfaceIndex = 0;

	s >> dbInterfaceIndex >> componentID >> entityDBID >> callbackID >> sid;
	OURO_ASSERT(entityDBID > 0);

	DBUtil::pThreadPool(g_ouroSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex))->
		addTask(new DBTaskDeleteEntityByDBID(pChannel->addr(), componentID, entityDBID, callbackID, sid));
}

//-------------------------------------------------------------------------------------
void Dbmgr::lookUpEntityByDBID(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	COMPONENT_ID componentID;
	ENTITY_SCRIPT_UID sid;
	CALLBACK_ID callbackID = 0;
	DBID entityDBID;
	uint16 dbInterfaceIndex = 0;

	s >> dbInterfaceIndex >> componentID >> entityDBID >> callbackID >> sid;
	OURO_ASSERT(entityDBID > 0);

	DBUtil::pThreadPool(g_ouroSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex))->
		addTask(new DBTaskLookUpEntityByDBID(pChannel->addr(), componentID, entityDBID, callbackID, sid));
}

//-------------------------------------------------------------------------------------
void Dbmgr::queryEntity(Network::Channel* pChannel, uint16 dbInterfaceIndex, COMPONENT_ID componentID, int8 queryMode, DBID dbid,
	std::string& entityType, CALLBACK_ID callbackID, ENTITY_ID entityID)
{
	bufferedDBTasksMaps_[g_ouroSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex)].
		addTask(new DBTaskQueryEntity(pChannel->addr(), queryMode, entityType, dbid, componentID, callbackID, entityID));

	numQueryEntity_++;
}

//-------------------------------------------------------------------------------------
void Dbmgr::syncEntityStreamTemplate(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	int rpos = s.rpos();
	EntityTables::ENTITY_TABLES_MAP::iterator iter = EntityTables::sEntityTables.begin();
	for (; iter != EntityTables::sEntityTables.end(); ++iter)
	{
		OUROAccountTable* pTable =
			static_cast<OUROAccountTable*>(iter->second.findOUROTable(OURO_TABLE_PERFIX "_accountinfos"));

		OURO_ASSERT(pTable);

		s.rpos(rpos);
		pTable->accountDefMemoryStream(s);
	}

	s.done();
}

//-------------------------------------------------------------------------------------
void Dbmgr::charge(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	findBestInterfacesHandler()->charge(pChannel, s);
}

//-------------------------------------------------------------------------------------
void Dbmgr::onChargeCB(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	findBestInterfacesHandler()->onChargeCB(s);
}

//-------------------------------------------------------------------------------------
void Dbmgr::eraseClientReq(Network::Channel* pChannel, std::string& logkey)
{
	std::vector<InterfacesHandler*>::iterator iter = pInterfacesHandlers_.begin();
	for(; iter != pInterfacesHandlers_.end(); ++iter)
		(*iter)->eraseClientReq(pChannel, logkey);
}

//-------------------------------------------------------------------------------------
void Dbmgr::accountActivate(Network::Channel* pChannel, std::string& scode)
{
	INFO_MSG(fmt::format("Dbmgr::accountActivate: code={}.\n", scode));
	findBestInterfacesHandler()->accountActivate(pChannel, scode);
}

//-------------------------------------------------------------------------------------
void Dbmgr::accountReqResetPassword(Network::Channel* pChannel, std::string& accountName)
{
	INFO_MSG(fmt::format("Dbmgr::accountReqResetPassword: accountName={}.\n", accountName));
	findBestInterfacesHandler()->accountReqResetPassword(pChannel, accountName);
}

//-------------------------------------------------------------------------------------
void Dbmgr::accountResetPassword(Network::Channel* pChannel, std::string& accountName, std::string& newpassword, std::string& code)
{
	INFO_MSG(fmt::format("Dbmgr::accountResetPassword: accountName={}.\n", accountName));
	findBestInterfacesHandler()->accountResetPassword(pChannel, accountName, newpassword, code);
}

//-------------------------------------------------------------------------------------
void Dbmgr::accountReqBindMail(Network::Channel* pChannel, ENTITY_ID entityID, std::string& accountName, 
							   std::string& password, std::string& email)
{
	INFO_MSG(fmt::format("Dbmgr::accountReqBindMail: accountName={}, email={}.\n", accountName, email));
	findBestInterfacesHandler()->accountReqBindMail(pChannel, entityID, accountName, password, email);
}

//-------------------------------------------------------------------------------------
void Dbmgr::accountBindMail(Network::Channel* pChannel, std::string& username, std::string& scode)
{
	INFO_MSG(fmt::format("Dbmgr::accountBindMail: username={}, scode={}.\n", username, scode));
	findBestInterfacesHandler()->accountBindMail(pChannel, username, scode);
}

//-------------------------------------------------------------------------------------
void Dbmgr::accountNewPassword(Network::Channel* pChannel, ENTITY_ID entityID, std::string& accountName, 
							   std::string& password, std::string& newpassword)
{
	INFO_MSG(fmt::format("Dbmgr::accountNewPassword: accountName={}.\n", accountName));
	findBestInterfacesHandler()->accountNewPassword(pChannel, entityID, accountName, password, newpassword);
}

//-------------------------------------------------------------------------------------
std::string Dbmgr::selectAccountDBInterfaceName(const std::string& name)
{
	std::string dbInterfaceName = "default";

	// hand the request to the script
	SCOPED_PROFILE(SCRIPTCALL_PROFILE);
	PyObject* pyResult = PyObject_CallMethod(getEntryScript().get(),
		const_cast<char*>("onSelectAccountDBInterface"),
		const_cast<char*>("s"),
		name.c_str());

	if (pyResult != NULL)
	{
		dbInterfaceName = PyUnicode_AsUTF8AndSize(pyResult, NULL);
		Py_DECREF(pyResult);
	}
	else
	{
		SCRIPT_ERROR_CHECK();
	}

	if (dbInterfaceName == "" || g_ouroSrvConfig.dbInterface(dbInterfaceName) == NULL)
	{
		ERROR_MSG(fmt::format("Dbmgr::selectAccountDBInterfaceName: not found dbInterface({}), accountName={}.\n", dbInterfaceName, name));
		return "default";
	}

	return dbInterfaceName;
}

//-------------------------------------------------------------------------------------
void Dbmgr::onChannelDeregister(Network::Channel * pChannel)
{
	// If the app is dead
	if (pChannel->isInternal())
	{
		Components::ComponentInfos* cinfo = Components::getSingleton().findComponent(pChannel);
		if (cinfo)
		{
			if (cinfo->componentType == BASEAPP_TYPE)
			{
				loseBaseappts_[cinfo->cid] = timestamp() + uint64(60 * stampsPerSecond());
				WARNING_MSG(fmt::format("Dbmgr::onChannelDeregister(): If the process cannot be resumed, the entitylog(baseapp={}) will be cleaned up after 60 seconds!\n", cinfo->cid));
			}
		}
	}
	
	ServerApp::onChannelDeregister(pChannel);
}

//-------------------------------------------------------------------------------------
}
