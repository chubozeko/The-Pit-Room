// (c) YuriNK (ykasczc@gmail.com), 2020. You're free to use it whatever way you want.

#include "SDeviceSerialBinding.h"
#include "SlateCore/Private/Application/ActiveTimerHandle.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/CoreStyle.h"
#include "EditorStyleSet.h"
#include "SteamVRTrackingStyle.h"
#include "Editor.h"
// Runtime Module
#include "SteamVRTrackingSetup.h"
#include "SteamVRFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "DeviceSerialNumberBinding"

void SDeviceSerialBinding::Construct(const FArguments& InArgs)
{
	TrackingSetupObject = InArgs._ModifiedObject;
	OnNodeWasSelected = InArgs._OnSelected;
	OnRemoveRequest = InArgs._OnRemove;
	OnUpdateNameRequest = InArgs._OnUpdateName;
	bTrackingStatus = InArgs._TrackingStatus;
	TrackedDeviceData = InArgs._DeviceData;
	bCanSupportFocus = true;
	SetCanTick(false);

	FrameColorNormal = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);
	FrameColorSelected = FLinearColor(1.f, 0.5f, 0.2f, 1.f);
	BackgroundColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.f);

	bIsSelected = false;
	
	if (TrackingSetupObject && TrackedDeviceData)
	{
		ConstructWidget();

		if (TrackedDeviceData->Id == INDEX_NONE) bTrackingStatus = false;

		TSharedRef<FActiveTimerHandle> h = RegisterActiveTimer(
			1.f,
			FWidgetActiveTimerDelegate::CreateSP(this, &SDeviceSerialBinding::TimerUpdateWidget));

		ActivePlayingTimer = h;
	}
}

void SDeviceSerialBinding::ConstructWidget()
{
	const FSlateFontInfo FontLarge = FCoreStyle::GetDefaultFontStyle("Bold", 16);
	const FSlateFontInfo FontTitle = FCoreStyle::GetDefaultFontStyle("Bold", 12);
	const FLinearColor ColorSN = FLinearColor(0.1285f, 0.4458f, 0.7708f, 1.f);
	const FLinearColor ColorTitle = FLinearColor(0.4219f, 0.4219f, 0.4219f, 1.f);

	FString BrushName = (TrackedDeviceData->Type == ESteamVRTrackedDeviceType::Controller)
		? TEXT("SteamVRTrackingLib.Device.MotionController")
		: TEXT("SteamVRTrackingLib.Device.ViveTracker");
	BrushName.Append((bTrackingStatus) ? TEXT("On") : TEXT("Off"));

	ChildSlot
		[
		// Main Content
		SAssignNew(ExternalBox, SBorder)
		.BorderImage(FEditorStyle::GetBrush(TEXT("ProjectBrowser.Tab.Highlight")))
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.Padding(FMargin(2))
			.BorderImage(FEditorStyle::GetBrush(TEXT("ProjectBrowser.Tab.ActiveBackground")))
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SHorizontalBox)
				
				// tracking status
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				.Padding(12.f, 0.f, 0.f, 0.f)
				.AutoWidth()
				[
					SAssignNew(TrackingStatusImage, SImage)
					.Image(FSteamVRTrackingStyle::Get()->GetBrush(FName(*BrushName)))
				]

				// serial number
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				.Padding(12.f, 0.f, 12.f, 0.f)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(this, &SDeviceSerialBinding::GetSerialNumberText)
					.Font(FontLarge)
					.ColorAndOpacity(FSlateColor(ColorSN))
				]

				// device ID
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(0.f, 0.f, 12.f, 0.f)
				.AutoWidth()
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(0.f, 2.f, 0.f, 0.f)
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("[OpenVR ID]")))
						.Justification(ETextJustify::Left)
						.Font(FontTitle)
						.ColorAndOpacity(FSlateColor(ColorTitle))
					]

					+ SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(0.f, 8.f, 0.f, 8.f)
					.AutoHeight()
					[
						SNew(STextBlock)						
						.Text(this, &SDeviceSerialBinding::GetDeviceIdText)
						.Justification(ETextJustify::Left)
						.Font(FontLarge)
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
					]
				]

				// friendly name
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(0.f, 0.f, 12.f, 0.f)
				.FillWidth(1.f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(0.f, 2.f, 0.f, 0.f)
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("[Device Name]")))
						.Justification(ETextJustify::Left)
						.Font(FontTitle)
						.ColorAndOpacity(FSlateColor(ColorTitle))
					]

					+ SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.Padding(0.f, 8.f, 0.f, 8.f)
					.AutoHeight()
					[
						SAssignNew(FriendlyNameTextBox, SEditableTextBox)
						.Text(NameToText(TrackedDeviceData->FriendlyName))
						.Justification(ETextJustify::Left)
						.OnTextCommitted(this, &SDeviceSerialBinding::OnFriendlyNameCommitted)
					]
				]

				// delete button
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)				
				.Padding(0.f, 0.f, 12.f, 0.f)
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "NoBorder")
					.OnClicked(this, &SDeviceSerialBinding::OnRemoveButtonClick)
					.IsFocusable(false)
					[
						SNew(SImage)
						.Image(FSteamVRTrackingStyle::Get()->GetBrush("SteamVRTrackingLib.Button.DeleteSmall"))
					]

				]
			]
		]
	];
}

void SDeviceSerialBinding::OnFriendlyNameCommitted(const FText& NewText, ETextCommit::Type CommitType) const
{
	if (TrackedDeviceData && !TrackedDeviceData->SerialNumber.IsNone())
	{
		//TrackedDeviceData->FriendlyName = FName(*NewText.ToString());
		OnUpdateNameRequest.Execute(TrackingSetupObject, TrackedDeviceData->SerialNumber, FName(*NewText.ToString()));
	}
}

FText SDeviceSerialBinding::NameToText(const FName& NameData)
{
	return NameData.IsNone() ? FText() : FText::FromName(NameData);
}

void SDeviceSerialBinding::ReconstructObject(FSteamVRDeviceBindingSetup* NewDeviceData)
{
	TrackedDeviceData = NewDeviceData;
	//OnFriendlyNameCommitted
	FriendlyNameTextBox->SetText(NameToText(TrackedDeviceData->FriendlyName));

	if (TrackedDeviceData && TrackingStatusImage.IsValid())
	{
		FString BrushName = (TrackedDeviceData->Type == ESteamVRTrackedDeviceType::Controller)
			? TEXT("SteamVRTrackingLib.Device.MotionController")
			: TEXT("SteamVRTrackingLib.Device.ViveTracker");
		BrushName.Append(bTrackingStatus ? TEXT("On") : TEXT("Off"));

		TrackingStatusImage->SetImage(FSteamVRTrackingStyle::Get()->GetBrush(FName(*BrushName)));
	}
}

FText SDeviceSerialBinding::GetSerialNumberText() const
{
	if (TrackedDeviceData && !TrackedDeviceData->SerialNumber.IsNone())
	{
		return FText::FromName(TrackedDeviceData->SerialNumber);
	}
	else
	{
		return FText::FromString(TEXT("UNKNOWN"));
	}
}

FText SDeviceSerialBinding::GetDeviceIdText() const
{
	if (TrackedDeviceData && TrackedDeviceData->Id > 0)
	{
		FString szId = (TEXT("000") + FString::FromInt(TrackedDeviceData->Id)).Right(3);
		return FText::FromString(szId);
	}
	else
	{
		return FText::FromString(TEXT("---"));
	}
}

void SDeviceSerialBinding::UpdateSelectionStatus()
{
	if (bIsSelected)
	{
		ExternalBox->SetBorderImage(FEditorStyle::GetBrush(TEXT("ProjectBrowser.Tab.ActiveHighlight")));
	}
	else
	{
		ExternalBox->SetBorderImage(FEditorStyle::GetBrush(TEXT("ProjectBrowser.Tab.Highlight")));
	}
}

FReply SDeviceSerialBinding::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bIsSelected)
	{
		const FName sNumber = TrackedDeviceData ? TrackedDeviceData->SerialNumber : TEXT("");
		OnNodeWasSelected.Execute(TrackingSetupObject, sNumber);
		MarkSelected();
	}
	return FReply::Unhandled();
}

FReply SDeviceSerialBinding::OnRemoveButtonClick()
{
	const FName sNumber = TrackedDeviceData ? TrackedDeviceData->SerialNumber : TEXT("");
	OnRemoveRequest.Execute(TrackingSetupObject, sNumber);
	return FReply::Handled();
}

/*
FText SDeviceSerialBinding::GetAssetName() const
{
	return LOCTEXT("SelectAudioAsset", "[Please select audio asset]");
}
*/

EActiveTimerReturnType SDeviceSerialBinding::TimerUpdateWidget(double InCurrentTime, float InDeltaTime)
{
	if (TrackingSetupObject && TrackedDeviceData)
	{
		if (TrackedDeviceData->Id > 0)
		{
			TArray<int32> ExistingIds;
			USteamVRFunctionLibrary::GetValidTrackedDeviceIds(TrackedDeviceData->Type, ExistingIds);

			bool bIsTracked = ExistingIds.Contains(TrackedDeviceData->Id);
			if (bIsTracked)
			{
				FVector Loc;
				FRotator Rot;
				USteamVRFunctionLibrary::GetTrackedDevicePositionAndOrientation(TrackedDeviceData->Id, Loc, Rot);
				if (Loc.IsZero()) bIsTracked = false;
			}

			if (bIsTracked != bTrackingStatus)
			{
				bTrackingStatus = bIsTracked;

				FString BrushName = (TrackedDeviceData->Type == ESteamVRTrackedDeviceType::Controller)
					? TEXT("SteamVRTrackingLib.Device.MotionController")
					: TEXT("SteamVRTrackingLib.Device.ViveTracker");
				BrushName.Append(bTrackingStatus ? TEXT("On") : TEXT("Off"));

				TrackingStatusImage->SetImage(FSteamVRTrackingStyle::Get()->GetBrush(FName(*BrushName)));
			}
		}

		return EActiveTimerReturnType::Continue;
	}
	else
	{	
		if (ActivePlayingTimer.IsValid())
		{
			UnRegisterActiveTimer(ActivePlayingTimer.ToSharedRef());
		}
		return EActiveTimerReturnType::Stop;
	}
}

USteamVRTrackingSetup* SDeviceSerialBinding::GetTrackingSetupObject() const
{
	return TrackingSetupObject;
}

FName SDeviceSerialBinding::GetSerialNumber() const
{
	return TrackedDeviceData ? TrackedDeviceData->SerialNumber : TEXT("");
}

#undef LOCTEXT_NAMESPACE