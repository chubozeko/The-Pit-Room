

using UnrealBuildTool;
using System.Collections.Generic;

public class MThesis_VREditorTarget : TargetRules
{
	public MThesis_VREditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "MThesis_VR" } );
	}
}
