#pragma once

#include "../../licensedinterfaces/cameradriverinterface.h"
#include "../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../licensedinterfaces/x2guiinterface.h"
#include "../../licensedinterfaces/subframeinterface.h"
#include "../../licensedinterfaces/noshutterinterface.h"
#include "../../licensedinterfaces/pixelsizeinterface.h"
#include "../../licensedinterfaces/addfitskeyinterface.h"
#include "../../licensedinterfaces/cameradependentsettinginterface.h"


#include "ASICamera2.h"
#include "tinyxml.h"
#include "XMLconfig.h"

class SerXInterface;		
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class BasicIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class TickCountInterface;


/*!
\brief The X2Camera example.

\ingroup Example

Use this example to write an X2Camera driver.
*/
class X2Camera: public CameraDriverInterface, public ModalSettingsDialogInterface, public X2GUIEventInterface, public SubframeInterface\
	, public NoShutterInterface, public PixelSizeInterface, public AddFITSKeyInterface, public CameraDependentSettingInterface
{
public: 
	/*!Standard X2 constructor*/
	X2Camera(const char* pszSelectionString, 
					const int& nISIndex,
					SerXInterface*						pSerX,
					TheSkyXFacadeForDriversInterface* pTheSkyXForMounts,
					SleeperInterface*					pSleeper,
					BasicIniUtilInterface*				pIniUtil,
					LoggerInterface*					pLogger,
					MutexInterface*					pIOMutex,
					TickCountInterface*				pTickCount);
	virtual ~X2Camera();  

	/*!\name DriverRootInterface Implementation
	See DriverRootInterface.*/
	//@{ 
	virtual int									queryAbstraction(const char* pszName, void** ppVal)			;
	//@} 

	/*!\name DriverInfoInterface Implementation
	See DriverInfoInterface.*/
	//@{ 
	virtual void								driverInfoDetailedInfo(BasicStringInterface& str) const		;
	virtual double								driverInfoVersion(void) const								;
	//@} 

	/*!\name HardwareInfoInterface Implementation
	See HardwareInfoInterface.*/
	//@{ 
	virtual void deviceInfoNameShort(BasicStringInterface& str) const										;
	virtual void deviceInfoNameLong(BasicStringInterface& str) const										;
	virtual void deviceInfoDetailedDescription(BasicStringInterface& str) const								;
	virtual void deviceInfoFirmwareVersion(BasicStringInterface& str)										;
	virtual void deviceInfoModel(BasicStringInterface& str)													;
	//@} 

public://Properties

	/*!\name CameraDriverInterface Implementation
	See CameraDriverInterface.*/
	//@{ 

	virtual enumCameraIndex	cameraId();
	virtual	void		setCameraId(enumCameraIndex Cam);
	virtual bool		isLinked()					{return m_bLinked;}
	virtual void		setLinked(const bool bYes)	{m_bLinked = bYes;}
	
	virtual int			GetVersion(void)			{return CAMAPIVERSION;}
	virtual CameraDriverInterface::ReadOutMode readoutMode(void);
	virtual int			pathTo_rm_FitsOnDisk(char* lpszPath, const int& nPathSize);

public://Methods

	virtual int CCSettings(const enumCameraIndex& Camera, const enumWhichCCD& CCD);

	virtual int CCEstablishLink(enumLPTPort portLPT, const enumWhichCCD& CCD, enumCameraIndex DesiredCamera, enumCameraIndex& CameraFound, const int nDesiredCFW, int& nFoundCFW);
	virtual int CCDisconnect(const bool bShutDownTemp);

	virtual int CCGetChipSize(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nXBin, const int& nYBin, const bool& bOffChipBinning, int& nW, int& nH, int& nReadOut);
	virtual int CCGetNumBins(const enumCameraIndex& Camera, const enumWhichCCD& CCD, int& nNumBins);
	virtual	int CCGetBinSizeFromIndex(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nIndex, long& nBincx, long& nBincy);

	virtual int CCSetBinnedSubFrame(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nLeft, const int& nTop, const int& nRight, const int& nBottom);

	virtual void CCMakeExposureState(int* pnState, enumCameraIndex Cam, int nXBin, int nYBin, int abg, bool bRapidReadout);//SBIG specific

	virtual int CCStartExposure(const enumCameraIndex& Cam, const enumWhichCCD CCD, const double& dTime, enumPictureType Type, const int& nABGState, const bool& bLeaveShutterAlone);
	virtual int CCIsExposureComplete(const enumCameraIndex& Cam, const enumWhichCCD CCD, bool* pbComplete, unsigned int* pStatus);
	virtual int CCEndExposure(const enumCameraIndex& Cam, const enumWhichCCD CCD, const bool& bWasAborted, const bool& bLeaveShutterAlone);

	virtual int CCReadoutLine(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& pixelStart, const int& pixelLength, const int& nReadoutMode, unsigned char* pMem);
	virtual int CCDumpLines(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& nReadoutMode, const unsigned int& lines);

	virtual int CCReadoutImage(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& nWidth, const int& nHeight, const int& nMemWidth, unsigned char* pMem);

	virtual int CCRegulateTemp(const bool& bOn, const double& dTemp);
	virtual int CCQueryTemperature(double& dCurTemp, double& dCurPower, char* lpszPower, const int nMaxLen, bool& bCurEnabled, double& dCurSetPoint);
	virtual int	CCGetRecommendedSetpoint(double& dRecSP);
	virtual int	CCSetFan(const bool& bOn);

	virtual int CCActivateRelays(const int& nXPlus, const int& nXMinus, const int& nYPlus, const int& nYMinus, const bool& bSynchronous, const bool& bAbort, const bool& bEndThread);

	virtual int CCPulseOut(unsigned int nPulse, bool bAdjust, const enumCameraIndex& Cam);

	virtual int CCSetShutter(bool bOpen);
	virtual int CCUpdateClock(void);

	virtual int CCSetImageProps(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nReadOut, void* pImage);	
	virtual int CCGetFullDynamicRange(const enumCameraIndex& Camera, const enumWhichCCD& CCD, unsigned long& dwDynRg);
	
	virtual void CCBeforeDownload(const enumCameraIndex& Cam, const enumWhichCCD& CCD);
	virtual void CCAfterDownload(const enumCameraIndex& Cam, const enumWhichCCD& CCD);

	//SubframeInterface
	/*!TheSkyX calls this fuunction to give the driver the size of the subframe in binned pixels. If there is no subframe, the size represents the entire CCD.
	  For example, a CCD chip that has a width of 1500 pixels and a height of 1200 will have 0,0,1500,1200 for left, top, nWidth, nHeight.*/
	virtual int CCSetBinnedSubFrame3(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, const int& nLeft, const int& nTop, const int& nWidth, const int& nHeight);
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

	//Implemenation below here
	virtual int CCHasShutter(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, bool &bHasShutter);

	virtual int PixelSize1x1InMicrons	( const enumCameraIndex & Camera, const enumWhichCCD & CCD, double & x, double & y);	

	virtual int countOfIntegerFields(int &nCount);
	virtual int valueForIntegerField(int nIndex, BasicStringInterface& sFieldName, BasicStringInterface& sFieldComment, int &nFieldValue);

	virtual int countOfDoubleFields (int &nCount);
	virtual int valueForDoubleField (int nIndex, BasicStringInterface& sFieldName, BasicStringInterface& sFieldComment, double &dFieldValue);

	virtual int countOfStringFields (int &nCount);
	virtual int valueForStringField (int nIndex, BasicStringInterface& sFieldName, BasicStringInterface& sFieldComment, BasicStringInterface &sFieldValue);
	
	virtual int CCGetExtendedSettingName(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, BasicStringInterface &sSettingName);
	virtual int CCGetExtendedValueCount(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, int &nCount);
	virtual int CCGetExtendedValueName(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, const int nIndex, BasicStringInterface &sName);
	virtual int CCStartExposureAdditionalArgInterface(const enumCameraIndex& Cam, const enumWhichCCD CCD, const double& dTime, enumPictureType Type,
		const int& nABGState, const bool& bLeaveShutterAlone, const int &nIndex);

private:
	SerXInterface 									*	GetSerX() {return m_pSerX; }		
	TheSkyXFacadeForDriversInterface				*	GetTheSkyXFacadeForDrivers() {return m_pTheSkyXForMounts;}
	SleeperInterface								*	GetSleeper() {return m_pSleeper; }
	BasicIniUtilInterface							*	GetBasicIniUtil() {return m_pIniUtil; }
	LoggerInterface									*	GetLogger() {return m_pLogger; }
	MutexInterface									*	GetMutex() const  {return m_pIOMutex;}
	TickCountInterface								*	GetTickCountInterface() {return m_pTickCount;}

	SerXInterface									*	m_pSerX;		
	TheSkyXFacadeForDriversInterface				*	m_pTheSkyXForMounts;
	SleeperInterface								*	m_pSleeper;
	BasicIniUtilInterface							*	m_pIniUtil;
	LoggerInterface									*	m_pLogger;
	MutexInterface									*	m_pIOMutex;
	TickCountInterface								*	m_pTickCount;

	int m_nPrivateISIndex;

	long GetBufLen();


	struct  CtrlCombo{
		ASI_CONTROL_CAPS CtlCap;		
		long lVal;
	};

	CtrlCombo ComboList[64];
	CtrlCombo ComboListSetting[64];//保存设置
	int iComboCount;
	int iCamIDSetting, iCamIDTemp;
	ASI_IMG_TYPE ImgTypeSetting;
	void refreshImgType(X2GUIExchangeInterface*	pdx, ASI_IMG_TYPE imgType);
	void refreshControl(X2GUIExchangeInterface*	pdx);
	void refreshControlVal(X2GUIExchangeInterface*	pdx, ASI_CONTROL_CAPS CtrlCaps, int itemIndex);
	int iWidthROI, iHeightROI, iBin;//实际的ROI(有别于显示的ROI)
	unsigned char* pImg;
	long lBufferLen;
	ASI_CAMERA_INFO ASICameraInfo;
	ASI_IMG_TYPE ImgTypeArray[8];
	unsigned long dwStartMs;
	void DeleteImgBuf();
	long lExpMs;
	long lVideoWaitMs;
	int startX_display, startY_display;
	void CloseCam();
	int iPreComboIndex;
	int iStartX, iStartY;//实际的值(有别于显示的值)
	int m_failedcount;
	char szCamNameSetting[256];
	bool bExpSuccess;
	int oldCamID;
	bool isVideoMode;
	int oldStartX_display , oldStartY_display, oldWidth, oldHeight, oldBin, iWidthMax, iHeightMax;
	long oldHardbin;
	long oldVideoTimeMs;


//	int iST4Direct;
	bool isImgTypeSupported(ASI_CAMERA_INFO* CamInfo, ASI_IMG_TYPE imgtype);
	bool getControlRange(int cam_ID, ASI_CONTROL_TYPE CtrlType, int* minVal, int* maxVal);
	int getDefaultUSBValue(ASI_CAMERA_INFO cam_info);
	void setShownCtrlVal(ASI_CONTROL_TYPE ctrlType, int iVal);
	
	/*从相机读取显示的控件值*/
	void ReadShownCtrlCaps();

	void saveXMLSetting(ASI_CAMERA_INFO info);
	bool loadXMLSetting(ASI_CAMERA_INFO info);

	void saveXMLAppSetting(const char* ItemName, const char* ElemName, const char* ElemVal, const int* ElemAttr);
	bool loadXMLAppSetting(const char* ItemName, const char* ElemName, unsigned char* ElemVal, int* ElemAttr);
	bool isShow(ASI_CONTROL_TYPE ctrltype);
	void prepareExposure();
	bool setCamROI(int width, int height, int bin, ASI_IMG_TYPE type);
};

//1, 1, 0, 1->20161216,针对SDK更新(用ID打开)修改
//1, 1, 0, 2->20170113,针对SDK更新
//1, 1, 0, 3->20170301,保存设置到xml,实现CCActivateRelays
//1, 1, 0, 4->20170328,CCGetNumBins和 CCGetBinSizeFromIndex在没连接时候返回1x1
//1, 1, 0, 5->20170425,实现CCHasShutter
//2, 0, 0, 4->20171130,保存gain和offset到fits文件头
//2, 0, 0, 6->20180130,更新SDK
//2, 0, 0, 6->20180130,更新SDK
//2, 0, 0, 7->20180208,修改debayer keyword
//2, 0, 0, 8->20181012,修改fits keyword
//2, 0, 1, 1->20181029,long exposure mode放在前面