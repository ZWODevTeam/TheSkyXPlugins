// x2camera.cpp  
//
 #ifdef _WINDOWS
 #include <windows.h>
#else
#include "comdef.h"
#define PBYTE unsigned char*
 #endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "x2camera.h"
#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/mutexinterface.h"
#include "../../licensedinterfaces/tickcountinterface.h"
#include "../../licensedinterfaces/basicstringinterface.h"
#include "../../licensedinterfaces/x2guiinterface.h"

//As far as a naming convention goes, X2 implementors could do a search and 
//replace in all files and change "X2Camera" to "CoolCompanyCamera" 
//where CoolCompany is your company name.  This is not a requirement.
#define DEVICE_AND_DRIVER_INFO_STRING "ZWO X2Camera Driver 19.3.6"

//For properties that need to be persistent
#define KEY_X2CAM_ROOT			"X2Camera"
#define KEY_WIDTH				"Width"
#define KEY_HEIGHT				"Height"

#ifndef _WINDOWS
static inline int max(int a, int b) {
	return a>b ? a:b;
}

static inline int min(int a, int b) {
	return a<b ? a:b;
}
#endif

#define OutputDbgPrint(...)	DbgPrint(-1, __FUNCTION__, __VA_ARGS__)
extern void DbgPrint(int pid, const char* funcName, const char* strOutPutString, ...);

X2Camera::X2Camera( const char* pszSelection, 
					const int& nISIndex,
					SerXInterface*						pSerX,
					TheSkyXFacadeForDriversInterface*	pTheSkyXForMounts,
					SleeperInterface*					pSleeper,
					BasicIniUtilInterface*				pIniUtil,
					LoggerInterface*					pLogger,
					MutexInterface*						pIOMutex,
					TickCountInterface*					pTickCount)
{   
	m_nPrivateISIndex				= nISIndex;
	m_pSerX							= pSerX;
	m_pTheSkyXForMounts				= pTheSkyXForMounts;
	m_pSleeper						= pSleeper;
	m_pIniUtil						= pIniUtil;
	m_pLogger						= pLogger;	
	m_pIOMutex						= pIOMutex;
	m_pTickCount					= pTickCount;

	setCameraId(CI_PLUGIN);


	ASICameraInfo.CameraID = -1;
	pImg = NULL;
	lBufferLen = 0;
	iBin = 1;
	ImgTypeSetting = ASI_IMG_END;
	iPreComboIndex = -1;
	iComboCount = m_failedcount = 0;
	memset(szCamNameSetting, 0, sizeof(szCamNameSetting));
	bExpSuccess = false;
	oldCamID = -1;
	isVideoMode = false;
	oldStartX_display = -1, oldStartY_display = -1, oldWidth = -1, oldHeight = -1, oldBin = -1, iWidthMax = 0, iHeightMax = 0;
	oldHardbin = -1;
	oldVideoTimeMs = -1;
//	iST4Direct = -1,
	OutputDbgPrint("Constructor\n");
}

X2Camera::~X2Camera()
{
	//Delete objects used through composition
	if (GetSerX())
		delete GetSerX();
	if (GetTheSkyXFacadeForDrivers())
		delete GetTheSkyXFacadeForDrivers();
	if (GetSleeper())
		delete GetSleeper();
	if (GetBasicIniUtil())
		delete GetBasicIniUtil();
	if (GetLogger())
		delete GetLogger();
	if (GetMutex())
		delete GetMutex();
	if (GetTickCountInterface())
		delete GetTickCountInterface();
	OutputDbgPrint("Destructor\n");
}

//DriverRootInterface
int	X2Camera::queryAbstraction(const char* pszName, void** ppVal)		
{
	X2MutexLocker ml(GetMutex());

	if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
		*ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
	else if (!strcmp(pszName, X2GUIEventInterface_Name))
		*ppVal = dynamic_cast<X2GUIEventInterface*>(this);
	else if (!strcmp(pszName, SubframeInterface_Name))
		*ppVal = dynamic_cast<SubframeInterface*>(this);
	else if(!strcmp(pszName, NoShutterInterface_Name))
		*ppVal = dynamic_cast<NoShutterInterface*>(this);
	else if(!strcmp(pszName, PixelSizeInterface_Name))
		*ppVal = dynamic_cast<PixelSizeInterface*>(this);
	else if (!strcmp(pszName, AddFITSKeyInterface_Name))
		*ppVal = dynamic_cast<AddFITSKeyInterface*>(this);
	else if (!strcmp(pszName, CameraDependentSettingInterface_Name))
		*ppVal = dynamic_cast<CameraDependentSettingInterface*>(this);
	return SB_OK;
}

//DriverInfoInterface
void X2Camera::driverInfoDetailedInfo(BasicStringInterface& str) const		
{
	X2MutexLocker ml(GetMutex());

	str = DEVICE_AND_DRIVER_INFO_STRING;
}
double X2Camera::driverInfoVersion(void) const								
{
	X2MutexLocker ml(GetMutex());

	return 3.33;
	//20190306 2.7
	//20190910 2.8
	//20200228 2.9
	//20201103 3.0
	//2021-02-18 3.1
	//2021-09-23 3.2
	//2021-09-28 3.3
	//2021-10-21 3.4
	//2022-1-28 3.5
	//2022-6-17 3.24 SDK v1.24
	//2022-7-21 3.25 SDK v1.25
	//2022-11-4 3.27 SDK v1.27
	//2023-4-23 3.29 SDK v1.29
	//2023-9-6  3.30 SDK v1.30
	//2023-11-29 3.32 SDK v1.32
	//2024-1-2 3.33 SDK v1.33
}

//HardwareInfoInterface
void X2Camera::deviceInfoNameShort(BasicStringInterface& str) const										
{
	X2MutexLocker ml(GetMutex());

	str = DEVICE_AND_DRIVER_INFO_STRING;
}
void X2Camera::deviceInfoNameLong(BasicStringInterface& str) const										
{
	X2MutexLocker ml(GetMutex());

	str = DEVICE_AND_DRIVER_INFO_STRING;
}
void X2Camera::deviceInfoDetailedDescription(BasicStringInterface& str) const								
{
	X2MutexLocker ml(GetMutex());

	str = DEVICE_AND_DRIVER_INFO_STRING;
}
void X2Camera::deviceInfoFirmwareVersion(BasicStringInterface& str)										
{
	X2MutexLocker ml(GetMutex());

	str = DEVICE_AND_DRIVER_INFO_STRING;
}
void X2Camera::deviceInfoModel(BasicStringInterface& str)													
{
	X2MutexLocker ml(GetMutex());

//	str = DEVICE_AND_DRIVER_INFO_STRING;	
	char temp[64];
	sprintf(temp, "ASI SDK V");
	strcat(temp, ASIGetSDKVersion());
	str = temp;
}

bool X2Camera::getControlRange(int cam_ID, ASI_CONTROL_TYPE CtrlType, int* minVal, int* maxVal)
{	
	ASI_CONTROL_CAPS CtrlCap;
	int CtrlNum; 
	* minVal = * maxVal = 0;
	if (ASIGetNumOfControls(cam_ID, &CtrlNum) != ASI_SUCCESS)
		return false;
	for (int i = 0; i < CtrlNum; i++)
	{		
		ASIGetControlCaps(cam_ID, i, &CtrlCap);
		if (CtrlCap.ControlType == CtrlType)
		{
			*minVal = (int)CtrlCap.MinValue;
			*maxVal = (int)CtrlCap.MaxValue;
			return true;
		}
	}
	return false;

}
int X2Camera::getDefaultUSBValue(ASI_CAMERA_INFO cam_info)
{
    int CtrlMin, CtrlMax;
    if (!getControlRange(cam_info.CameraID, ASI_BANDWIDTHOVERLOAD, &CtrlMin, &CtrlMax))
        return 0;
    if (cam_info.IsUSB3Camera == ASI_TRUE && cam_info.IsUSB3Host == ASI_FALSE)//USB2Host && USB3Camera
        return CtrlMax * 8 / 10;//20160615
/*          else//USB3Host || USB2Camera
        if (cam_info.IsUSB3Camera == ASICameraDll2.ASI_BOOL.ASI_TRUE)
            return CtrlMax * 5 / 10;*/
        else
            return CtrlMin;
}
int X2Camera::CCEstablishLink(const enumLPTPort portLPT, const enumWhichCCD& CCD, enumCameraIndex DesiredCamera, enumCameraIndex& CameraFound, const int nDesiredCFW, int& nFoundCFW)
{ 	
	X2MutexLocker ml(GetMutex());

	int iCamNum = ASIGetNumOfConnectedCameras();
	char szAppSettingElemName[256] = {0};
	if(iCamNum > 0)
	{
		if(CCD == CCD_IMAGER)			
			strcpy(szAppSettingElemName, "imager_cam");
		else
			strcpy(szAppSettingElemName, "guider_cam");
		if(!szCamNameSetting[0])//没在连接前设置过 从记录里读取
		{
			char ElemVal[256], ElemAttr[256];
			if(loadXMLAppSetting("camera_chooser", szAppSettingElemName, (unsigned char*)ElemVal, &iCamIDSetting))				
				strcpy(szCamNameSetting, ElemVal);	
		}
		if(szCamNameSetting[0])//打开 特定的型号和ID
		{

			ASI_CAMERA_INFO CamInfoBothMatch;
			ASI_CAMERA_INFO CamInfoNameMatch;
			CamInfoBothMatch.CameraID = CamInfoNameMatch.CameraID = -1;
			for(int i = 0; i < iCamNum; i++)
			{
				ASIGetCameraProperty(&ASICameraInfo, i);			
				if(!strcmp(ASICameraInfo.Name, szCamNameSetting))
				{
					CamInfoNameMatch = ASICameraInfo;//取列表排在最后的名字相同的
					if(ASICameraInfo.CameraID == iCamIDSetting)//型号和序号都相同)
					{
						CamInfoBothMatch = ASICameraInfo;
						break;
					}
				}
			}
			if (CamInfoBothMatch.CameraID != -1)
				ASICameraInfo = CamInfoBothMatch;
			else if (CamInfoNameMatch.CameraID != -1)
				ASICameraInfo = CamInfoNameMatch;
		}
		else//没设置过则打开第一个
			ASIGetCameraProperty(&ASICameraInfo, 0);	
		
	}
	else
		return ERR_COMMNOLINK;
	
	if(ASIOpenCamera(ASICameraInfo.CameraID) != ASI_SUCCESS)
		return ERR_CMDFAILED;
	
	ASIInitCamera(ASICameraInfo.CameraID);

	ASI_IMG_TYPE imgType = ASI_IMG_RAW8;

	/*******20170908*****/
	ASISetControlValue(ASICameraInfo.CameraID, ASI_FLIP, ASI_FLIP_NONE, ASI_FALSE);
	ASISetControlValue(ASICameraInfo.CameraID, ASI_GAMMA, 50, ASI_FALSE);
	ASISetControlValue(ASICameraInfo.CameraID, ASI_HIGH_SPEED_MODE, 0, ASI_FALSE);
	ASISetControlValue(ASICameraInfo.CameraID, ASI_HARDWARE_BIN, 0, ASI_FALSE);
	ASISetControlValue(ASICameraInfo.CameraID, ASI_WB_R, 50, ASI_FALSE);
	ASISetControlValue(ASICameraInfo.CameraID, ASI_WB_B, 50, ASI_FALSE);
	ASISetControlValue(ASICameraInfo.CameraID, ASI_OVERCLOCK, 0, ASI_FALSE);
	ASISetControlValue(ASICameraInfo.CameraID, ASI_PATTERN_ADJUST, 0, ASI_FALSE);
	
	if(loadXMLSetting(ASICameraInfo))//加载保存的设置
	{		
		imgType = ImgTypeSetting;
		for (int i = 0; i < iComboCount; i++)
			ASISetControlValue(ASICameraInfo.CameraID, ComboListSetting[i].CtlCap.ControlType, ComboListSetting[i].lVal, ASI_FALSE);//设置为setting里的参数
	}
	else//没有预设参数的，USBtraffic设置默认值
	{
		imgType = isImgTypeSupported(&ASICameraInfo, ASI_IMG_RAW16)?ASI_IMG_RAW16:ASI_IMG_RAW8;
		ASISetControlValue(ASICameraInfo.CameraID, ASI_BANDWIDTHOVERLOAD, getDefaultUSBValue(ASICameraInfo), ASI_FALSE);

		ASISetControlValue(ASICameraInfo.CameraID, ASI_ANTI_DEW_HEATER, 1, ASI_FALSE);		
	}
	
	iWidthROI = ASICameraInfo.MaxWidth/8*8;
	iHeightROI = ASICameraInfo.MaxHeight/2*2;
	setCamROI(iWidthROI, iHeightROI, 1, imgType);

	saveXMLAppSetting("camera_chooser", szAppSettingElemName, ASICameraInfo.Name, &ASICameraInfo.CameraID);

	setLinked(true);
	return SB_OK;
}


int X2Camera::CCQueryTemperature(double& dCurTemp, double& dCurPower, char* lpszPower, const int nMaxLen, bool& bCurEnabled, double& dCurSetPoint)
{   
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

	long lVal;
	ASI_BOOL bAuto;
	ASIGetControlValue(ASICameraInfo.CameraID, ASI_TEMPERATURE, &lVal, &bAuto);
	dCurTemp = lVal/10.0;
	if(ASICameraInfo.IsCoolerCam)
	{
		ASIGetControlValue(ASICameraInfo.CameraID, ASI_COOLER_POWER_PERC, &lVal, &bAuto);
		dCurPower = lVal;
		ASIGetControlValue(ASICameraInfo.CameraID, ASI_TARGET_TEMP, &lVal, &bAuto);
		dCurSetPoint = lVal;
	}
	else
	{
		dCurPower = -100;
		dCurSetPoint = -100;
	}
	
	char buf[32];
	strcpy(buf, "cooler power percent");
	
	strncpy(lpszPower, buf, min(strlen(buf), nMaxLen));

	ASIGetControlValue(ASICameraInfo.CameraID, ASI_COOLER_ON, &lVal, &bAuto);
	bCurEnabled = (lVal > 0);
	
	return SB_OK;
}

int X2Camera::CCRegulateTemp(const bool& bOn, const double& dTemp)
{ 
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

	ASISetControlValue(ASICameraInfo.CameraID, ASI_COOLER_ON, bOn, ASI_FALSE);
	ASISetControlValue(ASICameraInfo.CameraID, ASI_TARGET_TEMP, dTemp, ASI_FALSE);

	return SB_OK;
}

int X2Camera::CCGetRecommendedSetpoint(double& RecTemp)
{
	X2MutexLocker ml(GetMutex());

	if(ASICameraInfo.IsCoolerCam)
	{
		RecTemp = 0;//Set to 100 if you cannot recommend a setpoint
		return SB_OK;
	}
	else
	{
		RecTemp = 100;//Set to 100 if you cannot recommend a setpoint
		return ERR_NOT_IMPL;
	}
}  

//20180925 video时需要先停止再设置RIO
bool X2Camera::setCamROI(int width, int height, int bin, ASI_IMG_TYPE type)
{
	if(isVideoMode)//20180925 设置ROI时video不会被App停止，需要自己停止
		ASIStopVideoCapture(ASICameraInfo.CameraID);
	return ASISetROIFormat(ASICameraInfo.CameraID, width, height, bin, type) == ASI_SUCCESS;
}

//曝光前的准备，设置RIO
void X2Camera::prepareExposure()
{
	ASI_IMG_TYPE imgtype;
	int wid, hei, bin, startx, stary;
	ASIGetROIFormat(ASICameraInfo.CameraID, &wid, &hei, &bin, &imgtype);//13

	int x, y;
	long lHardbin;
	ASI_BOOL bAuto;
	ASIGetControlValue(ASICameraInfo.CameraID, ASI_HARDWARE_BIN, &lHardbin, &bAuto);

	if(lHardbin != oldHardbin || iBin != oldBin)
	{
		if(lHardbin)//硬件bin
		{
			iWidthMax = ASICameraInfo.MaxWidth/iBin/8*8;
			iHeightMax = ASICameraInfo.MaxHeight/iBin/2*2;
		}
		else
		{
			for (x = ASICameraInfo.MaxWidth / iBin ; x > 0; x --)
				if (x * iBin % 8 == 0)
					break;
			iWidthMax = x;
			for (y = ASICameraInfo.MaxHeight / iBin ; y > 0; y --)
				if (y * iBin % 2 == 0)
					break;
			iHeightMax = y;
		}
	}



	if(bin != iBin)//120 要满足1024，先设到最大
		if(setCamROI(iWidthMax, iHeightMax, iBin, imgtype))
		{
			oldWidth = iWidthMax;
			oldHeight = iHeightMax;
			oldBin = iBin;
		}
	
	if(iWidthROI != oldWidth || startX_display != oldStartX_display)
	{
		if (iWidthROI < 128)//20151210 ASCOM里每一次曝光前都设置NumX和NumY,但AstroArt没有,width_Display设置后不变,但startexposure()后width变成实际值而错误
			iWidthROI = (startX_display % 4 ? 4 : 0 ) + 128;
		else
		{
			if(lHardbin)
				iWidthROI = (startX_display % 4 ? 4 : 0) + (iWidthROI % 8 ? 8 : 0) + iWidthROI/8*8;//去黑边，使实际ROI略大于显示值,（start坐标取4倍后减小，和长宽取4倍减小
			{
				iWidthROI = (startX_display % 4 ? 4 : 0) + iWidthROI;
				for (x = iWidthROI ; x < iWidthMax; x ++)//20161010找到比显示尺寸大的可用宽度
					if (x * iBin % 8 == 0)//20161010 SDK里改成width*bin是8整数倍有效
					{
						iWidthROI = x;
						break;
					}
			}
		}	
	}
	if(iHeightROI != oldHeight ||  startY_display != oldStartY_display)
	{
		if (iHeightROI < 128)
			iHeightROI = (startY_display % 2 ? 2 : 0) + 128; 
		else
		{
			if(lHardbin)
				iHeightROI = (startY_display % 2 ? 2 : 0) + (iHeightROI % 2 ? 2 : 0) + iHeightROI/2*2;
			else
			{
				iHeightROI = (startY_display % 2 ? 2 : 0) + iHeightROI;// + (height_Display % 2 == 0 ? 0 : 2) + height_Display / 2 * 2;//这样裁剪后没有黑边
				for (y = iHeightROI ; y < iHeightMax; y ++)
					if (y * iBin % 2 == 0)
					{
						iHeightROI = y;
						break;
					}
			}
		}
	}

	if (iWidthROI > iWidthMax)//可能会大于最大尺寸,做限制
		iWidthROI = iWidthMax;
	if (iHeightROI > iHeightMax)
		iHeightROI = iHeightMax;
	if(imgtype != ImgTypeSetting || iWidthROI != wid || iHeightROI != hei )
		if(setCamROI(iWidthROI, iHeightROI, iBin, ImgTypeSetting))
		{
			oldWidth = iWidthROI;
			oldHeight = iHeightROI;
			oldBin = iBin;
		}
	ASIGetStartPos(ASICameraInfo.CameraID, &startx, &stary);
	if(startx != iStartX || stary != iStartY)
		if(ASISetStartPos(ASICameraInfo.CameraID, iStartX, iStartY) == ASI_SUCCESS)
		{
			oldStartX_display = startX_display;
			oldStartY_display = startY_display;
		}
	
	oldHardbin = lHardbin;	
}

int X2Camera::CCStartExposure(const enumCameraIndex& Cam, const enumWhichCCD CCD, const double& dTime, enumPictureType Type, const int& nABGState, const bool& bLeaveShutterAlone)
{   
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

	bool bLight = true;

	switch (Type)
	{
	case PT_FLAT:
	case PT_LIGHT:			bLight = true;	break;
	case PT_DARK:	
	case PT_AUTODARK:	
	case PT_BIAS:			bLight = false;	break;
	default:				return ERR_CMDFAILED;
	}

	prepareExposure();

	if (m_pTickCount)
		dwStartMs = m_pTickCount->elapsed();//ms
	//		m_dwFin = (unsigned long)(dTime*1000)+m_pTickCount->elapsed();//ms
	else
		dwStartMs = 0;

	ASI_BOOL bAuto;
	ASIGetControlValue(ASICameraInfo.CameraID, ASI_EXPOSURE, &lExpMs, &bAuto);
	lExpMs /= 1000;
	if(lExpMs != dTime*1000)
	{
		ASISetControlValue(ASICameraInfo.CameraID, ASI_EXPOSURE, dTime*1000*1000, ASI_FALSE);
		ASIGetControlValue(ASICameraInfo.CameraID, ASI_EXPOSURE, &lExpMs, &bAuto);
		lExpMs /= 1000;
	}

	ASIStartExposure(ASICameraInfo.CameraID, ASI_FALSE);
	m_failedcount = 0;
	return SB_OK;
}

int X2Camera::CCIsExposureComplete(const enumCameraIndex& Cam, const enumWhichCCD CCD, bool* pbComplete, unsigned int* pStatus)
{   
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;
	
	if(!isVideoMode)
	{
		if (m_pTickCount)
		{
			if (m_pTickCount->elapsed() - dwStartMs > 10000 && m_pTickCount->elapsed() - dwStartMs > 3*lExpMs )
			{
				ASIStopExposure(ASICameraInfo.CameraID);//20160429 stop后变成曝光状态变成IDLE， CCReadoutImage会返回超时错误，大分辨率+带宽小+16bit 的读取时间会比较长
				*pbComplete = true;
				return SB_OK;//20160603超时 直接返回
			}
		}
		else
			*pbComplete = true;
		ASI_EXPOSURE_STATUS ExpStatus;
		ASIGetExpStatus(ASICameraInfo.CameraID, &ExpStatus);
		if(ExpStatus == ASI_EXP_WORKING)
			*pbComplete = false;
		else
		{
			if (ExpStatus == ASI_EXP_FAILED)
			{

				if (m_failedcount++ < 3)//最多试3次
				{
					OutputDbgPrint("snap image failed, Retry %d!\n", m_failedcount);
					ASIStartExposure(ASICameraInfo.CameraID, ASI_FALSE);
					*pbComplete = false;
				}
				else//超过3次
				{
					//	state = CameraStates.cameraIdle;//the sky需要，以停止等待
					*pbComplete = true;
				}
			}
			else
				*pbComplete = true;
		}
	}
	else
	{
		*pbComplete = true;
	}


	return SB_OK;
}
long X2Camera::GetBufLen()
{
	int wid, hei, bin;
	ASI_IMG_TYPE img;
	ASIGetROIFormat(ASICameraInfo.CameraID, &wid, &hei, &bin, &img);
	switch (img)
	{
	case ASI_IMG_RAW8:
	case  ASI_IMG_Y8:
		return wid*hei;
	case ASI_IMG_RAW16:
		return wid*hei*2;
	default:
		return 0;
	
	}
}

int X2Camera::CCEndExposure(const enumCameraIndex& Cam, const enumWhichCCD CCD, const bool& bWasAborted, const bool& bLeaveShutterAlone)           
{   
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

	int nErr = SB_OK;

	if (bWasAborted)
	{
		ASIStopExposure(ASICameraInfo.CameraID);
	}

	if(!isVideoMode)
	{
		ASI_EXPOSURE_STATUS ExpStatus;
		ASIGetExpStatus(ASICameraInfo.CameraID, &ExpStatus);
		if(ExpStatus == ASI_EXP_SUCCESS)
		{
			if(lBufferLen < GetBufLen() || !pImg)
			{
				DeleteImgBuf();
				pImg = new unsigned char[GetBufLen()];
				lBufferLen = GetBufLen();
			}
			ASIGetDataAfterExp(ASICameraInfo.CameraID, pImg, lBufferLen);//读出图片后 会变成IDLE
			bExpSuccess = true;
		}
	}
	
	return nErr;
}
void X2Camera::DeleteImgBuf()
{
	if(pImg)
	{
		delete[] pImg;
		pImg = 0;
		lBufferLen = 0;
		OutputDbgPrint("clr\n");
	}
}
int X2Camera::CCGetChipSize(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nXBin, const int& nYBin, const bool& bOffChipBinning, int& nW, int& nH, int& nReadOut)
{
	X2MutexLocker ml(GetMutex());

	nW = ASICameraInfo.MaxWidth/nXBin;
	nH = ASICameraInfo.MaxHeight/nYBin;	
	iBin = nXBin;

	return SB_OK;
}

int X2Camera::CCGetNumBins(const enumCameraIndex& Camera, const enumWhichCCD& CCD, int& nNumBins)
{
	X2MutexLocker ml(GetMutex());
	if(isLinked())
	{
		int i = 0;
		while(ASICameraInfo.SupportedBins[i] != 0)
			i++;
		nNumBins = i;
	}
	else
		nNumBins = 1;
	

	return SB_OK;
}

int X2Camera::CCGetBinSizeFromIndex(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nIndex, long& nBincx, long& nBincy)
{
	X2MutexLocker ml(GetMutex());
	if(isLinked())
		nBincx = nBincy = ASICameraInfo.SupportedBins[nIndex];
	else
		nBincx = nBincy = 1;
	return SB_OK;
}

int X2Camera::CCUpdateClock(void)
{   
	X2MutexLocker ml(GetMutex());

	return SB_OK;
}

int X2Camera::CCSetShutter(bool bOpen)           
{   
	X2MutexLocker ml(GetMutex());

	return SB_OK;;
}

int X2Camera::CCActivateRelays(const int& nXPlus, const int& nXMinus, const int& nYPlus, const int& nYMinus, const bool& bSynchronous, const bool& bAbort, const bool& bEndThread)
{   
	X2MutexLocker ml(GetMutex());

	// If bSychronous is true, block until time is finished
	// If bSynchronous is false, someone is pressing the button and I get a zero when the button goes up

	OutputDbgPrint("nXPlus%d, nXMinus%d, nYPlus%d, nYMinus%d, bSynchronous%d,  bAbort%d, bEndThread%d\n",
		nXPlus, nXMinus, nYPlus, nYMinus, bSynchronous, bAbort, bEndThread);
	// Stop moving
	if(bAbort)
	{
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_EAST);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_WEST);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_SOUTH);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_NORTH);
		return SB_OK;
	}

	// North
	if(nYPlus != 0)
		ASIPulseGuideOn(ASICameraInfo.CameraID, ASI_GUIDE_NORTH);
	else
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_NORTH);

	// South
	if(nYMinus != 0)
		ASIPulseGuideOn(ASICameraInfo.CameraID, ASI_GUIDE_SOUTH);
	else
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_SOUTH);

	// East
	if(nXPlus != 0)
		ASIPulseGuideOn(ASICameraInfo.CameraID, ASI_GUIDE_EAST);
	else
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_EAST);

	// West
	if(nXMinus)
		ASIPulseGuideOn(ASICameraInfo.CameraID, ASI_GUIDE_WEST);
	else
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_WEST);
	// User is presssing and releasing buttons in the GUI
	if(!bSynchronous)
	{
		// Move, stop, whatever
		return SB_OK;
	}


	////////////////////////////////////////////////////////////////////
	// We are guiding or calibrating
	// Turn on flags for any values that are not == to zero (do integer compare)



	// One of these will always be zero, so this gives me the net
	// plus or minus movement
	int netX = nXPlus - nXMinus;
	int netY = nYPlus - nYMinus;

	unsigned long guidStartMs;
	if (m_pTickCount)
		guidStartMs = m_pTickCount->elapsed();//ms
	else
		return ERR_CMDFAILED;
	// Three cases
	if(netX == 0)
		// netY will not be zero
	{
		// One of nYPLus and nYMinus will be zero, so this expression will work
		int timeToWaitMs = (nYPlus + nYMinus)*10;
		// Just wait for time to expire and stop relay
		while(m_pTickCount->elapsed() - guidStartMs < timeToWaitMs);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_EAST);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_WEST);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_SOUTH);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_NORTH);
		return SB_OK;
	}

	if(netY == 0)
		// netX will not be zero
	{
		// Again, one of these will be zero
		int timeToWaitMs = (nXPlus + nXMinus)*10;
		// Just wait for time to expire and stop relay
		while(m_pTickCount->elapsed() - guidStartMs < timeToWaitMs);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_EAST);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_WEST);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_SOUTH);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_NORTH);
		return SB_OK;
	}

	// The most interesting case... multiple axis movment!
	// Are they both the same? If so, wait and then terminate both at same time
	if(abs(netY) == abs(netX))
	{
		// Pick one, doesn't matter which
		int timeToWaitMs = (nXPlus + nXMinus)*10;
		// Just wait for time to expire and stop relay
		while(m_pTickCount->elapsed() - guidStartMs < timeToWaitMs);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_EAST);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_WEST);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_SOUTH);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_NORTH);
		return SB_OK;
	}

	// Finally, the more interesting case, dual axis movement, not the same time for each!
	if(abs(netY) < abs(netX)) // East-West movement was greater
	{
		// Wait for shorter time
		int timeToWaitMs = (nYPlus + nYMinus)*10;
		while(m_pTickCount->elapsed() - guidStartMs < timeToWaitMs);

		// Turn off Y direction only
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_SOUTH);			
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_NORTH);


		// Longer time
		timeToWaitMs = (nXPlus + nXMinus)*10;
		while(m_pTickCount->elapsed() - guidStartMs < timeToWaitMs);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_EAST);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_WEST);
		return SB_OK;
	}
	else
	{ // North-South movement was greater
		// Wait for shorter time
		int timeToWaitMs = (nXPlus + nXMinus)*10;
		while(m_pTickCount->elapsed() - guidStartMs < timeToWaitMs);

		// Turn off Y direction only
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_EAST);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_WEST);

		// Longer time
		timeToWaitMs = (nYPlus + nYMinus)*10;
		while(m_pTickCount->elapsed() - guidStartMs < timeToWaitMs);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_SOUTH);
		ASIPulseGuideOff(ASICameraInfo.CameraID, ASI_GUIDE_NORTH);
		return SB_OK;
	}


	return SB_OK;
}



int X2Camera::CCPulseOut(unsigned int nPulse, bool bAdjust, const enumCameraIndex& Cam)
{   
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}

void X2Camera::CCBeforeDownload(const enumCameraIndex& Cam, const enumWhichCCD& CCD)
{
	X2MutexLocker ml(GetMutex());
}

void X2Camera::CCAfterDownload(const enumCameraIndex& Cam, const enumWhichCCD& CCD)
{
	X2MutexLocker ml(GetMutex());
	return;
}

int X2Camera::CCReadoutLine(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& pixelStart, const int& pixelLength, const int& nReadoutMode, unsigned char* pMem)
{   
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}           

int X2Camera::CCDumpLines(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& nReadoutMode, const unsigned int& lines)
{                                     
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}           


int X2Camera::CCReadoutImage(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& nWidth, const int& nHeight, const int& nMemWidth, unsigned char* pMem)
{
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

	unsigned short* pBuf16, *pBuf16From;
	ASI_EXPOSURE_STATUS ExpStatus;
/*
	ASIGetExpStatus(iCamIndex, &ExpStatus);
	if(ExpStatus != ASI_EXP_SUCCESS)
	return ERR_RXTIMEOUT;*/

	if(isVideoMode)
	{
		if(lBufferLen < GetBufLen() || !pImg)
		{
			DeleteImgBuf();
			pImg = new unsigned char[GetBufLen()];
			lBufferLen = GetBufLen();
		}
		if(ASIGetVideoData(ASICameraInfo.CameraID, pImg, lBufferLen, lVideoWaitMs) == ASI_SUCCESS)
			bExpSuccess = true;
		else 
			bExpSuccess = false;
	}
	
	if(!bExpSuccess)//20160603曝光失败 没得到图像
		return ERR_RXTIMEOUT;

	bExpSuccess = false;

	ASIGetStartPos(ASICameraInfo.CameraID, &iStartX, &iStartY);

	ASI_IMG_TYPE imgType;
	ASIGetROIFormat(ASICameraInfo.CameraID, &iWidthROI, &iHeightROI, &iBin, &imgType);


	int min_width, min_height;//根据起始坐标和长宽尺寸 从实际得到图像里取出要显示的区域
	int iPreCal = 0;
	int offsetX = 0, offsetY = 0;
	if (startX_display > iStartX)
		offsetX = startX_display - iStartX;
	if (startY_display > iStartY)
		offsetY = startY_display - iStartY;

	min_width = min(iStartX + iWidthROI, startX_display + nWidth) - max(iStartX, startX_display);
	min_height = min(iStartY + iHeightROI, startY_display + nHeight) - max(iStartY, startY_display);


	if(nMemWidth/nWidth == 2 && imgType != ASI_IMG_RAW16)//theSkyX都是按16bit显示
	{
		
		for(int y = 0; y < min_height; y++)//8bit数据
			{
				pBuf16 = (unsigned short*)(pMem + y*nMemWidth);
				iPreCal= iWidthROI * (offsetY + y) + offsetX;
				for(int x = 0; x < min_width; x++)				
					pBuf16[x] = pImg[iPreCal + x];
				
		}
	}
	else if(imgType == ASI_IMG_RAW16)
	{
		for(int y = 0; y < min_height; y++)
		{
			pBuf16 = (unsigned short*)(pMem + y*nMemWidth);
			pBuf16From = (unsigned short*)pImg;
			iPreCal= iWidthROI * (offsetY + y) + offsetX;
			memcpy(pBuf16, pBuf16From + iPreCal, min_width*2);		
		}
		
	}
	else
	{
		for(int y = 0; y < min_height; y++)//一般用不到
		{
			iPreCal= iWidthROI * (offsetY + y) + offsetX;
			memcpy(pMem + y*nMemWidth, pImg + iPreCal, min_width);
		}
	}
	


	return SB_OK;
}

int X2Camera::CCDisconnect(const bool bShutDownTemp)
{
	X2MutexLocker ml(GetMutex());

	if (m_bLinked)
	{
		setLinked(false);
		CloseCam();
		ASICameraInfo.CameraID = -1;

	}

	return SB_OK;
}

int X2Camera::CCSetImageProps(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nReadOut, void* pImage)
{
	X2MutexLocker ml(GetMutex());

	return SB_OK;
}

int X2Camera::CCGetFullDynamicRange(const enumCameraIndex& Camera, const enumWhichCCD& CCD, unsigned long& dwDynRg)
{
	X2MutexLocker ml(GetMutex());

	return SB_OK;
}

void X2Camera::CCMakeExposureState(int* pnState, enumCameraIndex Cam, int nXBin, int nYBin, int abg, bool bRapidReadout)
{
	X2MutexLocker ml(GetMutex());

	return;
}

int X2Camera::CCSetBinnedSubFrame(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nLeft, const int& nTop, const int& nRight, const int& nBottom)
{
	X2MutexLocker ml(GetMutex());


	iWidthROI = nRight - nLeft + 1;
	iHeightROI = nBottom - nTop + 1;	
//	iWidthROI = iWidthROI/4*4;
//	iHeightROI = iHeightROI/2*2;
	startX_display = iStartX = nLeft;
	startY_display = iStartY = nTop;	

	return SB_OK;
}
int X2Camera::CCSetBinnedSubFrame3(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, const int& nLeft, const int& nTop, const int& nWidth, const int& nHeight)
{
	X2MutexLocker ml(GetMutex());

	startX_display = iStartX = nLeft;
	startY_display = iStartY = nTop;

	iWidthROI = nWidth;
	iHeightROI = nHeight;

//	iWidthROI = iWidthROI/4*4;//774->772
//	iHeightROI = iHeightROI/2*2;//

	return SB_OK;
}

int X2Camera::CCSettings(const enumCameraIndex& Camera, const enumWhichCCD& CCD)
{
	X2MutexLocker ml(GetMutex());

	return ERR_NOT_IMPL;
}

int X2Camera::CCSetFan(const bool& bOn)
{
	X2MutexLocker ml(GetMutex());

	return SB_OK;
}
int X2Camera::CCHasShutter(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, bool &bHasShutter) 
{ 
	X2MutexLocker ml(GetMutex());

	if(isLinked())
		bHasShutter = (ASICameraInfo.MechanicalShutter == ASI_TRUE); 
	else
		bHasShutter = false; 
	return SB_OK;  
}
int X2Camera::PixelSize1x1InMicrons( const enumCameraIndex & Camera, const enumWhichCCD & CCD, double & x, double & y)
{
	X2MutexLocker ml(GetMutex());
	if(isLinked())	
		x = y = ASICameraInfo.PixelSize;	
	else
		x = y = 0; 
	return SB_OK;  
}

int X2Camera::countOfIntegerFields(int &nCount)
{
	if (!m_bLinked)
		return ERR_NOLINK;

	nCount = 3;
	return SB_OK;
}
int X2Camera::valueForIntegerField(int nIndex, BasicStringInterface& sFieldName, BasicStringInterface& sFieldComment, int &nFieldValue)
{
	X2MutexLocker ml(GetMutex());
	if (!m_bLinked)
		return ERR_NOLINK;

	long lVal = 0;
	ASI_BOOL bAuto;
	
	switch(nIndex)
	{
	case 0:
		sFieldName = "Offset";
		sFieldComment = "camera offset";
		if(ASIGetControlValue(ASICameraInfo.CameraID, ASI_OFFSET, &lVal, &bAuto) == ASI_SUCCESS)
			nFieldValue = lVal;
		break;
	case 1:
		sFieldName = "GAINRAW";
		sFieldComment = "Your gain value (integer)";
		if(ASIGetControlValue(ASICameraInfo.CameraID, ASI_GAIN, &lVal, &bAuto) == ASI_SUCCESS)
			nFieldValue = lVal;	
		break;
	case 2:
		sFieldName = "Bandwidth setting";
		sFieldComment = "Your gain value (integer)";
		if(ASIGetControlValue(ASICameraInfo.CameraID, ASI_BANDWIDTHOVERLOAD, &lVal, &bAuto) == ASI_SUCCESS)
			nFieldValue = lVal;	
		break;
	}
	return SB_OK;
}
int X2Camera::countOfDoubleFields (int &nCount)
{
	if (!m_bLinked)
		return ERR_NOLINK;

	nCount = 1;
	return SB_OK;
}
int X2Camera::valueForDoubleField (int nIndex, BasicStringInterface& sFieldName, BasicStringInterface& sFieldComment, double &dFieldValue)
{
	X2MutexLocker ml(GetMutex());
	switch(nIndex)
	{
	case 0:
		ASIGetCameraPropertyByID(ASICameraInfo.CameraID, &ASICameraInfo);
		sFieldName = "GAINADU";
		sFieldComment = "e/ADU";
		dFieldValue = ASICameraInfo.ElecPerADU;
		break;	
	}

	return SB_OK;
}

int X2Camera::countOfStringFields (int &nCount)
{
	X2MutexLocker ml(GetMutex());
	if (!m_bLinked)
		return ERR_NOLINK;

	nCount = 0;

	if(ASICameraInfo.IsColorCam == ASI_TRUE)
	{
		nCount = 3;

		long lVal;
		ASI_BOOL bAuto;
		if(ASIGetControlValue(ASICameraInfo.CameraID, ASI_MONO_BIN, &lVal, &bAuto) == ASI_SUCCESS && lVal > 0)
		{			
			ASI_IMG_TYPE imgtype;
			int wid, hei, bin;
			if(ASIGetROIFormat(ASICameraInfo.CameraID, &wid, &hei, &bin, &imgtype) == ASI_SUCCESS && bin > 1)
				nCount = 0;			

		}
	}	
	
	return SB_OK;
}
/*
mono_bin	bin	count
	0		1	3
	0		>1	3
	1		1	3
	1		>1	0

*/
int X2Camera::valueForStringField (int nIndex, BasicStringInterface& sFieldName, BasicStringInterface& sFieldComment, BasicStringInterface &sFieldValue)
{
	if (!m_bLinked)
		return ERR_NOLINK;

	long lVal;
	ASI_BOOL bAuto;

	if(nIndex == 0 || nIndex == 1 || nIndex == 2)
	{
		if(nIndex == 0)		
			sFieldName = "DEBAYER";
		else if(nIndex == 1)		
			sFieldName = "BAYERPAT";
		else if(nIndex == 2)		
			sFieldName = "COLORTYP";

		sFieldComment = "sensor BAYER matrix pattern";		
		switch (ASICameraInfo.BayerPattern)
		{
		case ASI_BAYER_RG:
			sFieldValue = "RGGB";
			break;
		case ASI_BAYER_GR:
			sFieldValue = "GRBG";
			break;
		case ASI_BAYER_BG:
			sFieldValue = "BGGR";
			break;
		case ASI_BAYER_GB:
			sFieldValue = "GBRG";
			break;
		}		
	
	}
	return SB_OK;
}
int	X2Camera::pathTo_rm_FitsOnDisk(char* lpszPath, const int& nPathSize)
{
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

	//Just give a file path to a FITS and TheSkyX will load it
		
	return SB_OK;
}

CameraDriverInterface::ReadOutMode X2Camera::readoutMode(void)		
{
	X2MutexLocker ml(GetMutex());

	return CameraDriverInterface::rm_Image;
}


enumCameraIndex	X2Camera::cameraId()
{
	X2MutexLocker ml(GetMutex());

	return m_Camera;
}

void X2Camera::setCameraId(enumCameraIndex Cam)	
{
	X2MutexLocker ml(GetMutex());
	m_Camera = Cam;
}

void X2Camera::refreshControlVal(X2GUIExchangeInterface*	pdx, ASI_CONTROL_CAPS CtrlCaps, int itemIndex)
{

	long lVal;
	lVal = ComboList[itemIndex].lVal;

	if(iPreComboIndex > -1)
	{
		char buf[64];	
		pdx->text("spinBox", buf, 64);	
		ComboList[iPreComboIndex].lVal = atol(buf);
	
	}
	iPreComboIndex = itemIndex;
	
	char buf[256] = {0};
	pdx->setPropertyDouble("spinBox","maximum", CtrlCaps.MaxValue);
	pdx->setPropertyDouble("spinBox","minimum", CtrlCaps.MinValue);
	pdx->setPropertyDouble("spinBox","singleStep", 1);
	pdx->setPropertyDouble("spinBox","value", lVal);
	sprintf(buf, "%s ,Scale: [%d, %d]", CtrlCaps.Description, CtrlCaps.MinValue, CtrlCaps.MaxValue);
	pdx->setText("label_2", buf);
}

static ASI_CONTROL_TYPE CtrlTypeShow[] = {
	ASI_GAIN,
	ASI_BRIGHTNESS,// return 10*temperature
	ASI_BANDWIDTHOVERLOAD,
	ASI_ANTI_DEW_HEATER,
	ASI_MONO_BIN
	
};	
void X2Camera::refreshControl(X2GUIExchangeInterface*	pdx)//把支持的控件插入到combobox,并刷新选定control的值和描述等
{
	pdx->invokeMethod("comboBox_3","clear");

	for(int i = 0; i<iComboCount; i++) 
	{
		pdx->comboBoxAppendString("comboBox_3", ComboList[i].CtlCap.Name);//调用on_comboBox_3_currentIndexChanged
	}

}
void X2Camera::refreshImgType(X2GUIExchangeInterface*	pdx, ASI_IMG_TYPE imgType)
{
	

	if(ASICameraInfo.CameraID < 0)
		return;

	int i = 0, iCount = 0;
	char sort[8];
	pdx->invokeMethod("comboBox_2","clear");
	while(ASICameraInfo.SupportedVideoFormat[i] != ASI_IMG_END)
	{
		
		switch (ASICameraInfo.SupportedVideoFormat[i])
		{
		case ASI_IMG_RAW8:
			pdx->comboBoxAppendString("comboBox_2", "RAW8");
			ImgTypeArray[iCount] = ASICameraInfo.SupportedVideoFormat[i];
			sort[ASICameraInfo.SupportedVideoFormat[i]] = iCount++;
			break;
		case ASI_IMG_Y8:
			pdx->comboBoxAppendString("comboBox_2", "Y8");
			ImgTypeArray[iCount] = ASICameraInfo.SupportedVideoFormat[i];
			sort[ASICameraInfo.SupportedVideoFormat[i]] = iCount++;
			break;
/* 		case ASI_IMG_RGB24:
 			pdx->comboBoxAppendString("comboBox_2", "color");
			sort[ASI_IMG_RGB24] = iCount++;
*/ 			break;
		case ASI_IMG_RAW16:
			pdx->comboBoxAppendString("comboBox_2", "RAW16");
			ImgTypeArray[iCount] = ASICameraInfo.SupportedVideoFormat[i];
			sort[ASICameraInfo.SupportedVideoFormat[i]] = iCount++;
			break;
		}
		i++;
	}	
	pdx->setCurrentIndex("comboBox_2", sort[(int)imgType]);

}
void X2Camera::CloseCam()
{
	ASICloseCamera(ASICameraInfo.CameraID);
	DeleteImgBuf();
}
int X2Camera::execModalSettingsDialog()
{
	int nErr = SB_OK;
	int iCamNum = 0;
	if ((iCamNum = ASIGetNumOfConnectedCameras()) > 0)
	{		
		if (isLinked())//已经选定ID，要找到对应的相机序号
		{
			int ctrlNum;		
			if(ASIGetNumOfControls(ASICameraInfo.CameraID, &ctrlNum)!= ASI_SUCCESS)//找到和已打开的相同的ID的,必须要打开的
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

	if (nErr = ui->loadUserInterface("ASI.ui", deviceType(), m_nPrivateISIndex))
		return nErr;

	if (NULL == (dx = uiutil.X2DX()))
		return ERR_POINTER;

	//Intialize the user interface
	
	
	ASI_CAMERA_INFO CamInfoTemp;
	int i;
	iPreComboIndex = -1;	

	int CtrlNum;
	char szBuf[256];
	dx->invokeMethod("comboBox","clear");
	int itemIndex = -1;
	for(i = 0; i <iCamNum;  i++)
	{	
		ASIGetCameraProperty( &CamInfoTemp, i);
		sprintf(szBuf, "%s (ID %d", CamInfoTemp.Name, CamInfoTemp.CameraID);
		if (isLinked())
		{
			if(CamInfoTemp.CameraID == ASICameraInfo.CameraID)
				itemIndex = i;
		}
		else
		{
			if(ASIGetNumOfControls(CamInfoTemp.CameraID, &CtrlNum) == ASI_SUCCESS)//打开
				strcat(szBuf, " opened");
			else if (itemIndex < 0)
				itemIndex = i;
		}
		
		strcat(szBuf, ")");
		dx->comboBoxAppendString("comboBox", szBuf);//下拉框插入新项, 调用UIevent()->on_comboBox_currentIndexChanged, index = 0	
	}	
	if (itemIndex < 0)
		itemIndex = 0;
	if (iCamNum > itemIndex && itemIndex >= 0)	
	{		
		if(isLinked())//已经有连接
			dx->setEnabled("comboBox", false);
		else		
			dx->setEnabled("comboBox", true);
		dx->setCurrentIndex("comboBox",itemIndex);//如果index!=0, 调用UIevent()->on_comboBox_currentIndexChanged
	}

	


	//Display the user interface
	if (nErr = ui->exec(bPressedOK))//有错误
	{
		if (!isLinked())
		{
			CloseCam();
		}		
		return nErr;
	}	
	
	//Retreive values from the user interface
	if (bPressedOK)
	{	
		char buf[64];
		dx->text("spinBox", buf, 64);
		ComboList[iPreComboIndex].lVal = atol(buf);
		ImgTypeSetting = ImgTypeArray[dx->currentIndex("comboBox_2")];
		strcpy(szCamNameSetting, ASICameraInfo.Name);//按“确定”才记录选择的型号
		memcpy(ComboListSetting, ComboList, sizeof(CtrlCombo)*iComboCount);//control值记录到setting里
		iCamIDSetting = iCamIDTemp;

		//20171221 保存设置到XML文件, 类似ASCOM里
		saveXMLSetting(ASICameraInfo);

		if(isLinked())//已经连接的 按OK后 把control和imagetype的值设置好
		{
			for (int i = 0; i < iComboCount; i++)
				ASISetControlValue(ASICameraInfo.CameraID, ComboListSetting[i].CtlCap.ControlType, ComboListSetting[i].lVal, ASI_FALSE);		

			ASI_IMG_TYPE imgtype;
			int wid, hei, bin;
			ASIGetROIFormat(ASICameraInfo.CameraID, &wid, &hei, &bin, &imgtype);
			if(imgtype != ImgTypeSetting)
			{				
				setCamROI(wid, hei, bin, ImgTypeSetting);//否则要到StartExposure里设置，这样按了take photo后才生效
			}
		}
		
	}
	if (!isLinked()  && oldCamID > -1)
		ASICloseCamera(oldCamID);

	return nErr;
}
void X2Camera::setShownCtrlVal(ASI_CONTROL_TYPE ctrlType, int iVal)
{
	for(int i = 0; i<iComboCount; i++) 
	{				
		if(ComboList[i].CtlCap.ControlType == ctrlType)
		{
			ComboList[i].lVal = iVal;
			break;
		}
	}
}

bool X2Camera::isShow(ASI_CONTROL_TYPE ctrltype)
{
	
	for(int j = 0; j < sizeof(CtrlTypeShow)/sizeof(ASI_CONTROL_TYPE); j++)
	{
		if(ctrltype == CtrlTypeShow[j])		
			return true;
		
	}
	return false;
}

void X2Camera::ReadShownCtrlCaps()//刷新用来显示的控件值,有些是不显示的,比如温度和制冷
{
	iComboCount = 0;                
	long lVal;
	ASI_BOOL bAuto;
	ASI_CONTROL_CAPS CtrlCap;
	int iCtrlNum, ComboListLen;
	ComboListLen = sizeof(ComboList)/sizeof(CtrlCombo);
	if (ASIGetNumOfControls(ASICameraInfo.CameraID, &iCtrlNum) != ASI_SUCCESS)
		return ;	
	
	for(int i = 0; i<iCtrlNum; i++)
	{
		ASIGetControlCaps(ASICameraInfo.CameraID, i, &CtrlCap);
		
		if(isShow(CtrlCap.ControlType))
		{
			if(iComboCount < ComboListLen)
			{			
				ComboList[iComboCount].CtlCap = CtrlCap;
				if(ASIGetControlValue(ASICameraInfo.CameraID, CtrlCap.ControlType, &lVal, &bAuto) == ASI_SUCCESS)
					ComboList[iComboCount].lVal = lVal;
				iComboCount++;
			}
			else
				break;
		}
	}					
}
void X2Camera::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
	char szEvt[DRIVER_MAX_STRING];

	sprintf(szEvt, pszEvent);

	//An example of showing another modal dialog
	OutputDbgPrint("%s\n", szEvt);
	if(!strcmp(szEvt,"on_comboBox_currentIndexChanged") )//combobox camera, 收到index改变的事件
	{
		ASI_IMG_TYPE imgType;
		if(!isLinked())//没有连接
		{
			if(oldCamID > -1)//打开之前的
				ASICloseCamera(ASICameraInfo.CameraID);

			int CamIndex = uiex->currentIndex("comboBox");
			ASIGetCameraProperty(&ASICameraInfo, CamIndex);
			iCamIDTemp = ASICameraInfo.CameraID;
			int CtrlNum;
			if (ASIGetNumOfControls(ASICameraInfo.CameraID, &CtrlNum) == ASI_SUCCESS)
				oldCamID = -1;//20160218没打开前就可以可以操作，说明其他ASCOM打开了，不要关闭
			else
				oldCamID = ASICameraInfo.CameraID;

			OutputDbgPrint("iCamIndex %d, ID: %d\n", CamIndex, ASICameraInfo.CameraID);				
			if(ASIOpenCamera(ASICameraInfo.CameraID) != ASI_SUCCESS)
				return;			
			
		//	if(strcmp(ASICameraInfo.Name, szCamNameSetting))//没有设置过, 部分参数用默认值
			if(!loadXMLSetting(ASICameraInfo))
			{
				//20160622 没设置过, 用默认值; 
				imgType = isImgTypeSupported(&ASICameraInfo, ASI_IMG_RAW16)?ASI_IMG_RAW16:ASI_IMG_RAW8;//选择摄像头时 设置为默认RAW16									

				ReadShownCtrlCaps();//读取参数
				setShownCtrlVal( ASI_BANDWIDTHOVERLOAD, getDefaultUSBValue(ASICameraInfo));

				//20161213 没有设置过的话 除雾显示为打开
				setShownCtrlVal( ASI_ANTI_DEW_HEATER, 1);
			}
			else//设置过, 显示为setting的值
			{
				memcpy(ComboList, ComboListSetting, sizeof(CtrlCombo)*iComboCount);
				imgType = ImgTypeSetting;
			}

		}
		else//已经连接,用camera里的值.
		{
			int wid, hei, bin;
			ASIGetROIFormat(ASICameraInfo.CameraID, &wid, &hei, &bin, &imgType);//13
			ReadShownCtrlCaps();
		}						
		refreshImgType(uiex, imgType);
		refreshControl(uiex);
		if(ASICameraInfo.IsUSB3Host)
			uiex->setText("label_3", "via USB3");
		else
			uiex->setText("label_3", "via USB2");
	}
	else if(!strcmp(szEvt,"on_comboBox_3_currentIndexChanged") )//combobox control
	{			
		int iComboIndex = uiex->currentIndex("comboBox_3");

		if(iComboIndex >= 0 && iComboCount > iComboIndex)								
			refreshControlVal(uiex, ComboList[iComboIndex].CtlCap, iComboIndex);
	}
	else if(!strcmp(szEvt,"on_comboBox_2_currentIndexChanged") )//image type
	{							
		ASI_IMG_TYPE imgtype = ImgTypeArray[uiex->currentIndex("comboBox_2")];
		if(imgtype == ASI_IMG_RAW16 && strstr(ASICameraInfo.Name, "120") != NULL && ASICameraInfo.IsUSB3Camera == ASI_FALSE)//是120 USB2.0
			//		uiex->setText("label_4", "");
			uiex->invokeMethod("label_4", "show");
		else
			uiex->invokeMethod("label_4", "hide");
	}

}
bool X2Camera::isImgTypeSupported(ASI_CAMERA_INFO* CamInfo, ASI_IMG_TYPE imgtype)
{	
	int i = 0;
	while (CamInfo->SupportedVideoFormat[i] != ASI_IMG_END)
	{
		if (CamInfo->SupportedVideoFormat[i] == imgtype)
			return true;
		i++;
	}
	return false;
}

bool strSplit(const char* strHead, const char* strTail, char* strInput)
{
	char* pszHead = 0;
	if(strHead)
	{
		pszHead = strstr(strInput, strHead);//ZWO ASI1600MC Cool
		if(!pszHead)//ASI
			return false;
		else
			pszHead += strlen(strHead);//1600MC Cool
	}
	else
		pszHead = strInput;
	
	if(pszHead)//ASI
	{				
		if(pszHead - strInput >= strlen(strInput))//ZWO ASI 7 - 0 = 7//超出范围了
			return false;
		else
		{
			char* pszTail = 0;
			if(strTail)
			{
				pszTail = strstr(pszHead, strTail);//"1600MC Cool" M
				if(!pszTail)
					return false;
			}
			else
				pszTail = strInput + strlen(strInput);
			if(pszTail && pszTail != pszHead)
			{
				char* pOri = strInput;
				while (pszHead < pszTail)
				{
					*pOri = *pszHead;
					pszHead++;
					pOri++;
				}
				*pOri = 0;
			}
			else
				return false;
		}
	}
	else
		return false;

	return true;
}



#define XML_NAME "X2ASIconfig.xml"
#define XML_CAM_SETTING "camera_settings"
void X2Camera::saveXMLSetting(ASI_CAMERA_INFO info)
{
	xmlHandle hkey;

	bool  lRet; 
	
	char CamName[64] = {0};
	char szItemName[256];
	strcpy(CamName, info.Name);
	strSplit(0, "(", CamName);
	strcpy(szItemName, XML_CAM_SETTING);
	strcat(szItemName, "\\");
	strcat(szItemName, CamName);

	lRet = XMLOpenKey(XML_NAME, szItemName, &hkey);
	if(!lRet)
		lRet = XMLCreateKey(XML_NAME, szItemName, &hkey);
	if(!lRet)
		return;

//	XMLSetValueEx(hkey, "CameraID",  NULL, REG_DWORD, (PBYTE)&iCamIDSetting, sizeof(long));
	XMLSetValueEx(hkey, "ImageType",  NULL,  REG_DWORD, (PBYTE)&ImgTypeSetting, sizeof(long));

	char  CtrlName[64];//szCtrlType[8],
	for(int i = 0; i < iComboCount; i++)
	{		
	//	sprintf(szCtrlType, "%d", ComboListSetting[i].CtlCap.ControlType);
		strcpy(CtrlName, ComboListSetting[i].CtlCap.Name);
		XMLSetValueEx(hkey,  CtrlName,  (int *)&ComboListSetting[i].CtlCap.ControlType, REG_DWORD, (PBYTE)&ComboListSetting[i].lVal, sizeof(long));
	}
	XMLCloseKey(&hkey);
}
bool X2Camera::loadXMLSetting(ASI_CAMERA_INFO info)
{
	int dwSize;
	int dwType;
	xmlHandle hkey;
	bool  lRet; 

	char CamName[64] = {0};
	char szItemName[256];
	strcpy(CamName, info.Name);
	strSplit(0, "(", CamName);
	strcpy(szItemName, XML_CAM_SETTING);
	strcat(szItemName, "\\");
	strcat(szItemName, CamName);

	lRet = XMLOpenKey(XML_NAME, szItemName, &hkey);

	if(lRet)
	{

		XMLQueryValueEx(hkey, "ImageType",  NULL, &dwType, (PBYTE)&ImgTypeSetting, &dwSize);		

		int iCtrlNum = 0;
		if (ASIGetNumOfControls(info.CameraID, &iCtrlNum) == ASI_SUCCESS)
		{
			char CtrlName[64];
			iComboCount = 0;
			for(int i = 0; i<iCtrlNum; i++)
			{
				ASIGetControlCaps(info.CameraID, i, &ComboListSetting[iComboCount].CtlCap);	
				if(isShow(ComboListSetting[iComboCount].CtlCap.ControlType))
				{
					strcpy(CtrlName, ComboListSetting[iComboCount].CtlCap.Name);
					if(XMLQueryValueEx(hkey, CtrlName,  NULL, &dwType, (PBYTE)&ComboListSetting[iComboCount].lVal, &dwSize))
					{
						iComboCount++;
					}
				}
				
			}
		}		
		
		XMLCloseKey(&hkey);
	}
	return lRet;
}

void X2Camera::saveXMLAppSetting(const char* ItemName, const char* ElemName, const char* ElemVal, const int* ElemAttr)//camera_chooser guider_cam ZWO_ASI120MC-S 0
{
	xmlHandle hkey;

	bool  lRet; 


	char szItemName[256];

	strcpy(szItemName, "app_setting");
	strcat(szItemName, "\\");
	strcat(szItemName, ItemName);

	lRet = XMLOpenKey(XML_NAME, szItemName, &hkey);
	if(!lRet)
		lRet = XMLCreateKey(XML_NAME, szItemName, &hkey);
	if(!lRet)
		return;

	XMLSetValueEx(hkey, ElemName,  ElemAttr, REG_BINARY, (PBYTE)ElemVal, strlen(ElemVal));
	XMLCloseKey(&hkey);
}
bool X2Camera::loadXMLAppSetting(const char* ItemName, const char* ElemName, unsigned char* ElemVal, int* ElemAttr)
{
	int dwSize;
	int dwType;
	xmlHandle hkey;	

	char szItemName[256];
	strcpy(szItemName, "app_setting");
	strcat(szItemName, "\\");
	strcat(szItemName, ItemName);

	if(XMLOpenKey(XML_NAME, szItemName, &hkey))
	{
		if(XMLQueryValueEx(hkey, ElemName,  ElemAttr, &dwType, ElemVal, &dwSize))
			ElemVal[dwSize] = 0;	
		else
			return false;
		
		XMLCloseKey(&hkey);
	}
	else
		return false;

	return true;
}

int X2Camera::CCGetExtendedSettingName(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, BasicStringInterface &sSettingName)
{
	sSettingName = "ZWO: Exposure Mode";
	return SB_OK;
}
int X2Camera::CCGetExtendedValueCount(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, int &nCount)
{
	nCount = 2;
	return SB_OK;
}
int X2Camera::CCGetExtendedValueName(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, const int nIndex, BasicStringInterface &sName)
{
	if(nIndex == 1)
		sName = "High Speed Mode";
	else if(nIndex == 0)
		sName = "Long Exposure Mode";
	else
		return ERR_LIMITSEXCEEDED;

	return SB_OK;
}
int X2Camera::CCStartExposureAdditionalArgInterface(const enumCameraIndex& Cam, const enumWhichCCD CCD, const double& dTime, enumPictureType Type,
	const int& nABGState, const bool& bLeaveShutterAlone, const int &nIndex)
{
	X2MutexLocker ml(GetMutex());
	if(nIndex == 1)//video mode
	{
		if(!isVideoMode)
			ASIStopExposure(ASICameraInfo.CameraID);
		isVideoMode = true;
		prepareExposure();
		
		ASI_BOOL bAuto;
		ASIGetControlValue(ASICameraInfo.CameraID, ASI_EXPOSURE, &lExpMs, &bAuto);
		lExpMs /= 1000;
		if(lExpMs != dTime*1000)
		{
			ASISetControlValue(ASICameraInfo.CameraID, ASI_EXPOSURE, dTime*1000*1000, ASI_FALSE);
			ASIGetControlValue(ASICameraInfo.CameraID, ASI_EXPOSURE, &lExpMs, &bAuto);
			lExpMs /= 1000;		
			
		}	
		
		if(lExpMs != oldVideoTimeMs)
		{
			lVideoWaitMs = lExpMs;		
			if(lVideoWaitMs < 1000)
				lVideoWaitMs = 500 + 2*lVideoWaitMs;
			else
				lVideoWaitMs = lVideoWaitMs + 2000;

			oldVideoTimeMs = lExpMs;
		}
		
		ASIStartVideoCapture(ASICameraInfo.CameraID);		
	}
	else if(nIndex == 0)
	{
		if(isVideoMode)
			ASIStopVideoCapture(ASICameraInfo.CameraID);
		isVideoMode = false;
		CCStartExposure(Cam, CCD, dTime, Type,  nABGState, bLeaveShutterAlone);
	}
	return SB_OK;
}