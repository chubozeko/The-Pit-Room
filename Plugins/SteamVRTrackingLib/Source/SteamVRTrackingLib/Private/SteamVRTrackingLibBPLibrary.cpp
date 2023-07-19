// (c) YuriNK (ykasczc@gmail.com), 2020. You're free to use it whatever way you want.

#include "SteamVRTrackingLibBPLibrary.h"
#include "openvr.h"
#include "Features/IModularFeatures.h"
#include "IMotionController.h"
#include "XRMotionControllerBase.h"
#include "SteamVRFunctionLibrary.h"
#include "Misc/CString.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "SteamVRTrackingSetup.h"
#include "SteamVRTrackingLib.h"

#include "Templates/SharedPointer.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "JsonObjectWrapper.h"

namespace JsonHelpers
{
	FString GetJsonTypeDesc(const EJson Type)
	{
		switch (Type)
		{
		case EJson::None:
			return TEXT("EJson::None");
		case EJson::Null:
			return TEXT("EJson::Null");
		case EJson::String:
			return TEXT("EJson::String");
		case EJson::Number:
			return TEXT("EJson::Number");
		case EJson::Boolean:
			return TEXT("EJson::Boolean");
		case EJson::Array:
			return TEXT("EJson::Array");
		case EJson::Object:
			return TEXT("EJson::Object");
		default:
			return TEXT("UNKNOWN TYPE, should never happen");
		}
	}

	int32 GetFormatVersion(const TSharedRef<const FJsonObject>& JsonRoot)
	{
		const FString KeyName = TEXT("FormatVersion");
		if (JsonRoot->Values.Contains(KeyName))
		{
			double Value = 0.f;
			if (JsonRoot->Values[KeyName]->TryGetNumber(Value))
			{
				return (int32)Value;
			}
		}
		return 0;
	}

	bool ReadJsonDevice(const TSharedRef<const FJsonObject>& JsonDevice, FSteamVRDeviceBindingSetup& OutBinding)
	{
		const FString KeySN = TEXT("SerialNumber");
		const FString KeyType = TEXT("Type");
		const FString KeyName = TEXT("Name");

		if (!JsonDevice->Values.Contains(KeySN) || !JsonDevice->Values.Contains(KeyType) || !JsonDevice->Values.Contains(KeyName))
		{
			return false;
		}

		FString DeviceType = JsonDevice->Values[KeyType]->AsString().ToLower();

		OutBinding.Id = INDEX_NONE;
		OutBinding.SerialNumber = FName(*JsonDevice->Values[KeySN]->AsString());

		if (DeviceType == TEXT("controller"))
			OutBinding.Type = ESteamVRTrackedDeviceType::Controller;
		else if (DeviceType == TEXT("vivetracker"))
			OutBinding.Type = ESteamVRTrackedDeviceType::Other;
		else
			return false;

		OutBinding.FriendlyName = FName(*JsonDevice->Values[KeyName]->AsString());

		return true;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

USteamVRTrackingLibBPLibrary::USteamVRTrackingLibBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

int32 USteamVRTrackingLibBPLibrary::GetDeviceIdByMotionSource(const FName MotionSource, bool bAnyDeviceType, ESteamVRTrackedDeviceType DeviceType)
{
	const float WorldToMetersScale = 100.f;

	bool bIsMotionSource = (MotionSource.IsEqual(TEXT("Right"))
		|| MotionSource.IsEqual(TEXT("Left"))
		|| MotionSource.ToString().Left(8) == TEXT("Special_"));

	// using friendly name instead?
	if (!bIsMotionSource)
	{
		return USteamVRTrackingLibBPLibrary::GetTrackedDeviceIdByName(MotionSource, false);
	}

	TArray<IMotionController*> MotionControllers = IModularFeatures::Get().GetModularFeatureImplementations<IMotionController>(IMotionController::GetModularFeatureName());

	FVector Location;
	FRotator Rotation;
	bool HasValidValue = false;
	for (auto MotionController : MotionControllers)
	{
		if (MotionController != nullptr && MotionController->GetControllerOrientationAndPosition(0, MotionSource, Rotation, Location, WorldToMetersScale))
		{
			HasValidValue = true;
			break;
		}
	}

	if (Location.IsZero())
	{
		HasValidValue = false;
	}

	if (HasValidValue)
	{
		float MinDistance = 10000.f;
		float MinId = INDEX_NONE;

		TArray<int32> OutDevices;
		if (bAnyDeviceType)
		{
			TArray<int32> OutTrackers;
			USteamVRFunctionLibrary::GetValidTrackedDeviceIds(ESteamVRTrackedDeviceType::Controller, OutDevices);
			USteamVRFunctionLibrary::GetValidTrackedDeviceIds(ESteamVRTrackedDeviceType::Other, OutTrackers);
			OutDevices.Append(OutTrackers);
		}
		else
		{
			USteamVRFunctionLibrary::GetValidTrackedDeviceIds(DeviceType, OutDevices);
		}

		for (auto DeviceId : OutDevices)
		{
			FVector CtrlLocation;
			FRotator CtrlRotation;

			USteamVRFunctionLibrary::GetTrackedDevicePositionAndOrientation(DeviceId, CtrlLocation, CtrlRotation);

			const float NewDistance = FVector::DistSquared(CtrlLocation, Location);
			if (NewDistance < MinDistance)
			{
				MinDistance = NewDistance;
				MinId = DeviceId;
			}
		}

		if (MinDistance > 5.f * 5.f)
		{
			MinId = INDEX_NONE;
		}

		return MinId;
	}
	else
	{
		FString szMotionSource = MotionSource.ToString();
		if (szMotionSource.Left(7) == TEXT("Special"))
		{
			FString szTrackerNumber = *MotionSource.ToString().Right(szMotionSource.Len() - 8);
			//UE_LOG(LogTemp, Log, TEXT("MotionSource is tracker, szTrackerNumber = "), *szTrackerNumber);

			int32 TrackerNumber = FCString::Atoi(*szTrackerNumber);

			TArray<int32> OutTrackers;
			USteamVRFunctionLibrary::GetValidTrackedDeviceIds(ESteamVRTrackedDeviceType::Other, OutTrackers);

			//UE_LOG(LogTemp, Log, TEXT("Have %d active trackers with required numbrr %d"), OutTrackers.Num(), TrackerNumber);

			if (TrackerNumber > 0 && OutTrackers.Num() >= TrackerNumber)
			{
				OutTrackers.Sort();
				return OutTrackers[TrackerNumber - 1];
			}
		}

		return INDEX_NONE;
	}
}

FString USteamVRTrackingLibBPLibrary::GetTrackedDeviceSerialNumber(int32 DeviceID)
{
	vr::IVRSystem* SteamVRSystem = vr::VRSystem();

	if (SteamVRSystem)
	{
		char OutString[256];

		vr::ETrackedPropertyError OutError = vr::ETrackedPropertyError::TrackedProp_Success;
		const vr::ETrackedDeviceProperty Prop_SN = vr::ETrackedDeviceProperty::Prop_SerialNumber_String;

		SteamVRSystem->GetStringTrackedDeviceProperty((vr::TrackedDeviceIndex_t)DeviceID, Prop_SN, OutString, 256, &OutError);

		if (OutError == vr::ETrackedPropertyError::TrackedProp_Success)
		{
			return ANSI_TO_TCHAR(OutString);
		}
		else
		{
			return TEXT("Property Read Error");
		}
	}
	else
	{
		return TEXT("Can't find SteamVR");
	}
	return TEXT("");
}

bool USteamVRTrackingLibBPLibrary::SetSteamVRTrackingSetup(USteamVRTrackingSetup* TrackingSetup)
{
	if (TrackingSetup)
	{
		FSteamVRTrackingLibModule& TrackingLibModule = FModuleManager::LoadModuleChecked<FSteamVRTrackingLibModule>(TEXT("SteamVRTrackingLib"));
		TrackingLibModule.InitializeTrackingNames(TrackingSetup);
		return true;
	}
	
	return false;
}

bool USteamVRTrackingLibBPLibrary::LoadSteamVRTrackingSetupFromFile(const FString& ImportFileName)
{
	TArray<FSteamVRDeviceBindingSetup> TrackingSetup;
	if (ImportTrackingSetupFromJSON(TrackingSetup, ImportFileName))
	{
		FSteamVRTrackingLibModule& TrackingLibModule = FModuleManager::LoadModuleChecked<FSteamVRTrackingLibModule>(TEXT("SteamVRTrackingLib"));
		TrackingLibModule.InitializeTrackingNamesFromArray(TrackingSetup);
		return true;
	}
	return false;
}

void USteamVRTrackingLibBPLibrary::ExportTrackingSetupToJSON(const TArray<FSteamVRDeviceBindingSetup>& TrackingSetup, const FString& ExportFileName)
{
	FString JsonString;
	const FString KeySN = TEXT("SerialNumber");
	const FString KeyType = TEXT("Type");
	const FString KeyName = TEXT("Name");

	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();

	// set format version
	JsonObject->SetField(TEXT("FormatVersion"), MakeShared<FJsonValueNumber>(1));

	TArray<TSharedPtr<FJsonValue>> JsonDevicesArray;

	for (const auto& DeviceBinding : TrackingSetup)
	{
		TSharedRef<FJsonObject> JsonDeviceObject = MakeShared<FJsonObject>();

		FString DevTyp = DeviceBinding.Type == ESteamVRTrackedDeviceType::Controller ? TEXT("Controller") : TEXT("ViveTracker");
		JsonDeviceObject->SetField(KeySN, MakeShared<FJsonValueString>(DeviceBinding.SerialNumber.ToString()));
		JsonDeviceObject->SetField(KeyType, MakeShared<FJsonValueString>(DevTyp));
		JsonDeviceObject->SetField(KeyName, MakeShared<FJsonValueString>(DeviceBinding.FriendlyName.ToString()));
		
		JsonDevicesArray.Push(MakeShared<FJsonValueObject>(JsonDeviceObject));
	}
	// set devices
	JsonObject->SetField(TEXT("Devices"), MakeShared<FJsonValueArray>(JsonDevicesArray));

	// build json string
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonString, 0);
	const bool bSuccess = FJsonSerializer::Serialize(JsonObject, JsonWriter);
	JsonWriter->Close();

	// save json string to file
	FFileHelper::SaveStringToFile(JsonString, *ExportFileName, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool USteamVRTrackingLibBPLibrary::ImportTrackingSetupFromJSON(TArray<FSteamVRDeviceBindingSetup>& OutTrackingSetup, const FString& ImportFileName)
{
	FString JsonString;

	if (!FFileHelper::LoadFileToString(JsonString, *ImportFileName))
	{
		return false;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to parse json=[%s]"), *JsonString);
		return false;
	}

	int32 FileFormatVersion = JsonHelpers::GetFormatVersion(JsonObject.ToSharedRef());
	UE_LOG(LogTemp, Log, TEXT("Reading TrackingSetup file. Format Version: %d"), FileFormatVersion);

	// Read json array	
	const FString KeyDevices = TEXT("Devices");

	if (!JsonObject->Values.Contains(KeyDevices))
	{
		UE_LOG(LogTemp, Error, TEXT("TrackingSetup file [%s] has invalid format."), *ImportFileName);
		return false;
	}
	const TSharedPtr<FJsonValue>& DevicesObject = JsonObject->Values[KeyDevices];
	if (DevicesObject->Type != EJson::Array)
	{
		UE_LOG(LogTemp, Error, TEXT("TrackingSetup file [%s] has invalid format."), *ImportFileName);
		return false;
	}

	OutTrackingSetup.Empty();

	// load all devices
	const auto DevicesList = DevicesObject->AsArray();
	for (const auto& Device : DevicesList)
	{
		//UE_LOG(LogTemp, Log, TEXT("Device loaded. Type is %s"), *JsonHelpers::GetJsonTypeDesc(Device->Type));
		if (Device->Type != EJson::Object)
		{
			UE_LOG(LogTemp, Error, TEXT("TrackingSetup file [%s] has invalid format."), *ImportFileName);
			return false;
		}

		const TSharedPtr<FJsonObject> DeviceObject = Device->AsObject();
		FSteamVRDeviceBindingSetup NewBinding;
		if (!JsonHelpers::ReadJsonDevice(DeviceObject.ToSharedRef(), NewBinding))
		{
			UE_LOG(LogTemp, Error, TEXT("TrackingSetup file [%s] has invalid format."), *ImportFileName);
			return false;
		}

		OutTrackingSetup.Add(NewBinding);
	}

	return true;
}

FName USteamVRTrackingLibBPLibrary::GetTrackedDeviceSerialNumberByName(const FName& DeviceFriendlyName)
{
	FSteamVRDeviceBindingSetup Data;

	FSteamVRTrackingLibModule& TrackingLibModule = FModuleManager::LoadModuleChecked<FSteamVRTrackingLibModule>(TEXT("SteamVRTrackingLib"));
	TrackingLibModule.GetTrackedDeviceSetupByName(DeviceFriendlyName, Data);

	return Data.SerialNumber;
}

int32 USteamVRTrackingLibBPLibrary::GetTrackedDeviceIdByName(const FName& FriendlyName, bool bForceUpdateID)
{
	FSteamVRTrackingLibModule& TrackingLibModule = FModuleManager::LoadModuleChecked<FSteamVRTrackingLibModule>(TEXT("SteamVRTrackingLib"));
	return TrackingLibModule.GetTrackedDeviceIdByName(FriendlyName, bForceUpdateID);
}