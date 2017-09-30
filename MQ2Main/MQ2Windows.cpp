/*****************************************************************************
MQ2Main.dll: MacroQuest2's extension DLL for EverQuest
Copyright (C) 2002-2003 Plazmic, 2003-2005 Lax

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2, as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
******************************************************************************/

#if !defined(CINTERFACE)
#error /DCINTERFACE
#endif
#ifdef _DEBUG
#define DBG_SPEW
#endif
//#define DEBUG_TRY

#include "MQ2Main.h"
#include <map>
#include <string>
#include <algorithm>
using namespace std;

map<string,CXWnd*> WindowMap;

PCHAR szClickNotification[] = { 
    "leftmouse",        //0
    "leftmouseup",      //1
    "leftmouseheld",    //2
    "leftmouseheldup",  //3
    "rightmouse",       //4
    "rightmouseup",     //5
    "rightmouseheld",   //6
    "rightmouseheldup", //7
}; 

struct _WindowInfo
{
    std::string Name;
    CXWnd *pWnd;
    CXWnd **ppWnd;
};

std::map<CXWnd *,_WindowInfo>WindowList;


bool GenerateMQUI();
void DestroyMQUI();

class CSidlInitHook
{
public:
    void Init_Trampoline(class CXStr*pName,int A);
    void Init_Detour(class CXStr*pName,int A)
    {
        CHAR Name[MAX_STRING]={0};
        if(pName)
			GetCXStr(pName->Ptr,Name,MAX_STRING);
        std::string WindowName=Name;
        MakeLower((WindowName));
		unsigned long N = 0;
		if (WindowMap.find(WindowName) != WindowMap.end()) {
			_WindowInfo wi;
			wi.Name = WindowName;
			wi.pWnd = (CXWnd*)this;
			wi.ppWnd = 0;
			WindowList[(CXWnd*)this] = wi;
            DebugSpew("Updating WndNotification target '%s'",Name);
        } else {
			_WindowInfo wi;
			wi.Name = WindowName;
			wi.pWnd = (CXWnd*)this;
			wi.ppWnd = 0;
			WindowList[(CXWnd*)this] = wi;
            WindowMap[WindowName]= (CXWnd*)this;
			if(Name[0]!='\0')
				DebugSpew("Adding WndNotification target '%s'",Name);
			else
				DebugSpew("Adding WndNotification target FAILED");
        }
        Init_Trampoline(pName, A);
    }
	int CTargetWnd__WndNotification_Tramp(class CXWnd *,unsigned __int32,void *);
	int CTargetWnd__WndNotification_Detour(class CXWnd *pWnd, unsigned int uiMessage, void* pData)
	{
		if (gUseTradeOnTarget) {
			if (uiMessage == XWM_LCLICK) {
				if (PCHARINFO2 pChar2 = GetCharInfo2()) {
					if (pTarget && pLocalPlayer && ((PSPAWNINFO)pTarget)->SpawnID != ((PSPAWNINFO)pLocalPlayer)->SpawnID && pEverQuest && pChar2->pInventoryArray && (pChar2->pInventoryArray->Inventory.Cursor || pChar2->CursorPlat || pChar2->CursorGold || pChar2->CursorSilver || pChar2->CursorCopper)) {
						//player has a item or coin on his cursor and clicked targetwindow, he wants to trade with target...
						pEverQuest->LeftClickedOnPlayer(pTarget);
						WeDidStuff();
					}
				}
			}
		}
		return CTargetWnd__WndNotification_Tramp(pWnd,uiMessage,pData);
	}
};
DETOUR_TRAMPOLINE_EMPTY(void CSidlInitHook::Init_Trampoline(class CXStr*,int));
DETOUR_TRAMPOLINE_EMPTY(int CSidlInitHook::CTargetWnd__WndNotification_Tramp(class CXWnd *,unsigned __int32,void *));

class CXWndManagerHook
{
public:
    int RemoveWnd_Trampoline(class CXWnd *);
    int RemoveWnd_Detour(class CXWnd *pWnd)
    {
		if (pWnd) {
			auto N = WindowList.find(pWnd);
			if (N != WindowList.end()) {
				std::string Name = N->second.Name;
				MakeLower(Name);
				if (WindowMap.find(Name) != WindowMap.end()) {
					WindowMap.erase(Name);
				}
				WindowList.erase(N);
			}
		}
        return RemoveWnd_Trampoline(pWnd);
    }
};
DETOUR_TRAMPOLINE_EMPTY(int CXWndManagerHook::RemoveWnd_Trampoline(class CXWnd *));

class CXMLSOMDocumentBaseHook
{
public:
    int XMLRead(CXStr *A, CXStr *B, CXStr *C, CXStr *D)
    {
		DWORD addr = (DWORD)pSidlMgr;
		DWORD tTHIS = (DWORD)this;
        char Temp[MAX_STRING]={0};
        GetCXStr(C->Ptr,Temp,MAX_STRING);
        DebugSpew("XMLRead(%s)",Temp);
        if (!_stricmp("EQUI.xml",Temp))
        {
            if (GenerateMQUI())
            {
                SetCXStr(&C->Ptr,"MQUI.xml");
                int Ret=XMLRead_Trampoline(A,B,C,D);
                DestroyMQUI();
                return Ret;
            }
        }
        return XMLRead_Trampoline(A,B,C,D);
    }
    int XMLRead_Trampoline(CXStr *A, CXStr *B, CXStr *C, CXStr *D);
};
DETOUR_TRAMPOLINE_EMPTY(int CXMLSOMDocumentBaseHook::XMLRead_Trampoline(CXStr *A, CXStr *B, CXStr *C, CXStr *D)); 

#ifndef ISXEQ
VOID ListWindows(PSPAWNINFO pChar, PCHAR szLine);
VOID WndNotify(PSPAWNINFO pChar, PCHAR szLine);
VOID ItemNotify(PSPAWNINFO pChar, PCHAR szLine);
VOID ListItemSlots(PSPAWNINFO pChar, PCHAR szLine);
#else
int ListWindows(int argc, char *argv[]);
int WndNotify(int argc, char *argv[]);
int ItemNotify(int argc, char *argv[]);
int ListItemSlots(int argc, char *argv[]); 
#endif

#ifndef ISXEQ_LEGACY

void InitializeMQ2Windows()
{
    DebugSpew("Initializing MQ2 Windows");

    extern PCHAR szItemSlot[];

    for(int i=0;i<NUM_INV_SLOTS;i++)
        ItemSlotMap[szItemSlot[i]]=i;

    CHAR szOut[MAX_STRING]={0};

#define AddSlotArray(name,count,start)    \
    for (int i = 0 ; i < count ; i++)\
    {\
    sprintf_s(szOut,#name"%d",i+1);\
    ItemSlotMap[szOut]=start+i;\
    }
    AddSlotArray(bank,24,2000);
    AddSlotArray(sharedbank,4,2500);
    AddSlotArray(trade,16,3000);
    AddSlotArray(world,10,4000);
    AddSlotArray(enviro,10,4000);
    AddSlotArray(loot,31,5000);
    AddSlotArray(merchant,80,6000);
    AddSlotArray(bazaar,80,7000);
    AddSlotArray(inspect,31,8000);
#undef AddSlotArray

    EzDetourwName(CXMLSOMDocumentBase__XMLRead,&CXMLSOMDocumentBaseHook::XMLRead,&CXMLSOMDocumentBaseHook::XMLRead_Trampoline,"CXMLSOMDocumentBase__XMLRead");
    EzDetourwName(CSidlScreenWnd__Init1,&CSidlInitHook::Init_Detour,&CSidlInitHook::Init_Trampoline,"CSidlScreenWnd__Init1");
	EzDetourwName(CTargetWnd__WndNotification,&CSidlInitHook::CTargetWnd__WndNotification_Detour,&CSidlInitHook::CTargetWnd__WndNotification_Tramp,"CTargetWnd__WndNotification");
    EzDetourwName(CXWndManager__RemoveWnd,&CXWndManagerHook::RemoveWnd_Detour,&CXWndManagerHook::RemoveWnd_Trampoline,"CXWndManager__RemoveWnd");

#ifndef ISXEQ
    AddCommand("/windows",ListWindows,false,true,false);
    AddCommand("/notify",WndNotify,false,true,false);
    AddCommand("/itemnotify",ItemNotify,false,true,false);
    AddCommand("/itemslots",ListItemSlots,false,true,false);
#else
    pISInterface->AddCommand("EQWindows",ListWindows);
    pISInterface->AddCommand("EQNotify",WndNotify);
    pISInterface->AddCommand("EQItemNotify",ItemNotify);
    pISInterface->AddCommand("EQItemSlots",ListItemSlots);
#endif 

	if (pWndMgr)
	{
		CHAR Name[MAX_STRING] = { 0 };
		for (int i = 0; i < ((_CXWNDMGR*)pWndMgr)->pWindows.Count; i++)
		{
			if (PCXWND pWnd = ((_CXWNDMGR*)pWndMgr)->pWindows[i]) {
				if (CXMLData *pXMLData = ((CXWnd*)pWnd)->GetXMLData())
				{
					if (pXMLData->Type == UI_Screen)
					{
						GetCXStr(pXMLData->Name.Ptr, Name, MAX_STRING);
						std::string WindowName = Name;
						MakeLower((WindowName));
						if (WindowMap.find(WindowName) != WindowMap.end()) {
							_WindowInfo wi;
							wi.Name = WindowName;
							wi.pWnd = (CXWnd*)pWnd;
							wi.ppWnd = 0;
							WindowList[(CXWnd*)pWnd] = wi;
						}
						else {
							_WindowInfo wi;
							wi.Name = WindowName;
							wi.pWnd = (CXWnd*)pWnd;
							wi.ppWnd = 0;
							WindowList[(CXWnd*)pWnd] = wi;
							WindowMap[WindowName] = (CXWnd*)pWnd;
						}
					}
				}
			}
		}
	}
}

void ShutdownMQ2Windows()
{
    DebugSpew("Shutting down MQ2 Windows");
#ifndef ISXEQ
    RemoveCommand("/windows");
    RemoveCommand("/notify");
    RemoveCommand("/itemnotify");
    RemoveCommand("/itemslots");
#else
    pISInterface->RemoveCommand("EQWindows");
    pISInterface->RemoveCommand("EQNotify");
    pISInterface->RemoveCommand("EQItemNotify");
    pISInterface->RemoveCommand("EQItemSlots");
#endif 
    RemoveDetour(CXMLSOMDocumentBase__XMLRead);
    RemoveDetour(CSidlScreenWnd__Init1);
    RemoveDetour(CTargetWnd__WndNotification);
    RemoveDetour(CXWndManager__RemoveWnd);
	//WindowList.clear();
}

bool GenerateMQUI()
{
    // create EverQuest\uifiles\default\MQUI.xml
    PCHARINFO       pCharInfo = NULL;
    CHAR            szFilename[MAX_PATH] = { 0 };
    CHAR            szOrgFilename[MAX_PATH] = { 0 };
    CHAR            UISkin[MAX_STRING] = { 0 };
    char            Buffer[2048];
    FILE           *forg,*fnew;

    if (!pXMLFiles) {
        DebugSpew("GenerateMQUI::Not Generating MQUI.xml, no files in our list");
        return false;
    }
    sprintf_s(UISkin, "default");
    sprintf_s(szOrgFilename, "%s\\uifiles\\%s\\EQUI.xml", gszEQPath, UISkin);
    sprintf_s(szFilename, "%s\\uifiles\\%s\\MQUI.xml", gszEQPath, UISkin);

    DebugSpew("GenerateMQUI::Generating %s", szFilename);

    errno_t err = fopen_s(&forg,szOrgFilename, "rt");
    if (err) {
        DebugSpew("GenerateMQUI::could not open %s", szOrgFilename);
        return false;
    }
    err = fopen_s(&fnew,szFilename, "wt");
    if (err) {
        DebugSpew("GenerateMQUI::could not open %s", szFilename);
        fclose(forg);
        return false;
    }
    while (fgets(Buffer, 2048, forg)) {
        if (strstr(Buffer, "</Composite>")) {
            DebugSpew("GenerateMQUI::Inserting our xml files");
            PMQXMLFILE      pFile = pXMLFiles;
            while (pFile) {
                DebugSpew("GenerateMQUI::Inserting %s",pFile->szFilename);
                fprintf(fnew, "<Include>%s</Include>\n", pFile->szFilename);
                pFile = pFile->pNext;
            }
        }
        fprintf(fnew, "%s", Buffer);
    }
    fclose(fnew);
    fclose(forg);

    if ((pCharInfo = GetCharInfo()) != NULL) {
        sprintf_s(szFilename, "%s\\UI_%s_%s.ini", gszEQPath, pCharInfo->Name, EQADDR_SERVERNAME);
        GetPrivateProfileString("Main", "UISkin", "default", UISkin, MAX_STRING, szFilename);

        if (strcmp(UISkin, "default")) {
            sprintf_s(szOrgFilename, "%s\\uifiles\\%s\\EQUI.xml", gszEQPath, UISkin);
            sprintf_s(szFilename, "%s\\uifiles\\%s\\MQUI.xml", gszEQPath, UISkin);

            DebugSpew("GenerateMQUI::Generating %s", szFilename);

            err = fopen_s(&forg,szOrgFilename, "rt");
            if (err) {
                DebugSpew("GenerateMQUI::could not open %s (non-fatal)", szOrgFilename);
                sprintf_s(szOrgFilename, "%s\\uifiles\\%s\\EQUI.xml", gszEQPath, "default");
                err = fopen_s(&forg,szOrgFilename, "rt");
                if (err) {
                    DebugSpew("GenerateMQUI::could not open %s", szOrgFilename);
                    DebugSpew("GenerateMQUI::giving up");
                    return false;
                }
            }
            err = fopen_s(&fnew,szFilename, "wt");
            if (err) {
                DebugSpew("GenerateMQUI::could not open %s", szFilename);
                fclose(forg);
                return false;
            }
            while (fgets(Buffer, 2048, forg)) {
                if (strstr(Buffer, "</Composite>")) {
                    //DebugSpew("GenerateMQUI::Inserting our xml files");
                    PMQXMLFILE      pFile = pXMLFiles;
                    while (pFile) {
                        //DebugSpew("GenerateMQUI::Inserting %s",pFile->szFilename);
                        fprintf(fnew, "<Include>%s</Include>\n", pFile->szFilename);
                        pFile = pFile->pNext;
                    }
                }
                fprintf(fnew, "%s", Buffer);
            }
            fclose(fnew);
            fclose(forg);
        }
    }
    return true;
}

void DestroyMQUI()
{
    // delete MQUI.xml files.
    PCHARINFO       pCharInfo = NULL;
    CHAR            szFilename[MAX_PATH] = { 0 };
    CHAR            UISkin[MAX_STRING] = { 0 };

    sprintf_s(szFilename, "%s\\uifiles\\%s\\MQUI.xml", gszEQPath, "default");
    DebugSpew("DestroyMQUI: removing file %s", szFilename);
    remove(szFilename);

    if ((pCharInfo = GetCharInfo()) != NULL) {
        sprintf_s(szFilename, "%s\\UI_%s_%s.ini", gszEQPath, pCharInfo->Name, EQADDR_SERVERNAME);
        //DebugSpew("UI File: %s", szFilename);
        GetPrivateProfileString("Main", "UISkin", "default", UISkin, MAX_STRING, szFilename);
        //DebugSpew("UISkin=%s", UISkin);
        sprintf_s(szFilename, "%s\\uifiles\\%s\\MQUI.xml", gszEQPath, UISkin);
        DebugSpew("DestroyMQUI: removing file %s", szFilename);
        remove(szFilename);
    }
}

void AddXMLFile(const char *filename)
{
    PMQXMLFILE      pFile = pXMLFiles;
    PMQXMLFILE      pLast = 0;
    while (pFile) {
        if (!_stricmp(pFile->szFilename, filename))
            return;        // already there.
        pLast = pFile;
        pFile = pFile->pNext;
    }
    CHAR            szBuffer[MAX_PATH] = { 0 };
    PCHARINFO       pCharInfo = NULL;
    CHAR            szFilename[MAX_PATH] = { 0 };
    CHAR            UISkin[MAX_STRING] = { 0 };
    sprintf_s(UISkin, "default");

    if ((pCharInfo = GetCharInfo()) != NULL) {
        sprintf_s(szFilename, "%s\\UI_%s_%s.ini", gszEQPath, pCharInfo->Name, EQADDR_SERVERNAME);
        GetPrivateProfileString("Main", "UISkin", "default", UISkin, MAX_STRING, szFilename);
    }
    sprintf_s(szBuffer, "%s\\uifiles\\%s\\%s", gszEQPath, UISkin, filename);

    if (!_FileExists(szBuffer)) {
        sprintf_s(szBuffer, "%s\\uifiles\\%s\\%s", gszEQPath, "default", filename);
        if (!_FileExists(szBuffer)) {
            WriteChatf("UI file %s not found in either uifiles\\%s or uifiles\\default.  Please copy it there, reload the UI, and reload this plugin.", filename, UISkin);
            return;
        }
    }

    DebugSpew("Adding XML File %s", filename);
    if (gGameState == GAMESTATE_INGAME) {
        WriteChatf("UI file %s added, you must reload your UI for this to take effect.", filename);
    }

    pFile = new MQXMLFILE;
    pFile->pLast = pLast;
    if (pLast)
        pLast->pNext = pFile;
    else
        pXMLFiles = pFile;
    pFile->pNext = 0;
    strcpy_s(pFile->szFilename, filename);
}

void RemoveXMLFile(const char *filename)
{
    PMQXMLFILE      pFile = pXMLFiles;
    while (pFile) {
        if (!_stricmp(pFile->szFilename, filename)) {
            DebugSpew("Removing XML File %s", filename);
            if (pFile->pLast)
                pFile->pLast->pNext = pFile->pNext;
            else
                pXMLFiles = pFile->pNext;
            if (pFile->pNext)
                pFile->pNext->pLast = pFile->pLast;
            delete pFile;
            return;
        }
        pFile = pFile->pNext;
    }
}
CXWnd *FindMQ2Window(PCHAR WindowName)
{
    _WindowInfo Info;
    std::string Name=WindowName;
    MakeLower(Name);
	//check toplevel windows first
	if (WindowMap.find(Name) != WindowMap.end()) {
		//found it no need to look further...
		return WindowMap[Name];
	} else {
		//didnt find one, is it a container?
		PCONTENTS pPack = 0;
		if (!_strnicmp(WindowName, "bank", 4)) {
			unsigned long nPack = atoi(&WindowName[4]);
			if (nPack && nPack <= NUM_BANK_SLOTS) {
				if (pCharData && ((PCHARINFO)pCharData)->pBankArray) {
					pPack = ((PCHARINFO)pCharData)->pBankArray->Bank[nPack - 1];
				}
			}
		}
		else if (!_strnicmp(WindowName, "pack", 4)) {
			unsigned long nPack = atoi(&WindowName[4]);
			if (nPack && nPack <= 10) {
				if (PCHARINFO2 pChar2 = GetCharInfo2()) {
					if (pChar2->pInventoryArray) {
						pPack = pChar2->pInventoryArray->Inventory.Pack[nPack - 1];
					}
				}
			}
		}
		else if (!_stricmp(WindowName, "enviro")) {
			pPack = ((PEQ_CONTAINERWND_MANAGER)pContainerMgr)->pWorldContents;
		}
		if (pPack) {
			return (CXWnd*)FindContainerForContents(pPack);
		}
	}
	//didnt find a toplevel window, is it a child then?
	bool namefound = false;
	for (std::map<CXWnd*, _WindowInfo>::iterator N = WindowList.begin(); N != WindowList.end(); N++)
	{
		if (N->second.Name == Name) {
			namefound = true;
			Info = N->second;
			break;
		}
	}
	if (!namefound) {
		WindowMap.erase(Name);
		return 0;
	}
    if (Info.pWnd) {
        return Info.pWnd;
    } else {
		if (Info.ppWnd) {
            return *Info.ppWnd;
        } else {
            WindowMap.erase(Name);
            WindowList.erase(Info.pWnd);
            return 0;
        }
    }
    return 0;
}

bool SendWndClick2(CXWnd *pWnd, PCHAR ClickNotification)
{
    if (!pWnd)
        return false;
    for (unsigned long i = 0 ; i < 8 ; i++)
    {
        if (!_stricmp(szClickNotification[i],ClickNotification))
        {
            DebugTry(CXRect rect= pWnd->GetScreenRect());
            DebugTry(CXPoint pt=rect.CenterPoint());
            switch(i)
            {
            case 0:
                DebugTry(pWnd->HandleLButtonDown(&pt,0));
                break;
            case 1:
                DebugTry(pWnd->HandleLButtonDown(&pt,0));
                DebugTry(pWnd->HandleLButtonUp(&pt,0));
                break;
            case 2:
                DebugTry(pWnd->HandleLButtonDown(&pt,0));
                DebugTry(pWnd->HandleLButtonHeld(&pt,0));
                break;
            case 3:
                DebugTry(pWnd->HandleLButtonDown(&pt,0));
                DebugTry(pWnd->HandleLButtonHeld(&pt,0));
                DebugTry(pWnd->HandleLButtonUpAfterHeld(&pt,0));
                break;
            case 4:
                DebugTry(pWnd->HandleRButtonDown(&pt,0));
                break;
            case 5:
                DebugTry(pWnd->HandleRButtonDown(&pt,0));
                DebugTry(pWnd->HandleRButtonUp(&pt,0));
                break;
            case 6:
                DebugTry(pWnd->HandleRButtonDown(&pt,0));
                DebugTry(pWnd->HandleRButtonHeld(&pt,0));
                break;
            case 7:
                DebugTry(pWnd->HandleRButtonDown(&pt,0));
                DebugTry(pWnd->HandleRButtonHeld(&pt,0));
                DebugTry(pWnd->HandleRButtonUpAfterHeld(&pt,0));
                break;
            default:
                return false;
            };
            WeDidStuff();
            return true;
        }
    }
    return false;
}
int WinCount = 0;
class CXWnd *GetChildByIndex(class CXWnd *pWnd, PCHAR Name,int index)
{
    CHAR Buffer[MAX_STRING]={0};
    class CXWnd *tmp;

    if (!pWnd) return pWnd;
    if (CXMLData *pXMLData=pWnd->GetXMLData()) {
        if (GetCXStr(pXMLData->Name.Ptr,Buffer,MAX_STRING) && !_stricmp(Buffer,Name)) {
            WinCount++;
        } else if (GetCXStr(pXMLData->ScreenID.Ptr,Buffer,MAX_STRING) && !_stricmp(Buffer,Name)) {
            WinCount++;
        }
    }
	if(WinCount==index)
		return pWnd;
    if (pWnd->pFirstChildWnd) {
        tmp = GetChildByIndex((class CXWnd *)pWnd->pFirstChildWnd, Name,index);
        if (tmp)
            return tmp;
    }
    return GetChildByIndex((class CXWnd *)pWnd->pNextSiblingWnd, Name,index);
}

bool SendWndClick(PCHAR WindowName, PCHAR ScreenID, PCHAR ClickNotification)
{
    CXWnd *pWnd=FindMQ2Window(WindowName);
    if (!pWnd)
    {
        MacroError("Window '%s' not available.",WindowName);
        return false;
    }
    if (ScreenID && ScreenID[0] && ScreenID[0]!='0')
    {
		CXWnd *pButton=0;
		if(!_stricmp(WindowName,"bartersearchwnd") && !_stricmp(ScreenID,"sellbutton")) {
			if(CXWnd*pList=((CSidlScreenWnd*)(pWnd))->GetChildItem("BuyLineList")) {
				int selection = ((CListWnd*)pList)->GetCurSel();
				if(selection==-1) {
					MacroError("Please select a Listitem in '%s' before issuing a '%s' Click",WindowName,ScreenID);
					return false;
				}
				int buttonindex = ((CListWnd*)pList)->GetItemData(selection);
				WinCount=0;
				pButton=GetChildByIndex(pWnd,ScreenID,buttonindex+1);
			}
		} else if(!_stricmp(WindowName,"bazaarsearchwnd") && !_stricmp(ScreenID,"BZR_BuyButton")) {
			if(CXWnd*pList=((CSidlScreenWnd*)(pWnd))->GetChildItem("BZR_ItemList")) {
				int selection = ((CListWnd*)pList)->GetCurSel();
				if(selection==-1) {
					MacroError("Please select a Listitem in '%s' before issuing a '%s' Click",WindowName,ScreenID);
					return false;
				}
				int buttonindex = ((CListWnd*)pList)->GetItemData(selection);
				WinCount=0;
				pButton=GetChildByIndex(pWnd,ScreenID,buttonindex+1);
			}
		} else {
			pButton=((CSidlScreenWnd*)(pWnd))->GetChildItem(ScreenID);
		}
        if (!pButton)
        {
            MacroError("Window '%s' child '%s' not found.",WindowName,ScreenID);
            return false;
        }
		pWnd=pButton;
    }

    for (unsigned long i = 0 ; i < 8 ; i++)
    {
        if (!_stricmp(szClickNotification[i],ClickNotification))
        {
            CXRect rect= pWnd->GetScreenRect();
            CXPoint pt=rect.CenterPoint();
            switch(i)
            {
            case 0:
                pWnd->HandleLButtonDown(&pt,0);
                break;
            case 1:
                pWnd->HandleLButtonDown(&pt,0);
                pWnd->HandleLButtonUp(&pt,0);
                break;
            case 2:
                pWnd->HandleLButtonHeld(&pt,0);
                break;
            case 3:
                pWnd->HandleLButtonDown(&pt,0);
                pWnd->HandleLButtonHeld(&pt,0);
                pWnd->HandleLButtonUpAfterHeld(&pt,0);
                break;
            case 4:
                pWnd->HandleRButtonDown(&pt,0);
                break;
            case 5:
                pWnd->HandleRButtonDown(&pt,0);
                pWnd->HandleRButtonUp(&pt,0);
                break;
            case 6:
                pWnd->HandleRButtonDown(&pt,0);
                pWnd->HandleRButtonHeld(&pt,0);
                break;
            case 7:
                pWnd->HandleRButtonDown(&pt,0);
                pWnd->HandleRButtonHeld(&pt,0);
                pWnd->HandleRButtonUpAfterHeld(&pt,0);
                break;
            default:
                return false;
            };
            WeDidStuff();
            return true;
        }
    }
    return false;
}

bool SendListSelect(PCHAR WindowName, PCHAR ScreenID, DWORD Value)
{
    CXWnd *pWnd=FindMQ2Window(WindowName);
    CXWnd *pParentWnd = 0;
    if (!pWnd)
    {
        MacroError("Window '%s' not available.",WindowName);
        return false;
    }
    if (ScreenID && ScreenID[0] && ScreenID[0]!='0')
    {
        CXWnd *pList=pWnd->GetChildItem(ScreenID);
        if (!pList)
        {
            MacroError("Window '%s' child '%s' not found.",WindowName,ScreenID);
            return false;
        }
        if (pList->GetType()==UI_Listbox)
        {
			((CListWnd*)pList)->SetCurSel(Value);
			int index = ((CListWnd*)pList)->GetCurSel();		
			((CListWnd*)pList)->EnsureVisible(index);
			CXRect rect = ((CListWnd*)pList)->GetItemRect(index,0);
            CXPoint pt = rect.CenterPoint();
			pList->HandleLButtonDown(&pt,0);
			pList->HandleLButtonUp(&pt,0);
            WeDidStuff();
        }
        else if (pList->GetType()==UI_Combobox)
        {
            CXRect comborect=pList->GetScreenRect(); 
			CXPoint combopt=comborect.CenterPoint(); 
			((CComboWnd*)pList)->SetChoice(Value);
			((CXWnd*)pList)->HandleLButtonDown(&combopt,0);
			CListWnd*pListWnd = ((CComboWnd*)pList)->pListWnd;
			int index = pListWnd->GetCurSel();
			CXRect listrect=pListWnd->GetItemRect(index,0);
			CXPoint listpt=listrect.CenterPoint();
			((CXWnd*)pListWnd)->HandleLButtonDown(&listpt,0);
			((CXWnd*)pListWnd)->HandleLButtonUp(&listpt,0);            
            WeDidStuff();
        }
        else
        {
            MacroError("Window '%s' child '%s' cannot accept this notification.",WindowName,ScreenID);
            return false;
        }
        return true;
    }
    return false;
}
bool SendListSelect2(CXWnd *pList, LONG ListIndex)
{
	if (!pList)	{
		MacroError("Window %x not available.", pList);
		return false;
	}
	if (pList->GetType() == UI_Listbox) {
		if ((LONG)((CListWnd*)pList)->ItemsArray.Count >= ListIndex) {
			((CListWnd*)pList)->SetCurSel(ListIndex);
			int index = ((CListWnd*)pList)->GetCurSel();
			((CListWnd*)pList)->EnsureVisible(index);
			CXRect rect = ((CListWnd*)pList)->GetItemRect(index, 0);
			CXPoint pt = rect.CenterPoint();
			pList->HandleLButtonDown(&pt, 0);
			pList->HandleLButtonUp(&pt, 0);
			WeDidStuff();
			return true;
		} else {
			MacroError("Index Out of Bounds in SendListSelect2");
			return false;
		}
	} else if (pList->GetType() == UI_Combobox)	{
		if (CListWnd*pListWnd = ((CComboWnd*)pList)->pListWnd) {
			if (pListWnd->ItemsArray.Count >= ListIndex) {
				CXRect comborect = pList->GetScreenRect();
				CXPoint combopt = comborect.CenterPoint();
				((CComboWnd*)pList)->SetChoice(ListIndex);
				((CXWnd*)pList)->HandleLButtonDown(&combopt, 0);
				int index = pListWnd->GetCurSel();
				CXRect listrect = pListWnd->GetItemRect(index, 0);
				CXPoint listpt = listrect.CenterPoint();
				((CXWnd*)pListWnd)->HandleLButtonDown(&listpt, 0);
				((CXWnd*)pListWnd)->HandleLButtonUp(&listpt, 0);
				WeDidStuff();
				return true;
			} else {
				MacroError("Index Out of Bounds in SendListSelect2");
				return false;
			}
		} else {
			MacroError("Invalid combobox in SendListSelect2");
			return false;
		}
	} else {
		MacroError("Window was neiter a UI_Listbox nor a UI_Combobox");
		return false;
	}
	return false;
}
bool SendComboSelect(PCHAR WindowName, PCHAR ScreenID, DWORD Value)
{
    CXWnd *pWnd=FindMQ2Window(WindowName);
    CXWnd *pParentWnd = 0;
    if (!pWnd)
    {
        MacroError("Window '%s' not available.",WindowName);
        return false;
    }
    if (ScreenID && ScreenID[0] && ScreenID[0]!='0')
    {
		CComboWnd *pCombo=(CComboWnd*)((CSidlScreenWnd*)(pWnd))->GetChildItem(ScreenID);
        if (!pCombo)
        {
            MacroError("Window '%s' child '%s' not found.",WindowName,ScreenID);
            return false;
        }
        if (((CXWnd*)pCombo)->GetType()==UI_Combobox)
        {
			CXRect comborect=((CXWnd*)pCombo)->GetScreenRect(); 
			CXPoint combopt=comborect.CenterPoint(); 
			pCombo->SetChoice(Value);
			((CXWnd*)pCombo)->HandleLButtonDown(&combopt,0);
			CListWnd*pListWnd = pCombo->pListWnd;
			int index = pListWnd->GetCurSel();
			CXRect listrect=pListWnd->GetItemRect(index,0);
			CXPoint listpt=listrect.CenterPoint();
			((CXWnd*)pListWnd)->HandleLButtonDown(&listpt,0);
			((CXWnd*)pListWnd)->HandleLButtonUp(&listpt,0);
            WeDidStuff();
        }
        else
        {
            MacroError("Window '%s' child '%s' cannot accept this notification.",WindowName,ScreenID);
            return false;
        }
        return true;
    }
    return false;
}

bool SendTabSelect(PCHAR WindowName, PCHAR ScreenID, DWORD Value)
{
    CXWnd *pWnd=FindMQ2Window(WindowName);
    if (!pWnd)
    {
        MacroError("Window '%s' not available.",WindowName);
        return false;
    }
    if (ScreenID && ScreenID[0] && ScreenID[0]!='0')
    {
        CTabWnd *pTab = (CTabWnd*)((CSidlScreenWnd*)(pWnd))->GetChildItem(ScreenID);
        if (!pTab)
        {
            MacroError("Window '%s' child '%s' not found.",WindowName,ScreenID);
            return false;
        }
		int uitype = ((CXWnd*)pTab)->GetType();
        if (uitype==UI_TabBox)
        {
            pTab->SetPage(Value,true);
            WeDidStuff();
        }
        else
        {
            MacroError("Window '%s' child '%s' cannot accept this notification.",WindowName,ScreenID);
            return false;
        }
        return true;
    }
    return false;
}
bool SendWndNotification(PCHAR WindowName, PCHAR ScreenID, DWORD Notification, VOID *Data)
{
    CHAR szOut[MAX_STRING] = {0};
    CXWnd *pWnd=FindMQ2Window(WindowName);
    if (!pWnd)
    {
        sprintf_s(szOut,"Window '%s' not available.",WindowName);
        WriteChatColor(szOut,USERCOLOR_DEFAULT);
        return false;
    }
    CXWnd *pButton=0;
    if (ScreenID && ScreenID[0])
    {
        pButton=((CSidlScreenWnd*)(pWnd))->GetChildItem(ScreenID);
        if (!pButton)
        {
            sprintf_s(szOut,"Window '%s' child '%s' not found.",WindowName,ScreenID);
            WriteChatColor(szOut,USERCOLOR_DEFAULT);
            return false;
        }
    }
	if(Notification==XWM_NEWVALUE) {
		((CSliderWnd*)(pButton))->SetValue((int)Data);
	}
    ((CXWnd*)(pWnd))->WndNotification(pButton,Notification,Data);
    WeDidStuff();
    return true;
}

void AddWindow(char *WindowName, CXWnd **ppWindow)
{
    string Name=WindowName;
    MakeLower(Name);

	if(WindowMap.find(Name)!=WindowMap.end()) {
		_WindowInfo pWnd;
		for (std::map<CXWnd*, _WindowInfo>::iterator N = WindowList.begin(); N != WindowList.end(); N++) {
			if (N->second.Name == Name) {
				pWnd = N->second;
				break;
			}
		}
        pWnd.pWnd=0;
        pWnd.ppWnd=ppWindow;
    } else {
        _WindowInfo pWnd;
		pWnd.Name = Name;
        pWnd.pWnd=0;
        pWnd.ppWnd=ppWindow;
        WindowList[*ppWindow]=pWnd;
		WindowMap[Name] = *ppWindow;
    }
}

void RemoveWindow(char *WindowName)
{
    std::string Name=WindowName;
    MakeLower(Name);
	if(WindowMap.find(Name)!=WindowMap.end()) {
        WindowMap.erase(Name);
        for (std::map<CXWnd*, _WindowInfo>::iterator N = WindowList.begin(); N != WindowList.end(); N++) {
			if (N->second.Name == Name) {
				WindowList.erase(N);
				break;
			}
        }
    }
}
#endif

CHAR tmpName[MAX_STRING]={0};
CHAR tmpAltName[MAX_STRING]={0};
CHAR tmpType[MAX_STRING]={0};

int RecurseAndListWindows(PCSIDLWND pWnd)
{
    int Count = 0;

    if (CXMLData *pXMLData=((CXWnd*)pWnd)->GetXMLData()) {
        Count++;
        GetCXStr(pXMLData->TypeName.Ptr,tmpType,MAX_STRING);
        GetCXStr(pXMLData->Name.Ptr,tmpName,MAX_STRING);
        GetCXStr(pXMLData->ScreenID.Ptr,tmpAltName,MAX_STRING);
        if (tmpAltName[0] && _stricmp(tmpName,tmpAltName)) {
			if(pWnd->pParentWindow && pWnd->pParentWindow->pParentWindow)
				WriteChatf("[0x%08X][P:0x%08X][PP:0x%08X] [\ay%s\ax] [\at%s\ax] [Custom UI-specific: \at%s\ax]",pWnd,pWnd->pParentWindow,pWnd->pParentWindow->pParentWindow,tmpType,tmpName,tmpAltName);
			else
				WriteChatf("[0x%08X][P:0x%08X] [\ay%s\ax] [\at%s\ax] [Custom UI-specific: \at%s\ax]",pWnd,pWnd->pParentWindow,tmpType,tmpName,tmpAltName);
		} else {
			WriteChatf("[0x%08X][P:0x%08X] [\ay%s\ax] [\at%s\ax]",pWnd,pWnd->pParentWindow,tmpType,tmpName);
		}
    }
    if (pWnd->pFirstChildWnd)
        Count += RecurseAndListWindows(pWnd->pFirstChildWnd);

    if (pWnd->pNextSiblingWnd)
        Count += RecurseAndListWindows(pWnd->pNextSiblingWnd);

    return Count;
}

#ifndef ISXEQ
VOID ListWindows(PSPAWNINFO pChar, PCHAR szLine)
{
    char szArg1[MAX_STRING] = {0};
    char szArg2[MAX_STRING] = {0};
    char szArg3[MAX_STRING] = {0};
    GetArg(szArg1,szLine,1);
    GetArg(szArg2,szLine,2);
    GetArg(szArg3,szLine,3);
 
#else
int ListWindows(int argc, char *argv[])
{
    char szArg1[MAX_STRING] = {0};
    char szArg2[MAX_STRING] = {0};
    char szArg3[MAX_STRING] = {0};
    PCHAR szLine = NULL;
    if (argc > 0)
        szLine = argv[1];
    if (argc > 1)
		strcpy_s(szArg1,argv[1]);
    if (argc > 2)
		strcpy_s(szArg2,argv[2]);
    if (argc > 3)
		strcpy_s(szArg3,argv[3]);
#endif
	BOOL bOpen = 0;
	BOOL bPartial = 0;
	if(!_stricmp(szArg1,"open")) {
		bOpen = 1;
		szLine[0] = '\0';
		szLine = 0;
		if(!_stricmp(szArg2,"partial")) {
			bPartial = 1;
			strcpy_s(szArg2,szArg3);
		}
	} else if(!_stricmp(szArg1,"partial")) {
		bPartial = 1;
		szLine[0] = '\0';
		szLine = 0;
	}
    unsigned long Count=0;
    if (!szLine || !szLine[0])
    {
        if(bOpen)
			WriteChatColor("List of available OPEN windows");
		else
			WriteChatColor("List of available windows");
        WriteChatColor("-------------------------");
		for (std::map<CXWnd*, _WindowInfo>::iterator N = WindowList.begin(); N != WindowList.end(); N++) {
			_WindowInfo Info = N->second;
			if (bOpen) {
				if (Info.pWnd && Info.pWnd->dShow == 1 && Info.pWnd->pParentWindow == 0) {
					if (bPartial) {
						if (Info.Name.find(szArg2) != Info.Name.npos) {
							WriteChatf("[PARTIAL MATCH][OPEN] %s", Info.Name.c_str());
							RecurseAndListWindows((PCSIDLWND)Info.pWnd);
								Count++;
							}
					}
					else {
						WriteChatf("[OPEN] %s", Info.Name.c_str());
							Count++;
						}
					}
			}
			else {
				if (bPartial) {
					if (Info.Name.find(szArg2) != Info.Name.npos) {
						WriteChatf("[PARTIAL MATCH] %s", Info.Name.c_str());
						RecurseAndListWindows((PCSIDLWND)Info.pWnd);
							Count++;
						}
				}
				else {
					WriteChatf("%s", Info.Name.c_str());
						Count++;
					}
				}
            }
		WriteChatf("%d window(s) found with %s in the name", Count,szArg2);
    }
    else
    {
        // list children of..
        std::string WindowName=szLine;
        MakeLower(WindowName);
		if (WindowMap.find(WindowName) == WindowMap.end()) {
			if (CXWnd *pWnd = FindMQ2Window(szLine)) {
                Count = RecurseAndListWindows((PCSIDLWND)pWnd);
				WriteChatf("%d child windows", Count);
				RETURN(0);
			}
			WriteChatf("Window '%s' not available", WindowName.c_str());
			RETURN(0);
 		}
        WriteChatf("Listing child windows of '%s'",WindowName.c_str());
        WriteChatColor("-------------------------");
		for (std::map<CXWnd*, _WindowInfo>::iterator N = WindowList.begin(); N != WindowList.end(); N++) {
			_WindowInfo Info = N->second;
			if (Info.Name == WindowName && Info.pWnd) {
				if (PCSIDLWND pWnd = Info.pWnd->pFirstChildWnd) {
					Count = RecurseAndListWindows(pWnd);
				}
				WriteChatf("%d child windows", Count);
			}
        }
    }
    RETURN(0);
}

PCHAR szWndNotification[] = { 
    0,            //0 
    "leftmouse",    //1
    "leftmouseup",    //2
    "rightmouse",        //3
    0,        //4
    0,        //5
    "enter",        //6
    0,        //7
    0,        //8
    "help",        //9
    "close",        //10
    0,        //11
    0,        //12
    0,        //13
    "newvalue",        //14
    0,        //15
    0,        //16
    0,        //17
    0,        //18
    0,        //19
    "contextmenu",        //20
    "mouseover",    //21
    "history",        //22
    "leftmousehold",    //23
    0,        //24
    0,        //25
    0,        //26
    "link",        //27
    0,        //28
    "resetdefaultposition",        //29
}; 

#ifndef ISXEQ
VOID WndNotify(PSPAWNINFO pChar, PCHAR szLine)
{
#else
int WndNotify(int argc, char *argv[])
{
    PSPAWNINFO pChar = (PSPAWNINFO)pLocalPlayer;
#endif
    unsigned long Data=0;
#ifndef ISXEQ 
    CHAR szArg1[MAX_STRING] = {0}; 
    CHAR szArg2[MAX_STRING] = {0}; 
    CHAR szArg3[MAX_STRING] = {0}; 
    CHAR szArg4[MAX_STRING] = {0}; 

    GetArg(szArg1, szLine, 1);
    GetArg(szArg2, szLine, 2);
    GetArg(szArg3, szLine, 3);
    GetArg(szArg4, szLine, 4);

    if (!szArg3[0] && !IsNumber(szArg1))
    {
        SyntaxError("Syntax: /notify <window|\"item\"> <control|0> <notification> [notification data]");
        return;
    }
    if (szArg4[0])
        Data=atoi(szArg4);
#else
    if (argc<3 && (argc>1 && !IsNumber(argv[2])))
    {
        printf("%s syntax: %s <window|\"item\"> <control|0> <notification> [notification data]",argv[0],argv[0]);
        RETURN(0);
    }
    if (argc>4)
        Data=atoi(argv[4]);
    CHAR *szArg1=argv[1];
    CHAR *szArg2=argv[2];
    CHAR *szArg3=argv[3];
    CHAR *szArg4=argv[4];
#endif 

    if (!_stricmp(szArg3,"link")) {
        DebugSpewAlways("WndNotify: link found, Data = 1");
        Data = 1;
    }
	if (IsNumber(szArg1)) {
		//ok we have a number it means the user want us to click a window he has found the address for...
		DWORD addr = atoi(szArg1);
		if (!_stricmp(szArg2, "listselect")) {
			SendListSelect2((CXWnd*)addr, atoi(szArg3));
			RETURN(0);
		}
		SendWndClick2((CXWnd*)addr,szArg2);
		RETURN(0);
	}
    if (!_stricmp(szArg3,"listselect")) {
        SendListSelect(szArg1,szArg2,Data-1);
        RETURN(0);
    } else if (!_stricmp(szArg3,"comboselect")) {
        SendComboSelect(szArg1,szArg2,Data-1);
        RETURN(0);
    } else if (!_stricmp(szArg3,"tabselect")) {
        SendTabSelect(szArg1,szArg2, Data-1);
        RETURN(0);
    } 
    if (Data==0 && SendWndClick(szArg1,szArg2,szArg3))
        RETURN(0);
    for (unsigned long i = 0 ; i < sizeof(szWndNotification)/sizeof(szWndNotification[0]) ; i++)
    {
        if (szWndNotification[i] && !_stricmp(szWndNotification[i],szArg3))
        {
            if (i==XWM_LINK) {
                if (!SendWndNotification(szArg1,szArg2,i,(void*)szArg4))
                {
                    MacroError("Could not send notification to %s %s",szArg1,szArg2);
                }
                RETURN(0);
            }
            if (szArg2[0]=='0')
            {
                if (!SendWndNotification(szArg1,0,i,(void*)Data))
                {
                    MacroError("Could not send notification to %s %s",szArg1,szArg2);
                }
            }
            else if (!SendWndNotification(szArg1,szArg2,i,(void*)Data))
            {
                MacroError("Could not send notification to %s %s",szArg1,szArg2);
            }
            RETURN(0);
        }
    }
    MacroError("Invalid notification '%s'",szArg3);
    RETURN(0);
}
bool IsCtrlKey()
{
	return (pWndMgr->GetKeyboardFlags() & 0x00000002) != 0;
}

// item slots:
// 2000-2015 bank window
// 2500-2503 shared bank
// 5000-5031 loot window
// 3000-3015 trade window (including npc) 3000-3007 are your slots, 3008-3015 are other character's slots
// 4000-4010 world container window
// 6000-6080 merchant window
// 7000-7080 bazaar window
// 8000-8031 inspect window
bool CheckLootArg(char*arg, char *search, int argcnt, int *slot)
{
	if (!_strnicmp(arg, search, argcnt)) {
		char *numptr = arg + argcnt;
		int theslot = -1;
		if (IsNumber(numptr)) {
			theslot = atoi(numptr) - 1;
			if (theslot < 0)
				theslot = 0;
			*slot = theslot;
			return true;
		}
	}
	return false;
}

#ifndef ISXEQ
VOID ItemNotify(PSPAWNINFO pChar, PCHAR szLine)
{
    CHAR szArg1[MAX_STRING] = {0}; 
    CHAR szArg2[MAX_STRING] = {0}; 
    CHAR szArg3[MAX_STRING] = {0};
    CHAR szArg4[MAX_STRING] = {0}; 

    GetArg(szArg1, szLine, 1);
    GetArg(szArg2, szLine, 2);
    GetArg(szArg3, szLine, 3);
    GetArg(szArg4, szLine, 4); 
    if (!szArg2[0])
    {
        WriteChatColor("Syntax: /itemnotify <slot|#> <notification>");
        WriteChatColor("     or /itemnotify in <bag slot> <slot # in bag> <notification>");
		WriteChatColor("     or /itemnotify <itemname> <notification>");
        RETURN(0);
    }

#else
int ItemNotify(int argc, char *argv[])
{
    if (argc!=3 && argc != 5)
    {
        //WriteChatf("ItemNotify got %d args", argc);
        WriteChatColor("Syntax: /itemnotify <slot|#> <notification>");
        WriteChatColor("     or /itemnotify in <bag slot> <slot # in bag> <notification>");
        RETURN(0);
    }
    char *szArg1tmp=argv[1];
    char *szArg2=argv[2];
    char *szArg3="";
    char *szArg4="";
	CHAR szArg1[2048] = { 0 };
	strcpy_s(szArg1, szArg1tmp);
    if (argc==5)
    {
        szArg3=argv[3];
        szArg4=argv[4];
    }
    PSPAWNINFO pChar = (PSPAWNINFO)pLocalPlayer;
#endif
    PCHAR pNotification=&szArg2[0];
    EQINVSLOT *pSlot=NULL;
    DWORD i;
    PEQINVSLOTMGR pInvMgr=(PEQINVSLOTMGR)pInvSlotMgr;
    short bagslot = -1;
    short invslot = -1;
    ItemContainerInstance type = eItemContainerInvalid;

    if (!_stricmp(szArg1,"in"))
    { 
        if (!szArg4[0])
        {
            WriteChatColor("Syntax: /itemnotify in <bag slot> <slot # in bag> <notification>");
            RETURN(0);
        }
        if (!_strnicmp(szArg2,"bank",4)) {
            invslot=atoi(&szArg2[4])-1;
            bagslot=atoi(szArg3)-1;
            type=eItemContainerBank;
        } else if (!_strnicmp(szArg2,"sharedbank",10)) {
            invslot=atoi(&szArg2[10])-1;
            bagslot=atoi(szArg3)-1;
            type=eItemContainerSharedBank;
        } else if (!_strnicmp(szArg2,"pack",4)) {
            invslot=atoi(&szArg2[4])-1+BAG_SLOT_START;
            bagslot=atoi(szArg3)-1;
            type=eItemContainerPossessions;
        }
		//ok look, I wish I could just call:
		//pSlot = (PEQINVSLOT)pInvSlotMgr->FindInvSlot(invslot,bagslot);
		//BUT it returns HB_InvSlot as well as containers AND it doesn't take "type" into account...
		//which is why I use GetInvSlot instead...
		pSlot = GetInvSlot(type,invslot,bagslot);
        pNotification=&szArg4[0];
		if(!pSlot && type!=-1) {//ok pSlot was not found (so bag is closed) BUT we can "click" it anyway with moveitem so lets just do that if pNotification is leftmoseup
			if(invslot<0 || invslot>NUM_INV_SLOTS) {
				WriteChatf("%d is not a valid invslot. (itemnotify)",invslot);
				RETURN(0);
			}
			if(pNotification && !_strnicmp(pNotification,"leftmouseup",11)) {
				PCONTENTS pContainer = FindItemBySlot(invslot);	// we dont care about the bagslot here
				if(!pContainer) {								// and we dont care if the user has something
																// on cursor either, cause we know they
																// specified "in" so a container MUST exist... -eqmule
					WriteChatf("There was no container in slot %d",invslot);
					RETURN(0);
				}
				if(bagslot<0 && bagslot >= (int)pContainer->Contents.ContentSize) {
					WriteChatf("%d is not a valid slot for this container.",bagslot);
					RETURN(0);
				}
				if(GetItemFromContents(pContainer)->Type!=ITEMTYPE_PACK) {
					WriteChatf("There was no container in slot %d",invslot);
					RETURN(0);
				}
				if (ItemOnCursor()) {
					DropItem(type,invslot, bagslot);
				} else {
					PickupItem(type, FindItemBySlot(invslot, bagslot));
				}
				RETURN(0);
			} else if(pNotification && !_strnicmp(pNotification,"rightmouseup",12)) {//we fake it with /useitem
				if ( HasExpansion(EXPANSION_VoA) )
				{
					PCONTENTS pItem = FindItemBySlot(invslot,bagslot);
					if(pItem) {
						if (GetItemFromContents(pItem)->Clicky.SpellID > 0 && GetItemFromContents(pItem)->Clicky.SpellID!=-1) {
							CHAR cmd[MAX_STRING] = {0};
							sprintf_s(cmd, "/useitem \"%s\"", GetItemFromContents(pItem)->Name);
							EzCommand(cmd);
							RETURN(0);
						}
					} else {//it doesnt matter if its a bag, since the user specified "in"
							//we cant open bags inside bags so lets just return...
						WriteChatf("Item '%s' not found.",szArg2);
						RETURN(0);
					}
				}
			}
		}
    } else {//user didnt specify "in" so it should be outside a container
			//OR it's an item, either way we can "click" it -eqmule
        unsigned long Slot=atoi(szArg1);
        if (Slot==0)
		{
			_strlwr_s(szArg1);
            Slot=ItemSlotMap[szArg1];
            if (Slot<NUM_INV_SLOTS && pInvSlotMgr) {
                DebugTry(pSlot=(EQINVSLOT *)pInvSlotMgr->FindInvSlot(Slot));
            } else {
                if (!_strnicmp(szArg1, "loot", 4)) {
                    invslot = atoi(szArg1+4) - 1;
					type = eItemContainerCorpse;
                } else if (!_strnicmp(szArg1, "enviro", 6)) {
                    invslot = atoi(szArg1+6) - 1;
                    type = eItemContainerWorld;
                } else if (!_strnicmp(szArg1, "pack", 4)) {
                    invslot = atoi(szArg1+4) - 1 + BAG_SLOT_START;
                    type = eItemContainerPossessions;
                } else if (!_strnicmp(szArg1, "bank", 4)) {
                    invslot = atoi(szArg1+4) - 1;
                    type = eItemContainerBank;
                } else if (!_strnicmp(szArg1, "sharedbank", 10)) {
                    invslot = atoi(szArg1+10) - 1;
                    type = eItemContainerSharedBank;
                } else if (!_strnicmp(szArg1, "trade", 5)) {
                    invslot = atoi(szArg1+5) - 1;
                    type = eItemContainerTrade;
                }
				CHAR szType[MAX_STRING] = {0};
                for (i=0;i<pInvMgr->TotalSlots;i++) {
                    pSlot = pInvMgr->SlotArray[i];
                    if (pSlot && pSlot->Valid && pSlot->pInvSlotWnd && pSlot->pInvSlotWnd->WindowType == type && (short)pSlot->pInvSlotWnd->InvSlot == invslot) {
						CXMLData *pXMLData=((CXWnd*)pSlot->pInvSlotWnd)->GetXMLData();
						if(pXMLData) {		
							GetCXStr(pXMLData->ScreenID.Ptr,szType,MAX_STRING);
							if(!_stricmp(szType,"HB_InvSlot")) {
								continue;
							}
						}
						Slot = 1;
                        break;
                    }
                }
                if (i == pInvMgr->TotalSlots)
					Slot = 0;
            }
        }
        if (Slot==0 && szArg1[0]!='0' && _stricmp(szArg1,"charm"))
        {
            //could it be an itemname?
			//lets check:
			if(PCONTENTS ptheitem = FindItemByName(szArg1,1)) {
				if(pNotification && !_strnicmp(pNotification,"leftmouseup",11)) {
					if (ItemOnCursor()) {
						DropItem(eItemContainerPossessions, bagslot, invslot);
					} else {
						PickupItem(eItemContainerPossessions, ptheitem);
					}
				} else if(pNotification && !_strnicmp(pNotification,"rightmouseup",12)) {//we fake it with /useitem
					//hmm better check if its a spell cause then it means we should mem it
					PITEMINFO pClicky = GetItemFromContents(ptheitem);
					if (pClicky && pClicky->ItemType == ITEMITEMTYPE_SCROLL) {
						if (IsItemInsideContainer(ptheitem)) {
							OpenContainer(ptheitem, true);
						}
						if (pInvSlotMgr) {
							pSlot = (PEQINVSLOT)pInvSlotMgr->FindInvSlot(ptheitem->GlobalIndex.Index.Slot1, ptheitem->GlobalIndex.Index.Slot2);
						}
						if (!pSlot || !pSlot->pInvSlotWnd || !SendWndClick2((CXWnd*)pSlot->pInvSlotWnd, pNotification)) {
							WriteChatf("Could not mem spell, most likely cause bag wasnt open and i didnt find it");
						}
						RETURN(0);
					} else if (pClicky && pClicky->Clicky.SpellID!=-1)	{
						CHAR cmd[512] = {0};
						sprintf_s(cmd, "/useitem \"%s\"", GetItemFromContents(ptheitem)->Name);
						EzCommand(cmd);
						RETURN(0);
					} else if(pClicky->Type == ITEMTYPE_PACK) {
						//ok its a pack, so just open it
						if(ptheitem->Open) {
							CloseContainer(ptheitem);
						} else {
							OpenContainer(ptheitem,false);
						}
					}
				}
				RETURN(0);
			}
			WriteChatf("[/itemnotify] Invalid item slot '%s'",szArg1);
            RETURN(0);
        } else if (Slot && !pSlot) {
            pSlot = pInvMgr->SlotArray[Slot];
        }
    }
    if (!pSlot)
    {
        WriteChatf("SLOT IS NULL: Could not send notification to %s %s",szArg1,szArg2);
        RETURN(0);
    }
    DebugSpew("ItemNotify: Calling SendWndClick");
    if (!pSlot->pInvSlotWnd || !SendWndClick2((CXWnd*)pSlot->pInvSlotWnd,pNotification))
    {
        WriteChatf("Could not send notification to %s %s",szArg1,szArg2);
    }
    RETURN(0);
}

#ifndef ISXEQ
VOID ListItemSlots(PSPAWNINFO pChar, PCHAR szLine)
#else
int ListItemSlots(int argc, char *argv[])
#endif
{
    PEQINVSLOTMGR pMgr=(PEQINVSLOTMGR)pInvSlotMgr;
    if (!pMgr)
        RETURN(0);
    unsigned long Count=0;
    WriteChatColor("List of available item slots");
    WriteChatColor("-------------------------");
    for (unsigned long N = 0 ; N < 0x800 ; N++)
        if (PEQINVSLOT pSlot=pMgr->SlotArray[N])
        {
            if (pSlot->pInvSlotWnd)
            {
                WriteChatf("%d %d %d", N, pSlot->pInvSlotWnd->WindowType, pSlot->InvSlot);
                Count++;
            } else if (pSlot->InvSlot) {
                WriteChatf("%d %d", N, pSlot->InvSlot);
            }
        }
        WriteChatf("%d available item slots",Count);
        RETURN(0)
}
