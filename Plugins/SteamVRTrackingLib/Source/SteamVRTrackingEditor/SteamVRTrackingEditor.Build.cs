// (c) YuriNK (ykasczc@gmail.com), 2020. You're free to use it whatever way you want.

using UnrealBuildTool;
using System.IO;

namespace UnrealBuildTool.Rules
{
    public class SteamVRTrackingEditor : ModuleRules
    {
        public SteamVRTrackingEditor(ReadOnlyTargetRules Target) : base(Target)
        {
            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
            PrivatePCHHeaderFile = "Public/SteamVRTrackingEditor.h";

            PrivateDependencyModuleNames.AddRange(
                new string[] {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "PropertyEditor",
                    "UnrealEd",
                    "EditorStyle",
                    //"ContentBrowser",
                    "SlateCore",
                    "Slate",
                    //"AssetRegistry",
                    "DesktopPlatform",
                    "Projects",
                    "SteamVR",
					"SteamVRTrackingLib"
                }
            );
        }
    }
}