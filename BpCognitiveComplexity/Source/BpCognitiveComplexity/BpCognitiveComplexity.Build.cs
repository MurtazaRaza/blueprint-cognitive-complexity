using UnrealBuildTool;

public class BpCognitiveComplexity : ModuleRules
{
	public BpCognitiveComplexity(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"BlueprintGraph",
			"GraphEditor",
			"InputCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"ApplicationCore",
			"UnrealEd",
			"Kismet",
			"KismetCompiler",
			"EditorStyle",
			"DeveloperSettings",
			"KismetWidgets",
			"ToolMenus"
		});
	}
}
