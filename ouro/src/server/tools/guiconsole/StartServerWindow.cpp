// StartServerWindow.cpp : implementation file
//

#include "stdafx.h"
#include "guiconsole.h"
#include "StartServerWindow.h"
#include "StartServerLayoutWindow.h"
#include "machine/machine_interface.h"

// CStartServerWindow dialog

IMPLEMENT_DYNAMIC(CStartServerWindow, CDialog)

CStartServerWindow::CStartServerWindow(CWnd* pParent /*=NULL*/)
	: CDialog(CStartServerWindow::IDD, pParent)
{

}

CStartServerWindow::~CStartServerWindow()
{
}

void CStartServerWindow::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
	DDX_Control(pDX, IDC_COMBO3, m_layoutlist);
	DDX_Control(pDX, IDC_LIST2, m_list1);
}


BEGIN_MESSAGE_MAP(CStartServerWindow, CDialog)
	ON_BN_CLICKED(IDC_BUTTON4, &CStartServerWindow::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON2, &CStartServerWindow::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CStartServerWindow::OnBnClickedButton3)
//	ON_CBN_SELCHANGE(IDC_COMBO3, &CStartServerWindow::OnCbnSelchangeCombo3)
ON_CBN_SELCHANGE(IDC_COMBO3, &CStartServerWindow::OnCbnSelchangeCombo3)
ON_NOTIFY(NM_THEMECHANGED, IDC_COMBO3, &CStartServerWindow::OnNMThemeChangedCombo3)
END_MESSAGE_MAP()


// CStartServerWindow message handlers
BOOL CStartServerWindow::OnInitDialog()
{
	CDialog::OnInitDialog();

	DWORD dwStyle = m_list.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT; //Select one to exercise the entire line highlighting (only applicable to the list style of report style)
	dwStyle |= LVS_EX_GRIDLINES; //Gridlines (only for list styles with report style)
	//dwStyle |= LVS_EX_ONECLICKACTIVATE;
	m_list.SetExtendedStyle(dwStyle); //Set the extended style

	dwStyle = m_list1.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT; //Select one to exercise the entire line highlighting (only applicable to the list style of report style)
	dwStyle |= LVS_EX_GRIDLINES; //Gridlines (only for list styles with report style)
	//dwStyle |= LVS_EX_ONECLICKACTIVATE;
	m_list1.SetExtendedStyle(dwStyle); //Set the extended style

	int idx = 0;
	m_list.InsertColumn(idx++, _T("componentType"),				LVCFMT_CENTER,	150);
	m_list.InsertColumn(idx++, _T("addr"),						LVCFMT_CENTER,	200);
	m_list.InsertColumn(idx++, _T("running"),					LVCFMT_CENTER,	200);

	idx = 0;
	m_list1.InsertColumn(idx++, _T("componentType"),			LVCFMT_CENTER,	200);
	m_list1.InsertColumn(idx++, _T("addr"),						LVCFMT_CENTER,	250);

	loadLayouts();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CStartServerWindow::OnBnClickedButton4()
{
	// TODO: Add your control notification handler code here
	CStartServerLayoutWindow dlg(this);
	dlg.DoModal();
}

void CStartServerWindow::OnBnClickedButton2()
{
	CString s;

	if(m_layoutlist.GetCurSel() < 0 || m_layoutlist.GetCurSel() > m_layoutlist.GetCount())
	{
		::AfxMessageBox(L"please select a layout!");
		return;
	}

	m_layoutlist.GetLBText(m_layoutlist.GetCurSel(), s);

	char* cs = Ouroboros::strutil::wchar2char(s.GetBuffer(0));
	OUROUnordered_map< std::string, std::vector<CStartServerWindow::LAYOUT_ITEM> >::iterator iter =
		layouts_.find(cs);

	free(cs);

	if(iter == layouts_.end())
	{
		::AfxMessageBox(L"please select a layout!");
		return;
	}

	std::vector<CStartServerWindow::LAYOUT_ITEM>::iterator iter1 = iter->second.begin();
	for(; iter1 != iter->second.end(); iter1++)
	{
		LAYOUT_ITEM& item = (*iter1);

		Ouroboros::COMPONENT_TYPE ctype = Ouroboros::ComponentName2ComponentType(item.componentName.c_str());

		std::vector<std::string> vec;
		Ouroboros::strutil::ouro_split(item.addr, ':', vec);
		if(vec.size() != 2)
		{
			continue;
		}

		Ouroboros::Network::EndPoint* endpoint = Ouroboros::Network::EndPoint::createPoolObject(OBJECTPOOL_POINT);

		Ouroboros::u_int32_t address;
		Ouroboros::Network::Address::string2ip(vec[0].c_str(), address);
		Ouroboros::Network::Address addr(address, htons(atoi(vec[1].c_str())));

		if(addr.ip == 0)
		{
			::AfxMessageBox(L"address error!");
			Ouroboros::Network::EndPoint::reclaimPoolObject(endpoint);
			continue;
		}

		endpoint->socket(SOCK_STREAM);
		if (!endpoint->good())
		{
			AfxMessageBox(L"couldn't create a socket\n");
			Ouroboros::Network::EndPoint::reclaimPoolObject(endpoint);
			continue;
		}

		endpoint->addr(addr);
		if(endpoint->connect(addr.port, addr.ip) == -1)
		{
			CString err;
			err.Format(L"connect server error! %d", ::WSAGetLastError());
			AfxMessageBox(err);
			Ouroboros::Network::EndPoint::reclaimPoolObject(endpoint);
			continue;
		}

		endpoint->setnonblocking(true);

		Ouroboros::uint64 cid = Ouroboros::genUUID64();
		Ouroboros::int16 gus = -1;

		Ouroboros::Network::Bundle bundle;
		bundle.newMessage(Ouroboros::MachineInterface::startserver);
		bundle << Ouroboros::getUserUID();
		bundle << ctype;
		endpoint->send(&bundle);
		Ouroboros::Network::TCPPacket packet;
		packet.resize(1024);

		fd_set	fds;
		struct timeval tv = { 0, 1000000 }; // 1000ms

		FD_ZERO( &fds );
		FD_SET((int)(*endpoint), &fds);

		int selgot = select((*endpoint)+1, &fds, NULL, NULL, &tv);
		if(selgot == 0)
		{
			Ouroboros::Network::EndPoint::reclaimPoolObject(endpoint);
			continue; // timeout may be busy
		}
		else if(selgot == -1)
		{
			Ouroboros::Network::EndPoint::reclaimPoolObject(endpoint);
			continue;
		}
		else
		{
			endpoint->recv(packet.data(), 1024);
		}

		bool success = true;
		packet << success;

		if(success)
		{
			for(int row = 0; row < m_list.GetItemCount(); row++)
			{
				CString name = m_list.GetItemText(row, 0);
				CString addr = m_list.GetItemText(row, 1);
				CString running = m_list.GetItemText(row, 2);

				char* cs1 = Ouroboros::strutil::wchar2char(name.GetBuffer(0));
				char* cs2 = Ouroboros::strutil::wchar2char(addr.GetBuffer(0));

				if(item.componentName == cs1 && item.addr == cs2 && running == L"false")
				{
					free(cs1);
					free(cs2);
					m_list.SetItemText(row, 2, L"true");
					break;
				}

				free(cs1);
				free(cs2);
			}
		}

		Ouroboros::Network::EndPoint::reclaimPoolObject(endpoint);
	}
}

void CStartServerWindow::OnBnClickedButton3()
{
	CString s;

	if(m_layoutlist.GetCurSel() < 0 || m_layoutlist.GetCurSel() > m_layoutlist.GetCount())
	{
		::AfxMessageBox(L"please select a layout!");
		return;
	}

	m_layoutlist.GetLBText(m_layoutlist.GetCurSel(), s);

	char* cs = Ouroboros::strutil::wchar2char(s.GetBuffer(0));
	OUROUnordered_map< std::string, std::vector<CStartServerWindow::LAYOUT_ITEM> >::iterator iter =
		layouts_.find(cs);

	free(cs);

	if(iter == layouts_.end())
	{
		::AfxMessageBox(L"please select a layout!");
		return;
	}

	std::vector<CStartServerWindow::LAYOUT_ITEM>::iterator iter1 = iter->second.begin();
	for(; iter1 != iter->second.end(); iter1++)
	{
		LAYOUT_ITEM& item = (*iter1);

		Ouroboros::COMPONENT_TYPE ctype = Ouroboros::ComponentName2ComponentType(item.componentName.c_str());

		std::vector<std::string> vec;
		Ouroboros::strutil::ouro_split(item.addr, ':', vec);
		if(vec.size() != 2)
		{
			continue;
		}

		Ouroboros::Network::EndPoint* endpoint = Ouroboros::Network::EndPoint::createPoolObject(OBJECTPOOL_POINT);

		Ouroboros::u_int32_t address;
		Ouroboros::Network::Address::string2ip(vec[0].c_str(), address);
		Ouroboros::Network::Address addr(address, htons(atoi(vec[1].c_str())));

		if(addr.ip == 0)
		{
			::AfxMessageBox(L"address error!");
			Ouroboros::Network::EndPoint::reclaimPoolObject(endpoint);
			continue;
		}

		endpoint->socket(SOCK_STREAM);
		if (!endpoint->good())
		{
			AfxMessageBox(L"couldn't create a socket\n");
			Ouroboros::Network::EndPoint::reclaimPoolObject(endpoint);
			continue;
		}

		endpoint->addr(addr);
		if(endpoint->connect(addr.port, addr.ip) == -1)
		{
			CString err;
			err.Format(L"connect server error! %d", ::WSAGetLastError());
			AfxMessageBox(err);
			Ouroboros::Network::EndPoint::reclaimPoolObject(endpoint);
			continue;
		}

		endpoint->setnonblocking(true);

		Ouroboros::Network::Bundle bundle;
		bundle.newMessage(Ouroboros::MachineInterface::stopserver);
		bundle << Ouroboros::getUserUID();
		bundle << ctype;
		Ouroboros::COMPONENT_ID cid = 0;
		bundle << cid;
		endpoint->send(&bundle);
		Ouroboros::Network::TCPPacket packet;
		packet.resize(1024);

		fd_set	fds;
		struct timeval tv = { 0, 1000000 }; // 1000ms

		FD_ZERO( &fds );
		FD_SET((int)(*endpoint), &fds);

		int selgot = select((*endpoint)+1, &fds, NULL, NULL, &tv);
		if(selgot == 0)
		{
			Ouroboros::Network::EndPoint::reclaimPoolObject(endpoint);
			continue; // timeout may be busy
		}
		else if(selgot == -1)
		{
			Ouroboros::Network::EndPoint::reclaimPoolObject(endpoint);
			continue;
		}
		else
		{
			endpoint->recv(packet.data(), 1024);
		}

		bool success = true;
		packet << success;

		if(success)
		{
			for(int row = 0; row < m_list.GetItemCount(); row++)
			{
				CString name = m_list.GetItemText(row, 0);
				CString addr = m_list.GetItemText(row, 1);
				CString running = m_list.GetItemText(row, 2);

				char* cs1 = Ouroboros::strutil::wchar2char(name.GetBuffer(0));
				char* cs2 = Ouroboros::strutil::wchar2char(addr.GetBuffer(0));

				if(item.componentName == cs1 && item.addr == cs2 && running == L"true")
				{
					free(cs1);
					free(cs2);
					m_list.SetItemText(row, 2, L"false");
					break;
				}

				free(cs1);
				free(cs2);
			}
		}

		Ouroboros::Network::EndPoint::reclaimPoolObject(endpoint);
	}
}

void CStartServerWindow::loadLayouts()
{
    CString appPath = GetAppPath();
    CString fullPath = appPath + L"\\layouts.xml";

	m_layoutlist.ResetContent();
	layouts_.clear();

	char fname[4096] = {0};

	int len = WideCharToMultiByte(CP_ACP, 0, fullPath, fullPath.GetLength(), NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP,0, fullPath, fullPath.GetLength(), fname, len, NULL, NULL);
	fname[len + 1] = '\0';

	TiXmlDocument *pDocument = new TiXmlDocument(fname);
	if(pDocument == NULL || !pDocument->LoadFile(TIXML_ENCODING_UTF8))
		return;

	TiXmlElement *rootElement = pDocument->RootElement();
	TiXmlNode* node = rootElement->FirstChild();
	if(node)
	{
		do
		{
			std::vector<LAYOUT_ITEM>& vec = layouts_[node->Value()];

			wchar_t* ws = Ouroboros::strutil::char2wchar(node->Value());
			m_layoutlist.AddString(ws);
			free(ws);

			TiXmlNode* childnode = node->FirstChild();
			if(childnode == NULL)
				break;
			do
			{
				LAYOUT_ITEM item;
				item.componentName = childnode->Value();
				item.addr = childnode->FirstChild()->Value();
				vec.push_back(item);
			}while((childnode = childnode->NextSibling()));
		}while((node = node->NextSibling()));
	}

	pDocument->Clear();
	delete pDocument;
}

void CStartServerWindow::saveLayouts()
{
    //Create an XML document object.
    TiXmlDocument *pDocument = new TiXmlDocument();

	int i = 0;
	OUROUnordered_map< std::string, std::vector<LAYOUT_ITEM> >::iterator iter = layouts_.begin();
	TiXmlElement *rootElement = new TiXmlElement("root");
	pDocument->LinkEndChild(rootElement);

	for(; iter != layouts_.end(); iter++)
	{
		std::vector<LAYOUT_ITEM>::iterator iter1 = iter->second.begin();

		TiXmlElement *rootElementChild = new TiXmlElement(iter->first.c_str());
		rootElement->LinkEndChild(rootElementChild);

		for(; iter1 != iter->second.end(); iter1++)
		{
			LAYOUT_ITEM& item = (*iter1);

			TiXmlElement *rootElementChild1 = new TiXmlElement(item.componentName.c_str());
			rootElementChild->LinkEndChild(rootElementChild1);

			TiXmlText *content = new TiXmlText(item.addr.c_str());
			rootElementChild1->LinkEndChild(content);
		}
	}

    CString appPath = GetAppPath();
    CString fullPath = appPath + L"\\layouts.xml";

	char fname[4096] = {0};

	int len = WideCharToMultiByte(CP_ACP, 0, fullPath, fullPath.GetLength(), NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP,0, fullPath, fullPath.GetLength(), fname, len, NULL, NULL);
	fname[len + 1] = '\0';

	pDocument->SaveFile(fname);
}
//void CStartServerWindow::OnCbnSelchangeCombo3()
//{
//	// TODO: Add your control notification handler code here
//}

void CStartServerWindow::OnCbnSelchangeCombo3()
{
	// TODO: Add your control notification handler code here
	CString s;
	if(m_layoutlist.GetEditSel() < 0 || m_layoutlist.GetEditSel() > (DWORD)m_layoutlist.GetCount())
		return;

	m_layoutlist.GetLBText(m_layoutlist.GetEditSel(), s);

	if(s.GetLength() <= 0 )
		return;

	m_list.DeleteAllItems();

	char* cs = Ouroboros::strutil::wchar2char(s.GetBuffer(0));
	OUROUnordered_map< std::string, std::vector<LAYOUT_ITEM> >::iterator iter = layouts_.find(cs);
	free(cs);

	if(iter == layouts_.end())
	{
		return;
	}

	std::vector<LAYOUT_ITEM>::iterator iter1 = iter->second.begin();
	for(; iter1 != iter->second.end(); iter1++)
	{
		LAYOUT_ITEM& item = (*iter1);

		wchar_t* ws1 = Ouroboros::strutil::char2wchar(item.componentName.c_str());
		wchar_t* ws2 = Ouroboros::strutil::char2wchar(item.addr.c_str());

		m_list.InsertItem(0, ws1);
		m_list.SetItemText(0, 1, ws2);
		m_list.SetItemText(0, 2, L"false");

		free(ws1);
		free(ws2);
	}
}

void CStartServerWindow::OnNMThemeChangedCombo3(NMHDR *pNMHDR, LRESULT *pResult)
{
	// This feature requires Windows XP or greater.
	// The symbol _WIN32_WINNT must be >= 0x0501.
	// TODO: Add your control notification handler code here
	*pResult = 0;
}
