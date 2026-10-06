using UnrealBuildTool;

public class BossArenaEditorTarget : TargetRules
{
	public BossArenaEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("BossArena");
	}
}
