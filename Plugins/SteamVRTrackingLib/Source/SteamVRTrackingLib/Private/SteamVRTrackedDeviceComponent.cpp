// Copyright Epic Games, Inc. All Rights Reserved.
// Modified by YuriNK, 2020

#include "SteamVRTrackedDeviceComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PrimitiveSceneProxy.h"
#include "Misc/ScopeLock.h"
#include "MotionDelayBuffer.h"
#include "RenderingThread.h"
#include "Modules/ModuleManager.h"
#include "Features/IModularFeatures.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "PrimitiveSceneInfo.h"
#include "IXRSystemAssets.h"
#include "SteamVRTrackingLibBPLibrary.h"
#include "SteamVRTrackingLib.h"
#include "Launch/Resources/Version.h"

namespace
{
	/** This is to prevent destruction of motion controller components while they are
		in the middle of being accessed by the render thread */
	FCriticalSection CritSect;

	/** Console variable for specifying whether motion controller late update is used */
	TAutoConsoleVariable<int32> CVarEnableMotionControllerLateUpdate(
		TEXT("vr.EnableMotionControllerLateUpdate"),
		1,
		TEXT("This command allows you to specify whether the motion controller late update is applied.\n")
		TEXT(" 0: don't use late update\n")
		TEXT(" 1: use late update (default)"),
		ECVF_Cheat);
} // anonymous namespace

//=============================================================================
USteamVRTrackedDeviceComponent::USteamVRTrackedDeviceComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, RenderThreadComponentScale(1.0f,1.0f,1.0f)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bTickEvenWhenPaused = true;

	PlayerIndex = 0;
	TrackedDeviceName = TEXT("Right");
	bTrackedDeviceNameIsMotionSource = true;
	bDisableLowLatencyUpdate = false;
	bHasAuthority = false;
	bAutoActivate = true;
	
	DisplayModelSource = TEXT("SteamVR");
	TrackingLibModule = nullptr;
	SteamVRIdUpdateInterval = 5.f;
	NextIdUpdateTime = 0.f;

	// ensure InitializeComponent() gets called
	bWantsInitializeComponent = true;
}

void USteamVRTrackedDeviceComponent::BeginPlay()
{
	Super::BeginPlay();

	bTrackedDeviceNameIsMotionSource =
		(TrackedDeviceName.IsEqual(TEXT("Right"))
		|| TrackedDeviceName.IsEqual(TEXT("Left"))
		|| TrackedDeviceName.ToString().Left(8) == TEXT("Special_"));
}

//=============================================================================
void USteamVRTrackedDeviceComponent::BeginDestroy()
{
	Super::BeginDestroy();
	if (ViewExtension.IsValid())
	{
		{
			// This component could be getting accessed from the render thread so it needs to wait
			// before clearing MotionControllerComponent and allowing the destructor to continue
			FScopeLock ScopeLock(&CritSect);
			ViewExtension->SteamVRTrackedDeviceComponent = NULL;
		}

		ViewExtension.Reset();
	}
}

void USteamVRTrackedDeviceComponent::CreateRenderState_Concurrent(FRegisterComponentContext* Context)
{
	Super::CreateRenderState_Concurrent(Context);
	RenderThreadRelativeTransform = GetRelativeTransform();
	RenderThreadComponentScale = GetComponentScale();
}

void USteamVRTrackedDeviceComponent::SendRenderTransform_Concurrent()
{
	struct FPrimitiveUpdateRenderThreadRelativeTransformParams
	{
		FTransform RenderThreadRelativeTransform;
		FVector RenderThreadComponentScale;
	};

	FPrimitiveUpdateRenderThreadRelativeTransformParams UpdateParams;
	UpdateParams.RenderThreadRelativeTransform = GetRelativeTransform();
	UpdateParams.RenderThreadComponentScale = GetComponentScale();

	ENQUEUE_RENDER_COMMAND(UpdateRTRelativeTransformCommand)(
		[UpdateParams, this](FRHICommandListImmediate& RHICmdList)
	{
		RenderThreadRelativeTransform = UpdateParams.RenderThreadRelativeTransform;
		RenderThreadComponentScale = UpdateParams.RenderThreadComponentScale;
	});

	Super::SendRenderTransform_Concurrent();
}

//=============================================================================
void USteamVRTrackedDeviceComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsActive())
	{
		FVector Position = GetRelativeTransform().GetTranslation();
		FRotator Orientation = GetRelativeTransform().GetRotation().Rotator();
		float WorldToMeters = GetWorld() ? GetWorld()->GetWorldSettings()->WorldToMeters : 100.0f;
		const bool bNewTrackedState = PollControllerState(Position, Orientation, WorldToMeters);
		if (bNewTrackedState)
		{
			SetRelativeLocationAndRotation(Position, Orientation);
		}

		// if controller tracking just kicked in 
		if (!bTracked && bNewTrackedState && bDisplayDeviceModel)
		{
			RefreshDisplayComponent();
		}
		bTracked = bNewTrackedState;

		if (!ViewExtension.IsValid() && GEngine)
		{
			ViewExtension = FSceneViewExtensions::NewExtension<FViewExtension>(this);
		}
	}
}

//=============================================================================
void USteamVRTrackedDeviceComponent::SetShowDeviceModel(const bool bShowDeviceModel)
{
	if (bDisplayDeviceModel != bShowDeviceModel)
	{
		bDisplayDeviceModel = bShowDeviceModel;
#if WITH_EDITORONLY_DATA
		const UWorld* MyWorld = GetWorld();
		const bool bIsGameInst = MyWorld && MyWorld->WorldType != EWorldType::Editor && MyWorld->WorldType != EWorldType::EditorPreview;

		if (!bIsGameInst)
		{
			// tear down and destroy the existing component if we're an editor inst
			RefreshDisplayComponent(/*bForceDestroy =*/true);
		}
		else
#endif
		if (DisplayComponent)
		{
			DisplayComponent->SetHiddenInGame(bShowDeviceModel, /*bPropagateToChildren =*/false);
		}
		else if (!bShowDeviceModel)
		{
			RefreshDisplayComponent();
		}
	}
}

//=============================================================================
void USteamVRTrackedDeviceComponent::SetTrackedDeviceName(const FName NewSource)
{
	TrackedDeviceName = NewSource;

	bTrackedDeviceNameIsMotionSource =
		(TrackedDeviceName.IsEqual(TEXT("Right"))
		|| TrackedDeviceName.IsEqual(TEXT("Left"))
		|| TrackedDeviceName.ToString().Left(8) == TEXT("Special_"));


	UWorld* MyWorld = GetWorld();
	if (MyWorld && MyWorld->IsGameWorld() && HasBeenInitialized())
	{
		FMotionDelayService::RegisterDelayTarget(this, PlayerIndex, NewSource);
	}
}

//=============================================================================
void USteamVRTrackedDeviceComponent::SetAssociatedPlayerIndex(const int32 NewPlayer)
{
	PlayerIndex = NewPlayer;

	UWorld* MyWorld = GetWorld();
	if (MyWorld && MyWorld->IsGameWorld() && HasBeenInitialized())
	{
		FMotionDelayService::RegisterDelayTarget(this, NewPlayer, TrackedDeviceName);
	}
}

#if WITH_EDITOR
//=============================================================================
void USteamVRTrackedDeviceComponent::PreEditChange(FProperty* PropertyAboutToChange)
{
	PreEditMaterialCount = DisplayMeshMaterialOverrides.Num();
	Super::PreEditChange(PropertyAboutToChange);
}

//=============================================================================
void USteamVRTrackedDeviceComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	const FName PropertyName = (PropertyThatChanged != nullptr) ? PropertyThatChanged->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(USteamVRTrackedDeviceComponent, bDisplayDeviceModel))
	{
		RefreshDisplayComponent(/*bForceDestroy =*/true);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(USteamVRTrackedDeviceComponent, DisplayMeshMaterialOverrides))
	{
		RefreshDisplayComponent(/*bForceDestroy =*/DisplayMeshMaterialOverrides.Num() < PreEditMaterialCount);
	}
}
#endif

//=============================================================================
void USteamVRTrackedDeviceComponent::OnRegister()
{
	Super::OnRegister();

	if (DisplayComponent == nullptr)
	{
		RefreshDisplayComponent();
	}
}

//=============================================================================
void USteamVRTrackedDeviceComponent::InitializeComponent()
{
	Super::InitializeComponent();

	UWorld* MyWorld = GetWorld();
	if (MyWorld && MyWorld->IsGameWorld())
	{
		FMotionDelayService::RegisterDelayTarget(this, PlayerIndex, TrackedDeviceName);
	}
}

//=============================================================================
void USteamVRTrackedDeviceComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);

	if (DisplayComponent)
	{
		DisplayComponent->DestroyComponent();
	}
}

//=============================================================================
void USteamVRTrackedDeviceComponent::RefreshDisplayComponent(const bool bForceDestroy)
{
	if (IsRegistered())
	{
		TArray<USceneComponent*> DisplayAttachChildren;
		auto DestroyDisplayComponent = [this, &DisplayAttachChildren]()
		{
			DisplayDeviceId.Clear();

			if (DisplayComponent)
			{
				// @TODO: save/restore socket attachments as well
				DisplayAttachChildren = DisplayComponent->GetAttachChildren();

				DisplayComponent->DestroyComponent(/*bPromoteChildren =*/true);
				DisplayComponent = nullptr;
			}
		};
		if (bForceDestroy)
		{
			DestroyDisplayComponent();
		}

		UPrimitiveComponent* NewDisplayComponent = nullptr;
		if (bDisplayDeviceModel)
		{
			const EObjectFlags SubObjFlags = RF_Transactional | RF_TextExportTransient;

			TArray<IXRSystemAssets*> XRAssetSystems = IModularFeatures::Get().GetModularFeatureImplementations<IXRSystemAssets>(IXRSystemAssets::GetModularFeatureName());				
			for (IXRSystemAssets* AssetSys : XRAssetSystems)
			{
				if (!DisplayModelSource.IsNone() && AssetSys->GetSystemName() != DisplayModelSource)
				{
					continue;
				}

				int32 DeviceId = USteamVRTrackingLibBPLibrary::GetDeviceIdByMotionSource(TrackedDeviceName, true, ESteamVRTrackedDeviceType::Invalid);

				if (DisplayComponent && DisplayDeviceId.IsOwnedBy(AssetSys) && DisplayDeviceId.DeviceId == DeviceId)
				{
					// assume that the current DisplayComponent is the same one we'd get back, so don't recreate it
					// @TODO: maybe we should add a IsCurrentlyRenderable(int32 DeviceId) to IXRSystemAssets to confirm this in some manner
					break;
				}

				// needs to be set before CreateRenderComponent() since the LoadComplete callback may be triggered before it returns (for syncrounous loads)
				DisplayModelLoadState = EModelLoadStatus::Pending;
				FXRComponentLoadComplete LoadCompleteDelegate = FXRComponentLoadComplete::CreateUObject(this, &USteamVRTrackedDeviceComponent::OnDisplayModelLoaded);
					
				NewDisplayComponent = AssetSys->CreateRenderComponent(DeviceId, GetOwner(), SubObjFlags, /*bForceSynchronous=*/false, LoadCompleteDelegate);
				if (NewDisplayComponent != nullptr)
				{
					if (DisplayModelLoadState != EModelLoadStatus::Complete)
					{
						DisplayModelLoadState = EModelLoadStatus::InProgress;
					}
					DestroyDisplayComponent();
					DisplayDeviceId = FXRDeviceId(AssetSys, DeviceId);
					break;
				}
				else
				{
					DisplayModelLoadState = EModelLoadStatus::Unloaded;
				}
			}

			if (NewDisplayComponent == nullptr)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to create a display component for the MotionController - no XR system (if there were any) had a model for the specified source ('%s')"), *TrackedDeviceName.ToString());
			}
			else if (NewDisplayComponent != DisplayComponent)
			{
				NewDisplayComponent->SetupAttachment(this);
				// force disable collision - if users wish to use collision, they can setup their own sub-component
				NewDisplayComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				NewDisplayComponent->RegisterComponent();

				for (USceneComponent* Child : DisplayAttachChildren)
				{
					Child->SetupAttachment(NewDisplayComponent);
				}

				DisplayComponent = NewDisplayComponent;
			}

			if (DisplayComponent)
			{
				if (DisplayModelLoadState != EModelLoadStatus::InProgress)
				{
					OnDisplayModelLoaded(DisplayComponent);
				}

				DisplayComponent->SetHiddenInGame(bHiddenInGame);
				DisplayComponent->SetVisibility(GetVisibleFlag());
			}
		}
		else if (DisplayComponent)
		{
			DisplayComponent->SetHiddenInGame(true, /*bPropagateToChildren =*/false);
		}
	}
}

//=============================================================================
bool USteamVRTrackedDeviceComponent::PollControllerState(FVector& Position, FRotator& Orientation, float WorldToMetersScale)
{
	if (IsInGameThread())
	{
		// Cache state from the game thread for use on the render thread
		const AActor* MyOwner = GetOwner();
		bHasAuthority = MyOwner->HasLocalNetOwner();
	}

	if(bHasAuthority)
	{
		if (!TrackingLibModule)
		{
			TrackingLibModule = FModuleManager::LoadModulePtr<FSteamVRTrackingLibModule>(TEXT("SteamVRTrackingLib"));
			if (!TrackingLibModule)
			{
				return false;
			}
		}

		// using friendly name instead?
		int32 DeviceId = INDEX_NONE;
		if (bTrackedDeviceNameIsMotionSource)
		{
			DeviceId = USteamVRTrackingLibBPLibrary::GetDeviceIdByMotionSource(TrackedDeviceName, true, ESteamVRTrackedDeviceType::Invalid);
		}
		else
		{
			bool bForceUpdateId = false;
			if (SteamVRIdUpdateInterval > 0.f)
			{
				const float CurrentTime = GetWorld()->GetRealTimeSeconds();

				if (CurrentTime > NextIdUpdateTime)
				{
					NextIdUpdateTime = CurrentTime + SteamVRIdUpdateInterval;
					bForceUpdateId = true;
				}
			}
			else if (SteamVRIdUpdateInterval == 0.f)
			{
				bForceUpdateId = true;
			}

			DeviceId = TrackingLibModule->GetTrackedDeviceIdByName(TrackedDeviceName, bForceUpdateId);
		}

		if (DeviceId == INDEX_NONE)
		{
			return false;
		}

		return USteamVRFunctionLibrary::GetTrackedDevicePositionAndOrientation(DeviceId, Position, Orientation);
	}

	return false;
}

//=============================================================================
USteamVRTrackedDeviceComponent::FViewExtension::FViewExtension(const FAutoRegister& AutoRegister, USteamVRTrackedDeviceComponent* InSteamVRTrackedDeviceComponent)
	: FSceneViewExtensionBase(AutoRegister)
	, SteamVRTrackedDeviceComponent(InSteamVRTrackedDeviceComponent)
{}

//=============================================================================
void USteamVRTrackedDeviceComponent::FViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	if (!SteamVRTrackedDeviceComponent)
	{
		return;
	}

	// Set up the late update state for the controller component
	LateUpdate.Setup(SteamVRTrackedDeviceComponent->CalcNewComponentToWorld(FTransform()), SteamVRTrackedDeviceComponent, false);
}

//=============================================================================
void USteamVRTrackedDeviceComponent::FViewExtension::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily)
{
	if (!SteamVRTrackedDeviceComponent)
	{
		return;
	}

	FTransform OldTransform;
	FTransform NewTransform;
	{
		FScopeLock ScopeLock(&CritSect);
		if (!SteamVRTrackedDeviceComponent)
		{
			return;
		}

		// Find a view that is associated with this player.
		float WorldToMetersScale = -1.0f;
		for (const FSceneView* SceneView : InViewFamily.Views)
		{
			if (SceneView && SceneView->PlayerIndex == SteamVRTrackedDeviceComponent->PlayerIndex)
			{
				WorldToMetersScale = SceneView->WorldToMetersScale;
				break;
			}
		}
		// If there are no views associated with this player use view 0.
		if (WorldToMetersScale < 0.0f)
		{
			check(InViewFamily.Views.Num() > 0);
			WorldToMetersScale = InViewFamily.Views[0]->WorldToMetersScale;
		}

		// Poll state for the most recent controller transform
		FVector Position = SteamVRTrackedDeviceComponent->RenderThreadRelativeTransform.GetTranslation();
		FRotator Orientation = SteamVRTrackedDeviceComponent->RenderThreadRelativeTransform.GetRotation().Rotator();
		if (!SteamVRTrackedDeviceComponent->PollControllerState(Position, Orientation, WorldToMetersScale))
		{
			return;
		}

		OldTransform = SteamVRTrackedDeviceComponent->RenderThreadRelativeTransform;
		NewTransform = FTransform(Orientation, Position, SteamVRTrackedDeviceComponent->RenderThreadComponentScale);
	} // Release the lock on the SteamVRTrackedDeviceComponent

	// Tell the late update manager to apply the offset to the scene components
#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION > 26
	LateUpdate.Apply_RenderThread(InViewFamily.Scene, InViewFamily.bLateLatchingEnabled ? InViewFamily.FrameNumber : -1, OldTransform, NewTransform);
#else
	LateUpdate.Apply_RenderThread(InViewFamily.Scene, OldTransform, NewTransform);
#endif
}

bool USteamVRTrackedDeviceComponent::FViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext&) const
{
	check(IsInGameThread());
	return SteamVRTrackedDeviceComponent && !SteamVRTrackedDeviceComponent->bDisableLowLatencyUpdate && CVarEnableMotionControllerLateUpdate.GetValueOnGameThread();
}

void USteamVRTrackedDeviceComponent::OnDisplayModelLoaded(UPrimitiveComponent* InDisplayComponent)
{
	if (InDisplayComponent == DisplayComponent || DisplayModelLoadState == EModelLoadStatus::Pending)
	{
		if (InDisplayComponent)
		{
			const int32 MatCount = FMath::Min(InDisplayComponent->GetNumMaterials(), DisplayMeshMaterialOverrides.Num());
			for (int32 MatIndex = 0; MatIndex < MatCount; ++MatIndex)
			{
				InDisplayComponent->SetMaterial(MatIndex, DisplayMeshMaterialOverrides[MatIndex]);
			}
		}
		DisplayModelLoadState = EModelLoadStatus::Complete;
	}	
}
