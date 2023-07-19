// (c) YuriNK (ykasczc@gmail.com), 2020. You're free to use it whatever way you want.

#pragma once

#include "Engine/DataAsset.h"
#include "SteamVRFunctionLibrary.h"
#include "SteamVRTrackingSetup.generated.h"

USTRUCT(BlueprintType)
struct STEAMVRTRACKINGLIB_API FSteamVRDeviceBindingSetup
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamVR Device Setup")
	ESteamVRTrackedDeviceType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamVR Device Setup")
	int32 Id;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamVR Device Setup")
	FName SerialNumber;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamVR Device Setup")
	FName FriendlyName;

	FSteamVRDeviceBindingSetup()
		: Type(ESteamVRTrackedDeviceType::Invalid)
		, Id(INDEX_NONE)
		, SerialNumber(TEXT(""))
		, FriendlyName(TEXT(""))
	{}
};

UCLASS(Blueprintable, BlueprintType)
class STEAMVRTRACKINGLIB_API USteamVRTrackingSetup : public UDataAsset
{
	GENERATED_BODY()

public:

	USteamVRTrackingSetup(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SteamVR Tracking Setup")
	TMap<FName, FSteamVRDeviceBindingSetup> DeviceSetup;
};
