using UnrealBuildTool;
using System.Collections.Generic;

public class HowToSpearfishEditorTarget : TargetRules
{
	public HowToSpearfishEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		bOverrideBuildEnvironment = true;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("HowToSpearfish");
	}
}
