// Copyright Epic Games, Inc. All Rights Reserved.

#include "VolumetricFlowFog.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FVolumetricFlowFogModule"

void FVolumetricFlowFogModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module 
	FString PluginShaderDir = FPaths::Combine(
		FPaths::ProjectPluginsDir(),
		TEXT("VolumetricFog/Shaders/Private") 
	); 
	
	AddShaderSourceDirectoryMapping(TEXT("/VolumetricFog") /* Virtaul Shader Path */, PluginShaderDir/* Real Shader Path */);
}

void FVolumetricFlowFogModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE	
	
IMPLEMENT_MODULE(FVolumetricFlowFogModule, VolumetricFog)