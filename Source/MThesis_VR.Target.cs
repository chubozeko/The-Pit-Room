

using UnrealBuildTool;
using System.Collections.Generic;

public class MThesis_VRTarget : TargetRules
{
	public MThesis_VRTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "MThesis_VR" } );
	}
}
