


#include "BPFL_TextFileManager.h"
#include <Runtime\Core\Public\Misc\FileHelper.h>
#include <Runtime\Core\Public\HAL\PlatformFilemanager.h>

bool UBPFL_TextFileManager::SaveArrayText(FString saveDirectory, FString fileName, TArray<FString> saveText, bool allowOverwriting = false)
{
	saveDirectory += "\\";
	saveDirectory += fileName;

	if (!allowOverwriting)
	{
		if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*saveDirectory))
		{
			return false;
		}
	}

	FString finalString = "";
	for (FString& Each : saveText)
	{
		finalString += Each;
		finalString += LINE_TERMINATOR;
	}

	return FFileHelper::SaveStringToFile(finalString, *saveDirectory);

}

