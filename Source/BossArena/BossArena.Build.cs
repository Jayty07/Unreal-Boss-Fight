using UnrealBuildTool;

public class BossArena : ModuleRules
{
	public BossArena(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"AIModule",
			"ProceduralMeshComponent",
			"Niagara",
			"NetCore"
		});
	}
}
