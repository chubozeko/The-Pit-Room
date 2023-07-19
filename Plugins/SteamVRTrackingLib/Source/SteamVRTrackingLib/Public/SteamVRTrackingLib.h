// (c) YuriNK (ykasczc@gmail.com), 2020. You're free to use it whatever way you want.

#pragma once

#include "Modules/ModuleManager.h"
#include "SteamVRTrackingSetup.h"

class FSteamVRTrackingLibModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	int32 GetTrackedDeviceIdByName(const FName& FriendlyName, bool bForceUpdateId = false);
	void GetTrackedDeviceSetupByName(const FName& FriendlyName, FSteamVRDeviceBindingSetup& OutData) const;

	/* Initialize SerialNumber-to-FriendlyName bindings from USteamVRTrackingSetup object */
	void InitializeTrackingNames(const USteamVRTrackingSetup* SteamVRTrackingSetup);
	void InitializeTrackingNamesFromArray(const TArray<FSteamVRDeviceBindingSetup>& SteamVRTrackingDevices);

private:
	TMap<FName, FSteamVRDeviceBindingSetup> DeviceSetup;
};
