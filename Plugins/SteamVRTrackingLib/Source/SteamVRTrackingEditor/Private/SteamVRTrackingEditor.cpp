// VR IK Body Component
// (c) Yuri N Kalinin, 2017, ykasczc@gmail.com. All right reserved.

#include "SteamVRTrackingEditor.h"
#include "PropertyEditorModule.h"
#include "ContentBrowserModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "EditorDirectories.h"
#include "ScopedTransaction.h"

#include "SteamVRTrackingSetupDetailsCustomization.h"
#include "SteamVRTrackingSetup.h"
#include "SteamVRTrackingStyle.h"
#include "SteamVRTrackingLibBPLibrary.h"

#define LOCTEXT_NAMESPACE "FSteamVRTrackingEditor"

void FSteamVRTrackingEditor::StartupModule()
{
	// Initialize Style
	FSteamVRTrackingStyle::Initialize();
		
	// Details Customization
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	PropertyModule.RegisterCustomClassLayout(USteamVRTrackingSetup::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FSteamVRTrackingSetupDetailsCustomization::MakeInstance));

	// Custom Popup Menu Items
	CommandList = MakeShareable(new FUICommandList);

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	ContentBrowserModule.GetAllAssetViewContextMenuExtenders().Add(FContentBrowserMenuExtender_SelectedAssets::CreateLambda([this](const TArray<FAssetData>& SelectedAssets)
	{
		TSharedRef<FExtender> Extender = MakeShared<FExtender>();

		if (SelectedAssets.ContainsByPredicate([](const FAssetData& AssetData) { return AssetData.GetClass() == USteamVRTrackingSetup::StaticClass(); }))
		{
			Extender->AddMenuExtension(
				"AssetContextMoveActions",
				EExtensionHook::After,
				CommandList,
				FMenuExtensionDelegate::CreateLambda([this, SelectedAssets](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("STR_ExportTrackingJson", "Export to JSON..."),
					LOCTEXT("STR_ExportTrackingJsonToolTip", "Export SteamVR tracking setup to JSON file"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateRaw(this, &FSteamVRTrackingEditor::ExportTrackingSetupToJson, SelectedAssets)));
			}));

			Extender->AddMenuExtension(
				"CommonAssetActions",
				EExtensionHook::After,
				CommandList,
				FMenuExtensionDelegate::CreateLambda([this, SelectedAssets](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("STR_ImportTrackingJson", "Import from JSON..."),
					LOCTEXT("STR_ImportTrackingJsonToolTip", "Import SteamVR tracking setup from JSON file"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateRaw(this, &FSteamVRTrackingEditor::ImportTrackingSetupFromJson, SelectedAssets)));
			}));
		}

		return Extender;
	}));
	ContentBrowserMenuExtenderHandle = ContentBrowserModule.GetAllAssetViewContextMenuExtenders().Last().GetHandle();

}

void FSteamVRTrackingEditor::ShutdownModule()
{
	// Unregister details customization
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout(USteamVRTrackingSetup::StaticClass()->GetFName());
	
	// Unregister style set
	FSteamVRTrackingStyle::Shutdown();
}

void FSteamVRTrackingEditor::ExportTrackingSetupToJson(TArray<FAssetData> SelectedAssets)
{
	IDesktopPlatform* const DesktopPlatform = FDesktopPlatformModule::Get();

	const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	for (const auto& Asset : SelectedAssets)
	{
		if (Asset.GetClass() == USteamVRTrackingSetup::StaticClass())
		{
			const FString DefaultPath = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT);

			TArray<FString> OutFiles;
			if (DesktopPlatform->SaveFileDialog(
				ParentWindowWindowHandle,
				LOCTEXT("ExportTrackingJsonTitle", "Choose file name to save...").ToString(),
				DefaultPath,
				TEXT(""),
				TEXT("JavaScript Object Notation (*.json)|*.json"),
				EFileDialogFlags::None,
				OutFiles
			))
			{
				const USteamVRTrackingSetup* SteamVRTrackingSetup = Cast<USteamVRTrackingSetup>(Asset.GetAsset());
				if (SteamVRTrackingSetup && OutFiles.Num() > 0)
				{
					TArray<FSteamVRDeviceBindingSetup> TrackingSetupArray;
					SteamVRTrackingSetup->DeviceSetup.GenerateValueArray(TrackingSetupArray);
					USteamVRTrackingLibBPLibrary::ExportTrackingSetupToJSON(TrackingSetupArray, OutFiles[0]);
				}
			}
		}
	}
}

void FSteamVRTrackingEditor::ImportTrackingSetupFromJson(TArray<FAssetData> SelectedAssets)
{
	IDesktopPlatform* const DesktopPlatform = FDesktopPlatformModule::Get();

	const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	for (const auto& Asset : SelectedAssets)
	{
		if (Asset.GetClass() == USteamVRTrackingSetup::StaticClass())
		{
			const FString DefaultPath = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT);

			TArray<FString> OutFiles;
			if (DesktopPlatform->OpenFileDialog(
				ParentWindowWindowHandle,
				LOCTEXT("ExportTrackingJsonTitle", "Choose file name to import...").ToString(),
				DefaultPath,
				TEXT(""),
				TEXT("JavaScript Object Notation (*.json)|*.json"),
				EFileDialogFlags::None,
				OutFiles
			))
			{
				USteamVRTrackingSetup* SteamVRTrackingSetup = Cast<USteamVRTrackingSetup>(Asset.GetAsset());
				if (SteamVRTrackingSetup && OutFiles.Num() > 0)
				{
					TArray<FSteamVRDeviceBindingSetup> TrackingSetupArray;
					USteamVRTrackingLibBPLibrary::ImportTrackingSetupFromJSON(TrackingSetupArray, OutFiles[0]);

					const FScopedTransaction Transaction(LOCTEXT("SteamVRTrackingSetupImport", "Import SteamVR Tracking Setup from JSON"));
					SteamVRTrackingSetup->Modify();

					SteamVRTrackingSetup->DeviceSetup.Empty();
					for (const auto& Value : TrackingSetupArray)
					{
						SteamVRTrackingSetup->DeviceSetup.Add(Value.SerialNumber, Value);
					}
				}
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSteamVRTrackingEditor, SteamVRTrackingEditor)
