// (c) YuriNK (ykasczc@gmail.com), 2020. You're free to use it whatever way you want.

#include "SteamVRTrackingStyle.h"
#include "Styling/SlateStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

// Const icon sizes
static const FVector2D Icon12x12(12.0f, 12.0f);
static const FVector2D Icon32x32(32.0f, 32.0f);

// The private ones
TSharedPtr<FSlateStyleSet> FSteamVRTrackingStyle::StyleSet = nullptr;
FString FSteamVRTrackingStyle::EngineContentRoot = FString();
FString FSteamVRTrackingStyle::PluginContentRoot = FString();
FString FSteamVRTrackingStyle::PluginResourcesRoot = FString();

void FSteamVRTrackingStyle::Initialize()
{
	// Only register once
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	EngineContentRoot = FPaths::EngineContentDir() / TEXT("Editor/Slate");
	TSharedPtr<IPlugin> CurrentPlugin = IPluginManager::Get().FindPlugin("SteamVRTrackingLib");
	if (CurrentPlugin.IsValid())
	{
		StyleSet->SetContentRoot(CurrentPlugin->GetContentDir());
		PluginContentRoot = CurrentPlugin->GetContentDir();
		PluginResourcesRoot = CurrentPlugin->GetBaseDir() / TEXT("Resources");
	}
	else
	{
		return;
	}

	StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	StyleSet->Set(TEXT("SteamVRTrackingLib.Button.DeleteSmall"), new FSlateImageBrush(GetEngineContentPath("Icons/Cross_12x.png"), FVector2D(12, 12)));
	StyleSet->Set(TEXT("SteamVRTrackingLib.Background"), new FSlateColorBrush(FColor(128, 128, 128, 255)));
	StyleSet->Set(TEXT("SteamVRTrackingLib.Border.Normal"), new FSlateColorBrush(FColor(255, 127, 39, 255)));
	StyleSet->Set(TEXT("SteamVRTrackingLib.Border.Selected"), new FSlateColorBrush(FColor(172, 172, 172, 255)));
	
	StyleSet->Set(TEXT("SteamVRTrackingLib.Device.MotionControllerOff"), new FSlateImageBrush(GetPluginResourcesPath("MotionController_Off.png"), Icon32x32));
	StyleSet->Set(TEXT("SteamVRTrackingLib.Device.MotionControllerOn"), new FSlateImageBrush(GetPluginResourcesPath("MotionController_On.png"), Icon32x32));
	StyleSet->Set(TEXT("SteamVRTrackingLib.Device.ViveTrackerOff"), new FSlateImageBrush(GetPluginResourcesPath("ViveTracker_Off.png"), Icon32x32));
	StyleSet->Set(TEXT("SteamVRTrackingLib.Device.ViveTrackerOn"), new FSlateImageBrush(GetPluginResourcesPath("ViveTracker_On.png"), Icon32x32));
	
	// Register the current style
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FSteamVRTrackingStyle::Shutdown()
{
	// unregister the style
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

