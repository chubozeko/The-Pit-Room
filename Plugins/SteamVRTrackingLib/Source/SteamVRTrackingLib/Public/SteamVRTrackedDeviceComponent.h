// Copyright Epic Games, Inc. All Rights Reserved.
// Modified by YuriNK, 2020

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/PrimitiveComponent.h"
#include "SceneViewExtension.h"
#include "LateUpdateManager.h"
#include "IIdentifiableXRDevice.h"
#include "SteamVRTrackingLib.h"
#include "SteamVRTrackedDeviceComponent.generated.h"

class FPrimitiveSceneInfo;
class FRHICommandListImmediate;
class FSceneView;
class FSceneViewFamily;

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = SteamVR)
class STEAMVRTRACKINGLIB_API USteamVRTrackedDeviceComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	void BeginDestroy() override;

	/** Which player index this motion controller should automatically follow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetAssociatedPlayerIndex, Category = "SteamVR Tracked Device")
	int32 PlayerIndex;

	/** It can me an user-friendly name associated with serial number or standard MotionSource like Left, Right, Special_X */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetTrackedDeviceName, Category = "SteamVR Tracked Device")
	FName TrackedDeviceName;

	UPROPERTY()
	bool bTrackedDeviceNameIsMotionSource;

	/** Force SteamVR Device ID update each N seconds. Negative value to disable, 0 to update in every tick */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="SteamVR ID Update Interval"), Category = "SteamVR Tracked Device")
	float SteamVRIdUpdateInterval;

	/** If false, render transforms within the motion controller hierarchy will be updated a second time immediately before rendering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamVR Tracked Device")
	uint32 bDisableLowLatencyUpdate:1;

	/** The tracking status for the device (e.g. full tracking, inertial tracking only, no tracking) */
	UPROPERTY(BlueprintReadOnly, Category = "SteamVR Tracked Device")
	ETrackingStatus CurrentTrackingStatus;

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	/** Whether or not this component had a valid tracked device this frame */
	UFUNCTION(BlueprintPure, Category = "SteamVR Tracked Device")
	bool IsTracked() const
	{
		return bTracked;
	}

	/** Used to automatically render a model associated with the set hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter=SetShowDeviceModel, Category="Visualization")
	bool bDisplayDeviceModel;

	UFUNCTION(BlueprintSetter)
	void SetShowDeviceModel(const bool bShowControllerModel);

	/** Determines the source of the desired model. Only SteamVR is supported. */
	UPROPERTY(BlueprintReadOnly, Category="Visualization")
	FName DisplayModelSource;

	/** Material overrides for the specified display mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visualization", meta=(editcondition="bDisplayDeviceModel"))
	TArray<UMaterialInterface*> DisplayMeshMaterialOverrides;

	UFUNCTION(BlueprintSetter)
	void SetTrackedDeviceName(const FName NewSource);

	UFUNCTION(BlueprintSetter)
	void SetAssociatedPlayerIndex(const int32 NewPlayer);

public:

#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif 

public:
	//~ UActorComponent interface
	virtual void OnRegister() override;
	virtual void InitializeComponent() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	virtual void BeginPlay() override;

protected:
	//~ Begin UActorComponent Interface.
	virtual void CreateRenderState_Concurrent(FRegisterComponentContext* Context) override;
	virtual void SendRenderTransform_Concurrent() override;
	//~ End UActorComponent Interface.

	FSteamVRTrackingLibModule* TrackingLibModule;

	void RefreshDisplayComponent(const bool bForceDestroy = false);

private:

	/** Whether or not this component had a valid tracked controller associated with it this frame*/
	bool bTracked;

	/** Whether or not this component has authority within the frame*/
	bool bHasAuthority;

	/** If true, the Position and Orientation args will contain the most recent controller state */
	bool PollControllerState(FVector& Position, FRotator& Orientation, float WorldToMetersScale);

	FTransform RenderThreadRelativeTransform;
	FVector RenderThreadComponentScale;
	float NextIdUpdateTime;

	/** View extension object that can persist on the render thread without the motion controller component */
	class FViewExtension : public FSceneViewExtensionBase
	{
	public:
		FViewExtension(const FAutoRegister& AutoRegister, USteamVRTrackedDeviceComponent* InSteamVRTrackedDeviceComponent);
		virtual ~FViewExtension() {}

		/** ISceneViewExtension interface */
		virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
		virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
		virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
		virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override {}
		virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
		virtual int32 GetPriority() const override { return -10; }
		virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const;

	private:
		friend class USteamVRTrackedDeviceComponent;

		/** Motion controller component associated with this view extension */
		USteamVRTrackedDeviceComponent* SteamVRTrackedDeviceComponent;
		FLateUpdateManager LateUpdate;
	};
	TSharedPtr< FViewExtension, ESPMode::ThreadSafe > ViewExtension;	
 
	UPROPERTY(Transient, BlueprintReadOnly, Category=Visualization, meta=(AllowPrivateAccess="true"))
	UPrimitiveComponent* DisplayComponent;

	/** Callback for asynchronous display model loads (to set materials, etc.) */
	void OnDisplayModelLoaded(UPrimitiveComponent* DisplayComponent);

	enum class EModelLoadStatus : uint8
	{
		Unloaded,
		Pending,
		InProgress,
		Complete
	};
	EModelLoadStatus DisplayModelLoadState = EModelLoadStatus::Unloaded;

	FXRDeviceId DisplayDeviceId;
#if WITH_EDITOR
	int32 PreEditMaterialCount = 0;
#endif
};
