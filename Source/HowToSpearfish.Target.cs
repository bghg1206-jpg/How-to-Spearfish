using UnrealBuildTool;
using System.Collections.Generic;

public class HowToSpearfishTarget : TargetRules
{
	public HowToSpearfishTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		bOverrideBuildEnvironment = true;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("HowToSpearfish");
	}
}
