using UnrealBuildTool;

public class HowToSpearfish : ModuleRules
{
	public HowToSpearfish(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// All includes inside the module are written relative to the module root,
		// e.g. #include "Diving/SpearfishCharacter.h".
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"NetCore",
			"DeveloperSettings",
			"PhysicsCore",
			"ProceduralMeshComponent",
			"Json",
			"JsonUtilities",
			"Slate",
			"SlateCore",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"OnlineSubsystem",
			"OnlineSubsystemUtils"
		});
	}
}
