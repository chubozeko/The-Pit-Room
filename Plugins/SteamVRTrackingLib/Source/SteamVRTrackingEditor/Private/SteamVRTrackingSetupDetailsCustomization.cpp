// Copyright YuriNK (Garden Horse Studio). All Rights Reserved, 2018
// ykasczc@gmail.com

#include "SteamVRTrackingSetupDetailsCustomization.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "SteamVRFunctionLibrary.h"
#include "Styling/CoreStyle.h"
#include "EditorStyleSet.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "SDeviceSerialBinding.h"
#include "ScopedTransaction.h"
// Runtime Module
#include "SteamVRTrackingSetup.h"
#include "SteamVRTrackingLibBPLibrary.h"

#define LOCTEXT_NAMESPACE "SteamVRTrackingSetupDetailsCustomization"

#define CATEGORY_SteamVRTrackingSetup "SteamVR Tracking Setup"

void FSteamVRTrackingSetupDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	if (ObjectsBeingCustomized.Num() != 1)
	{
		return;
	}

	TWeakObjectPtr<UObject> obj = ObjectsBeingCustomized[0];
	CustomizedItem = CastChecked<USteamVRTrackingSetup>(obj.Get());
	if (!CustomizedItem)
	{
		return;
	}
	IDetailCategoryBuilder& DevicesCategory = DetailBuilder.EditCategory(TEXT("SteamVR Devices Setup"), FText::GetEmpty());
	DetailBuilder.HideCategory(CATEGORY_SteamVRTrackingSetup);

	// Remove empty saved devices
	CustomizedItem->DeviceSetup.Remove(FName());

	// Create Update Button
	DevicesCategory.AddCustomRow(FText::FromString(TEXT("BT_Update")))
	.WholeRowContent()
	[
		SNew(SButton)
		.ButtonStyle(FEditorStyle::Get(), "NoBorder")
		.ContentPadding(4.f)
		.OnClicked(this, &FSteamVRTrackingSetupDetailsCustomization::OnUpdateButtonClick)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Update Devices List")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
			.ColorAndOpacity(FSlateColor(FLinearColor::White))
			.Justification(ETextJustify::Center)
		]
	];

	// Create hidden widgets for future tracked devices
	for (int32 Index = 0; Index < 24; Index++)
	{
		FString RowName = TEXT("Device_") + FString::FromInt(Index);
		TSharedPtr<SDeviceSerialBinding> NewRowWidget;

		DevicesCategory.AddCustomRow(FText::FromString(RowName))
		.WholeRowContent()
		[
			SAssignNew(NewRowWidget, SDeviceSerialBinding)
			.ModifiedObject(CustomizedItem)
			.DeviceData(&EmptyBindingItem)
			.TrackingStatus(true)
			.Visibility(EVisibility::Collapsed)
			.OnSelected(this, &FSteamVRTrackingSetupDetailsCustomization::OnDeviceSelected)
			.OnRemove(this, &FSteamVRTrackingSetupDetailsCustomization::OnDeviceRemove)
			.OnUpdateName(this, &FSteamVRTrackingSetupDetailsCustomization::OnDeviceUpdateName)
		]
		.ValueWidget.HorizontalAlignment = EHorizontalAlignment::HAlign_Fill;

		RowWidgets.Add(NewRowWidget);
	}

	ActiveWidgets = 0;
	ActiveWidgetSerials.Empty();

	UpdateWidgetsList();
}

void FSteamVRTrackingSetupDetailsCustomization::UpdateWidgetsList()
{
	// Get connected devices
	TArray<int32> ControllerIds, TrackerIds, ActiveIds;
	USteamVRFunctionLibrary::GetValidTrackedDeviceIds(ESteamVRTrackedDeviceType::Controller, ControllerIds);
	USteamVRFunctionLibrary::GetValidTrackedDeviceIds(ESteamVRTrackedDeviceType::Other, TrackerIds);
	ActiveIds.Append(ControllerIds);
	ActiveIds.Append(TrackerIds);

	// Update existing and add connected devices
	for (const auto& DeviceId : ActiveIds)
	{
		FString szSerial = USteamVRTrackingLibBPLibrary::GetTrackedDeviceSerialNumber(DeviceId);
		FName Serial = *szSerial;

		FSteamVRDeviceBindingSetup* DeviceBinding = CustomizedItem->DeviceSetup.Find(Serial);
		if (DeviceBinding)
		{
			DeviceBinding->Id = DeviceId;
		}
		else
		{
			FSteamVRDeviceBindingSetup NewItem;
			NewItem.SerialNumber = Serial;
			NewItem.Id = DeviceId;
			NewItem.Type = ControllerIds.Contains(DeviceId) ? ESteamVRTrackedDeviceType::Controller : ESteamVRTrackedDeviceType::Other;

			CustomizedItem->DeviceSetup.Add(Serial, NewItem);
		}
	}

	// Create edit widgets
	for (auto& Device : CustomizedItem->DeviceSetup)
	{
		// currently device is turned off
		if (!ActiveIds.Contains(Device.Value.Id))
		{
			Device.Value.Id = INDEX_NONE;
		}

		// add widget
		if (!ActiveWidgetSerials.Contains(Device.Key))
		{
			if (ActiveWidgets < RowWidgets.Num())
			{
				RowWidgets[ActiveWidgets]->ReconstructObject(&Device.Value);
				RowWidgets[ActiveWidgets]->SetVisibility(EVisibility::Visible);

				ActiveWidgets++;
			}
			ActiveWidgetSerials.Add(Device.Key);
		}
	}
}

void FSteamVRTrackingSetupDetailsCustomization::OnDeviceUpdateName(USteamVRTrackingSetup* SetupObject, const FName& SerialNumber, const FName& NewName)
{
	if (FSteamVRDeviceBindingSetup* DeviceBinding = CustomizedItem->DeviceSetup.Find(SerialNumber))
	{
		if (!DeviceBinding->FriendlyName.IsEqual(NewName))
		{
			const FScopedTransaction Transaction(LOCTEXT("TrackedDeviceNameUpdate", "Update name of Tracked Device"));
			CustomizedItem->Modify();
			DeviceBinding->FriendlyName = NewName;
		}
	}
}

void FSteamVRTrackingSetupDetailsCustomization::OnDeviceSelected(USteamVRTrackingSetup* SetupObject, const FName& SerialNumber)
{
	for (auto& Widget : RowWidgets)
	{
		if (!Widget->GetSerialNumber().IsEqual(SerialNumber))
		{
			Widget->MarkUnselected();
		}
	}
}

void FSteamVRTrackingSetupDetailsCustomization::OnDeviceRemove(USteamVRTrackingSetup* SetupObject, const FName& SerialNumber)
{
	if (FSteamVRDeviceBindingSetup* DeviceBinding = CustomizedItem->DeviceSetup.Find(SerialNumber))
	{
		if (DeviceBinding->Id != INDEX_NONE)
		{
			return;
		}
	}

	const FScopedTransaction Transaction(LOCTEXT("TrackedDeviceDeleteRecord", "Delete Saved Tracked Device"));
	CustomizedItem->Modify();
	CustomizedItem->DeviceSetup.Remove(SerialNumber);

	for (auto& Widget : RowWidgets)
	{
		if (Widget->GetSerialNumber().IsEqual(SerialNumber))
		{
			Widget->SetVisibility(EVisibility::Collapsed);
			break;
		}
	}
}

FReply FSteamVRTrackingSetupDetailsCustomization::OnUpdateButtonClick()
{
	UpdateWidgetsList();
	return FReply::Handled();
}

TSharedRef<IDetailCustomization> FSteamVRTrackingSetupDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FSteamVRTrackingSetupDetailsCustomization);
}

#undef CATEGORY_SteamVRTrackingSetup
#undef LOCTEXT_NAMESPACE