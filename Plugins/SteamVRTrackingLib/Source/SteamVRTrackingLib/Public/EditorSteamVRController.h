// (c) YuriNK (ykasczc@gmail.com), 2020. You're free to use it whatever way you want.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EditorSteamVRController.generated.h"

USTRUCT(BlueprintType)
struct FSteamVRTrackedComponent
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamVR Tracked Component")
	AActor* Actor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamVR Tracked Component")
	FName ComponentName;
};

USTRUCT(BlueprintType)
struct FSteamVRTrackingBinding
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamVR Tracking Binding")
	int32 DeviceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamVR Tracking Binding")
	FName MotionSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamVR Tracking Binding")
	USceneComponent* AttachedComponent;
};

UCLASS()
class STEAMVRTRACKINGLIB_API AEditorSteamVRController : public AActor
{
	GENERATED_BODY()
	
public:	
	AEditorSteamVRController();
	virtual void Tick(float DeltaTime) override;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	class USteamVRTrackingSetup* SteamVRTrackingSetup;

	/**
	* Objects to track
	* Key - motion source (Right, Left, Special_1...)
	* Value - component attached to this tracker
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	TMap<FName, FSteamVRTrackedComponent> TrackedObjects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	bool bIsEnabled;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Setup")
	void UpdateObjectsList();

	UFUNCTION(BlueprintCallable, Category = "Setup")
	bool EnsureUpdated();

	virtual bool ShouldTickIfViewportsOnly() const override
	{
		return true;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Editor SteamVR Controller")
	void GetUpdatedObjects(TArray<FSteamVRTrackingBinding>& Objects) const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "EditorSVRC")
	TArray<FSteamVRTrackingBinding> ObjectsToUpdate;
};
