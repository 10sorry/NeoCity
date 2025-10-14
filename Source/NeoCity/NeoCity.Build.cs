using UnrealBuildTool;

public class NeoCity : ModuleRules
{
	public NeoCity(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG"
		});
		
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Http",
			"Json",
			"JsonUtilities"
		});
	}
}