#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphUtils.h"

#ifndef MAX_FLUID_INTERACTION_FORCE_SOURCE
#define MAX_FLUID_INTERACTION_FORCE_SOURCE 8
#endif


class FFluidAdvectVelocityCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFluidAdvectVelocityCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidAdvectVelocityCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_SRV(Texture2D<float2>, VelocityInput)
			SHADER_PARAMETER_UAV(RWTexture2D<float2>, VelocityOutput)
			SHADER_PARAMETER_SAMPLER(SamplerState, BilinearSampler)
			SHADER_PARAMETER(float, DeltaTime)
			SHADER_PARAMETER(FVector2f, InvResolution)
			SHADER_PARAMETER(FIntPoint, Resolution)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

class FFluidAdvectCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFluidAdvectCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidAdvectCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Texture2D<float2>, VelocityInput)
		SHADER_PARAMETER_SRV(Texture2D<float>, DensityInput)
		SHADER_PARAMETER_UAV(RWTexture2D<float>, DensityOutput)
		SHADER_PARAMETER_SAMPLER(SamplerState, BilinearSampler)
		SHADER_PARAMETER(float, DeltaTime)
		SHADER_PARAMETER(FVector2f, InvResolution)
		SHADER_PARAMETER(FIntPoint, Resolution)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

class FFluidDensityMaintenanceCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFluidDensityMaintenanceCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidDensityMaintenanceCS, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Texture2D<float>, DensityInput)
		SHADER_PARAMETER_UAV(RWTexture2D<float>, DensityOutput)
		
		SHADER_PARAMETER_TEXTURE(Texture2D,NoiseTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, NoiseSampler)
		
		SHADER_PARAMETER(float, DeltaTime)
		SHADER_PARAMETER(float, BaseDensityTarget)
		SHADER_PARAMETER(float, BaseDensityRecoverySpeed)
		SHADER_PARAMETER(float, BaseDensityDeadbandRatio)
		SHADER_PARAMETER(float, BaseDensityNoiseRepeat)
	
		SHADER_PARAMETER(FVector2f, InvResolution)
		SHADER_PARAMETER(FIntPoint, Resolution)
	
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
}; 

class FFluidDiffuseCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFluidDiffuseCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidDiffuseCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Texture2D<float>, InputTexture)
		SHADER_PARAMETER_SRV(Texture2D<float>, PrevTexture)
		SHADER_PARAMETER_UAV(RWTexture2D<float>, OutputTexture)
		SHADER_PARAMETER(float, Alpha)
		SHADER_PARAMETER(float, InvBeta)
		SHADER_PARAMETER(FIntPoint, Resolution)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};


class FFluidDiffuseVelocityCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFluidDiffuseVelocityCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidDiffuseVelocityCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Texture2D<float2>, InputTexture)
		SHADER_PARAMETER_SRV(Texture2D<float2>, PrevTexture)
		SHADER_PARAMETER_UAV(RWTexture2D<float2>, OutputTexture)
		SHADER_PARAMETER(float, Alpha)
		SHADER_PARAMETER(float, InvBeta)
		SHADER_PARAMETER(FIntPoint, Resolution)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

class FFluidForceCS : public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FFluidForceCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidForceCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Texture2D<float>, DensityInput)
		SHADER_PARAMETER_SRV(Texture2D<float2>, VelocityInput)
		SHADER_PARAMETER_UAV(RWTexture2D<float>, DensityOutput)
		SHADER_PARAMETER_UAV(RWTexture2D<float2>, VelocityOutput)

		SHADER_PARAMETER(FVector2f, ForcePosition)
		SHADER_PARAMETER(FVector2f, ForceDirection)

		SHADER_PARAMETER(float, ForceRadius)
		SHADER_PARAMETER(float, ForceStrength)
		SHADER_PARAMETER(float, DensityAmount)
		SHADER_PARAMETER(float, DeltaTime)
		SHADER_PARAMETER(float, Dissipation)

		//Curl Noise
		SHADER_PARAMETER_TEXTURE(Texture2D, CurlNoiseTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, CurlNoiseSampler)
		SHADER_PARAMETER(float, Time)
		SHADER_PARAMETER(float, CurlSimulationTiling)
		SHADER_PARAMETER(float, CurlSimulationSpeed)
		SHADER_PARAMETER(float, CurlVelocityStrength)
		SHADER_PARAMETER(float, CurlDensityMaskScale)
		
		// Interaction Params
		SHADER_PARAMETER(uint32, InteractionForceSourceCount) 
		SHADER_PARAMETER_ARRAY(FVector4f, InteractionForcePositionRadius, [MAX_FLUID_INTERACTION_FORCE_SOURCE])
		SHADER_PARAMETER_ARRAY(FVector4f, InteractionForceVectorDensity, [MAX_FLUID_INTERACTION_FORCE_SOURCE])
	
		SHADER_PARAMETER(FVector2f, InvResolution)
		SHADER_PARAMETER(FIntPoint, Resolution)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};


class FFluidDivergenceCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFluidDivergenceCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidDivergenceCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Texture2D<float2>, VelocityInput)
		SHADER_PARAMETER_UAV(RWTexture2D<float>, DivergenceOutput)
		SHADER_PARAMETER(FIntPoint, Resolution)
		SHADER_PARAMETER(float, HalfInvDx)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

class FFluidGradientSubtractCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFluidGradientSubtractCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidGradientSubtractCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Texture2D<float2>, VelocityInput)
		SHADER_PARAMETER_SRV(Texture2D<float>, PressureInput)
		SHADER_PARAMETER_UAV(RWTexture2D<float2>, VelocityOutput)
		SHADER_PARAMETER(FIntPoint, Resolution)
		SHADER_PARAMETER(float, HalfInvDx)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

class FFluidVorticityCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFluidVorticityCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidVorticityCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Texture2D<float2>, VelocityInput)
		SHADER_PARAMETER_UAV(RWTexture2D<float>, VorticityOutput)
		SHADER_PARAMETER(FIntPoint, Resolution)
		SHADER_PARAMETER(float, HalfInvDx)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

class FFluidVorticityForceCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFluidVorticityForceCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidVorticityForceCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Texture2D<float2>, VelocityInput)
		SHADER_PARAMETER_SRV(Texture2D<float>, VorticityInput)
		SHADER_PARAMETER_UAV(RWTexture2D<float2>, VelocityOutput)
		SHADER_PARAMETER(FIntPoint, Resolution)
		SHADER_PARAMETER(float, VorticityStrength)
		SHADER_PARAMETER(float, DeltaTime)
		SHADER_PARAMETER(float, HalfInvDx)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};