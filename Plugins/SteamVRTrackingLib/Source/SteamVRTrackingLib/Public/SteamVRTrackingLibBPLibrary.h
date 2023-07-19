// (c) YuriNK (ykasczc@gmail.com), 2020. You're free to use it whatever way you want.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SteamVRTrackingSetup.h"
#include "SteamVRTrackingLibBPLibrary.generated.h"

/* 
*	Function library class.
*/
UCLASS(meta = (DisplayName = "SteamVR Tracking Library Extended"))
class STEAMVRTRACKINGLIB_API USteamVRTrackingLibBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Device ID By Motion Source"), Category = "SteamVR Tracking Library Extended")
	static int32 GetDeviceIdByMotionSource(const FName MotionSource, bool bAnyDeviceType, ESteamVRTrackedDeviceType DeviceType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SteamVR Tracking Library Extended")
	static FString GetTrackedDeviceSerialNumber(int32 DeviceID);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SteamVR Tracking Library Extended")
	static FName GetTrackedDeviceSerialNumberByName(const FName& DeviceFriendlyName);

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Tracked Device ID by Friendly Name"), Category = "SteamVR Tracking Library Extended")
	static int32 GetTrackedDeviceIdByName(const FName& FriendlyName, bool bForceUpdateID = false);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set SteamVR Tracking Setup"), Category = "SteamVR Tracking Library Extended")
	static bool SetSteamVRTrackingSetup(class USteamVRTrackingSetup* TrackingSetup);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Load SteamVR Tracking Setup from File"), Category = "SteamVR Tracking Library Extended")
	static bool LoadSteamVRTrackingSetupFromFile(const FString& ImportFileName);

	UFUNCTION(BlueprintCallable, Category = "SteamVR Tracking Library Extended")
	static void ExportTrackingSetupToJSON(const TArray<FSteamVRDeviceBindingSetup>& TrackingSetup, const FString& ExportFileName);

	UFUNCTION(BlueprintCallable, Category = "SteamVR Tracking Library Extended")
	static bool ImportTrackingSetupFromJSON(TArray<FSteamVRDeviceBindingSetup>& OutTrackingSetup, const FString& ImportFileName);
};
