// (c) YuriNK, 2017

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ViveMocapTypes.h"
#include "SessionCalibrationSave.generated.h"

/**
 * 
 */
UCLASS()
class STEAMVRTRACKINGLIB_API USessionCalibrationSave : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = "Session Calibration")
	FBodyCalibrationData Data;
};
