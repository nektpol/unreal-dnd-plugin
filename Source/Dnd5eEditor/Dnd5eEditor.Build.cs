using UnrealBuildTool;

public class Dnd5eEditor : ModuleRules
{
	public Dnd5eEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"Dnd5e",
			"Dnd5eJson",
			"UnrealEd"
		});
	}
}
