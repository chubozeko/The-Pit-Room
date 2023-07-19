

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BPFL_TextFileManager.generated.h"

/**
 * 
 */
UCLASS()
class MTHESIS_VR_API UBPFL_TextFileManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

		UFUNCTION(BlueprintCallable, Category = "File I/O", meta = (Keywords = "Save"))
		static bool SaveArrayText(FString saveDirectory, FString fileName, TArray<FString> saveText, bool allowOverwriting);

};
