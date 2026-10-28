using UnrealBuildTool;

public class Dnd5eJson : ModuleRules
{
	public Dnd5eJson(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"Dnd5e",
			"Json"
		});
	}
}
