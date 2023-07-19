


#include "BPFL_FileIO.h"
#include <Runtime\Core\Public\Misc\Paths.h>
#include <Runtime\Core\Public\HAL\PlatformFilemanager.h>

FString UBPFL_FileIO::LoadFileToString(FString Filename)
{
	FString directory = FPaths::ProjectContentDir();
	FString result;
	IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();

	if (file.CreateDirectory(*directory)) {
		FString myFile = directory + "/" + Filename;
		FFileHelper::LoadFileToString(result, *myFile);
	}

	return result;
}

TArray<FString> UBPFL_FileIO::LoadFileToStringArray(FString Filename)
{
	FString directory = FPaths::ProjectContentDir();
	TArray<FString> result;
	IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();

	if (file.CreateDirectory(*directory)) {
		FString myFile = directory + "/" + Filename;
		FFileHelper::LoadFileToStringArray(result, *myFile);
	}

	return result;
}

bool UBPFL_FileIO::SaveStringToFile(FString String, FString Filename)
{
	FString directory = FPaths::ProjectContentDir();
	bool result = false;
	IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();

	if (file.CreateDirectory(*directory)) {
		FString myFile = directory + "/" + Filename;
		result = FFileHelper::SaveStringToFile(String, *myFile);
	}

	return result;
}

bool UBPFL_FileIO::SaveStringArrayToFile(FString Filename, TArray<FString> StringArrayToSave)
{
	FString directory = FPaths::ProjectContentDir();
	bool result = false;
	IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();

	if (file.CreateDirectory(*directory)) {
		FString myFile = directory + "/" + Filename;
		result = FFileHelper::SaveStringArrayToFile(StringArrayToSave, *myFile);
	}

	return result;
}

