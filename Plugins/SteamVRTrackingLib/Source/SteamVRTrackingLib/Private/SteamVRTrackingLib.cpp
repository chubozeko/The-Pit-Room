// (c) YuriNK (ykasczc@gmail.com), 2020. You're free to use it whatever way you want.

#include "SteamVRTrackingLib.h"
#include "SteamVRTrackingSetup.h"
#include "SteamVRFunctionLibrary.h"
#include "SteamVRTrackingLibBPLibrary.h"

#define LOCTEXT_NAMESPACE "FSteamVRTrackingLibModule"

void FSteamVRTrackingLibModule::StartupModule()
{
}

void FSteamVRTrackingLibModule::ShutdownModule()
{
}

int32 FSteamVRTrackingLibModule::GetTrackedDeviceIdByName(const FName& FriendlyName, bool bForceUpdateId)
{
	if (FSteamVRDeviceBindingSetup* BidningSetup = DeviceSetup.Find(FriendlyName))
	{
		if (BidningSetup->Id == INDEX_NONE || bForceUpdateId)
		{
			TArray<int32> DeviceIds;
			USteamVRFunctionLibrary::GetValidTrackedDeviceIds(BidningSetup->Type, DeviceIds);

			for (const auto Id : DeviceIds)
			{
				FString SN = USteamVRTrackingLibBPLibrary::GetTrackedDeviceSerialNumber(Id);

				if (BidningSetup->SerialNumber.IsEqual(FName(*SN)))
				{
					BidningSetup->Id = Id;
					break;
				}
			}
		}

		return BidningSetup->Id;
	}
	else
	{
		return INDEX_NONE;
	}
}

void FSteamVRTrackingLibModule::InitializeTrackingNames(const USteamVRTrackingSetup* SteamVRTrackingSetup)
{
	TArray<FSteamVRDeviceBindingSetup> SteamVRTrackingDevices;
	SteamVRTrackingSetup->DeviceSetup.GenerateValueArray(SteamVRTrackingDevices);
	InitializeTrackingNamesFromArray(SteamVRTrackingDevices);
}

void FSteamVRTrackingLibModule::InitializeTrackingNamesFromArray(const TArray<FSteamVRDeviceBindingSetup>& SteamVRTrackingDevices)
{
	DeviceSetup.Empty();
	for (const auto& DeviceData : SteamVRTrackingDevices)
	{
		if (!DeviceData.SerialNumber.IsNone() && !DeviceData.FriendlyName.IsNone())
		{
			FSteamVRDeviceBindingSetup NewItem = DeviceData;
			NewItem.Id = INDEX_NONE;
			DeviceSetup.Add(DeviceData.FriendlyName, NewItem);
		}
	}
}

void FSteamVRTrackingLibModule::GetTrackedDeviceSetupByName(const FName& FriendlyName, FSteamVRDeviceBindingSetup& OutData) const
{
	if (const FSteamVRDeviceBindingSetup* BidningSetup = DeviceSetup.Find(FriendlyName))
	{
		OutData = *BidningSetup;
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSteamVRTrackingLibModule, SteamVRTrackingLib)