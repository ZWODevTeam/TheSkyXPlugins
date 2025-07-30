#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WINDOWS
#include <windows.h>
//#include <process.h>
#else
//#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>//usleep
#endif
#include "x2focuser.h"

#include "../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/mutexinterface.h"
#include "../../licensedinterfaces/basicstringinterface.h"
#include "../../licensedinterfaces/tickcountinterface.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/sberrorx.h"

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
#else
	printf("%s",strBuf);
#endif

#endif
}

X2Focuser::X2Focuser(const char* pszDisplayName, 
												const int& nInstanceIndex,
												SerXInterface						* pSerXIn, 
												TheSkyXFacadeForDriversInterface	* pTheSkyXIn, 
												SleeperInterface					* pSleeperIn,
												BasicIniUtilInterface				* pIniUtilIn,
												LoggerInterface						* pLoggerIn,
												MutexInterface						* pIOMutexIn,
												TickCountInterface					* pTickCountIn)

{
	m_pSerX							= pSerXIn;		
	m_pTheSkyXForMounts				= pTheSkyXIn;
	m_pSleeper						= pSleeperIn;
	m_pIniUtil						= pIniUtilIn;
	m_pLogger						= pLoggerIn;	
	m_pIOMutex						= pIOMutexIn;
	m_pTickCount					= pTickCountIn;

	m_bLinked = false;
	oldEAFID = -1;
	m_nMaxstep = -1;
	m_nBacklash = -1;
	m_bReverse = false;
	m_bBeep = false;

}

X2Focuser::~X2Focuser()
{
	//Delete objects used through composition
	if (GetSerX())
		delete GetSerX();
	if (GetTheSkyXFacadeForDrivers())
		delete GetTheSkyXFacadeForDrivers();
	if (GetSleeper())
		delete GetSleeper();
	if (GetSimpleIniUtil())
		delete GetSimpleIniUtil();
	if (GetLogger())
		delete GetLogger();
	if (GetMutex())
		delete GetMutex();
}

int	X2Focuser::queryAbstraction(const char* pszName, void** ppVal)
{
	*ppVal = NULL;

	if (!strcmp(pszName, LinkInterface_Name))
		*ppVal = (LinkInterface*)this;
	else if (!strcmp(pszName, FocuserGotoInterface2_Name))
		*ppVal = (FocuserGotoInterface2*)this;
	else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
		*ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
	else if (!strcmp(pszName, X2GUIEventInterface_Name))
		*ppVal = dynamic_cast<X2GUIEventInterface*>(this);
	else if (!strcmp(pszName, FocuserTemperatureInterface_Name))
		*ppVal = dynamic_cast<FocuserTemperatureInterface*>(this);

	return SB_OK;
}

#define DISPLAY_NAME "ZWO X2 Focuser Plug In"

void X2Focuser::driverInfoDetailedInfo(BasicStringInterface& str) const
{
	str = DISPLAY_NAME;
}
double X2Focuser::driverInfoVersion(void) const							
{
	//1.2加温度接口
	//20220628 1.3更新sdk
	//20220930 1.4更新sdk
	return 1.5;

}
void X2Focuser::deviceInfoNameShort(BasicStringInterface& str) const				
{
	str = DISPLAY_NAME;
}
void X2Focuser::deviceInfoNameLong(BasicStringInterface& str) const				
{
	str = DISPLAY_NAME;
}
void X2Focuser::deviceInfoDetailedDescription(BasicStringInterface& str) const		
{
	str = DISPLAY_NAME;
}
void X2Focuser::deviceInfoFirmwareVersion(BasicStringInterface& str)				
{
	str = DISPLAY_NAME;
}

char* get_EAF_SDK_Ver();

void X2Focuser::deviceInfoModel(BasicStringInterface& str)							
{
	char temp[256];
	sprintf(temp, "EAF SDK V");
	strcat(temp, get_EAF_SDK_Ver());

	unsigned char major = 0, minor = 0, build = 0;
	if(EAFGetFirmwareVersion(EAFInfo.ID, &major, &minor, &build) == EAF_SUCCESS)
	{
		sprintf(temp + strlen(temp), " (FW V%d.%d.%d)", major, minor, build);
	}

	str = temp;
}

int	X2Focuser::establishLink(void)
{
	X2MutexLocker ml(GetMutex());

	int iEAFNum = EAFGetNum();         
	if (iEAFNum > 0)
	{
		int pos;
		EAFInfo.ID = iSelectedEAFIDSetting;
		if (EAFOpen(EAFInfo.ID) != EAF_SUCCESS)//此ID的相机已经被移除		
		{
			EAFGetID(0, &EAFInfo.ID);
			if (EAFOpen(EAFInfo.ID) != EAF_SUCCESS)
				return ERR_COMMNOLINK;
		}
	}
	else
		return ERR_COMMNOLINK;

	EAFGetProperty(EAFInfo.ID, &EAFInfo);

	m_bLinked = true;

	return SB_OK;
}
int	X2Focuser::terminateLink(void)
{
	X2MutexLocker ml(GetMutex());

	EAFClose(EAFInfo.ID);
	m_bLinked = false;
	return SB_OK;
}
bool X2Focuser::isLinked(void) const
{
	return m_bLinked;
}

int	X2Focuser::focPosition(int& nPosition) 			
{
	X2MutexLocker ml(GetMutex());

	if(EAFGetPosition(EAFInfo.ID, &nPosition) != EAF_SUCCESS)
		return ERR_CMDFAILED;
	
	return SB_OK;
}
int	X2Focuser::focMinimumLimit(int& nMinLimit) 		
{
	nMinLimit = 0;
	return SB_OK;
}
int	X2Focuser::focMaximumLimit(int& nMaxLimit)			
{
	X2MutexLocker ml(GetMutex());

	if(!m_bLinked)
		return ERR_COMMNOLINK;

	nMaxLimit = EAFInfo.MaxStep;
	return SB_OK;
} 
int	X2Focuser::focAbort()								
{
	X2MutexLocker ml(GetMutex());

	if(EAFStop(EAFInfo.ID) != EAF_SUCCESS)
		return ERR_CMDFAILED;

	return SB_OK;
}

int	X2Focuser::startFocGoto(const int& nRelativeOffset)	
{
	X2MutexLocker ml(GetMutex());

	int position;
	if(EAFGetPosition(EAFInfo.ID, &position) != EAF_SUCCESS)
		return ERR_CMDFAILED;

	position += nRelativeOffset;
	if(EAFMove(EAFInfo.ID, position) != EAF_SUCCESS)
		return ERR_CMDFAILED;

	return SB_OK;
}
int	X2Focuser::isCompleteFocGoto(bool& bComplete) const	
{
	X2Focuser* pMe = (X2Focuser*)this;
	X2MutexLocker ml(pMe->GetMutex());

	bool bVal, bHandControl;
	if (EAFIsMoving(EAFInfo.ID, &bVal, &bHandControl) != EAF_SUCCESS)
		return ERR_CMDFAILED;

	bComplete = !bVal;

	return SB_OK;
}
int	X2Focuser::endFocGoto(void)							
{
	X2MutexLocker ml(GetMutex());

	if(EAFStop(EAFInfo.ID) != EAF_SUCCESS)
		return ERR_CMDFAILED;

	return SB_OK;
}

int X2Focuser::amountCountFocGoto(void) const					
{ 
	return 3;
}
int	X2Focuser::amountNameFromIndexFocGoto(const int& nZeroBasedIndex, BasicStringInterface& strDisplayName, int& nAmount)
{
	switch (nZeroBasedIndex)
	{
		default:
		case 0: strDisplayName="10";	 nAmount=10;break;
		case 1: strDisplayName="100"; nAmount=100;break;
		case 2: strDisplayName="1000";	 nAmount=1000;break;
	}
	return SB_OK;
}
int	X2Focuser::amountIndexFocGoto(void)
{
	return 0;
}

//Please see the X2Camera example for more extensive use of different type of GUI controls
int X2Focuser::execModalSettingsDialog()
{
	int nErr = SB_OK;

	int EAFNum;
	if ((EAFNum = EAFGetNum()) > 0)
	{                
		if (isLinked())
		{
			int pos;
			if (EAFGetPosition(EAFInfo.ID, &pos) != EAF_SUCCESS)//此ID的已经被移除			
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

	if (nErr = ui->loadUserInterface("x2focuserEAF.ui", deviceType(), m_nPrivateMulitInstanceIndex))
		return nErr;

	if (NULL == (dx = uiutil.X2DX()))
		return ERR_POINTER;

	//Intialize the user interface
	pdx = dx;


	dx->invokeMethod("comboBox","clear");

	int itemIndex = 0, pos;
	EAF_INFO EAFInfoTemp;
	char szBuf[256];
	for (int i = 0; i < EAFNum; i++)
	{
		EAFGetID(i, &EAFInfoTemp.ID);
		EAFGetProperty(EAFInfoTemp.ID, &EAFInfoTemp);//没打开的不能得到孔数 名字和ID能得到		
		if (isLinked() && EAFInfoTemp.ID == EAFInfo.ID)		
			itemIndex = i;
		sprintf(szBuf, "%s (ID %d)", EAFInfoTemp.Name, EAFInfoTemp.ID);
		dx->comboBoxAppendString("comboBox", szBuf);
	}

	dx->setEnabled("comboBox", !isLinked());
	dx->setCurrentIndex("comboBox", itemIndex);

//	dx->setText("label_1", "Set zero success");	

	//Display the user interface
	if (nErr = ui->exec(bPressedOK))
	{
		if (!isLinked())
			terminateLink();
		return nErr;
	}

	//Retreive values from the user interface
	if (bPressedOK)
	{
		iSelectedEAFIDSetting = EAFInfo.ID;

		char buf[64] = {0};

		if (m_bReverse != dx->isChecked("checkBox") && EAFSetReverse(EAFInfo.ID, !m_bReverse) != EAF_SUCCESS)
			OutputDbgPrint("Fail to SetReverse!");

		if (m_bBeep != dx->isChecked("checkBox_2") && EAFSetBeep(EAFInfo.ID, !m_bBeep) != EAF_SUCCESS)
			OutputDbgPrint("Fail to SetBeep!");

		dx->text("textBox_maxstep", buf, 64);
		int iValue = atoi(buf);
		if (iValue != m_nMaxstep && EAFSetMaxStep(EAFInfo.ID, iValue) != EAF_SUCCESS)
			OutputDbgPrint("Fail to SetMaxStep!");

		dx->text("textBox_backlash", buf, 64);
		iValue = atoi(buf);
		if (iValue != m_nBacklash && EAFSetBacklash(EAFInfo.ID, iValue) != EAF_SUCCESS)		
			OutputDbgPrint("Fail to SetBacklash!");
	}

	if (!isLinked() && oldEAFID > -1)
	{
		EAFClose(oldEAFID);
		oldEAFID = -1;
	}

	return nErr;
}

void X2Focuser::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
	OutputDbgPrint(pszEvent);
/*
控件的名称是固定的，用debugview可以看到，只接收下面这些
[9668] QMetaObject::connectSlotsByName: No matching signal for on_pushButton_2_clicked()
...
[9668] QMetaObject::connectSlotsByName: No matching signal for on_pushButton_10_clicked()
[9668] QMetaObject::connectSlotsByName: No matching signal for on_radioButton_clicked()
[9668] QMetaObject::connectSlotsByName: No matching signal for on_radioButton_2_clicked()
...
[9668] QMetaObject::connectSlotsByName: No matching signal for on_radioButton_10_clicked()
[9668] QMetaObject::connectSlotsByName: No matching signal for on_checkBox_stateChanged(int)
[9668] QMetaObject::connectSlotsByName: No matching signal for on_checkBox_2_stateChanged(int)
...
[9668] QMetaObject::connectSlotsByName: No matching signal for on_checkBox_10_stateChanged(int)
[9668] QMetaObject::connectSlotsByName: No matching signal for on_comboBox_2_currentIndexChanged(int)
...
[9668] QMetaObject::connectSlotsByName: No matching signal for on_comboBox_10_currentIndexChanged(int)

*/
	if (!strcmp(pszEvent, "on_timer"))
	{
		float fTemp;
		if(EAFGetTemp(EAFInfo.ID, &fTemp) == EAF_SUCCESS)
		{
			char szbuf[256];
			sprintf(szbuf, "T: %.1fC", fTemp);
			uiex->setText("label_2", szbuf);
		}

	}
	else if (!strcmp(pszEvent, "on_comboBox_currentIndexChanged"))
	{
		//打开对话框会运行一次

		bool bSet;

		if(!isLinked())//没有连接
		{
			if(oldEAFID > -1)
				EAFClose(EAFInfo.ID);//把已经打开的关闭

			int index = uiex->currentIndex("comboBox");
			EAFGetID(index, &EAFInfo.ID);
			int pos;
			if (EAFGetPosition(EAFInfo.ID, &pos) == EAF_SUCCESS)
				oldEAFID = -1;//说明其他ASCOM打开了，不要关闭                    
			else
				oldEAFID = EAFInfo.ID;//没有打开

			if (EAFOpen(EAFInfo.ID) != EAF_SUCCESS)
				return;
		}

		bool bVal, bHandControl;
		if (EAFIsMoving(EAFInfo.ID, &bVal, &bHandControl) == EAF_SUCCESS)
		{	
			uiex->setText("label_1", bVal ? "Moving" : "Ready");	
		}

		EAFGetReverse(EAFInfo.ID, &m_bReverse);
		uiex->setChecked("checkBox", m_bReverse);

		EAFGetBeep(EAFInfo.ID, &m_bBeep);
		uiex->setChecked("checkBox_2", m_bBeep);

		EAFGetMaxStep(EAFInfo.ID, &m_nMaxstep);
		char szbuf[64] = {0};
		sprintf(szbuf, "%d", m_nMaxstep);
		uiex->setText("textBox_maxstep", szbuf);	

		EAFGetBacklash(EAFInfo.ID, &m_nBacklash);
		sprintf(szbuf, "%d", m_nBacklash);
		uiex->setText("textBox_backlash", szbuf);
	
	}
/*	else if (!strcmp(pszEvent, "on_checkBox_stateChanged"))
	{
		m_bReverse = uiex->isChecked("checkBox");
		if (EAFSetReverse(EAFInfo.ID, bChecked) != EAF_SUCCESS)
			uiex->setText("label_1", "Fail to operate!");	
	}
	else if (!strcmp(pszEvent, "on_checkBox_2_stateChanged"))
	{
		m_bBeep = uiex->isChecked("checkBox_2");
		if (EAFSetBeep(EAFInfo.ID, bChecked) != EAF_SUCCESS)
			uiex->setText("label_1", "Fail to operate!");	
	}*/
	else if (!strcmp(pszEvent, "on_pushButton_clicked"))
	{		
		if (EAFResetPostion(EAFInfo.ID, 0) == EAF_SUCCESS)
			uiex->setText("label_1", "Set zero success");	
		else
			uiex->setText("label_1", "Fail to reset!");	
	}
}
int X2Focuser::focTemperature(double &dTemperature)
{
	X2MutexLocker ml(GetMutex());

	float fTemp;
	if(EAFGetTemp(EAFInfo.ID, &fTemp) != EAF_SUCCESS)
		return ERR_CMDFAILED;

	dTemperature = fTemp;
	return SB_OK;
}
