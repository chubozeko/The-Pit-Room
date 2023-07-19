// (c) YuriNK, 2017

#pragma once

#include "CoreMinimal.h"
#include "ViveMocapTypes.h"
#include "GameFramework/Actor.h"
#include "EditorSteamVRController.h"
#include "Animation/PoseSnapshot.h"
#include "EditorViveMocapController.generated.h"

/** Real-time filters for captured animation */
UENUM(BlueprintType)
enum class EEditorCaptureType : uint8
{
	CCT_PoseableMesh			UMETA(DisplayName = "Poseable Mesh"),
	CCT_SkeletalMesh			UMETA(DisplayName = "Skeletal Mesh"),
	CCT_PoseSnapshot			UMETA(DisplayName = "Pose Snapshot Variable")
};

UCLASS()
class STEAMVRTRACKINGLIB_API AEditorViveMocapController : public AActor
{
	GENERATED_BODY()
	
public:	
	AEditorViveMocapController();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Actor root component
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	class USceneComponent* RootComp;

	// Mocap Mesh
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UPoseableMeshComponent* BodyMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* SkeletalBodyMesh;

	// Motion Capture Component
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UCaptureDevice* CaptureDevice;

	UPROPERTY(EditAnywhere, Category = "Setup")
	TSet<FName> AdditionalEditorSkeletalMeshComponents;

	/**
	* Input objects to track
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	AEditorSteamVRController* InputController;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	FName CalibrationFileName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	FRotator DefaultSkeletalMeshRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	EEditorCaptureType CaptureType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	bool bHideCaptureMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	bool bScaledCapture;

	UPROPERTY(BlueprintReadWrite, Category = "Setup")
	FPoseSnapshot MeshPoseSnapshot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	bool bDebug;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Setup")
	void InitializeDevice();

	UFUNCTION(BlueprintCallable, Category = "Setup")
	void GetInputComponents(TArray<USceneComponent*>& InputTrackers, uint8& RightId, uint8& LeftId);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Setup")
	void StartMocap();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Setup")
	void StopMocap();

	UFUNCTION()
	void GetBodyCalibration(FBodyCalibrationData& OutCalibration);

	virtual bool ShouldTickIfViewportsOnly() const override
	{
		return true;
	}

protected:

	UPROPERTY()
	bool bEnabled;

	UPROPERTY()
	bool bInitialized;

	UPROPERTY()
	int32 InputCompsNum;

	FVMK_BoneRotatorSetup ComponentSpaceSetup;

	UFUNCTION()
	bool BuildTPose();

	FTransform RestoreRefBonePose_WS(const FName& BoneName) const;
	EAxis::Type FindCoDirection(const FRotator& BoneRotator, const FVector& Direction, float& ResultMultiplier);
	FRotator MakeRotByTwoAxes(EAxis::Type MainAxis, FVector MainAxisDirection, EAxis::Type SecondaryAxis, FVector SecondaryAxisDirection) const;
};
