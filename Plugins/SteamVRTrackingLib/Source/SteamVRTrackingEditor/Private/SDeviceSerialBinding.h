// (c) YuriNK (ykasczc@gmail.com), 2020. You're free to use it whatever way you want.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "SteamVRTrackingSetup.h"

class SEditableTextBox;
class SImage;
class SBorder;

class SDeviceSerialBinding : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_TwoParams(FOnSelected, USteamVRTrackingSetup*, const FName&)
	DECLARE_DELEGATE_TwoParams(FOnRemove, USteamVRTrackingSetup*, const FName&)
	DECLARE_DELEGATE_ThreeParams(FOnUpdateName, USteamVRTrackingSetup*, const FName&, const FName&)

public:
	SLATE_BEGIN_ARGS(SDeviceSerialBinding)
		: _ModifiedObject(static_cast<USteamVRTrackingSetup*>(NULL))
		, _DeviceData(static_cast<FSteamVRDeviceBindingSetup*>(NULL))
		, _TrackingStatus(false)
		{};

		SLATE_ARGUMENT(USteamVRTrackingSetup*, ModifiedObject)
		SLATE_ARGUMENT(FSteamVRDeviceBindingSetup*, DeviceData)
		SLATE_ARGUMENT(bool, TrackingStatus)
		SLATE_EVENT(FOnSelected, OnSelected)
		SLATE_EVENT(FOnRemove, OnRemove)
		SLATE_EVENT(FOnUpdateName, OnUpdateName)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void ConstructWidget();

	// General external functions
	void MarkSelected() { bIsSelected = true; UpdateSelectionStatus(); };
	void MarkUnselected() { bIsSelected = false; UpdateSelectionStatus(); };
	void ReconstructObject(FSteamVRDeviceBindingSetup* NewDeviceData);
	void UpdateSelectionStatus();
	bool IsSelected() const { return bIsSelected; };
	
	// Begin SWidget interface
	virtual bool SupportsKeyboardFocus() const override { return true; };
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	EActiveTimerReturnType TimerUpdateWidget(double InCurrentTime, float InDeltaTime);
	// End SWidget interface

	// Default Colours
	FLinearColor FrameColorNormal;
	FLinearColor FrameColorSelected;
	FLinearColor BackgroundColor;

	// Properties
	FText GetSerialNumberText() const;
	FText GetDeviceIdText() const;

	// Other control events
	FReply OnRemoveButtonClick();
	USteamVRTrackingSetup* GetTrackingSetupObject() const;
	FName GetSerialNumber() const;
	void OnFriendlyNameCommitted(const FText& NewText, ETextCommit::Type CommitType) const;

protected:
	USteamVRTrackingSetup* TrackingSetupObject;
	FSteamVRDeviceBindingSetup* TrackedDeviceData;

	// delegates
	FOnSelected OnNodeWasSelected;
	FOnRemove OnRemoveRequest;
	FOnUpdateName OnUpdateNameRequest;

	// properties
	bool bIsSelected;
	bool bTrackingStatus;

	FText NameToText(const FName& NameData);

	TSharedPtr<class FActiveTimerHandle> ActivePlayingTimer;
	// child widgets
	TSharedPtr<SBorder> ExternalBox;
	TSharedPtr<SImage> TrackingStatusImage;
	TSharedPtr<SEditableTextBox> FriendlyNameTextBox;
};
