// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Contract : ModuleRules
{
	public Contract(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Niagara", "UMG", "AIModule", "NavigationSystem" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        PublicIncludePaths.Add("Contract/Public");
        PublicIncludePaths.Add("Contract/Public/NPC");
        PublicIncludePaths.Add("Contract/Public/NPC/Enemy");
        PublicIncludePaths.Add("Contract/Public/NPC/Enemy/Task");
        PublicIncludePaths.Add("Contract/Public/Player");
        PublicIncludePaths.Add("Contract/Public/Player/Item");
        PublicIncludePaths.Add("Contract/Public/Quest");
    }
}
