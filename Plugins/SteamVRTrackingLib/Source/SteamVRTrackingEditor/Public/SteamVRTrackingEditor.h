// VR IK Body Component
// (c) Yuri N Kalinin, 2017, ykasczc@gmail.com. All right reserved.

#pragma once
 
#include "Modules/ModuleManager.h"
#include "Framework/Commands/UICommandList.h"
#include "AssetData.h"
 
class FSteamVRTrackingEditor : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void ExportTrackingSetupToJson(TArray<FAssetData> SelectedAssets);
	void ImportTrackingSetupFromJson(TArray<FAssetData> SelectedAssets);

private:
	TSharedPtr<FUICommandList> CommandList;
	FDelegateHandle ContentBrowserMenuExtenderHandle;
};