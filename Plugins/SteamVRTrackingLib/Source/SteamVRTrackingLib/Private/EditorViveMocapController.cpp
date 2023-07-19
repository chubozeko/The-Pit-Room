// (c) YuriNK, 2017


#include "EditorViveMocapController.h"
#include "EditorSteamVRController.h"
#include "Animation/AnimInstance.h"
#include "CaptureAnimBlueprint.h"
#include "Animation/PoseSnapshot.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Math/RotationMatrix.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "SessionCalibrationSave.h"
#include "CaptureDevice.h"
#include "DrawDebugHelpers.h"

#define RotatorDirection(Rotator, Axis) FRotationMatrix(Rotator).GetScaledAxis(Axis)
#define ComponentForwardVector FRotationMatrix(FRotator::ZeroRotator).GetScaledAxis(ComponentSpaceSetup.ForwardAxis) * ComponentSpaceSetup.ForwardDirection
#define ComponentRightVector FRotationMatrix(FRotator::ZeroRotator).GetScaledAxis(ComponentSpaceSetup.HorizontalAxis) * ComponentSpaceSetup.RightDirection
#define ComponentUpVector FRotationMatrix(FRotator::ZeroRotator).GetScaledAxis(ComponentSpaceSetup.VerticalAxis) * ComponentSpaceSetup.UpDirection

AEditorViveMocapController::AEditorViveMocapController()
	: DefaultSkeletalMeshRotation(FRotator(0.f, -90.f, 0.f))
	, CaptureType(EEditorCaptureType::CCT_PoseableMesh)
	, bScaledCapture(true)
	, bDebug(false)
	, bEnabled(false)
	, bInitialized(false)
	, InputCompsNum(0)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootComp;

	BodyMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComp);
	BodyMesh->SetRelativeRotation(DefaultSkeletalMeshRotation);

	SkeletalBodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalBodyMesh"));
	SkeletalBodyMesh->SetupAttachment(RootComp);
	SkeletalBodyMesh->SetRelativeRotation(DefaultSkeletalMeshRotation);

	CaptureDevice = CreateDefaultSubobject<UCaptureDevice>(TEXT("CaptureDevice"));
	CaptureDevice->bMultiMeshUpdate = true;
}

void AEditorViveMocapController::BeginPlay()
{
	Super::BeginPlay();
}

void AEditorViveMocapController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Capture animation?
	if (bEnabled)
	{
		CaptureDevice->TickComponent(DeltaTime, ELevelTick::LEVELTICK_TimeOnly, nullptr);

		if (CaptureType == EEditorCaptureType::CCT_SkeletalMesh && CaptureDevice->IsInitialized())
		{
			CaptureDevice->GetSkeletalMeshPose(MeshPoseSnapshot, bScaledCapture);
			SkeletalBodyMesh->SetWorldTransform(BodyMesh->GetComponentTransform());

			// update skeletal mesh
			UAnimInstance* SkelAnimInstahce = SkeletalBodyMesh->GetAnimInstance();
			if (SkelAnimInstahce)
			{
				if (UCaptureAnimBlueprint* CaptureAnimInst = Cast<UCaptureAnimBlueprint>(SkelAnimInstahce))
				{
					CaptureAnimInst->CaptureDevice = CaptureDevice;
					CaptureAnimInst->bIsCaptureActive = true;
					CaptureAnimInst->CurrentPose = MeshPoseSnapshot;
				}
			}
		}
		else
		{
			CaptureDevice->UpdatePoseableMesh(bScaledCapture, BodyMesh);
			CaptureDevice->GetLastPoseSnapshot(MeshPoseSnapshot);
		}
	}

	if (bHideCaptureMesh)
	{
		BodyMesh->SetVisibility(false);
		SkeletalBodyMesh->SetVisibility(false);
	}
	else
	{
		BodyMesh->SetVisibility(CaptureType == EEditorCaptureType::CCT_PoseableMesh);
		SkeletalBodyMesh->SetVisibility(CaptureType == EEditorCaptureType::CCT_SkeletalMesh);
	}

	if (bDebug)
	{
		for (int32 Index = 0; Index < InputCompsNum; Index++)
		{
			const USceneComponent* Comp = CaptureDevice->GetInputComponent(Index);
			if (Comp)
			{
				DrawDebugSphere(GetWorld(), Comp->GetComponentLocation(), 5, 3, FColor::Red, false, 0.05f, 0, 0.5f);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Invalid input component %d"), Index);
			}
		}
	}
}

void AEditorViveMocapController::InitializeDevice()
{
	if (!IsValid(InputController))
	{
		UE_LOG(LogTemp, Warning, TEXT("AEditorViveMocapController. Invalid InputController Reference."));
		return;
	}

	if (!BuildTPose())
	{
		UE_LOG(LogTemp, Warning, TEXT("AEditorViveMocapController. Can't build T-pose for mesh, check BodyMesh settings CaptureDevice bones map."));
		return;
	}

	SkeletalBodyMesh->SetUpdateAnimationInEditor(true);

	TInlineComponentArray<UActorComponent*> Comps;
	GetComponents(USkeletalMeshComponent::StaticClass(), Comps);
	for (auto& Comp : Comps)
	{
		if (AdditionalEditorSkeletalMeshComponents.Contains(Comp->GetFName()))
		{
			USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(Comp);
			if (SkelComp)
			{
				SkelComp->SetUpdateAnimationInEditor(true);
				UE_LOG(LogTemp, Log, TEXT("Component %s - animation enabled in editor"), *Comp->GetName());
			}
		}
	}

	if (!CaptureDevice->IsInitialized())
	{
		CaptureDevice->InitializeReferences(BodyMesh);
	}
	CaptureDevice->UpdateSkeletonSetup();

	TArray<USceneComponent*> TrackerComps;
	uint8 RightId = 255, LeftId = 255;
	GetInputComponents(TrackerComps, RightId, LeftId);

	if (TrackerComps.Num() > 3)
	{
		CaptureDevice->InitializeInputFromComponents(TrackerComps, RightId, LeftId);
		InputCompsNum = TrackerComps.Num();

		bInitialized = CaptureDevice->IsInitialized();
		UE_LOG(LogTemp, Log, TEXT("InitializeDevice. Result = %d"), (int32)bInitialized);

		if (bInitialized)
		{
			CaptureDevice->InitializeFingers(true);
		}
	}
}

void AEditorViveMocapController::GetInputComponents(TArray<USceneComponent*>& InputTrackers, uint8& RightId, uint8& LeftId)
{
	if (!IsValid(InputController))
	{
		UE_LOG(LogTemp, Warning, TEXT("GetInputComponents. Invalid InputController Reference."));
		return;
	}
	if (!InputController->EnsureUpdated())
	{
		UE_LOG(LogTemp, Warning, TEXT("GetInputComponents. InputController can't initialize input components."));
		return;
	}

	InputTrackers.Empty();
	TArray<FSteamVRTrackingBinding> Trackers;
	InputController->GetUpdatedObjects(Trackers);

	uint8 Index = 0;
	for (auto& TrackerSetup : Trackers)
	{
		InputTrackers.Add(TrackerSetup.AttachedComponent);
		if (TrackerSetup.MotionSource.IsEqual(TEXT("Right")))
		{
			RightId = Index;
		}
		else if (TrackerSetup.MotionSource.IsEqual(TEXT("Left")))
		{
			LeftId = Index;
		}
		if (TrackerSetup.AttachedComponent)
		{
			UE_LOG(LogTemp, Log, TEXT("InitializeDevice. Input Device %d - %s"), Index, *TrackerSetup.AttachedComponent->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("InitializeDevice. Input Device %d - invalid input"), Index);
		}
		Index++;
	}
}

void AEditorViveMocapController::StartMocap()
{
	if (!bInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("AEditorViveMocapController: Actor wasn't initialized. Check InputController."));
		return;
	}
	if (!CaptureDevice->IsInitialized())
	{
		UE_LOG(LogTemp, Warning, TEXT("AEditorViveMocapController: CaptureDevice wasn't initialized. Check Skeletal Mesh."));
		bInitialized = false;
		return;
	}

	if (!CaptureDevice->IsCalibrated())
	{
		FBodyCalibrationData Calibration;
		GetBodyCalibration(Calibration);
		if (Calibration.TrackersData.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("AEditorViveMocapController: Can't find calibration. Check overrided GetBodyCalibration function."));
			return;
		}
		CaptureDevice->ApplyCurrentCalibration(Calibration);

		if (!CaptureDevice->IsCalibrated())
		{
			UE_LOG(LogTemp, Warning, TEXT("AEditorViveMocapController: Can't find calibration. Calibration apply error."));
			return;
		}
	}

	if (CaptureDevice->TrackersData.Num() != InputCompsNum)
	{
		TArray<USceneComponent*> TrackerComps;
		uint8 RightId = 255, LeftId = 255;
		GetInputComponents(TrackerComps, RightId, LeftId);
		if (TrackerComps.Num() == CaptureDevice->TrackersData.Num())
		{
			CaptureDevice->InitializeInputFromComponents(TrackerComps, RightId, LeftId);
			InputCompsNum = TrackerComps.Num();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AEditorViveMocapController: Can't calibrate. Mocap devices num: %d / Current devices num: %d"), CaptureDevice->TrackersData.Num(), TrackerComps.Num());
			return;
		}
	}

	CaptureDevice->ToggleCapture(true);
	bEnabled = true;
}

void AEditorViveMocapController::StopMocap()
{
	CaptureDevice->ToggleCapture(false);
	bEnabled = false;
}

void AEditorViveMocapController::GetBodyCalibration(FBodyCalibrationData& OutCalibration)
{
	USessionCalibrationSave* gs = Cast<USessionCalibrationSave>(UGameplayStatics::LoadGameFromSlot(CalibrationFileName.ToString(), 0));

	if (gs)
	{
		OutCalibration = gs->Data;
	}
	else
	{
		OutCalibration.ForearmLength = -1.f;
	}
}

bool AEditorViveMocapController::BuildTPose()
{
	// can't update mesh?
	if (!BodyMesh->SkeletalMesh)
	{
		return false;
	}

	BodyMesh->SetWorldLocationAndRotation(FVector::ZeroVector, DefaultSkeletalMeshRotation);

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
	const FReferenceSkeleton& RefSkeleton = BodyMesh->SkeletalMesh->GetRefSkeleton();
#else
	const FReferenceSkeleton& RefSkeleton = BodyMesh->SkeletalMesh->RefSkeleton;
#endif
	const int32 BonesNum = RefSkeleton.GetRefBonePose().Num();

	for (int32 BoneIndex = 0; BoneIndex < BonesNum; BoneIndex++)
	{
		const FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
		BodyMesh->SetBoneTransformByName(BoneName, RestoreRefBonePose_WS(BoneName), EBoneSpaces::WorldSpace);
	}

	const FName* UpperarmRight = CaptureDevice->SkeletonBonesMap.Find(EHumanoidBone::UpperarmRight);
	const FName* LowerarmRight = CaptureDevice->SkeletonBonesMap.Find(EHumanoidBone::ForearmRight);
	const FName* HandRight = CaptureDevice->SkeletonBonesMap.Find(EHumanoidBone::PalmRight);
	const FName* UpperarmLeft = CaptureDevice->SkeletonBonesMap.Find(EHumanoidBone::UpperarmLeft);
	const FName* LowerarmLeft = CaptureDevice->SkeletonBonesMap.Find(EHumanoidBone::ForearmLeft);
	const FName* HandLeft = CaptureDevice->SkeletonBonesMap.Find(EHumanoidBone::PalmLeft);

	if (UpperarmRight && LowerarmRight && HandRight && UpperarmLeft && LowerarmLeft && HandLeft)
	{
		// Find component forward axis (Y usually)
		FTransform HandRTr = BodyMesh->GetBoneTransformByName(*HandRight, EBoneSpaces::WorldSpace);
		FTransform HandLTr = BodyMesh->GetBoneTransformByName(*HandLeft, EBoneSpaces::WorldSpace);

		// body forward in world space
		const FVector SkMRight = (HandRTr.GetTranslation() - HandLTr.GetTranslation()).GetSafeNormal2D();
		FVector SkMForward = SkMRight.RotateAngleAxis(-90.f, FVector(0.f, 0.f, 1.f));

		ComponentSpaceSetup.ForwardAxis = FindCoDirection(BodyMesh->GetComponentRotation(), SkMForward, ComponentSpaceSetup.ForwardDirection);
		ComponentSpaceSetup.HorizontalAxis = FindCoDirection(BodyMesh->GetComponentRotation(), SkMRight, ComponentSpaceSetup.RightDirection);
		ComponentSpaceSetup.VerticalAxis = FindCoDirection(BodyMesh->GetComponentRotation(), BodyMesh->GetUpVector(), ComponentSpaceSetup.UpDirection);

		// Hands orientation
		HandRTr = BodyMesh->GetBoneTransformByName(*HandRight, EBoneSpaces::ComponentSpace);
		HandLTr = BodyMesh->GetBoneTransformByName(*HandLeft, EBoneSpaces::ComponentSpace);
		FTransform UpperarmTr = BodyMesh->GetBoneTransformByName(*UpperarmRight, EBoneSpaces::ComponentSpace);

		FVector HandForwardDirection = (HandRTr.GetTranslation() - UpperarmTr.GetTranslation()).GetSafeNormal();
		if (FMath::Abs(FVector::DotProduct(FVector::UpVector, HandForwardDirection)) < 0.2f)
		{
			// hands are horizontal in reference pose
			return true;
		}

		// character forward
		SkMForward = (HandRTr.GetTranslation() - HandLTr.GetTranslation()).GetSafeNormal2D();
		SkMForward = SkMForward.RotateAngleAxis(-90.f, FVector(0.f, 0.f, 1.f));

		// right hand
		FVMK_BoneRotatorSetup RightHandSetup, LeftHandSetup;
		RightHandSetup.ForwardAxis = FindCoDirection(UpperarmTr.Rotator(), HandForwardDirection, RightHandSetup.ForwardDirection);
		RightHandSetup.HorizontalAxis = FindCoDirection(UpperarmTr.Rotator(), SkMForward * -1.f, RightHandSetup.RightDirection);

		{
			// right hand
			FRotator HandRotFinalR = MakeRotByTwoAxes(
				RightHandSetup.ForwardAxis, ComponentRightVector * RightHandSetup.ForwardDirection,
				RightHandSetup.HorizontalAxis, ComponentForwardVector * -1.f * RightHandSetup.RightDirection);

			BodyMesh->SetBoneRotationByName(*UpperarmRight, HandRotFinalR, EBoneSpaces::ComponentSpace);
			BodyMesh->SetBoneRotationByName(*LowerarmRight, HandRotFinalR, EBoneSpaces::ComponentSpace);

			// left hand
			UpperarmTr = BodyMesh->GetBoneTransformByName(*UpperarmLeft, EBoneSpaces::ComponentSpace);
			HandForwardDirection = (HandLTr.GetTranslation() - UpperarmTr.GetTranslation()).GetSafeNormal();

			LeftHandSetup.ForwardAxis = FindCoDirection(UpperarmTr.Rotator(), HandForwardDirection, LeftHandSetup.ForwardDirection);
			LeftHandSetup.HorizontalAxis = FindCoDirection(UpperarmTr.Rotator(), SkMForward, LeftHandSetup.RightDirection);

			const FRotator HandRotFinalL = MakeRotByTwoAxes(
				LeftHandSetup.ForwardAxis, ComponentRightVector * -1.f * LeftHandSetup.ForwardDirection,
				LeftHandSetup.HorizontalAxis, ComponentForwardVector * LeftHandSetup.RightDirection);

			BodyMesh->SetBoneRotationByName(*UpperarmLeft, HandRotFinalL, EBoneSpaces::ComponentSpace);
			BodyMesh->SetBoneRotationByName(*LowerarmLeft, HandRotFinalL, EBoneSpaces::ComponentSpace);
		}
	}
	else
	{
		return false;
	}

	return true;
}

FTransform AEditorViveMocapController::RestoreRefBonePose_WS(const FName& BoneName) const
{
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
	const FReferenceSkeleton& RefSkeleton = BodyMesh->SkeletalMesh->GetRefSkeleton();
#else
	const FReferenceSkeleton& RefSkeleton = BodyMesh->SkeletalMesh->RefSkeleton;
#endif
	const TArray<FTransform>& RefPoseSpaceBaseTMs = RefSkeleton.GetRefBonePose();

	// calculate world transform transform
	int32 TransformIndex = RefSkeleton.FindBoneIndex(BoneName);
	int32 ParentIndex = RefSkeleton.GetParentIndex(TransformIndex);
	FTransform tr_bone = RefPoseSpaceBaseTMs[TransformIndex];
	FTransform tr_par;

	// stop on root
	while (ParentIndex != INDEX_NONE)
	{
		TransformIndex = ParentIndex;

		tr_par = RefPoseSpaceBaseTMs[TransformIndex];
		tr_par.NormalizeRotation();

		tr_bone.NormalizeRotation();
		tr_bone = tr_bone * tr_par;

		ParentIndex = RefSkeleton.GetParentIndex(TransformIndex);
	}
	// add component transform
	tr_bone.NormalizeRotation();
	tr_bone = tr_bone * BodyMesh->GetComponentTransform();

	return tr_bone;
}

/* Helper for rig building. Find axis of rotator the closest to being parallel to the specified vectors. Returns +1.f in Multiplier if co-directed and -1.f otherwise */
EAxis::Type AEditorViveMocapController::FindCoDirection(const FRotator& BoneRotator, const FVector& Direction, float& ResultMultiplier)
{
	EAxis::Type RetAxis;
	FVector dir = Direction;
	dir.Normalize();

	const FVector v1 = RotatorDirection(BoneRotator, EAxis::X);
	const FVector v2 = RotatorDirection(BoneRotator, EAxis::Y);
	const FVector v3 = RotatorDirection(BoneRotator, EAxis::Z);

	const float dp1 = FVector::DotProduct(v1, dir);
	const float dp2 = FVector::DotProduct(v2, dir);
	const float dp3 = FVector::DotProduct(v3, dir);

	if (FMath::Abs(dp1) > FMath::Abs(dp2) && FMath::Abs(dp1) > FMath::Abs(dp3))
	{
		RetAxis = EAxis::X;
		ResultMultiplier = dp1 > 0.f ? 1.f : -1.f;
	}
	else if (FMath::Abs(dp2) > FMath::Abs(dp1) && FMath::Abs(dp2) > FMath::Abs(dp3))
	{
		RetAxis = EAxis::Y;
		ResultMultiplier = dp2 > 0.f ? 1.f : -1.f;
	}
	else
	{
		RetAxis = EAxis::Z;
		ResultMultiplier = dp3 > 0.f ? 1.f : -1.f;
	}

	return RetAxis;
}

/* Helper. Build rotator by two axis. */
FRotator AEditorViveMocapController::MakeRotByTwoAxes(EAxis::Type MainAxis, FVector MainAxisDirection, EAxis::Type SecondaryAxis, FVector SecondaryAxisDirection) const
{
	FRotator r;

	if (MainAxis == EAxis::X)
	{
		if (SecondaryAxis == EAxis::Y)
			r = FRotationMatrix::MakeFromXY(MainAxisDirection, SecondaryAxisDirection).Rotator();
		else if (SecondaryAxis == EAxis::Z)
			r = FRotationMatrix::MakeFromXZ(MainAxisDirection, SecondaryAxisDirection).Rotator();
	}
	else if (MainAxis == EAxis::Y)
	{
		if (SecondaryAxis == EAxis::X)
			r = FRotationMatrix::MakeFromYX(MainAxisDirection, SecondaryAxisDirection).Rotator();
		else if (SecondaryAxis == EAxis::Z)
			r = FRotationMatrix::MakeFromYZ(MainAxisDirection, SecondaryAxisDirection).Rotator();
	}
	else if (MainAxis == EAxis::Z)
	{
		if (SecondaryAxis == EAxis::X)
			r = FRotationMatrix::MakeFromZX(MainAxisDirection, SecondaryAxisDirection).Rotator();
		else if (SecondaryAxis == EAxis::Y)
			r = FRotationMatrix::MakeFromZY(MainAxisDirection, SecondaryAxisDirection).Rotator();
	}

	return r;
}