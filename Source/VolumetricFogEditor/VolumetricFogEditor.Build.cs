using UnrealBuildTool;

public class VolumetricFogEditor : ModuleRules
{
    public VolumetricFogEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
              "Core",
              "CoreUObject",
              "Engine",
              "UnrealEd",
              "VolumetricFog"
        });
    }
}