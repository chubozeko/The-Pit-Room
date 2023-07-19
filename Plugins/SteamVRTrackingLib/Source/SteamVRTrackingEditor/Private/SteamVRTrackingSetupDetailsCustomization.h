// Copyright YuriNK (Garden Horse Studio). All Rights Reserved, 2018
// ykasczc@gmail.com

#pragma once

#include "IDetailCustomization.h"
#include "SteamVRTrackingSetup.h"

class IDetailCategoryBuilder;

class FSteamVRTrackingSetupDetailsCustomization : public IDetailCustomization
{
public:
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	static TSharedRef<IDetailCustomization> MakeInstance();

	void OnDeviceSelected(USteamVRTrackingSetup* SetupObject, const FName& SerialNumber);
	void OnDeviceRemove(USteamVRTrackingSetup* SetupObject, const FName& SerialNumber);
	void OnDeviceUpdateName(USteamVRTrackingSetup* SetupObject, const FName& SerialNumber, const FName& NewName);
	FReply OnUpdateButtonClick();

	void UpdateWidgetsList();

private:
	USteamVRTrackingSetup* CustomizedItem;
	FSteamVRDeviceBindingSetup EmptyBindingItem;

	TArray<TSharedPtr<class SDeviceSerialBinding>> RowWidgets;
	TSet<FName> ActiveWidgetSerials;
	int32 ActiveWidgets;
};