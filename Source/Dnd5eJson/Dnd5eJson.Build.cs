using UnrealBuildTool;

public class Dnd5eJson : ModuleRules
{
	public Dnd5eJson(ReadOnlyTargetRules Target) : base(Target)
	{

		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Dnd5e",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Json",
			"GameplayTags",
		});
	}
}
