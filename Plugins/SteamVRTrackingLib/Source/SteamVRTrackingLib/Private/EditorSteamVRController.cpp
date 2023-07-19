// (c) YuriNK, 2017


#include "EditorSteamVRController.h"
#include "Components/StaticMeshComponent.h"
#include "SteamVRTrackingSetup.h"
#include "SteamVRFunctionLibrary.h"
#include "SteamVRTrackingLibBPLibrary.h"

AEditorSteamVRController::AEditorSteamVRController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

bool AEditorSteamVRController::EnsureUpdated()
{
	bool bValidNum = (TrackedObjects.Num() == ObjectsToUpdate.Num() && TrackedObjects.Num() > 0);
	bool bValidRefs = true;
	if (bValidNum)
	{
		for (const auto& Ref : ObjectsToUpdate)
		{
			if (!IsValid(Ref.AttachedComponent))
			{
				bValidRefs = false;
				break;
			}
		}
	}

	if (bValidNum && bValidRefs)
	{
		return true;
	}
	else
	{
		UpdateObjectsList();
		return (TrackedObjects.Num() == ObjectsToUpdate.Num() && TrackedObjects.Num() > 0);
	}
}

void AEditorSteamVRController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsEnabled)
	{
		EnsureUpdated();

		FVector loc;
		FRotator rot;
		for (auto& Object : ObjectsToUpdate)
		{
			if (Object.DeviceId == INDEX_NONE)
			{
				Object.DeviceId = USteamVRTrackingLibBPLibrary::GetDeviceIdByMotionSource(Object.MotionSource, true, ESteamVRTrackedDeviceType::Other);
				if (Object.DeviceId == INDEX_NONE)
				{
					continue;
				}
			}

			if (Object.AttachedComponent)
			{
				USteamVRFunctionLibrary::GetTrackedDevicePositionAndOrientation(Object.DeviceId, loc, rot);
				Object.AttachedComponent->SetRelativeLocationAndRotation(loc, rot);
			}
		}
	}
}

void AEditorSteamVRController::UpdateObjectsList()
{
	if (SteamVRTrackingSetup)
	{
		USteamVRTrackingLibBPLibrary::SetSteamVRTrackingSetup(SteamVRTrackingSetup);
	}

	ObjectsToUpdate.Empty();

	for (const auto& ObjectDesc : TrackedObjects)
	{
		if (ObjectDesc.Value.Actor && !ObjectDesc.Key.IsNone())
		{
			// get target component
			USceneComponent* NewTarget = nullptr;

			TArray<UActorComponent*> Components;
			ObjectDesc.Value.Actor->GetComponents(Components);
			for (const auto& Comp : Components)
			{
				if (Comp->GetFName().IsEqual(ObjectDesc.Value.ComponentName))
				{
					NewTarget = Cast<USceneComponent>(Comp);
					break;
				}
			}

			if (NewTarget)
			{
				// get source SteamVR device ID
				int32 TrackerId = USteamVRTrackingLibBPLibrary::GetDeviceIdByMotionSource(ObjectDesc.Key, true, ESteamVRTrackedDeviceType::Other);

				FSteamVRTrackingBinding NewBinding;
				NewBinding.MotionSource = ObjectDesc.Key;
				NewBinding.DeviceId = TrackerId;
				NewBinding.AttachedComponent = NewTarget;
				ObjectsToUpdate.Add(NewBinding);
			}
		}
	}
}

void AEditorSteamVRController::GetUpdatedObjects(TArray<FSteamVRTrackingBinding>& Objects) const
{
	Objects = ObjectsToUpdate;
}

