#include <string.h>
#include <stdio.h>
#include "x2filterwheel.h"
#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/basicstringinterface.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/mutexinterface.h"
#include "../../licensedinterfaces/tickcountinterface.h"

inline static void OutputDbgPrint(const char* strOutPutString, ...)
{
#ifdef _DEBUG
	char strBuf[128] = {0};
	sprintf(strBuf, "<%s> ", "x2EFW");
	va_list vlArgs;
	va_start(vlArgs, strOutPutString);
	vsnprintf((char*)(strBuf+strlen(strBuf)), sizeof(strBuf)-strlen(strBuf), strOutPutString, vlArgs);
	va_end(vlArgs);

#ifdef _WINDOWS
	OutputDebugStringA(strBuf);
#elif defined _LIN
	printf("%s",strBuf);
#endif

#endif
}

X2FilterWheel::X2FilterWheel(const char* pszDriverSelection,
							 const int& nInstanceIndex,
							 SerXInterface					* pSerX, 
							 TheSkyXFacadeForDriversInterface	* pTheSkyX, 
							 SleeperInterface					* pSleeper,
							 BasicIniUtilInterface			* pIniUtil,
							 LoggerInterface					* pLogger,
							 MutexInterface					* pIOMutex,
							 TickCountInterface				* pTickCount)
{
	m_nPrivateMulitInstanceIndex	= nInstanceIndex;
	m_pSerX							= pSerX;		
	m_pTheSkyXForMounts				= pTheSkyX;
	m_pSleeper						= pSleeper;
	m_pIniUtil						= pIniUtil;
	m_pLogger						= pLogger;	
	m_pIOMutex						= pIOMutex;
	m_pTickCount					= pTickCount;
	m_bLinked = false;
	bThrGetSlotNumRun = false;
	Thr_getSlotNum = NULL;
	bUniDirSetting = bUniDirTemp = false;
	oldEFWID = -1;

}
X2FilterWheel::~X2FilterWheel()
{
	if (m_pSerX)
		delete m_pSerX;
	if (m_pTheSkyXForMounts)
		delete m_pTheSkyXForMounts;
	if (m_pSleeper)
		delete m_pSleeper;
	if (m_pIniUtil)
		delete m_pIniUtil;
	if (m_pLogger)
		delete m_pLogger;
	if (m_pIOMutex)
		delete m_pIOMutex;
	if (m_pTickCount)
		delete m_pTickCount;

}


int	X2FilterWheel::queryAbstraction(const char* pszName, void** ppVal)
{
	X2MutexLocker ml(GetMutex());

	*ppVal = NULL;

	if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
		*ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
	else
		if (!strcmp(pszName, X2GUIEventInterface_Name))
			*ppVal = dynamic_cast<X2GUIEventInterface*>(this);

	return SB_OK;
}

//LinkInterface
int	X2FilterWheel::establishLink(void)					
{
	X2MutexLocker ml(GetMutex());

	int iEFWNum = EFWGetNum();         
	if (iEFWNum > 0)
	{
		int pos;
		EFWInfo.ID = iSelectedEFWIDSetting;
		if (EFWOpen(EFWInfo.ID) == EFW_SUCCESS)
		{
			EFWSetDirection(EFWInfo.ID, bUniDirSetting);
		}
		else//此ID的相机已经被移除
		{
			EFWGetID(0, & EFWInfo.ID);
			if (EFWOpen(EFWInfo.ID) != EFW_SUCCESS)
				return ERR_COMMNOLINK;
		}
	}
	else
		return ERR_COMMNOLINK;
	
	m_bLinked = true;
//	EFWGetProperty(EFWInfo.ID, & EFWInfo);                  

	return SB_OK;
}
int	X2FilterWheel::terminateLink(void)						
{
	X2MutexLocker ml(GetMutex());

	EFWClose(EFWInfo.ID);
	m_bLinked = false;

	return SB_OK;
}
bool X2FilterWheel::isLinked(void) const					
{
	return m_bLinked;
}
bool X2FilterWheel::isEstablishLinkAbortable(void) const	{return false;}

//AbstractDriverInfo

#define DISPLAY_NAME "ZWO X2 Filter Wheel Plug In"
void	X2FilterWheel::driverInfoDetailedInfo(BasicStringInterface& str) const
{
	str = DISPLAY_NAME;
}
double	X2FilterWheel::driverInfoVersion(void) const				
{
	return 1.7;//20221104 - 对应SDK1.7
}

//AbstractDeviceInfo
void X2FilterWheel::deviceInfoNameShort(BasicStringInterface& str) const				
{
	str = DISPLAY_NAME;
}
void X2FilterWheel::deviceInfoNameLong(BasicStringInterface& str) const				
{
	str = DISPLAY_NAME;

}
void X2FilterWheel::deviceInfoDetailedDescription(BasicStringInterface& str) const	
{
	str = DISPLAY_NAME;

}
void X2FilterWheel::deviceInfoFirmwareVersion(BasicStringInterface& str)		
{
	str = DISPLAY_NAME;
}
char* get_EFW_SDK_Ver();
void X2FilterWheel::deviceInfoModel(BasicStringInterface& str)				
{
//	str = DISPLAY_NAME;
	char temp[64];
	sprintf(temp, "EFW SDK V");
	strcat(temp, get_EFW_SDK_Ver());
	str = temp;
}

//FilterWheelMoveToInterface
int	X2FilterWheel::filterCount(int& nCount)
{
	X2MutexLocker ml(GetMutex());
	EFWGetProperty(EFWInfo.ID, &EFWInfo);
	nCount = EFWInfo.slotNum;

	return SB_OK;
}
int	X2FilterWheel::defaultFilterName(const int& nIndex, BasicStringInterface& strFilterNameOut)
{
	X2MutexLocker ml(GetMutex());

	char buf[16] = {0};
	sprintf(buf, "%d", nIndex + 1);
	//strcpy(strFilterNameOut, buf);
	strFilterNameOut = buf;

	return SB_OK;
}
int	X2FilterWheel::startFilterWheelMoveTo(const int& nTargetPosition)
{
	X2MutexLocker ml(GetMutex());
	
	if(nTargetPosition >= 0 && nTargetPosition < EFWInfo.slotNum)
	{
		if(EFWSetPosition(EFWInfo.ID, nTargetPosition) != EFW_SUCCESS)
			return ERR_CMDFAILED;
	}
	else
		return ERR_LIMITSEXCEEDED;
	return SB_OK;
}
int	X2FilterWheel::isCompleteFilterWheelMoveTo(bool& bComplete) const
{
	X2FilterWheel* pMe = (X2FilterWheel*)this;
	X2MutexLocker ml(pMe->GetMutex());

	int pos;
	if(EFWGetPosition(EFWInfo.ID, &pos) != EFW_SUCCESS)
		return ERR_CMDFAILED;

	bComplete = (pos != -1);

	return SB_OK;
}
int	X2FilterWheel::endFilterWheelMoveTo(void)
{
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}
int	X2FilterWheel::abortFilterWheelMoveTo(void)
{
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}

//Please see the X2Camera example for more extensive use of different type of GUI controls
int X2FilterWheel::execModalSettingsDialog()
{
	int nErr = SB_OK;
	
	int EFWNum;
	if ((EFWNum = EFWGetNum()) > 0)
	{                
		if (isLinked())
		{
			int pos;
			if (EFWGetPosition(EFWInfo.ID, &pos) != EFW_SUCCESS)//此ID的已经被移除			
				return ERR_NOLINK;				
		}               

	}
	else	
		return ERR_NOLINK;	
	


	X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
	X2GUIInterface*					ui = uiutil.X2UI();
	X2GUIExchangeInterface*			dx = NULL;//Comes after ui is loaded
	bool bPressedOK = false;

	if (NULL == ui)
		return ERR_POINTER;

	if (nErr = ui->loadUserInterface("x2filterwheelEFW.ui", deviceType(), m_nPrivateMulitInstanceIndex))
		return nErr;

	if (NULL == (dx = uiutil.X2DX()))
		return ERR_POINTER;

	//Intialize the user interface
	pdx = dx;
	
	
	dx->invokeMethod("comboBox","clear");

	int itemIndex = 0, pos;
	EFW_INFO EFWInfoTemp;
	char szBuf[256];
	for (int i = 0; i < EFWNum; i++)
	{
		EFWGetID(i, &EFWInfoTemp.ID);
		EFWGetProperty(EFWInfoTemp.ID, &EFWInfoTemp);//没打开的不能得到孔数 名字和ID能得到		
		if (isLinked() && EFWInfoTemp.ID == EFWInfo.ID)		
			itemIndex = i;
		sprintf(szBuf, "%s (ID %d)", EFWInfoTemp.Name, EFWInfoTemp.ID);
		dx->comboBoxAppendString("comboBox", szBuf);
	}
	
	if(isLinked())//已经有连接
	{
		dx->setEnabled("comboBox", false);
	}	
	else//没有连接
	{ 
		dx->setEnabled("comboBox", true);	
	}
	dx->setCurrentIndex("comboBox", itemIndex);



	//Display the user interface
	if (nErr = ui->exec(bPressedOK))
	{
		if (!isLinked())
			terminateLink();
		return nErr;
	}


	WaitThrExit();

	//Retreive values from the user interface
	if (bPressedOK)
	{
		bUniDirSetting = bUniDirTemp;
		iSelectedEFWIDSetting = EFWInfo.ID;

		if(isLinked())
		{
			EFWSetDirection(EFWInfo.ID, bUniDirSetting);
		}
		
	//	int nModel;

		//Model
	//	nModel = dx->currentIndex("comboBox");
	}

	if (!isLinked() && oldEFWID > -1)
		EFWClose(oldEFWID);

	return nErr;
}
#ifdef _WINDOWS
static void GetSlotsNumFunc(void* dummy)
#elif defined _LIN
static void* GetSlotsNumFunc(void* dummy)
#endif
{
	X2FilterWheel* dlg =(X2FilterWheel*)dummy;
	int iID = dlg->EFWInfo.ID;
	EFW_ERROR_CODE err;
	while(dlg->bThrGetSlotNumRun)
	{
		err = EFWGetProperty(iID, &dlg->EFWInfo);
		if(err != EFW_ERROR_MOVING || err == EFW_ERROR_INVALID_ID)
			break;
		Sleep(500);
	}
	dlg->bThrGetSlotNumRun = false;
	if(dlg->EFWInfo.ID == iID)
		dlg->refreshUI();

#ifdef _WINDOWS
	_endthread();
#elif defined _LIN	
	return (void*)0;
#endif

}

void X2FilterWheel::WaitThrExit()
{
	if(bThrGetSlotNumRun)
	{
		OutputDbgPrint("wait Thr_getSlotNum exit >>");

		bThrGetSlotNumRun = false;
#ifdef _WINDOWS
		WaitForSingleObject(Thr_getSlotNum, 800);//最多等800ms
#else
		void * res;
		pthread_join(Thr_getSlotNum, &res);
#endif
		OutputDbgPrint("wait Thr_getSlotNum exit <<");
	}
}

void X2FilterWheel::refreshUI()
{
	EFW_ERROR_CODE err = EFWGetProperty(EFWInfo.ID, &EFWInfo);
	char buf[32] = {0};
	if(err == EFW_SUCCESS)
	{
		sprintf(buf, "%d slots", EFWInfo.slotNum);
		pdx->setText("label_1", buf);		
	}
	else if (err == EFW_ERROR_MOVING)
	{
		pdx->setText("label_1", "moving...");
		if (!bThrGetSlotNumRun)
		{
			bThrGetSlotNumRun = true;
#ifdef _WINDOWS
			Thr_getSlotNum = (HANDLE)_beginthread(GetSlotsNumFunc,  NULL, (void*)this);
#else
			if(pthread_create(&Thr_getSlotNum, 0, GetSlotsNumFunc, this)!=0)
				bThrGetSlotNumRun = false;  
#endif
		}

	}
}
void X2FilterWheel::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
//	OutputDbgPrint(pszEvent);
//	OutputDbgPrint("\n");

	char szEvt[DRIVER_MAX_STRING];

	sprintf(szEvt, pszEvent);

	if (!strcmp(szEvt, "on_comboBox_currentIndexChanged"))
	{
		bool bSet;
		
		
		if(!isLinked())//没有连接
		{
			WaitThrExit();
			if(oldEFWID > -1)
				EFWClose(EFWInfo.ID);//把已经打开的关闭

			int index = uiex->currentIndex("comboBox");
			EFWGetID(index, &EFWInfo.ID);
			int pos;
			if (EFWGetPosition(EFWInfo.ID, &pos) == EFW_SUCCESS)
				oldEFWID = -1;//说明其他ASCOM打开了，不要关闭                    
			else
				oldEFWID = EFWInfo.ID;//没有打开
			
			
			if (EFWOpen(EFWInfo.ID) != EFW_SUCCESS)
				return;
			
		//	EFWGetProperty(EFWInfo.ID, &EFWInfo);
			if(iSelectedEFWIDSetting == EFWInfo.ID)
				uiex->setChecked("checkBox", bUniDirSetting);
			else
			{
				EFWGetDirection(EFWInfo.ID, &bSet);
				uiex->setChecked("checkBox", bSet);
			}
			
		}
		else
		{			
			EFWGetDirection(EFWInfo.ID, &bSet);
			uiex->setChecked("checkBox", bSet);
		}
		refreshUI();

	}
	else if (!strcmp(szEvt, "on_checkBox_stateChanged"))
	{
		bUniDirTemp = uiex->isChecked("checkBox");
	}
	else if (!strcmp(szEvt, "on_pushButton_clicked"))
	{
		if (EFWCalibrate(EFWInfo.ID) != EFW_SUCCESS)		
			pdx->setText("label_1", "Can't start calibrate!");
		else
			refreshUI();		
	}
}
