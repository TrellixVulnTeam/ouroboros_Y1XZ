// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com


#include "machine.h"
#include "machine_interface.h"
#include "network/common.h"
#include "network/tcp_packet.h"
#include "network/udp_packet.h"
#include "network/message_handler.h"
#include "network/bundle_broadcast.h"
#include "helper/sys_info.h"
#include "thread/threadpool.h"
#include "server/component_active_report_handler.h"

#include "../baseappmgr/baseappmgr_interface.h"
#include "../cellappmgr/cellappmgr_interface.h"
#include "../baseapp/baseapp_interface.h"
#include "../cellapp/cellapp_interface.h"
#include "../dbmgr/dbmgr_interface.h"
#include "../loginapp/loginapp_interface.h"
#include "../tools/logger/logger_interface.h"
#include "../../server/tools/interfaces/interfaces_interface.h"
#include "../../server/tools/bots/bots_interface.h"
#include "common/md5.h"

namespace Ouroboros{
	
ServerConfig g_serverConfig;
OURO_SINGLETON_INIT(Machine);

//-------------------------------------------------------------------------------------
Machine::Machine(Network::EventDispatcher& dispatcher, 
				 Network::NetworkInterface& ninterface, 
				 COMPONENT_TYPE componentType,
				 COMPONENT_ID componentID):
	ServerApp(dispatcher, ninterface, componentType, componentID),
	broadcastAddr_(0),
	ep_(),
	epBroadcast_(),
	epLocal_(),
	pEPPacketReceiver_(NULL),
	pEBPacketReceiver_(NULL),
	pEPLocalPacketReceiver_(NULL),
	localuids_()
{
	SystemInfo::getSingleton().getCPUPer();
	Ouroboros::Network::MessageHandlers::pMainMessageHandlers = &MachineInterface::messageHandlers;
}

//-------------------------------------------------------------------------------------
Machine::~Machine()
{
	ep_.close();
	epBroadcast_.close();
	epLocal_.close();
	localuids_.clear();

	SAFE_RELEASE(pEPPacketReceiver_);
	SAFE_RELEASE(pEBPacketReceiver_);
	SAFE_RELEASE(pEPLocalPacketReceiver_);
}

//-------------------------------------------------------------------------------------
void Machine::onBroadcastInterface(Network::Channel* pChannel, int32 uid, std::string& username,
								   COMPONENT_TYPE componentType, COMPONENT_ID componentID, COMPONENT_ID componentIDEx, 
								   COMPONENT_ORDER globalorderid, COMPONENT_ORDER grouporderid, COMPONENT_GUS gus,
									uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport, std::string& extaddrEx, uint32 pid,
									float cpu, float mem, uint32 usedmem, int8 state, uint32 machineID, uint64 extradata,
									uint64 extradata1, uint64 extradata2, uint64 extradata3, uint32 backRecvAddr, uint16 backRecvPort)
{
	// First check if the same identity exists. If it is the same identity and not a process, we need to inform the other party to start illegal.
	Components::ComponentInfos* pinfos = Components::getSingleton().findComponent(componentID);
	if(pinfos && isGameServerComponentType((COMPONENT_TYPE)componentType) && checkComponentUsable(pinfos, false, true))
	{
		if(pinfos->pid != pid || pinfos->pIntAddr->ip != intaddr ||
			username != pinfos->username || uid != pinfos->uid)
		{
			Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);

			MachineInterface::onBroadcastInterfaceArgs25::staticAddToBundle((*pBundle), pinfos->uid,
				pinfos->username, pinfos->componentType, pinfos->cid, componentIDEx, pinfos->globalOrderid, pinfos->groupOrderid, pinfos->gus,
				pinfos->pIntAddr->ip, pinfos->pIntAddr->port,
				pinfos->pExtAddr->ip, pinfos->pExtAddr->port, pinfos->externalAddressEx, pinfos->pid, pinfos->cpu, pinfos->mem, pinfos->usedmem, 
				(int8)pinfos->state, Ouroboros::getProcessPID(), pinfos->extradata, pinfos->extradata1, pinfos->extradata2, pinfos->extradata3, 0, 0);

			if(backRecvAddr != 0 && backRecvPort != 0)
			{
				Network::EndPoint ep;
				ep.socket(SOCK_DGRAM);

				if (!ep.good())
				{
					ERROR_MSG("Machine::onBroadcastInterface: Failed to create socket.\n");
					return;
				}

				ep.sendto(pBundle, backRecvPort, backRecvAddr);
				Network::Bundle::reclaimPoolObject(pBundle);
			}
			else
			{
				pChannel->send(pBundle);
			}

			return;
		}
	}

	// Only record the process started by the machine
	if(this->networkInterface().intTcpAddr().ip == intaddr ||
				this->networkInterface().extTcpAddr().ip == intaddr)
	{
		pinfos = Components::getSingleton().findComponent((COMPONENT_TYPE)componentType, uid, componentID);
		if(pinfos)
		{
			if(checkComponentUsable(pinfos, false, true))
			{
				DEBUG_MSG(fmt::format("Machine::onBroadcastInterface: {} update, pid={}, uid={}, globalorderid={}, grouporderid={}!\n", 
					COMPONENT_NAME_EX((COMPONENT_TYPE)componentType), pid, uid, globalorderid, grouporderid));

				pinfos->globalOrderid = globalorderid;
				pinfos->groupOrderid = grouporderid;
				return;
			}
		}

		// Only one machine can exist on one hardware
		if(componentType == MACHINE_TYPE)
		{
			ERROR_MSG("Machine::onBroadcastInterface: A single computer cannot run multiple \"machine\" process!\n");
			return;
		}
	
		std::vector<int32>::iterator iter = std::find(localuids_.begin(), localuids_.end(), uid);
		if(iter == localuids_.end())
		{
			INFO_MSG(fmt::format("Machine::onBroadcastInterface: added newUID {0}.\n", uid));
			localuids_.push_back(uid);
		}

		INFO_MSG(fmt::format("Machine::onBroadcastInterface[{0}]: uid:{1}, username:{2}, componentType:{3}, "
			"componentID:{4}, globalorderid={9}, grouporderid={10}, pid:{11}, intaddr:{5}, intport:{6}, extaddr:{7}, extport:{8}.\n",
				pChannel->c_str(),
				uid,
				username.c_str(),
				COMPONENT_NAME_EX((COMPONENT_TYPE)componentType),
				componentID,
				inet_ntoa((struct in_addr&)intaddr),
				ntohs(intport),
				(extaddr != 0 ? inet_ntoa((struct in_addr&)extaddr) : "nonsupport"),
				ntohs(extport),
				((int32)globalorderid),
				((int32)grouporderid),
				pid));

		Components::getSingleton().addComponent(uid, username.c_str(), 
			(Ouroboros::COMPONENT_TYPE)componentType, componentID, globalorderid, grouporderid, gus, intaddr, intport, extaddr, extport, extaddrEx,
			pid, cpu, mem, usedmem, extradata, extradata1, extradata2, extradata3);
	}
}

//-------------------------------------------------------------------------------------
void Machine::onFindInterfaceAddr(Network::Channel* pChannel, int32 uid, std::string& username, COMPONENT_TYPE componentType, COMPONENT_ID componentID,
								  COMPONENT_TYPE findComponentType, uint32 finderAddr, uint16 finderRecvPort)
{
	Ouroboros::COMPONENT_TYPE tfindComponentType = (Ouroboros::COMPONENT_TYPE)findComponentType;
	Ouroboros::COMPONENT_TYPE tComponentType = (Ouroboros::COMPONENT_TYPE)componentType;

	// If it is not issued by guiconsole, uid is not equal to the current server's uid and will be ignored.
	if(tComponentType != CONSOLE_TYPE)
	{
		std::vector<int32>::iterator iter = std::find(localuids_.begin(), localuids_.end(), uid);
		if(iter == localuids_.end())
			return;
	}

	INFO_MSG(fmt::format("Machine::onFindInterfaceAddr[{0}]: uid:{1}, username:{2}, "
			"componentType:{3}, componentID:{7}, find:{4}, finderaddr:{5}, finderRecvPort:{6}.\n",
		pChannel->c_str(), uid, username.c_str(), 
		COMPONENT_NAME_EX((COMPONENT_TYPE)componentType),
		COMPONENT_NAME_EX((COMPONENT_TYPE)findComponentType),
		inet_ntoa((struct in_addr&)finderAddr), ntohs(finderRecvPort),
		componentID));

	Network::EndPoint ep;
	ep.socket(SOCK_DGRAM);

	if (!ep.good())
	{
		ERROR_MSG("Machine::onFindInterfaceAddr: Failed to create socket.\n");
		return;
	}
	
	Components::COMPONENTS& components = Components::getSingleton().getComponents(tfindComponentType);
	Components::COMPONENTS::iterator iter = components.begin();

	bool found = false;
	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);

	for(; iter != components.end(); )
	{
		if(tComponentType != CONSOLE_TYPE)
		{
			if((*iter).uid != uid)
			{
				++iter;
				continue;
			}
		}

		const Components::ComponentInfos* pinfos = &(*iter);
		
		bool usable = checkComponentUsable(pinfos, false, false);

		if(usable)
		{
			if(this->networkInterface().intTcpAddr().ip == pinfos->pIntAddr->ip ||
				this->networkInterface().extTcpAddr().ip == pinfos->pIntAddr->ip)
			{
				found = true;

				MachineInterface::onBroadcastInterfaceArgs25::staticAddToBundle((*pBundle), pinfos->uid, 
					pinfos->username, findComponentType, pinfos->cid, componentID, pinfos->globalOrderid, pinfos->groupOrderid, pinfos->gus,
					pinfos->pIntAddr->ip, pinfos->pIntAddr->port,
					pinfos->pExtAddr->ip, pinfos->pExtAddr->port, pinfos->externalAddressEx, pinfos->pid, pinfos->cpu, pinfos->mem, pinfos->usedmem, 
					(int8)pinfos->state, Ouroboros::getProcessPID(), pinfos->extradata, pinfos->extradata1, pinfos->extradata2, pinfos->extradata3, 0, 0);
			}

			++iter;
		}
		else
		{
			WARNING_MSG(fmt::format("Machine::onFindInterfaceAddr: {}[{}] invalid, erase {}.\n",
				COMPONENT_NAME_EX(pinfos->componentType),
				pinfos->cid,
				COMPONENT_NAME_EX(pinfos->componentType)));

			removeComponentID(pinfos->componentType, pinfos->cid, uid);
			iter = components.erase(iter);
		}
	}

	if(!found)
	{
		// If it is a console, and uid is not consistent then there is no need to return a message not found
		// The console may be broadcast to other groups.
		if(tComponentType == CONSOLE_TYPE)
		{
			std::vector<int32>::iterator iter = std::find(localuids_.begin(), localuids_.end(), uid);
			if(iter == localuids_.end())
				return;
		}

		WARNING_MSG(fmt::format("Machine::onFindInterfaceAddr: {} not found {}.\n",
			COMPONENT_NAME_EX(tComponentType),
			COMPONENT_NAME_EX(tfindComponentType)));

		MachineInterface::onBroadcastInterfaceArgs25::staticAddToBundle((*pBundle), Ouroboros::getUserUID(),
			"", UNKNOWN_COMPONENT_TYPE, 0, componentID, -1, -1, -1, 0, 0, 0, 0, "", 0, 0.f, 0.f, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}

	if(finderAddr != 0 && finderRecvPort != 0)
	{
		ep.sendto(pBundle, finderRecvPort, finderAddr);
		Network::Bundle::reclaimPoolObject(pBundle);
	}
	else
	{
		pChannel->send(pBundle);
	}
}

//-------------------------------------------------------------------------------------
bool Machine::checkComponentUsable(const Components::ComponentInfos* info, bool getdatas, bool autoerase)
{
	bool ret = false;
	
	if(!getdatas)
		ret = Components::getSingleton().lookupLocalComponentRunning(info->pid) != NULL;
	else
		ret = Components::getSingleton().updateComponentInfos(info);

	// Erase if it is already unavailable and allows automatic erasure
	if(!ret && autoerase)
		Components::getSingleton().delComponent(info->uid, info->componentType, info->cid);

	return ret;
}

//-------------------------------------------------------------------------------------
void Machine::onQueryMachines(Network::Channel* pChannel, int32 uid, std::string& username,
	uint16 finderRecvPort)
{
	INFO_MSG(fmt::format("Machine::onQueryMachines[{}]: uid:{}, username:{}, "
		"finderRecvPort:{}.\n",
		pChannel->c_str(), uid, username.c_str(),
		ntohs(finderRecvPort)));

	Network::EndPoint ep;
	ep.socket(SOCK_DGRAM);

	if (!ep.good())
	{
		ERROR_MSG("Machine::onQueryMachines: Failed to create socket.\n");
		return;
	}

	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);

	uint64 cidex = 0;
	float cpu = SystemInfo::getSingleton().getCPUPer();
	uint64 totalmem = SystemInfo::getSingleton().getMemInfos().total;
	uint64 totalusedmem = SystemInfo::getSingleton().getMemInfos().used;

	MachineInterface::onBroadcastInterfaceArgs25::staticAddToBundle((*pBundle), getUserUID(), getUsername(),
		g_componentType, g_componentID, cidex, g_componentGlobalOrder, g_componentGroupOrder, g_genuuid_sections,
		networkInterface_.intTcpAddr().ip, networkInterface_.intTcpAddr().port,
		networkInterface_.extTcpAddr().ip, networkInterface_.extTcpAddr().port, "", getProcessPID(),
		cpu, float((totalusedmem * 1.0 / totalmem) * 100.0), (uint32)SystemInfo::getSingleton().getMemUsedByPID(), 0,
		getProcessPID(), totalmem, totalusedmem, uint64(SystemInfo::getSingleton().getCPUPerByPID() * 100), 0, 0, 0);

	if (finderRecvPort != 0)
	{
		ep.sendto(pBundle, finderRecvPort, pChannel->addr().ip);
		Network::Bundle::reclaimPoolObject(pBundle);
	}
	else
	{
		pChannel->send(pBundle);
	}
}

//-------------------------------------------------------------------------------------
void Machine::queryComponentID(Network::Channel* pChannel, COMPONENT_TYPE componentType, COMPONENT_ID componentID, 
	int32 uid, uint16 finderRecvPort, int macMD5, int32 pid)
{
	INFO_MSG(fmt::format("Machine::queryComponentID[{}]: component:{}({}) uid:{} finderRecvPort:{} macMD5:{} pid:{}.\n",
		pChannel->c_str(), COMPONENT_NAME_EX(componentType), componentID, uid, finderRecvPort, macMD5, pid));

	uint32 ip = pChannel->addr().ip;
	std::string data = std::to_string(ip) + std::to_string(pid) + std::to_string(finderRecvPort);
	std::string md5 = std::to_string(getMD5(data));
	std::string pidMD5 = std::to_string(pid) + "-" + md5;

	std::map<std::string, COMPONENT_ID>::iterator pidIter = pidMD5Map_.find(pidMD5);
	if (pidIter != pidMD5Map_.end())
	{
		WARNING_MSG(fmt::format("Machine::queryComponentID[{}]: component({}) process({}) has queried componentID({}).\n", 
			pChannel->c_str(), COMPONENT_NAME_EX(componentType), pid, pidIter->second));

		return;
	}

	if (this->networkInterface().intTcpAddr().ip == ip ||
		this->networkInterface().extTcpAddr().ip == ip)
	{
		COMPONENT_ID cid1 = (COMPONENT_ID)uid * COMPONENT_ID_MULTIPLE;
		COMPONENT_ID cid2 = (COMPONENT_ID)macMD5 * 10000;
		COMPONENT_ID cid3 = (COMPONENT_ID)componentType * 100;

		COMPONENT_ID cid = cid1 + cid2 + cid3 + 1;

		std::map<int32, CID_MAP>::iterator iter = cidMap_.find(uid);
		if (iter == cidMap_.end())
		{
			ID_LOGS cidLog;
			cidLog.push_back(cid);

			CID_MAP cidMap;
			cidMap.insert(std::make_pair(componentType, cidLog));
			cidMap_.insert(std::make_pair(uid, cidMap));
		}
		else
		{
			CID_MAP cids = iter->second;
			CID_MAP::iterator iter1 = cids.find(componentType);

			if (iter1 == cids.end())
			{
				ID_LOGS cidLog;
				cidLog.push_back(cid);
				cids.insert(std::make_pair(componentType, cidLog));
			}
			else
			{
				ID_LOGS::iterator idIter;
				ID_LOGS cidLog = iter1->second;

				for (idIter = cidLog.begin(); idIter != cidLog.end();)
				{
					bool found = false;

					std::map<std::string, COMPONENT_ID>::iterator pidIter = pidMD5Map_.begin();
					for (; pidIter != pidMD5Map_.end(); pidIter++)
					{
						if (pidIter->second == *idIter)
						{
							std::vector<std::string> vec;
							strutil::ouro_split(pidIter->first, '-', vec);
							if (vec.size() == 2)
							{
								int32 oldPid = std::stoi(vec[0]);
								std::string oldMD5 = vec[1];
								SystemInfo::PROCESS_INFOS sysinfos = SystemInfo::getSingleton().getProcessInfo(oldPid);

								if (sysinfos.error || (pid == oldPid && oldMD5 != md5))
								{
									pidMD5Map_.erase(pidIter);
									idIter = cidLog.erase(idIter);
									found = true;
									break;
								}
							}
						}
					}

					if (!found)
						idIter++;
				}
				
				while((idIter = std::find(cidLog.begin(), cidLog.end(), cid)) != cidLog.end())
				{
					cid += 1;
				}

				cidLog.push_back(cid);
				cids[componentType] = cidLog;
			}

			cidMap_[uid] = cids;
		}

		Network::EndPoint ep;
		ep.socket(SOCK_DGRAM);

		Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
		MachineInterface::queryComponentIDArgs6::staticAddToBundle((*pBundle), componentType, cid, uid, finderRecvPort, macMD5, pid);
		ep.sendto(pBundle, finderRecvPort, ip);
		Network::Bundle::reclaimPoolObject(pBundle);

		pidMD5Map_.insert(std::make_pair(pidMD5, cid));

		INFO_MSG(fmt::format("Machine::queryComponentID[{}], set componentID success: component:{}({}) uid:{} pidMD5:{}.\n",
			pChannel->c_str(), COMPONENT_NAME_EX(componentType), cid, uid, pidMD5));
	}
}

//-------------------------------------------------------------------------------------
void Machine::removeComponentID(COMPONENT_TYPE componentType, COMPONENT_ID componentID, int32 uid)
{
	INFO_MSG(fmt::format("Machine::removeComponentID: component={}({}), uid={} \n", 
		COMPONENT_NAME[componentType], componentID, uid));

	std::map<int32, CID_MAP>::iterator iter = cidMap_.find(uid);
	if (iter != cidMap_.end())
	{
		CID_MAP cids = iter->second;
		if (cids.size() > 0)
		{
			CID_MAP::iterator iter1 = cids.find(componentType);
			if (iter1 != cids.end())
			{
				ID_LOGS cidLogs = iter1->second;

				if (cidLogs.size() > 0)
				{
					ID_LOGS::iterator iter2 = cidLogs.begin();
					for (; iter2 != cidLogs.end(); )
					{
						if (*iter2 == componentID)
						{
							INFO_MSG(fmt::format("--> remove componentID({})\n", componentID));
							iter2 = cidLogs.erase(iter2);
							std::map<std::string, COMPONENT_ID>::iterator pidIter = pidMD5Map_.begin();
							for (; pidIter != pidMD5Map_.end(); pidIter++)
							{
								if (pidIter->second == componentID)
								{
									INFO_MSG(fmt::format("--> remove pidMD5({})\n", pidIter->first));
									pidMD5Map_.erase(pidIter);
									break;
								}
							}
							break;
						}
						else
						{
							iter2++;
						}
					}
				}

				cids[componentType] = cidLogs;
				
				if (cidLogs.size() == 0)
				{
					cids.erase(iter1);
				}
			}
		}
		
		if (cids.size() == 0)
		{
			cidMap_.erase(iter);
		}
		else
		{
			cidMap_[uid] = cids;
		}
	}
}

//-------------------------------------------------------------------------------------
void Machine::onQueryAllInterfaceInfos(Network::Channel* pChannel, int32 uid, std::string& username, 
	uint16 finderRecvPort)
{
	INFO_MSG(fmt::format("Machine::onQueryAllInterfaceInfos[{}]: uid:{}, username:{}, "
			"finderRecvPort:{}.\n",
		pChannel->c_str(), uid, username.c_str(),
		ntohs(finderRecvPort)));

	Network::EndPoint ep;
	ep.socket(SOCK_DGRAM);

	if (!ep.good())
	{
		ERROR_MSG("Machine::onQueryAllInterfaceInfos: Failed to create socket.\n");
		return;
	}

	{
		Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
		
		uint64 cidex = 0;
		float cpu = SystemInfo::getSingleton().getCPUPer();
		uint64 totalmem = SystemInfo::getSingleton().getMemInfos().total;
		uint64 totalusedmem = SystemInfo::getSingleton().getMemInfos().used;

		MachineInterface::onBroadcastInterfaceArgs25::staticAddToBundle((*pBundle), getUserUID(), getUsername(), 
			g_componentType, g_componentID, cidex, g_componentGlobalOrder, g_componentGroupOrder, g_genuuid_sections,
			networkInterface_.intTcpAddr().ip, networkInterface_.intTcpAddr().port,
			networkInterface_.extTcpAddr().ip, networkInterface_.extTcpAddr().port, "", getProcessPID(),
			cpu, float((totalusedmem * 1.0 / totalmem) * 100.0), (uint32)SystemInfo::getSingleton().getMemUsedByPID(), 0, 
			getProcessPID(), totalmem, totalusedmem, uint64(SystemInfo::getSingleton().getCPUPerByPID() * 100), 0, 0, 0);

		if(finderRecvPort != 0)
		{
			ep.sendto(pBundle, finderRecvPort, pChannel->addr().ip);
			Network::Bundle::reclaimPoolObject(pBundle);
		}
		else
		{
			pChannel->send(pBundle);
		}
	}

	// uid is not equal to the current server's uid and ignores it.
	if (uid > 0)
	{
		std::vector<int32>::iterator iter = std::find(localuids_.begin(), localuids_.end(), uid);
		if (iter == localuids_.end())
			return;
	}

	int i = 0;

	while(ALL_SERVER_COMPONENT_TYPES[i] != UNKNOWN_COMPONENT_TYPE)
	{
		COMPONENT_TYPE findComponentType = ALL_SERVER_COMPONENT_TYPES[i++];
		Components::COMPONENTS& components = Components::getSingleton().getComponents(findComponentType);
		Components::COMPONENTS::iterator iter = components.begin();

		for(; iter != components.end(); )
		{
			if(uid > 0 && (*iter).uid != uid)
			{
				++iter;
				continue;
			}

			const Components::ComponentInfos* pinfos = &(*iter);
			
			bool islocal = this->networkInterface().intTcpAddr().ip == pinfos->pIntAddr->ip ||
					this->networkInterface().extTcpAddr().ip == pinfos->pIntAddr->ip;

			bool usable = checkComponentUsable(pinfos, true, false);

			if(usable)
			{
				if(islocal)
				{
					Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
					
					MachineInterface::onBroadcastInterfaceArgs25::staticAddToBundle((*pBundle), pinfos->uid, 
						pinfos->username, findComponentType, pinfos->cid, pinfos->cid, pinfos->globalOrderid, pinfos->groupOrderid, pinfos->gus,
						pinfos->pIntAddr->ip, pinfos->pIntAddr->port,
						pinfos->pExtAddr->ip, pinfos->pExtAddr->port, pinfos->externalAddressEx, pinfos->pid, 
						pinfos->cpu, pinfos->mem, pinfos->usedmem, 
						(int8)pinfos->state, Ouroboros::getProcessPID(), pinfos->extradata, pinfos->extradata1, pinfos->extradata2, pinfos->extradata3, 0, 0);

					if(finderRecvPort != 0)
					{
						ep.sendto(pBundle, finderRecvPort, pChannel->addr().ip);
						Network::Bundle::reclaimPoolObject(pBundle);
					}
					else
					{
						pChannel->send(pBundle);
					}
				}

				++iter;
			}
			else
			{
				WARNING_MSG(fmt::format("Machine::onQueryAllInterfaceInfos: {}[{}] invalid, erase {}.\n",
					COMPONENT_NAME_EX(pinfos->componentType),
					pinfos->cid,
					COMPONENT_NAME_EX(pinfos->componentType)));

				removeComponentID(pinfos->componentType, pinfos->cid, uid);
				iter = components.erase(iter);

				if(islocal)
					SystemInfo::getSingleton().clear();
			}
		}
	}
}

//-------------------------------------------------------------------------------------
bool Machine::findBroadcastInterface()
{

	std::map<u_int32_t, std::string> interfaces;
	Network::BundleBroadcast bhandler(networkInterface_, OURO_PORT_BROADCAST_DISCOVERY);

	if (!bhandler.epListen().getInterfaces(interfaces))
	{
		ERROR_MSG("Machine::findBroadcastInterface: Failed to discover network interfaces\n");
		return false;
	}

	uint8 data = 1;
	bhandler << data;
	if (!bhandler.broadcast(OURO_PORT_BROADCAST_DISCOVERY))
	{
		ERROR_MSG(fmt::format("Machine::findBroadcastInterface:Failed to send broadcast discovery message. error:{}\n",
			ouro_strerror()));
		return false;
	}
	
	sockaddr_in	sin;
	memset(&sin, 0, sizeof(sin));

	if(bhandler.receive(NULL, &sin))
	{
		INFO_MSG(fmt::format("Machine::findBroadcastInterface:Machine::findBroadcastInterface: Broadcast discovery receipt from {}.\n",
					inet_ntoa((struct in_addr&)sin.sin_addr.s_addr)));

		std::map< u_int32_t, std::string >::iterator iter;

		iter = interfaces.find( (u_int32_t &)sin.sin_addr.s_addr );
		if (iter != interfaces.end())
		{
			INFO_MSG(fmt::format("Machine::findBroadcastInterface: Confirmed {} ({}) as default broadcast route interface.\n",
				inet_ntoa((struct in_addr&)sin.sin_addr.s_addr),
				iter->second.c_str()));

			broadcastAddr_ = sin.sin_addr.s_addr;
			return true;
		}
	}
	
	std::string sinterface = "\t[";
	std::map< u_int32_t, std::string >::iterator iter = interfaces.begin();
	for(; iter != interfaces.end(); ++iter)
	{
		sinterface += inet_ntoa((struct in_addr&)iter->first);
		sinterface += ", ";
	}

	sinterface += "]";

	ERROR_MSG(fmt::format("Machine::findBroadcastInterface: Broadcast discovery [{}] "
		"not a valid interface. available interfaces:{}\n",
		inet_ntoa((struct in_addr&)sin.sin_addr.s_addr), sinterface.c_str()));

	return false;
}

//-------------------------------------------------------------------------------------
bool Machine::initNetwork()
{
	epBroadcast_.socket(SOCK_DGRAM);
	ep_.socket(SOCK_DGRAM);
	epLocal_.socket(SOCK_DGRAM); 

	Network::Address address;
	address.ip = 0;
	address.port = 0;

	if (broadcastAddr_ == 0 && !this->findBroadcastInterface())
	{
		ERROR_MSG("Machine::initNetwork: Failed to determine default broadcast interface. "
				"Make sure that your broadcast route is set correctly. "
				"e.g. /sbin/ip route add broadcast 255.255.255.255 dev eth0, eth0 is internalInterface!\n" );

		return false;
	}

	if (!ep_.good() ||
		ep_.bind(htons(OURO_MACHINE_BROADCAST_SEND_PORT), broadcastAddr_) == -1)
	{
		ERROR_MSG(fmt::format("Machine::initNetwork: Failed to bind UDP-socket to '{}:{}'. {}.\n",
							inet_ntoa((struct in_addr &)broadcastAddr_),
							(OURO_MACHINE_BROADCAST_SEND_PORT),
							ouro_strerror()));

		return false;
	}

	address.ip = broadcastAddr_;
	address.port = htons(OURO_MACHINE_BROADCAST_SEND_PORT);
	ep_.setbroadcast( true );
	ep_.setnonblocking(true);
	ep_.addr(address);
	pEPPacketReceiver_ = new Network::UDPPacketReceiver(ep_, this->networkInterface());

	if(!this->dispatcher().registerReadFileDescriptor(ep_, pEPPacketReceiver_))
	{
		ERROR_MSG("Machine::initNetwork: registerReadFileDescriptor ep is failed!\n");
		return false;
	}
	
#if OURO_PLATFORM == PLATFORM_WIN32
	u_int32_t baddr = htonl(INADDR_ANY);
#else
	u_int32_t baddr = Network::BROADCAST;
#endif

	if (!epBroadcast_.good() ||
		epBroadcast_.bind(htons(OURO_MACHINE_BROADCAST_SEND_PORT), baddr) == -1)
	{
		ERROR_MSG(fmt::format("Machine::initNetwork: Failed to bind socket to '{}:{}'. {}.\n",
							inet_ntoa((struct in_addr &)baddr),
							(OURO_MACHINE_BROADCAST_SEND_PORT),
							ouro_strerror()));

#if OURO_PLATFORM != PLATFORM_WIN32
		return false;
#endif
	}
	else
	{
		address.ip = baddr;
		address.port = htons(OURO_MACHINE_BROADCAST_SEND_PORT);
		epBroadcast_.setnonblocking(true);
		epBroadcast_.addr(address);
		pEBPacketReceiver_ = new Network::UDPPacketReceiver(epBroadcast_, this->networkInterface());
	
		if(!this->dispatcher().registerReadFileDescriptor(epBroadcast_, pEBPacketReceiver_))
		{
			ERROR_MSG("Machine::initNetwork: registerReadFileDescriptor epBroadcast is failed!\n");
			return false;
		}

	}

	if (!epLocal_.good() ||
		epLocal_.bind(htons(OURO_MACHINE_BROADCAST_SEND_PORT), Network::LOCALHOST) == -1)
	{
		ERROR_MSG(fmt::format("Machine::initNetwork: Failed to bind socket to (lo). {}.\n",
							ouro_strerror()));

		return false;
	}

	address.ip = Network::LOCALHOST;
	address.port = htons(OURO_MACHINE_BROADCAST_SEND_PORT);
	epLocal_.setnonblocking(true);
	epLocal_.addr(address);
	pEPLocalPacketReceiver_ = new Network::UDPPacketReceiver(epLocal_, this->networkInterface());

	if(!this->dispatcher().registerReadFileDescriptor(epLocal_, pEPLocalPacketReceiver_))
	{
		ERROR_MSG("Machine::initNetwork: registerReadFileDescriptor epLocal is failed!\n");
		return false;
	}

	INFO_MSG(fmt::format("Machine::initNetwork: bind broadcast successfully! addr:{}\n", ep_.addr().c_str()));
	return true;
}

//-------------------------------------------------------------------------------------
bool Machine::run()
{
	bool ret = true;

	while(!this->dispatcher().hasBreakProcessing())
	{
		threadPool_.onMainThreadTick();
		this->dispatcher().processOnce(false);
		networkInterface().processChannels(&MachineInterface::messageHandlers);
		Ouroboros::sleep(100);
	};

	return ret;
}

//-------------------------------------------------------------------------------------
void Machine::handleTimeout(TimerHandle handle, void * arg)
{
	ServerApp::handleTimeout(handle, arg);
}

//-------------------------------------------------------------------------------------
bool Machine::initializeBegin()
{
	if(!initNetwork())
		return false;

	return true;
}

//-------------------------------------------------------------------------------------
bool Machine::inInitialize()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool Machine::initializeEnd()
{
	pActiveTimerHandle_->cancel(); // machine does not need to be active with other components
	return true;
}

//-------------------------------------------------------------------------------------
void Machine::finalise()
{
	ServerApp::finalise();
}

//-------------------------------------------------------------------------------------
void Machine::startserver(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	int32 uid = 0;
	COMPONENT_TYPE componentType;
	uint64 cid = 0;
	uint16 gus = 0;
	std::string OURO_ROOT, OURO_RES_PATH, OURO_BIN_PATH;

	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
	bool success = true;

	uint16 finderRecvPort = 0;

	s >> uid;
	s >> componentType;
	s >> cid;
	s >> gus;
	
	if(s.length() > 0)
	{
		s >> finderRecvPort;
	}

	if (s.length() > 0)
	{
		s >> OURO_ROOT;
		s >> OURO_RES_PATH;
		s >> OURO_BIN_PATH;
	}

	INFO_MSG(fmt::format("Machine::startserver: uid={}, [{}], addr={}, cid={}, gus={}, OURO_ROOT={}, OURO_RES_PATH={}, OURO_BIN_PATH={}\n", 
		uid, COMPONENT_NAME_EX(componentType), pChannel->c_str(), cid, gus, OURO_ROOT, OURO_RES_PATH, OURO_BIN_PATH));
	
	if(ComponentName2ComponentType(COMPONENT_NAME_EX(componentType)) == UNKNOWN_COMPONENT_TYPE)
		return;

#if OURO_PLATFORM == PLATFORM_WIN32
	if (startWindowsProcess(uid, componentType, cid, gus, OURO_ROOT, OURO_RES_PATH, OURO_BIN_PATH) <= 0)
	{
		success = false;
	}
#else
	if (startLinuxProcess(uid, componentType, cid, gus, OURO_ROOT, OURO_RES_PATH, OURO_BIN_PATH) <= 0)
	{
		success = false;
	}
#endif
	
	(*pBundle) << success;

	if(finderRecvPort != 0)
	{
		Network::EndPoint ep;
		ep.socket(SOCK_DGRAM);

		if (!ep.good())
		{
			ERROR_MSG("Machine::startserver: Failed to create socket.\n");
			return;
		}
	
		ep.sendto(pBundle, htons(finderRecvPort), pChannel->addr().ip);
		Network::Bundle::reclaimPoolObject(pBundle);
	}
	else
	{
		pChannel->send(pBundle);
	}
}

//-------------------------------------------------------------------------------------
void Machine::stopserver(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	int32 uid = 0;
	COMPONENT_TYPE componentType;
	COMPONENT_ID componentID;
	bool success = true;

	uint16 finderRecvPort = 0;

	s >> uid;
	s >> componentType;
	
	// If the component ID is greater than 0, only stop the component of the specified ID
	s >> componentID;
	
	if(s.length() > 0)
	{
		s >> finderRecvPort;
	}

	INFO_MSG(fmt::format("Machine::stopserver: request uid={}, componentType={}, componentID={},  addr={}\n", 
		uid,  COMPONENT_NAME_EX(componentType), componentID, pChannel->c_str()));

	if(ComponentName2ComponentType(COMPONENT_NAME_EX(componentType)) == UNKNOWN_COMPONENT_TYPE)
	{
		ERROR_MSG(fmt::format("Machine::stopserver: component({}) error!", 
			(int)ComponentName2ComponentType(COMPONENT_NAME_EX(componentType))));

		return;
	}

	Components::COMPONENTS& components = Components::getSingleton().getComponents(componentType);

	if(components.size() > 0)
	{
		Components::COMPONENTS::iterator iter = components.begin();

		for(; iter != components.end(); )
		{
			Components::ComponentInfos* cinfos = &(*iter);

			if(componentID > 0 && componentID != cinfos->cid)
			{
				iter++;
				continue;
			}

			if(cinfos->uid != uid)
			{
				iter++;
				continue;
			}

			if(componentType != cinfos->componentType)
			{
				iter++;
				continue;
			}

			if(((*iter).flags & COMPONENT_FLAG_SHUTTINGDOWN) > 0)
			{
				iter++;
				continue;
			}

			INFO_MSG(fmt::format("--> stop {}({}), addr={}\n", 
				(*iter).cid, COMPONENT_NAME[componentType], (cinfos->pIntAddr != NULL ? cinfos->pIntAddr->c_str() : "unknown")));

			bool usable = checkComponentUsable(&(*iter), false, false);
		
			if(!usable)
			{
				removeComponentID(componentType, (*iter).cid, uid);
				iter = components.erase(iter);
				continue;
			}

			Network::Bundle closebundle;
			if(componentType != BOTS_TYPE)
			{
				COMMON_NETWORK_MESSAGE(componentType, closebundle, reqCloseServer);
			}
			else
			{
				closebundle.newMessage(BotsInterface::reqCloseServer);
			}

			Network::EndPoint ep1;
			ep1.socket(SOCK_STREAM);

			if (!ep1.good())
			{
				ERROR_MSG("Machine::stopserver: Failed to create socket.\n");
				success = false;
				break;
			}
		
			if(ep1.connect((*iter).pIntAddr.get()->port, (*iter).pIntAddr.get()->ip) == -1)
			{
				ERROR_MSG(fmt::format("Machine::stopserver: connect server error({})!\n", ouro_strerror()));
				success = false;
				break;
			}

			ep1.setnonblocking(false);
			ep1.send(&closebundle);

			Network::TCPPacket recvpacket;
			recvpacket.resize(255);

			fd_set	fds;
			struct timeval tv = { 0, 1000000 }; // 1000ms

			FD_ZERO( &fds );
			FD_SET((int)ep1, &fds);

			int selgot = select(ep1+1, &fds, NULL, NULL, &tv);
			if(selgot == 0)
			{
				// timeout, maybe the other party is busy
				ERROR_MSG(fmt::format("--> stop {}({}), addr={}, timeout!\n", 
					(*iter).cid, COMPONENT_NAME[componentType], (cinfos->pIntAddr != NULL ? 
					cinfos->pIntAddr->c_str() : "unknown")));

				iter++;
				continue;
			}
			else if(selgot == -1)
			{
				WARNING_MSG(fmt::format("--> stop {}({}), addr={}, recv_len == -1!\n", 
					(*iter).cid, COMPONENT_NAME[componentType], (cinfos->pIntAddr != NULL ? 
					cinfos->pIntAddr->c_str() : "unknown")));

				iter++;
				continue;
			}

			(*iter).flags |= COMPONENT_FLAG_SHUTTINGDOWN;

			int len = ep1.recv(recvpacket.data(), 1);
			if(len != 1)
			{
				ERROR_MSG(fmt::format("--> stop {}({}), addr={}, recv_len != 1!\n", 
					(*iter).cid, COMPONENT_NAME[componentType], (cinfos->pIntAddr != NULL ? 
					cinfos->pIntAddr->c_str() : "unknown")));

				success = false;
				break;
			}

			recvpacket >> success;
			if (success)
			{
				removeComponentID(componentType, (*iter).cid, uid);
			}
			
			iter++;
		}
	}
	else
	{
		INFO_MSG(fmt::format("Machine::stopserver: uid={}, {} size is 0, addr={}\n", 
			uid,  COMPONENT_NAME_EX(componentType), pChannel->c_str()));
	}

	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
	(*pBundle) << success;

	if(finderRecvPort != 0)
	{
		Network::EndPoint ep;
		ep.socket(SOCK_DGRAM);

		if (!ep.good())
		{
			ERROR_MSG("Machine::stopserver: Failed to create socket.\n");
			Network::Bundle::reclaimPoolObject(pBundle);
			return;
		}
	
		ep.sendto(pBundle, finderRecvPort, pChannel->addr().ip);
		Network::Bundle::reclaimPoolObject(pBundle);
	}
	else
	{
		pChannel->send(pBundle);
	}
}

//-------------------------------------------------------------------------------------		
void Machine::killserver(Network::Channel* pChannel, Ouroboros::MemoryStream& s)
{
	int32 uid = 0;
	COMPONENT_TYPE componentType;
	COMPONENT_ID componentID;
	bool success = true;

	uint16 finderRecvPort = 0;

	s >> uid;
	s >> componentType;

	// If the component ID is greater than 0, only stop the component of the specified ID
	s >> componentID;

	if (s.length() > 0)
	{
		s >> finderRecvPort;
	}

	INFO_MSG(fmt::format("Machine::killserver: request uid={}, componentType={}, componentID={},  addr={}\n",
		uid, COMPONENT_NAME_EX(componentType), componentID, pChannel->c_str()));

	if (ComponentName2ComponentType(COMPONENT_NAME_EX(componentType)) == UNKNOWN_COMPONENT_TYPE)
	{
		ERROR_MSG(fmt::format("Machine::killserver: component({}) error!",
			(int)ComponentName2ComponentType(COMPONENT_NAME_EX(componentType))));

		return;
	}

	Components::COMPONENTS& components = Components::getSingleton().getComponents(componentType);

	if (components.size() > 0)
	{
		Components::COMPONENTS::iterator iter = components.begin();

		for (; iter != components.end();)
		{
			Components::ComponentInfos* cinfos = &(*iter);

			if (componentID > 0 && componentID != cinfos->cid)
			{
				iter++;
				continue;
			}

			if (cinfos->uid != uid)
			{
				iter++;
				continue;
			}

			if (componentType != cinfos->componentType)
			{
				iter++;
				continue;
			}

			INFO_MSG(fmt::format("--> kill {}({}), addr={}\n",
				(*iter).cid, COMPONENT_NAME[componentType], (cinfos->pIntAddr != NULL ? cinfos->pIntAddr->c_str() : "unknown")));

			int killtry = 0;
			bool killed = false;

			while (killtry++ < 10)
			{
				// kill the process
				std::string killcmd;

#if OURO_PLATFORM == PLATFORM_WIN32
				killcmd = fmt::format("taskkill /f /t /pid {}", cinfos->pid);
				
#else
				killcmd = fmt::format("kill -s 9 {}", cinfos->pid);
#endif

				if (-1 == system(killcmd.c_str()))
				{
					ERROR_MSG(fmt::format("Machine::killserver: system({}) error!",
						killcmd));
				}

				bool usable = checkComponentUsable(&(*iter), false, false);

				if (!usable)
				{
					removeComponentID(componentType, (*iter).cid, uid);
					iter = components.erase(iter);
					killed = true;
					break;
				}

				sleep(100);
			}

			if (killed)
				continue;

			success = false;
			break;
		}
	}
	else
	{
		INFO_MSG(fmt::format("Machine::killserver: uid={}, {} size is 0, addr={}\n",
			uid, COMPONENT_NAME_EX(componentType), pChannel->c_str()));
	}

	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
	(*pBundle) << success;

	if (finderRecvPort != 0)
	{
		Network::EndPoint ep;
		ep.socket(SOCK_DGRAM);

		if (!ep.good())
		{
			ERROR_MSG("Machine::killserver: Failed to create socket.\n");
			Network::Bundle::reclaimPoolObject(pBundle);
			return;
		}

		ep.sendto(pBundle, finderRecvPort, pChannel->addr().ip);
		Network::Bundle::reclaimPoolObject(pBundle);
	}
	else
	{
		pChannel->send(pBundle);
	}
}

//-------------------------------------------------------------------------------------		
bool Machine::installSignals()
{
	ServerApp::installSignals();
	g_ouroSignalHandlers.addSignal(SIGCHLD, this);
	return true;
}

//-------------------------------------------------------------------------------------	
void Machine::onSignalled(int sigNum)
{
	if (sigNum == SIGCHLD)
	{
#if OURO_PLATFORM != PLATFORM_WIN32
		/* Wait for all dead processes.
		* We use a non-blocking call to be sure this signal handler will not
		* block if a child was cleaned up in another part of the program. */
		while (waitpid(-1, NULL, WNOHANG) > 0) {
		}
#endif
	}
	else
	{
		ServerApp::onSignalled(sigNum);
	}
}

//-------------------------------------------------------------------------------------
#if OURO_PLATFORM != PLATFORM_WIN32
uint16 Machine::startLinuxProcess(int32 uid, COMPONENT_TYPE componentType, uint64 cid, uint16 gus, 
	std::string& OURO_ROOT, std::string& OURO_RES_PATH, std::string& OURO_BIN_PATH)
{
	uint16 childpid;

	if ((childpid = fork()) == 0)
	{
		if (setuid(uid) == -1)
		{
			ERROR_MSG(fmt::format("Machine::startLinuxProcess: Failed to setuid to {}, aborting exec for '{}'\n", 
				uid,  COMPONENT_NAME_EX(componentType)));

			exit(1);
		}

		if (OURO_ROOT == "")
		{
			OURO_ROOT = Resmgr::getSingleton().getEnv().root_path;
		}

		if (OURO_RES_PATH == "")
		{
			OURO_RES_PATH = Resmgr::getSingleton().getEnv().res_path;
		}

		if (OURO_BIN_PATH == "")
		{
			OURO_BIN_PATH = Resmgr::getSingleton().getEnv().bin_path;
		}

		std::string bin_path = OURO_BIN_PATH;
		
		setenv("OURO_ROOT", OURO_ROOT.c_str(), 1);
		setenv("OURO_RES_PATH", OURO_RES_PATH.c_str(), 1);
		setenv("OURO_BIN_PATH", bin_path.c_str(), 1);

		std::string cmdLine = bin_path + COMPONENT_NAME_EX(componentType);

		// Change the current directory to make the core can be generated here when the problem occurs
		//chdir(bin_path.c_str());

		const char *argv[6];
		const char **pArgv = argv;
		std::string scid = fmt::format("--cid={}", cid);
		std::string sgus = fmt::format("--gus={}", gus);
		
		*pArgv++ = cmdLine.c_str();
		*pArgv++ = scid.c_str();
		*pArgv++ = sgus.c_str();
		*pArgv = NULL;

		// Close the socket of the parent class
		ep_.close();
		epBroadcast_.close();
		epLocal_.close();

		INFO_MSG(fmt::format("Machine::startLinuxProcess: UID {} execing '{}', cid = {}, gus = {}\n", uid, cmdLine, cid, gus));

		DebugHelper::getSingleton().closeLogger();
		int result = execv(cmdLine.c_str(), (char * const *)argv);

		if (result == -1)
		{
			ERROR_MSG(fmt::format("Machine::startLinuxProcess: Failed to exec '{}'\n", cmdLine));
		}

		exit(1);
		return 0;
	}
	else
		return childpid;

	return 0;
}

#else

DWORD Machine::startWindowsProcess(int32 uid, COMPONENT_TYPE componentType, uint64 cid, uint16 gus,
	std::string& OURO_ROOT, std::string& OURO_RES_PATH, std::string& OURO_BIN_PATH)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	std::string str = Resmgr::getSingleton().getEnv().bin_path;
	str += COMPONENT_NAME_EX(componentType);
	str += ".exe";

	// Enclose the command line in double quotation marks to avoid spaces in the path and execute the error
	str = "\"" + str + "\"";

	// add parameters
	str += fmt::format(" --cid={}", cid);
	str += fmt::format(" --gus={}", gus);

	wchar_t* szCmdline = Ouroboros::strutil::char2wchar(str.c_str());

	// Use the current working directory of the machine as the working directory for the new process.
	// A working directory (such as a log) that is consistent with the operation of the file in the relative directory.
	wchar_t currdir[1024];
	GetCurrentDirectory(sizeof(currdir), currdir);

	ZeroMemory( &si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi));

	if(!CreateProcess( NULL,   // No module name (use command line)
		szCmdline,      // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		CREATE_NEW_CONSOLE,    // No creation flags
		NULL,           // Use parent's environment block
		currdir,        // Use parent's starting directory
		&si,            // Pointer to STARTUPINFO structure
		&pi )           // Pointer to PROCESS_INFORMATION structure
		)
	{
		ERROR_MSG(fmt::format("Machine::startWindowsProcess: CreateProcess failed ({}).\n",
			GetLastError()));

		return 0;
	}

	free(szCmdline);

	return pi.dwProcessId;
}

#endif

}
