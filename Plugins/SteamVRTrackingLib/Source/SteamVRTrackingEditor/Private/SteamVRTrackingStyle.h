// Copyright YuriNK (Garden Horse Studio). All Rights Reserved, 2018
// ykasczc@gmail.com

#pragma once

#include "Styling/SlateStyle.h"

// how everything looks, fancy stuff
class FSteamVRTrackingStyle
{
public:
	static void Initialize();

	static void Shutdown();

	static TSharedPtr<ISlateStyle> Get() { return StyleSet; }

	/** Gets the style name. */
	static FName GetStyleSetName() { return TEXT("SteamVRTrackingStyle"); }

	/** Gets the small property name variant */
	static FName GetSmallProperty(const FName& PropertyName)
	{
		return FName(*(PropertyName.ToString() + TEXT(".Small")));
	}

	/** Get the RelativePath to the DlgSystem Content Dir */
	static FString GetPluginContentPath(const FString& RelativePath)
	{
		return PluginContentRoot / RelativePath;
	}

	/** Get the RelativePath to the Engine Content Dir */
	static FString GetEngineContentPath(const FString& RelativePath)
	{
		return EngineContentRoot / RelativePath;
	}

	/** Get the RelativePath to the Engine Content Dir */
	static FString GetPluginResourcesPath(const FString& RelativePath)
	{
		return PluginResourcesRoot / RelativePath;
	}

private:
	/** Singleton instance. */
	static TSharedPtr<FSlateStyleSet> StyleSet;

	/** Engine content root. */
	static FString EngineContentRoot;

	/** DlgSystem Content Root */
	static FString PluginContentRoot;

	/** DlgSystem Resources Root */
	static FString PluginResourcesRoot;
};
