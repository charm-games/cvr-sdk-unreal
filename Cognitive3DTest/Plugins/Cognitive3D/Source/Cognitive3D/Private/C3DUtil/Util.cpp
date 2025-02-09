/*
** Copyright (c) 2024 Cognitive3D, Inc. All rights reserved.
*/

#include "C3DUtil/Util.h"
#include "HeadMountedDisplayFunctionLibrary.h"

double FUtil::GetTimestamp()
{
	//#pragma warning(push)
	//#pragma warning(disable:4244) //Disable warning regarding loss of accuracy, no concern.

	long ts = time(0);
	double miliseconds = FDateTime::UtcNow().GetMillisecond();
	double finalTime = ts + miliseconds*0.001;

	return finalTime;
	//http://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c

	//#pragma warning(pop)
}

FString FUtil::GetDeviceName(FString DeviceName)
{
	if (DeviceName == "OculusRift")
	{
		return "rift";
	}
	if (DeviceName == "OculusXRHMD")
	{
		return "rift";
	}
	if (DeviceName == "OculusHMD")
	{
		return "rift";
	}
	if (DeviceName == "OSVR")
	{
		return "rift";
	}
	if (DeviceName == "SimpleHMD")
	{
		return "rift";
	}
	if (DeviceName == "SteamVR")
	{
		return "vive";
	}
	return FString("unknown");
}

void FUtil::SetSessionProperties()
{
	auto cog = FAnalyticsCognitive3D::Get().GetCognitive3DProvider().Pin();

	FString HMDDeviceName = UHeadMountedDisplayFunctionLibrary::GetHMDDeviceName().ToString();
	cog->SetSessionProperty("c3d.device.hmd.type", HMDDeviceName);

#if defined TOBII_EYETRACKING_ACTIVE
	cog->SetSessionProperty("c3d.device.eyetracking.enabled", true);
	cog->SetSessionProperty("c3d.device.eyetracking.type", FString("Tobii"));
	cog->SetSessionProperty("c3d.app.sdktype", FString("Tobii"));
#elif defined SRANIPAL_1_2_API
	cog->SetSessionProperty("c3d.device.eyetracking.enabled", true);
	cog->SetSessionProperty("c3d.device.eyetracking.type", FString("Tobii"));
	cog->SetSessionProperty("c3d.app.sdktype", FString("SRAnipal"));
#elif defined SRANIPAL_1_3_API
	cog->SetSessionProperty("c3d.device.eyetracking.enabled", true);
	cog->SetSessionProperty("c3d.device.eyetracking.type", FString("Tobii"));
	cog->SetSessionProperty("c3d.app.sdktype", FString("SRAnipal"));
#elif defined VARJOEYETRACKER_API
	cog->SetSessionProperty("c3d.device.eyetracking.enabled", true);
	cog->SetSessionProperty("c3d.device.eyetracking.type", FString("Varjo"));
	cog->SetSessionProperty("c3d.app.sdktype", FString("Varjo"));
#elif defined PICOMOBILE_API
	//TODO check that pico eye tracking is enabled
	cog->SetSessionProperty("c3d.device.eyetracking.enabled", true);
	cog->SetSessionProperty("c3d.device.eyetracking.type", FString("Tobii"));
	cog->SetSessionProperty("c3d.app.sdktype", FString("Pico"));
	cog->SetSessionProperty("c3d.device.hmd.type", FPlatformMisc::GetCPUBrand()); //returns pretty device name
#elif defined HPGLIA_API
	//TODO check that omnicept eye tracking is enabled
	cog->SetSessionProperty("c3d.device.eyetracking.enabled", true);
	cog->SetSessionProperty("c3d.device.eyetracking.type", FString("Tobii"));
	cog->SetSessionProperty("c3d.app.sdktype", FString("HP Omnicept"));
#elif defined INCLUDE_OCULUS_PLUGIN
	cog->SetSessionProperty("c3d.app.sdktype", FString("Oculus HMD"));
	cog->SetSessionProperty("c3d.device.hmd.type", FPlatformMisc::GetCPUBrand());
#elif defined INCLUDE_PICO_PLUGIN
	cog->SetSessionProperty("c3d.device.eyetracking.enabled", true);
	cog->SetSessionProperty("c3d.app.sdktype", FString("PICO"));
	cog->SetSessionProperty("c3d.device.hmd.type", FPlatformMisc::GetCPUBrand());
#else
	cog->SetSessionProperty("c3d.device.eyetracking.enabled", false);
	cog->SetSessionProperty("c3d.device.eyetracking.type", FString("None"));
	cog->SetSessionProperty("c3d.app.sdktype", FString("Default"));
#endif

	FString sdkVersion = Cognitive3D_SDK_VERSION;
	cog->SetSessionProperty("c3d.version", sdkVersion);
	cog->SetSessionProperty("c3d.app.engine", FString("Unreal"));

	if (!cog->GetUserID().IsEmpty())
		cog->SetParticipantId(cog->GetUserID());
	cog->SetSessionProperty("c3d.deviceid", cog->GetDeviceID());

	const UGeneralProjectSettings& projectSettings = *GetDefault< UGeneralProjectSettings>();
	cog->SetSessionProperty("c3d.app.version", projectSettings.ProjectVersion);
	FString projName = FApp::GetProjectName();
	cog->SetSessionProperty("c3d.app.name", projName);

	FString engineVersion = FEngineVersion::Current().ToString().Replace(TEXT("+"), TEXT(" "));;
	cog->SetSessionProperty("c3d.app.engine.version", engineVersion);

	auto platformName = UGameplayStatics::GetPlatformName();
	if (platformName.Compare("Windows", ESearchCase::IgnoreCase) == 0 || platformName.Compare("Mac", ESearchCase::IgnoreCase) == 0 || platformName.Compare("Linux", ESearchCase::IgnoreCase) == 0)
	{
		cog->SetSessionProperty("c3d.device.type", FString("Desktop"));
	}
	else if (platformName.Compare("IOS", ESearchCase::IgnoreCase) == 0 || platformName.Compare("Android", ESearchCase::IgnoreCase) == 0)
	{
		cog->SetSessionProperty("c3d.device.type", FString("Mobile"));
	}
	else if (platformName.Compare("PS", ESearchCase::IgnoreCase) == 0 || platformName.Contains("xbox", ESearchCase::IgnoreCase) || platformName.Contains("Switch", ESearchCase::IgnoreCase))
	{
		cog->SetSessionProperty("c3d.device.type", FString("Console"));
	}
	else
	{
		cog->SetSessionProperty("c3d.device.type", FString("Unknown"));
	}

#if PLATFORM_ANDROID
	cog->SetSessionProperty("c3d.device.cpu", FAndroidMisc::GetCPUChipset());
	cog->SetSessionProperty("c3d.device.gpu", FAndroidMisc::GetPrimaryGPUBrand());
	cog->SetSessionProperty("c3d.device.os", "Android OS " + FAndroidMisc::GetOSVersion());


#elif PLATFORM_WINDOWS
	cog->SetSessionProperty("c3d.device.cpu", FPlatformMisc::GetCPUBrand());
	cog->SetSessionProperty("c3d.device.gpu", FPlatformMisc::GetPrimaryGPUBrand());
	cog->SetSessionProperty("c3d.device.os", FPlatformMisc::GetOSVersion());
#endif

	const FPlatformMemoryConstants& MemoryConstants = FPlatformMemory::GetConstants();
	cog->SetSessionProperty("c3d.device.memory", (int)MemoryConstants.TotalPhysicalGB);
}