// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com

#ifndef OURO_ENTITY_APP_H
#define OURO_ENTITY_APP_H

// common include
#include "pyscript/py_gc.h"
#include "pyscript/script.h"
#include "pyscript/pyprofile.h"
#include "pyscript/pyprofile_handler.h"
#include "common/common.h"
#include "common/timer.h"
#include "common/smartpointer.h"
#include "pyscript/pyobject_pointer.h"
#include "pyscript/pywatcher.h"
#include "helper/debug_helper.h"
#include "helper/script_loglevel.h"
#include "helper/profile.h"
#include "server/ouromain.h"	
#include "server/script_timers.h"
#include "server/idallocate.h"
#include "server/serverconfig.h"
#include "server/globaldata_client.h"
#include "server/globaldata_server.h"
#include "server/callbackmgr.h"	
#include "entitydef/entitydef.h"
#include "entitydef/entities.h"
#include "entitydef/entity_call.h"
#include "entitydef/entity_component.h"
#include "entitydef/scriptdef_module.h"
#include "network/message_handler.h"
#include "resmgr/resmgr.h"
#include "helper/console_helper.h"
#include "server/serverapp.h"

#if OURO_PLATFORM == PLATFORM_WIN32
#pragma warning (disable : 4996)
#endif

	
namespace Ouroboros{

template<class E>
class EntityApp : public ServerApp
{
public:
	enum TimeOutType
	{
		TIMEOUT_GAME_TICK = TIMEOUT_SERVERAPP_MAX + 1,
		TIMEOUT_ENTITYAPP_MAX
	};

public:
	EntityApp(Network::EventDispatcher& dispatcher, 
		Network::NetworkInterface& ninterface, 
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID);

	~EntityApp();
	
	/** 
		Related processing interface
	*/
	virtual void handleTimeout(TimerHandle handle, void * arg);
	virtual void handleGameTick();

	/**
		Find the corresponding instance by entityID
	*/
	E* findEntity(ENTITY_ID entityID);

	/** 
		Destroy an entity by entityID
	*/
	virtual bool destroyEntity(ENTITY_ID entityID, bool callScript);

	/**
		Try to get an instance of entity by entityCall, etc.
		Because this entity does not necessarily exist on this component.
	*/
	PyObject* tryGetEntity(COMPONENT_ID componentID, ENTITY_ID eid);

	/**
		Try to get an instance of channel by entityCall
	*/
	Network::Channel* findChannelByEntityCall(EntityCallAbstract& entityCall);

	Ouroboros::script::Script& getScript(){ return script_; }
	PyObjectPtr getEntryScript(){ return entryScript_; }

	void registerScript(PyTypeObject*);
	int registerPyObjectToScript(const char* attrName, PyObject* pyObj);
	int unregisterPyObjectToScript(const char* attrName);

	bool installPyScript();
	virtual bool installPyModules();
	virtual void onInstallPyModules() {};
	virtual bool uninstallPyModules();
	bool uninstallPyScript();
	bool installEntityDef();
	
	virtual bool initializeWatcher();

	virtual void finalise();
	virtual bool inInitialize();
	virtual bool initialize();

	virtual void onSignalled(int sigNum);
	
	Entities<E>* pEntities() const{ return pEntities_; }
	ArraySize entitiesSize() const { return (ArraySize)pEntities_->size(); }

	PY_CALLBACKMGR& callbackMgr(){ return pyCallbackMgr_; }	

	EntityIDClient& idClient(){ return idClient_; }

	/**
		Create an entity
	*/
	E* createEntity(const char* entityType, PyObject* params,
		bool isInitializeScript = true, ENTITY_ID eid = 0, bool initProperty = true);

	virtual E* onCreateEntity(PyObject* pyEntity, ScriptDefModule* sm, ENTITY_ID eid);

		/** Network Interface
		Request to assign a callback to the ENTITY_ID segment
	*/
	void onReqAllocEntityID(Network::Channel* pChannel, ENTITY_ID startID, ENTITY_ID endID);

		/** Network Interface
		Dbmgr sends initial information
		startID: initial allocation ENTITY_ID segment start position
		endID: initial allocation ENTITY_ID segment end position
		startGlobalOrder: global startup sequence including various components
		startGroupOrder: The startup order within the group, such as the first few starts in all baseapps.
	*/
	void onDbmgrInitCompleted(Network::Channel* pChannel, 
		GAME_TIME gametime, ENTITY_ID startID, ENTITY_ID endID, COMPONENT_ORDER startGlobalOrder, 
		COMPONENT_ORDER startGroupOrder, const std::string& digest);

		/** Network Interface
		Dbmgr broadcast changes to global data
	*/
	void onBroadcastGlobalDataChanged(Network::Channel* pChannel, Ouroboros::MemoryStream& s);


		/** Network Interface
		Request to execute a python instruction
	*/
	void onExecScriptCommand(Network::Channel* pChannel, Ouroboros::MemoryStream& s);

	/** 
		Console request start profile
	*/
	virtual void startProfile_(Network::Channel* pChannel, std::string profileName, int8 profileType, uint32 timelen);

	/**
		Allow script to assert the underlying
	*/
	static PyObject* __py_assert(PyObject* self, PyObject* args);
	
	/**
		Get the app release status, which can be obtained in the script
	*/
	static PyObject* __py_getAppPublish(PyObject* self, PyObject* args);

	/**
		Set the script output type prefix
	*/
	static PyObject* __py_setScriptLogType(PyObject* self, PyObject* args);

	/**
		Get watcher value
	*/
	static PyObject* __py_getWatcher(PyObject* self, PyObject* args);
	static PyObject* __py_getWatcherDir(PyObject* self, PyObject* args);

	/**
		Re-import all scripts
	*/
	virtual void reloadScript(bool fullReload);
	virtual void onReloadScript(bool fullReload);

	/**
		Get the full path of the resource through the relative path
	*/
	static PyObject* __py_getResFullPath(PyObject* self, PyObject* args);

	/**
		Determine whether a resource exists through a relative path
	*/
	static PyObject* __py_hasRes(PyObject* self, PyObject* args);

	/**
		Open file
	*/
	static PyObject* __py_ouroOpen(PyObject* self, PyObject* args);

	/**
		List all files in the directory
	*/
	static PyObject* __py_listPathRes(PyObject* self, PyObject* args);

	/**
		Match the relative path to get the full path
	*/
	static PyObject* __py_matchPath(PyObject* self, PyObject* args);

	/**
		Update load situation
	*/
	int tickPassedPercent(uint64 curr = timestamp());
	float getLoad() const { return load_; }
	void updateLoad();
	virtual void onUpdateLoad(){}
	virtual void calcLoad(float spareTime);
	uint64 checkTickPeriod();

protected:
	Ouroboros::script::Script								script_;
	std::vector<PyTypeObject*>								scriptBaseTypes_;

	PyObjectPtr												entryScript_;

	EntityIDClient											idClient_;

	// store all the containers of the entity
	Entities<E>*											pEntities_;

	TimerHandle												gameTimer_;

	// globalData
	GlobalDataClient*										pGlobalData_;

	PY_CALLBACKMGR											pyCallbackMgr_;

	uint64													lastTimestamp_;

	// process current load
	float													load_;
};


template<class E>
EntityApp<E>::EntityApp(Network::EventDispatcher& dispatcher, 
					 Network::NetworkInterface& ninterface, 
					 COMPONENT_TYPE componentType,
					 COMPONENT_ID componentID):
ServerApp(dispatcher, ninterface, componentType, componentID),
script_(),
scriptBaseTypes_(),
entryScript_(),
idClient_(),
pEntities_(NULL),
gameTimer_(),
pGlobalData_(NULL),
pyCallbackMgr_(),
lastTimestamp_(timestamp()),
load_(0.f)
{
	ScriptTimers::initialize(*this);
	idClient_.pApp(this);

	// Initialize the EntityDef module to get the entity entity function address
	EntityDef::setGetEntityFunc(std::tr1::bind(&EntityApp<E>::tryGetEntity, this,
		std::tr1::placeholders::_1, std::tr1::placeholders::_2));

	// Initialize the entityCall module to get the channel function address
	EntityCallAbstract::setFindChannelFunc(std::tr1::bind(&EntityApp<E>::findChannelByEntityCall, this,
		std::tr1::placeholders::_1));
}

template<class E>
EntityApp<E>::~EntityApp()
{
}

template<class E>
bool EntityApp<E>::inInitialize()
{
	if(!installPyScript())
		return false;

	if(!installPyModules())
		return false;
	
	return installEntityDef();
}

template<class E>
bool EntityApp<E>::initialize()
{
	bool ret = ServerApp::initialize();
	if(ret)
	{
		gameTimer_ = this->dispatcher().addTimer(1000000 / g_ouroSrvConfig.gameUpdateHertz(), this,
								reinterpret_cast<void *>(TIMEOUT_GAME_TICK));
	}

	lastTimestamp_ = timestamp();
	return ret;
}

template<class E>
bool EntityApp<E>::initializeWatcher()
{
	WATCH_OBJECT("entitiesSize", this, &EntityApp<E>::entitiesSize);
	return ServerApp::initializeWatcher();
}

template<class E>
void EntityApp<E>::finalise(void)
{
	gameTimer_.cancel();

	WATCH_FINALIZE;
	
	pyCallbackMgr_.finalise();
	ScriptTimers::finalise(*this);

	if(pEntities_)
		pEntities_->finalise();
	
	uninstallPyScript();

	ServerApp::finalise();
}

template<class E>
bool EntityApp<E>::installEntityDef()
{
	EntityDef::entityAliasID(ServerConfig::getSingleton().getCellApp().aliasEntityID);
	EntityDef::entitydefAliasID(ServerConfig::getSingleton().getCellApp().entitydefAliasID);
	
	if(!EntityDef::installScript(this->getScript().getModule()))
		return false;

	// Initialize all extension modules
	// assets/scripts/
	if(!EntityDef::initialize(scriptBaseTypes_, componentType_)){
		return false;
	}

	ENGINE_COMPONENT_INFO& dbcfg = g_ouroSrvConfig.getDBMgr();
	ScriptDefModule* pModule = EntityDef::findScriptModule(dbcfg.dbAccountEntityScriptType);
	if(pModule == NULL)
	{
		ERROR_MSG(fmt::format("EntityApp::installEntityDef(): not found account script[{}], defined(ouroboros[_defs].xml->dbmgr->account_system->accountEntityScriptType and entities.xml)!\n", 
			dbcfg.dbAccountEntityScriptType));

		return false;
	}

	return true;
}

template<class E>
int EntityApp<E>::registerPyObjectToScript(const char* attrName, PyObject* pyObj)
{ 
	return script_.registerToModule(attrName, pyObj); 
}

template<class E>
int EntityApp<E>::unregisterPyObjectToScript(const char* attrName)
{ 
	return script_.unregisterToModule(attrName); 
}

template<class E>
bool EntityApp<E>::installPyScript()
{
	if (Resmgr::getSingleton().respaths().size() <= 0 ||
		Resmgr::getSingleton().getPyUserResPath().size() == 0 ||
		Resmgr::getSingleton().getPySysResPath().size() == 0 ||
		Resmgr::getSingleton().getPyUserScriptsPath().size() == 0)
	{
		OURO_ASSERT(false && "EntityApp::installPyScript: OURO_RES_PATH error!\n");
		return false;
	}

	std::pair<std::wstring, std::wstring> pyPaths = getComponentPythonPaths(g_componentType);
	if (pyPaths.first.size() == 0)
	{
		OURO_ASSERT(false && "EntityApp::installPyScript: OURO_RES_PATH error[char2wchar]!\n");
		return false;
	}

	return getScript().install(pyPaths.first.c_str(), pyPaths.second, "Ouroboros", componentType_);
}

template<class E>
void EntityApp<E>::registerScript(PyTypeObject* pto)
{
	scriptBaseTypes_.push_back(pto);
}

template<class E>
bool EntityApp<E>::uninstallPyScript()
{
	return uninstallPyModules() && getScript().uninstall();
}

template<class E>
bool EntityApp<E>::installPyModules()
{
	Entities<E>::installScript(NULL);
	EntityGarbages<E>::installScript(NULL);
	//Entity::installScript(g_script.getModule());

	pEntities_ = new Entities<E>();
	registerPyObjectToScript("entities", pEntities_);

	// Add pywatcher support
	if(!initializePyWatcher(&this->getScript()))
		return false;

	// add globalData, globalBases support
	pGlobalData_ = new GlobalDataClient(DBMGR_TYPE, GlobalDataServer::GLOBAL_DATA);
	registerPyObjectToScript("globalData", pGlobalData_);
	
	// Register to create an entity method to py
	// Allow the assertion of the underlying, used to debug the script at a certain time when the underlying state
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),	kbassert,			__py_assert,							METH_VARARGS,	0);
	
	// Register the app release status to the script
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),	publish,			__py_getAppPublish,						METH_VARARGS,	0);

	// registration settings script output type
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),	scriptLogType,		__py_setScriptLogType,					METH_VARARGS,	0);
	
	// Get the full path of the resource
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),	getResFullPath,		__py_getResFullPath,					METH_VARARGS,	0);

	// Is there a resource?
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),	hasRes,				__py_hasRes,							METH_VARARGS,	0);

	// open a file
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),	open,				__py_ouroOpen,							METH_VARARGS,	0);

	// List all files in the directory
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),	listPathRes,		__py_listPathRes,						METH_VARARGS,	0);

	// Match the relative path to get the full path
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),	matchPath,			__py_matchPath,							METH_VARARGS,	0);

	// Get the watcher value
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),	getWatcher,			__py_getWatcher,						METH_VARARGS,	0);

	// Get the watcher directory
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),	getWatcherDir,		__py_getWatcherDir,						METH_VARARGS,	0);

	// debug trace ouro package py object count
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),	debugTracing,		script::PyGC::__py_debugTracing,		METH_VARARGS,	0);

	if(PyModule_AddIntConstant(this->getScript().getModule(), "LOG_TYPE_NORMAL", log4cxx::ScriptLevel::SCRIPT_INT))
	{
		ERROR_MSG( "EntityApp::installPyModules: Unable to set Ouroboros.LOG_TYPE_NORMAL.\n");
	}

	if(PyModule_AddIntConstant(this->getScript().getModule(), "LOG_TYPE_INFO", log4cxx::ScriptLevel::SCRIPT_INFO))
	{
		ERROR_MSG( "EntityApp::installPyModules: Unable to set Ouroboros.LOG_TYPE_INFO.\n");
	}

	if(PyModule_AddIntConstant(this->getScript().getModule(), "LOG_TYPE_ERR", log4cxx::ScriptLevel::SCRIPT_ERR))
	{
		ERROR_MSG( "EntityApp::installPyModules: Unable to set Ouroboros.LOG_TYPE_ERR.\n");
	}

	if(PyModule_AddIntConstant(this->getScript().getModule(), "LOG_TYPE_DBG", log4cxx::ScriptLevel::SCRIPT_DBG))
	{
		ERROR_MSG( "EntityApp::installPyModules: Unable to set Ouroboros.LOG_TYPE_DBG.\n");
	}

	if(PyModule_AddIntConstant(this->getScript().getModule(), "LOG_TYPE_WAR", log4cxx::ScriptLevel::SCRIPT_WAR))
	{
		ERROR_MSG( "EntityApp::installPyModules: Unable to set Ouroboros.LOG_TYPE_WAR.\n");
	}

	if(PyModule_AddIntConstant(this->getScript().getModule(), "NEXT_ONLY", OURO_NEXT_ONLY))
	{
		ERROR_MSG( "EntityApp::installPyModules: Unable to set Ouroboros.NEXT_ONLY.\n");
	}

	for(int i = 0; i < SERVER_ERR_MAX; i++)
	{
		if(PyModule_AddIntConstant(getScript().getModule(), SERVER_ERR_STR[i], i))
		{
			ERROR_MSG( fmt::format("EntityApp::installPyModules: Unable to set Ouroboros.{}.\n", SERVER_ERR_STR[i]));
		}
	}
	
	// Install the entry module
	std::string entryScriptFileName = "";
	if (componentType() == BASEAPP_TYPE)
	{
		ENGINE_COMPONENT_INFO& info = g_ouroSrvConfig.getBaseApp();
		entryScriptFileName = info.entryScriptFile;
	}
	else if (componentType() == CELLAPP_TYPE)
	{
		ENGINE_COMPONENT_INFO& info = g_ouroSrvConfig.getCellApp();
		entryScriptFileName = info.entryScriptFile;
	}

	if(entryScriptFileName.size() > 0)
	{
		PyObject *pyEntryScriptFileName = PyUnicode_FromString(entryScriptFileName.c_str());
		entryScript_ = PyImport_Import(pyEntryScriptFileName);

		if (PyErr_Occurred())
		{
			INFO_MSG(fmt::format("EntityApp::installPyModules: importing scripts/{}{}.py...\n", 
				(componentType() == BASEAPP_TYPE ? "base/" : "cell/"), 
				entryScriptFileName));

			PyErr_PrintEx(0);
		}

		S_RELEASE(pyEntryScriptFileName);

		if(entryScript_.get() == NULL)
		{
			return false;
		}
	}

	onInstallPyModules();
	return true;
}

template<class E>
bool EntityApp<E>::uninstallPyModules()
{
	// script::PyGC::set_debug(script::PyGC::DEBUG_STATS|script::PyGC::DEBUG_LEAK);
	// script::PyGC::collect();
	unregisterPyObjectToScript("globalData");
	S_RELEASE(pGlobalData_); 

	S_RELEASE(pEntities_);
	unregisterPyObjectToScript("entities");

	Entities<E>::uninstallScript();
	EntityGarbages<E>::uninstallScript();
	//Entity::uninstallScript();
	EntityDef::uninstallScript();

	script::PyGC::debugTracing();
	return true;
}

template<class E>
E* EntityApp<E>::createEntity(const char* entityType, PyObject* params,
										 bool isInitializeScript, ENTITY_ID eid, bool initProperty)
{
	// Check if the ID is enough, insufficient to return NULL
	if(eid <= 0 && idClient_.size() == 0)
	{
		PyErr_SetString(PyExc_SystemError, "EntityApp::createEntity: is Failed. not enough entityIDs.");
		PyErr_PrintEx(0);
		return NULL;
	}
	
	ScriptDefModule* sm = EntityDef::findScriptModule(entityType);
	if(sm == NULL)
	{
		PyErr_Format(PyExc_TypeError, "EntityApp::createEntity: entityType [%s] not found! Please register in entities.xml and implement a %s.def and %s.py\n", 
			entityType, entityType, entityType);
		
		PyErr_PrintEx(0);
		return NULL;
	}
	else if(componentType_ == CELLAPP_TYPE ? !sm->hasCell() : !sm->hasBase())
	{
		PyErr_Format(PyExc_TypeError, "EntityApp::createEntity: cannot create %s(%s=false)! Please check the setting of the entities.xml and the implementation of %s.py\n", 
			entityType, 
			(componentType_ == CELLAPP_TYPE ? "hasCell()" : "hasBase()"), 
			entityType);
		
		PyErr_PrintEx(0);
		return NULL;
	}

	PyObject* obj = sm->createObject();

	// Determine whether you want to assign a new id
	ENTITY_ID id = eid;
	if(id <= 0)
		id = idClient_.alloc();
	
	EntityDef::context().currEntityID = id;

	E* entity = onCreateEntity(obj, sm, id);

	if(initProperty)
		entity->initProperty();

	// Add entity to entities
	pEntities_->add(id, entity); 

	// initialization script
	if(isInitializeScript)
		entity->initializeEntity(params);

	SCRIPT_ERROR_CHECK();

	if(g_debugEntity)
	{
		INFO_MSG(fmt::format("EntityApp::createEntity: new {} ({}) refc={}.\n", entityType, id, obj->ob_refcnt));
	}
	else
	{
		INFO_MSG(fmt::format("EntityApp::createEntity: new {0} {1}\n", entityType, id));
	}

	return entity;
}

template<class E>
E* EntityApp<E>::onCreateEntity(PyObject* pyEntity, ScriptDefModule* sm, ENTITY_ID eid)
{
	// Execute the Entity constructor
	return new(pyEntity) E(eid, sm);
}

template<class E>
void EntityApp<E>::onSignalled(int sigNum)
{
	this->ServerApp::onSignalled(sigNum);
	
	switch (sigNum)
	{
	case SIGQUIT:
		CRITICAL_MSG(fmt::format("Received QUIT signal. This is likely caused by the "
				"{}Mgr killing this {} because it has been "
					"unresponsive for too long. Look at the callstack from "
					"the core dump to find the likely cause.\n",
				COMPONENT_NAME_EX(componentType_), 
				COMPONENT_NAME_EX(componentType_)));
		
		break;
	default: 
		break;
	}
}

template<class E>
PyObject* EntityApp<E>::tryGetEntity(COMPONENT_ID componentID, ENTITY_ID eid)
{
	if(componentID != componentID_)
		return NULL;
	
	E* entity = pEntities_->find(eid);
	if(entity == NULL){
		ERROR_MSG(fmt::format("EntityApp::tryGetEntity: can't found entity:{}.\n", eid));
		return NULL;
	}

	return entity;
}

template<class E>
Network::Channel* EntityApp<E>::findChannelByEntityCall(EntityCallAbstract& entityCall)
{
	// Find component if component ID is greater than 0
	if(entityCall.componentID() > 0)
	{
		Components::ComponentInfos* cinfos = 
			Components::getSingleton().findComponent(entityCall.componentID());

		if(cinfos != NULL && cinfos->pChannel != NULL)
			return cinfos->pChannel; 
	}
	else
	{
		return Components::getSingleton().pNetworkInterface()->findChannel(entityCall.addr());
	}

	return NULL;
}

template<class E>
void EntityApp<E>::handleTimeout(TimerHandle handle, void * arg)
{
	switch (reinterpret_cast<uintptr>(arg))
	{
		case TIMEOUT_GAME_TICK:
			this->handleGameTick();
			break;
		default:
			break;
	}

	ServerApp::handleTimeout(handle, arg);
}

template<class E>
void EntityApp<E>::handleGameTick()
{
	// time_t t = ::time(NULL);
	// DEBUG_MSG(fmt::format("EntityApp::handleGameTick[{}]:{}\n", t, g_ourotime));

	++g_ourotime;
	threadPool_.onMainThreadTick();
	handleTimers();
	
	{
		networkInterface().processChannels(Ouroboros::Network::MessageHandlers::pMainMessageHandlers);
	}
}

template<class E>
bool EntityApp<E>::destroyEntity(ENTITY_ID entityID, bool callScript)
{
	PyObjectPtr entity = pEntities_->erase(entityID);
	if(entity != NULL)
	{
		static_cast<E*>(entity.get())->destroy(callScript);
		return true;
	}

	ERROR_MSG(fmt::format("EntityApp::destroyEntity: not found {}!\n", entityID));
	return false;
}

template<class E>
E* EntityApp<E>::findEntity(ENTITY_ID entityID)
{
	AUTO_SCOPED_PROFILE("findEntity");
	return pEntities_->find(entityID);
}

template<class E>
void EntityApp<E>::onReqAllocEntityID(Network::Channel* pChannel, ENTITY_ID startID, ENTITY_ID endID)
{
	if(pChannel->isExternal())
		return;
	
	// INFO_MSG("EntityApp::onReqAllocEntityID: entityID alloc(%d-%d).\n", startID, endID);
	idClient_.onAddRange(startID, endID);
}

template<class E>
PyObject* EntityApp<E>::__py_assert(PyObject* self, PyObject* args)
{
	OURO_ASSERT(false && "kbassert");
	return NULL;
}

template<class E>
PyObject* EntityApp<E>::__py_getAppPublish(PyObject* self, PyObject* args)
{
	return PyLong_FromLong(g_appPublish);
}

template<class E>
PyObject* EntityApp<E>::__py_getWatcher(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	if(argCount != 1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::getWatcher(): args[strpath] error!");
		PyErr_PrintEx(0);
		return 0;
	}
	
	char* path;

	if(PyArg_ParseTuple(args, "s", &path) == -1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::getWatcher(): args[strpath] error!");
		PyErr_PrintEx(0);
		return 0;
	}

	//DebugHelper::getSingleton().setScriptMsgType(type);

	OUROShared_ptr< WatcherObject > pWobj = WatcherPaths::root().getWatcher(path);
	if(pWobj.get() == NULL)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::getWatcher(): not found watcher[%s]!", path);
		PyErr_PrintEx(0);
		return 0;
	}

	WATCHER_VALUE_TYPE wtype = pWobj->getType();
	PyObject* pyval = NULL;
	MemoryStream* stream = MemoryStream::createPoolObject(OBJECTPOOL_POINT);
	pWobj->addToStream(stream);
	WATCHER_ID id;
	(*stream) >> id;

	switch(wtype)
	{
	case WATCHER_VALUE_TYPE_UINT8:
		{
			uint8 v;
			(*stream) >> v;
			pyval = PyLong_FromUnsignedLong(v);
		}
		break;
	case WATCHER_VALUE_TYPE_UINT16:
		{
			uint16 v;
			(*stream) >> v;
			pyval = PyLong_FromUnsignedLong(v);
		}
		break;
	case WATCHER_VALUE_TYPE_UINT32:
		{
			uint32 v;
			(*stream) >> v;
			pyval = PyLong_FromUnsignedLong(v);
		}
		break;
	case WATCHER_VALUE_TYPE_UINT64:
		{
			uint64 v;
			(*stream) >> v;
			pyval = PyLong_FromUnsignedLongLong(v);
		}
		break;
	case WATCHER_VALUE_TYPE_INT8:
		{
			int8 v;
			(*stream) >> v;
			pyval = PyLong_FromLong(v);
		}
		break;
	case WATCHER_VALUE_TYPE_INT16:
		{
			int16 v;
			(*stream) >> v;
			pyval = PyLong_FromLong(v);
		}
		break;
	case WATCHER_VALUE_TYPE_INT32:
		{
			int32 v;
			(*stream) >> v;
			pyval = PyLong_FromLong(v);
		}
		break;
	case WATCHER_VALUE_TYPE_INT64:
		{
			int64 v;
			(*stream) >> v;
			pyval = PyLong_FromLongLong(v);
		}
		break;
	case WATCHER_VALUE_TYPE_FLOAT:
		{
			float v;
			(*stream) >> v;
			pyval = PyLong_FromDouble(v);
		}
		break;
	case WATCHER_VALUE_TYPE_DOUBLE:
		{
			double v;
			(*stream) >> v;
			pyval = PyLong_FromDouble(v);
		}
		break;
	case WATCHER_VALUE_TYPE_CHAR:
	case WATCHER_VALUE_TYPE_STRING:
		{
			std::string v;
			(*stream) >> v;
			pyval = PyUnicode_FromString(v.c_str());
		}
		break;
	case WATCHER_VALUE_TYPE_BOOL:
		{
			bool v;
			(*stream) >> v;
			pyval = PyBool_FromLong(v);
		}
		break;
	case WATCHER_VALUE_TYPE_COMPONENT_TYPE:
		{
			COMPONENT_TYPE v;
			(*stream) >> v;
			pyval = PyBool_FromLong(v);
		}
		break;
	default:
		OURO_ASSERT(false && "no support!\n");
	};

	MemoryStream::reclaimPoolObject(stream);
	return pyval;
}

template<class E>
PyObject* EntityApp<E>::__py_getWatcherDir(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	if(argCount != 1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::getWatcherDir(): args[strpath] error!");
		PyErr_PrintEx(0);
		return 0;
	}
	
	char* path;

	if(PyArg_ParseTuple(args, "s", &path) == -1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::getWatcherDir(): args[strpath] error!");
		PyErr_PrintEx(0);
		return 0;
	}

	std::vector<std::string> vec;
	WatcherPaths::root().dirPath(path, vec);

	PyObject* pyTuple = PyTuple_New(vec.size());
	std::vector<std::string>::iterator iter = vec.begin();
	int i = 0;
	for(; iter != vec.end(); ++iter)
	{
		PyTuple_SET_ITEM(pyTuple, i++, PyUnicode_FromString((*iter).c_str()));
	}

	return pyTuple;
}

template<class E>
PyObject* EntityApp<E>::__py_setScriptLogType(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	if(argCount != 1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::scriptLogType(): args error!");
		PyErr_PrintEx(0);
		return 0;
	}

	int type = -1;

	if(PyArg_ParseTuple(args, "i", &type) == -1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::scriptLogType(): args error!");
		PyErr_PrintEx(0);
		return 0;
	}

	DebugHelper::getSingleton().setScriptMsgType(type);
	S_Return;
}

template<class E>
PyObject* EntityApp<E>::__py_getResFullPath(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	if(argCount != 1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::getResFullPath(): args error!");
		PyErr_PrintEx(0);
		return 0;
	}

	char* respath = NULL;

	if(PyArg_ParseTuple(args, "s", &respath) == -1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::getResFullPath(): args error!");
		PyErr_PrintEx(0);
		return 0;
	}

	if(!Resmgr::getSingleton().hasRes(respath))
		return PyUnicode_FromString("");

	std::string fullpath = Resmgr::getSingleton().matchRes(respath);
	return PyUnicode_FromString(fullpath.c_str());
}

template<class E>
PyObject* EntityApp<E>::__py_hasRes(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	if(argCount != 1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::hasRes(): args error!");
		PyErr_PrintEx(0);
		return 0;
	}

	char* respath = NULL;

	if(PyArg_ParseTuple(args, "s", &respath) == -1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::hasRes(): args error!");
		PyErr_PrintEx(0);
		return 0;
	}

	return PyBool_FromLong(Resmgr::getSingleton().hasRes(respath));
}

template<class E>
PyObject* EntityApp<E>::__py_ouroOpen(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	if (argCount < 1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::open(): args error!");
		PyErr_PrintEx(0);
		return 0;
	}

	char* respath = NULL;
	char* fargs = NULL;
	char* encodingArg = NULL;

	if (PyArg_ParseTuple(args, "s|ss", &respath, &fargs, &encodingArg) == -1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::open(): args error!");
		PyErr_PrintEx(0);
		return 0;
	}

	std::string sfullpath = Resmgr::getSingleton().matchRes(respath);

	PyObject *ioMod = PyImport_ImportModule("io");

	// SCOPED_PROFILE(SCRIPTCALL_PROFILE);
	PyObject *openedFile = NULL;
	if (argCount > 1)
	{
		openedFile = PyObject_CallMethod(ioMod, const_cast<char*>("open"),
			const_cast<char*>("ssis"),
			const_cast<char*>(sfullpath.c_str()),
			fargs,
			-1,
			encodingArg);
	}
	else
	{
		openedFile = PyObject_CallMethod(ioMod, const_cast<char*>("open"),
			const_cast<char*>("s"),
			const_cast<char*>(sfullpath.c_str()));
	}

	Py_DECREF(ioMod);

	if (openedFile == NULL)
	{
		SCRIPT_ERROR_CHECK();
	}

	return openedFile;
}

template<class E>
PyObject* EntityApp<E>::__py_matchPath(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	if(argCount != 1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::matchPath(): args error!");
		PyErr_PrintEx(0);
		return 0;
	}

	char* respath = NULL;

	if(PyArg_ParseTuple(args, "s", &respath) == -1)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::matchPath(): args error!");
		PyErr_PrintEx(0);
		return 0;
	}

	std::string path = Resmgr::getSingleton().matchPath(respath);
	return PyUnicode_FromStringAndSize(path.c_str(), path.size());
}

template<class E>
PyObject* EntityApp<E>::__py_listPathRes(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	if(argCount < 1 || argCount > 2)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::listPathRes(): args[path, pathargs=\'*.*\'] error!");
		PyErr_PrintEx(0);
		return 0;
	}

	std::wstring wExtendName = L"*";
	PyObject* pathobj = NULL;
	PyObject* path_argsobj = NULL;

	if(argCount == 1)
	{
		if(PyArg_ParseTuple(args, "O", &pathobj) == -1)
		{
			PyErr_Format(PyExc_TypeError, "Ouroboros::listPathRes(): args[path] error!");
			PyErr_PrintEx(0);
			return 0;
		}
	}
	else
	{
		if(PyArg_ParseTuple(args, "O|O", &pathobj, &path_argsobj) == -1)
		{
			PyErr_Format(PyExc_TypeError, "Ouroboros::listPathRes(): args[path, pathargs=\'*.*\'] error!");
			PyErr_PrintEx(0);
			return 0;
		}
		
		if(PyUnicode_Check(path_argsobj))
		{
			wchar_t* fargs = NULL;
			fargs = PyUnicode_AsWideCharString(path_argsobj, NULL);
			wExtendName = fargs;
			PyMem_Free(fargs);
		}
		else
		{
			if(PySequence_Check(path_argsobj))
			{
				wExtendName = L"";
				Py_ssize_t size = PySequence_Size(path_argsobj);
				for(int i=0; i<size; ++i)
				{
					PyObject* pyobj = PySequence_GetItem(path_argsobj, i);
					if(!PyUnicode_Check(pyobj))
					{
						PyErr_Format(PyExc_TypeError, "Ouroboros::listPathRes(): args[path, pathargs=\'*.*\'] error!");
						PyErr_PrintEx(0);
						return 0;
					}
					
					wchar_t* wtemp = NULL;
					wtemp = PyUnicode_AsWideCharString(pyobj, NULL);
					wExtendName += wtemp;
					wExtendName += L"|";
					PyMem_Free(wtemp);
				}
			}
			else
			{
				PyErr_Format(PyExc_TypeError, "Ouroboros::listPathRes(): args[pathargs] error!");
				PyErr_PrintEx(0);
				return 0;
			}
		}
	}

	if(!PyUnicode_Check(pathobj))
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::listPathRes(): args[path] error!");
		PyErr_PrintEx(0);
		return 0;
	}

	if(PyUnicode_GET_LENGTH(pathobj) == 0)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::listPathRes(): args[path] is NULL!");
		PyErr_PrintEx(0);
		return 0;
	}

	if(wExtendName.size() == 0)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::listPathRes(): args[pathargs] is NULL!");
		PyErr_PrintEx(0);
		return 0;
	}

	if(wExtendName[0] == '.')
		wExtendName.erase(wExtendName.begin());

	if(wExtendName.size() == 0)
		wExtendName = L"*";

	wchar_t* respath = PyUnicode_AsWideCharString(pathobj, NULL);
	if(respath == NULL)
	{
		PyErr_Format(PyExc_TypeError, "Ouroboros::listPathRes(): args[path] is NULL!");
		PyErr_PrintEx(0);
		return 0;
	}

	char* cpath = strutil::wchar2char(respath);
	std::string foundPath = Resmgr::getSingleton().matchPath(cpath);
	free(cpath);
	PyMem_Free(respath);

	respath = strutil::char2wchar(foundPath.c_str());

	std::vector<std::wstring> results;
	Resmgr::getSingleton().listPathRes(respath, wExtendName, results);
	PyObject* pyresults = PyTuple_New(results.size());

	std::vector<std::wstring>::iterator iter = results.begin();
	int i = 0;

	for(; iter != results.end(); ++iter)
	{
		PyTuple_SET_ITEM(pyresults, i++, PyUnicode_FromWideChar((*iter).c_str(), (*iter).size()));
	}

	free(respath);
	return pyresults;
}

template<class E>
void EntityApp<E>::startProfile_(Network::Channel* pChannel, std::string profileName, int8 profileType, uint32 timelen)
{
	if(pChannel->isExternal())
		return;
	
	switch(profileType)
	{
	case 0:	// pyprofile
		new PyProfileHandler(this->networkInterface(), timelen, profileName, pChannel->addr());
		return;
	default:
		break;
	};

	ServerApp::startProfile_(pChannel, profileName, profileType, timelen);
}

template<class E>
void EntityApp<E>::onDbmgrInitCompleted(Network::Channel* pChannel, 
						GAME_TIME gametime, ENTITY_ID startID, ENTITY_ID endID, 
						COMPONENT_ORDER startGlobalOrder, COMPONENT_ORDER startGroupOrder, 
						const std::string& digest)
{
	if(pChannel->isExternal())
		return;
	
	INFO_MSG(fmt::format("EntityApp::onDbmgrInitCompleted: entityID alloc({}-{}), startGlobalOrder={}, startGroupOrder={}, digest={}.\n",
		startID, endID, startGlobalOrder, startGroupOrder, digest));

	startGlobalOrder_ = startGlobalOrder;
	startGroupOrder_ = startGroupOrder;
	g_componentGlobalOrder = startGlobalOrder;
	g_componentGroupOrder = startGroupOrder;

	idClient_.onAddRange(startID, endID);
	g_ourotime = gametime;

	setEvns();

	if(digest != EntityDef::md5().getDigestStr())
	{
		ERROR_MSG(fmt::format("EntityApp::onDbmgrInitCompleted: digest not match. curr({}) != dbmgr({}, addr={})\n",
			EntityDef::md5().getDigestStr(), digest, pChannel->c_str()));

		this->shutDown();
	}
}

template<class E>
void EntityApp<E>::onBroadcastGlobalDataChanged(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;
	
	std::string key, value;
	bool isDelete;
	
	s >> isDelete;
	s.readBlob(key);

	if(!isDelete)
	{
		s.readBlob(value);
	}

	PyObject * pyKey = script::Pickler::unpickle(key);
	if(pyKey == NULL)
	{
		ERROR_MSG("EntityApp::onBroadcastGlobalDataChanged: no has key!\n");
		return;
	}

	if(isDelete)
	{
		if(pGlobalData_->del(pyKey))
		{
			// notification script
			// SCOPED_PROFILE(SCRIPTCALL_PROFILE);
			SCRIPT_OBJECT_CALL_ARGS1(getEntryScript().get(), const_cast<char*>("onGlobalDataDel"), 
				const_cast<char*>("O"), pyKey, false);
		}
	}
	else
	{
		PyObject * pyValue = script::Pickler::unpickle(value);
		if(pyValue == NULL)
		{
			ERROR_MSG("EntityApp::onBroadcastGlobalDataChanged: no has value!\n");
			Py_DECREF(pyKey);
			return;
		}

		if(pGlobalData_->write(pyKey, pyValue))
		{
			// notification script
			// SCOPED_PROFILE(SCRIPTCALL_PROFILE);
			SCRIPT_OBJECT_CALL_ARGS2(getEntryScript().get(), const_cast<char*>("onGlobalData"), 
				const_cast<char*>("OO"), pyKey, pyValue, false);
		}

		Py_DECREF(pyValue);
	}

	Py_DECREF(pyKey);
}

template<class E>
void EntityApp<E>::onExecScriptCommand(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;
	
	std::string cmd;
	s.readBlob(cmd);

	PyObject* pycmd = PyUnicode_DecodeUTF8(cmd.data(), cmd.size(), NULL);
	if(pycmd == NULL)
	{
		SCRIPT_ERROR_CHECK();
		return;
	}

	DEBUG_MSG(fmt::format("EntityApp::onExecScriptCommand: size({}), command={}.\n", 
		cmd.size(), cmd));

	std::string retbuf = "";
	PyObject* pycmd1 = PyUnicode_AsEncodedString(pycmd, "utf-8", NULL);
	script_.run_simpleString(PyBytes_AsString(pycmd1), &retbuf);

	if(retbuf.size() == 0)
	{
		retbuf = "\r\n";
	}

	// return the result to the client
	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
	ConsoleInterface::ConsoleExecCommandCBMessageHandler msgHandler;
	(*pBundle).newMessage(msgHandler);
	ConsoleInterface::ConsoleExecCommandCBMessageHandlerArgs1::staticAddToBundle((*pBundle), retbuf);
	pChannel->send(pBundle);

	Py_DECREF(pycmd);
	Py_DECREF(pycmd1);
}

template<class E>
int EntityApp<E>::tickPassedPercent(uint64 curr)
{
	// Get the time elapsed since the last tick
	uint64 pass_stamps = (curr - lastTimestamp_) * uint64(1000) / stampsPerSecond();

	// Get the number of milliseconds per Hertz
	static int expected = (1000 / g_ouroSrvConfig.gameUpdateHertz());

	// get the current elapsed time as a percentage of a clock cycle
	return int(pass_stamps) * 100 / expected;
}

template<class E>
uint64 EntityApp<E>::checkTickPeriod()
{
	uint64 curr = timestamp();
	int percent = tickPassedPercent(curr);

	if (percent > 200)
	{
		WARNING_MSG(fmt::format("EntityApp::checkTickPeriod: tick took {0}% ({1:.2f} seconds)!\n",
					percent, (float(percent)/1000.f)));
	}

	uint64 elapsed = curr - lastTimestamp_;
	lastTimestamp_ = curr;
	return elapsed;
}


template<class E>
void EntityApp<E>::updateLoad()
{
	uint64 lastTickInStamps = checkTickPeriod();

	// Get the free time ratio
	double spareTime = 1.0;
	if (lastTickInStamps != 0)
	{
		spareTime = double(dispatcher_.getSpareTime()) / double(lastTickInStamps);
	}

	dispatcher_.clearSpareTime();

	// If the ratio of idle time is less than 0 or greater than 1, the timing is not accurate.
	if ((spareTime < 0.f) || (1.f < spareTime))
	{
		if (g_timingMethod == RDTSC_TIMING_METHOD)
		{
			CRITICAL_MSG(fmt::format("EntityApp::handleGameTick: "
						"Invalid timing result {:.3f}.\n"
						"Please change the environment variable OURO_TIMING_METHOD to [rdtsc|gettimeofday|gettime](curr = {1})!",
						spareTime, getTimingMethodName()));
		}
		else
		{
			CRITICAL_MSG(fmt::format("EntityApp::handleGameTick: Invalid timing result {:.3f}.\n",
					spareTime));
		}
	}

	calcLoad((float)spareTime);
	onUpdateLoad();
}

template<class E>
void EntityApp<E>::calcLoad(float spareTime)
{
	// The value of the load is 1.0 - the ratio of idle time, must be between 0-1.f
	float load = OUROClamp(1.f - spareTime, 0.f, 1.f);

	// Here the algorithm looks at server_operations_guide.pdf to introduce loadSmoothingBias
	// loadSmoothingBias determines the remaining ratio of loadSmoothingBias for the last load of this load + the loadSmoothingBias ratio of the current load
	static float loadSmoothingBias = g_ouroSrvConfig.getConfig().loadSmoothingBias;
	load_ = (1 - loadSmoothingBias) * load_ + loadSmoothingBias * load;
}

template<class E>
void EntityApp<E>::onReloadScript(bool fullReload)
{
	EntityCall::ENTITYCALLS::iterator iter = EntityCall::entityCalls.begin();
	for(; iter != EntityCall::entityCalls.end(); ++iter)
	{
		(*iter)->reload();
	}

	EntityComponent::ENTITY_COMPONENTS::iterator iter1 = EntityComponent::entity_components.begin();
	for (; iter1 != EntityComponent::entity_components.end(); ++iter1)
	{
		(*iter1)->reload();
	}
}

template<class E>
void EntityApp<E>::reloadScript(bool fullReload)
{
	EntityDef::reload(fullReload);
	onReloadScript(fullReload);

	// SCOPED_PROFILE(SCRIPTCALL_PROFILE);

	// All scripts are loaded
	PyObject* pyResult = PyObject_CallMethod(getEntryScript().get(), 
										const_cast<char*>("onInit"), 
										const_cast<char*>("i"), 
										1);

	if(pyResult != NULL)
		Py_DECREF(pyResult);
	else
		SCRIPT_ERROR_CHECK();
}

}

#endif // OURO_ENTITY_APP_H
