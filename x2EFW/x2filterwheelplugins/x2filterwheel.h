#pragma once
#ifdef _WINDOWS
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>//usleep
#define Sleep(a) usleep((a)*1000)
#endif

#include "../../licensedinterfaces/filterwheeldriverinterface.h"
#include "../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../licensedinterfaces/x2guiinterface.h"
#include "../../../FilterSDK/EFW_filter/EFW_filter.h"


// Forward declare the interfaces that the this driver is "given" by TheSkyX
class SerXInterface;
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class BasicIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class TickCountInterface;

/*!
\brief The X2FilterWheel example.

\ingroup Example

Use this example to write an X2FilterWheel driver.

Utilizes  ModalSettingsDialogInterface and  X2GUIEventInterface for a basic, cross-platform, user interface.
*/
class X2FilterWheel : public FilterWheelDriverInterface, public ModalSettingsDialogInterface, public X2GUIEventInterface 
{
public:
	/*!Standard X2 constructor*/
	X2FilterWheel(const char* pszDriverSelection,
				const int& nInstanceIndex,
				SerXInterface					* pSerX, 
				TheSkyXFacadeForDriversInterface	* pTheSkyX, 
				SleeperInterface					* pSleeper,
				BasicIniUtilInterface			* pIniUtil,
				LoggerInterface					* pLogger,
				MutexInterface					* pIOMutex,
				TickCountInterface				* pTickCount);

	~X2FilterWheel();

// Operations
public:

	/*!\name DriverRootInterface Implementation
	See DriverRootInterface.*/
	//@{ 
	virtual DeviceType							deviceType(void)							  {return DriverRootInterface::DT_FILTERWHEEL;}
	virtual int									queryAbstraction(const char* pszName, void** ppVal) ;
	//@} 

	/*!\name LinkInterface Implementation
	See LinkInterface.*/
	//@{ 
	virtual int									establishLink(void)						;
	virtual int									terminateLink(void)						;
	virtual bool								isLinked(void) const					;
	virtual bool								isEstablishLinkAbortable(void) const	;
	//@} 

	/*!\name DriverInfoInterface Implementation
	See DriverInfoInterface.*/
	//@{ 
	virtual void								driverInfoDetailedInfo(BasicStringInterface& str) const;
	virtual double								driverInfoVersion(void) const				;
	//@} 

	/*!\name HardwareInfoInterface Implementation
	See HardwareInfoInterface.*/
	//@{ 
	virtual void deviceInfoNameShort(BasicStringInterface& str) const				;
	virtual void deviceInfoNameLong(BasicStringInterface& str) const				;
	virtual void deviceInfoDetailedDescription(BasicStringInterface& str) const	;
	virtual void deviceInfoFirmwareVersion(BasicStringInterface& str)				;
	virtual void deviceInfoModel(BasicStringInterface& str)						;
	//@} 

	/*!\name FilterWheelMoveToInterface Implementation
	See FilterWheelMoveToInterface.*/
	//@{ 
	virtual int									filterCount(int& nCount)							;
	virtual int									defaultFilterName(const int& nIndex, BasicStringInterface& strFilterNameOut);
	virtual int									startFilterWheelMoveTo(const int& nTargetPosition)	;
	virtual int									isCompleteFilterWheelMoveTo(bool& bComplete) const	;
	virtual int									endFilterWheelMoveTo(void)							;
	virtual int									abortFilterWheelMoveTo(void)						;
	//@}

//
	/*!\name ModalSettingsDialogInterface Implementation
	See ModalSettingsDialogInterface.*/
	//@{ 
	virtual int								initModalSettingsDialog(void){return 0;}
	virtual int								execModalSettingsDialog(void);
	//@} 
	
	//
	/*!\name X2GUIEventInterface Implementation
	See X2GUIEventInterface.*/
	//@{ 
	virtual void uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent);
	//@} 

	int			iSelectedEFWIDSetting;	
	bool		bThrGetSlotNumRun;
	bool		bUniDirTemp, bUniDirSetting;
	EFW_INFO	EFWInfo;
	void		refreshUI();

// Implementation
private:	

	SerXInterface 							*GetSerX() {return m_pSerX; }		
	TheSkyXFacadeForDriversInterface		*GetTheSkyXFacadeForDrivers() {return m_pTheSkyXForMounts;}
	SleeperInterface						*GetSleeper() {return m_pSleeper; }
	BasicIniUtilInterface					*GetSimpleIniUtil() {return m_pIniUtil; }
	LoggerInterface							*GetLogger() {return m_pLogger; }
	MutexInterface							*GetMutex()  {return m_pIOMutex;}
	TickCountInterface						*GetTickCountInterface() {return m_pTickCount;}
	

	int m_nPrivateMulitInstanceIndex;
	SerXInterface*							m_pSerX;		
	TheSkyXFacadeForDriversInterface* 		m_pTheSkyXForMounts;
	SleeperInterface*						m_pSleeper;
	BasicIniUtilInterface*					m_pIniUtil;
	LoggerInterface*							m_pLogger;
	MutexInterface*							m_pIOMutex;
	TickCountInterface*						m_pTickCount;

#ifdef _WINDOWS
	HANDLE
#else
	pthread_t
#endif
	Thr_getSlotNum;

	bool		m_bLinked;
	X2GUIExchangeInterface*			pdx;
	void WaitThrExit();
	int oldEFWID;


};
//1.0.0.3 ->2016.12.27