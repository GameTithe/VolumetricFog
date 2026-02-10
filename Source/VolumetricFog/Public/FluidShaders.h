#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphUtils.h"

class FFluidAdvectCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFluidAdvectCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidAdvectCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FFluidAdvectCS, )
		SHADER_PARAMETER_TEXTURE(Texture2D<float2>, VelocityInput)
		SHDAER_PARAMETER_TEXTURE(Texture2D<float>, DensityInput)
		SHDAER_PARAMETER_UAV(RWTexture2D<float>, DensityOutput)
		SHDAER_PARAMETER_SAMPLER(SamplerState, BilinearSampler)
		SHADER_PARAMETER(float, DeltaTime)
		SHADER_PARAMETER(FVector2f, InvResolution)
		SHADER_PARAMETER(FIntPoint, Resolution) 
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

class FFluidDiffuseCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFluidDiffuseCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidDiffuseCS, FGlobalShader);

	BEGIN_SHADER_PARAMTER_STRUCT()
		SHADER_PARAMETER_TEXTURE(Texture2D<float2>, InputTexture)
		SHDAER_PARAMETER_TEXTURE(Texture2D<float>, PrevTexture)
		SHDAER_PARAMETER_UAV(RWTexture2D<float>, OutputTexture)
		SHADER_PARAMTER(float, Alpha)
		SHADER_PARAMTER(float, InvBeta)
		SHADER_PARAMTER(FIntPoint,  Resolution)
	END_SHADER_PARAMTER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

class FFluidForceCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FFluidForceCS, FGlobalShader);
	SHADER_USE_PARAMETER_STRUCT(FFluidForceCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT()
		SHADER_PARAMETER_TEXTURE(Texture2D<float>, DensityInput)
		SHADER_PARAMETER_TEXTURE(Texture2D<float2>, VelocityInput)
		SHADER_PARAMETER_UAV(RWTexture2D<float>, DensityOutput)
		SHADER_PARAMETER_UAV(RWTexture2D<float2>, VelocityOutput)
		SHADER_PARAMETER(FVector2f, ForcePosition)
		SHADER_PARAMETER(FVector2f, ForceDirection)
		SHADER_PARAMETER(float, ForceRadius)
		SHADER_PARAMETER(float, ForceStrength)
		SHADER_PARAMETER(float, DensityAmount)
		SHADER_PARAMETER(float, DeltaTime)
		SHADER_PARAMETER(float, Dissipation)
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
		SHADER_PARAMETER_TEXTURE(Texture2D<float2>, VelocityInput)
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
		SHADER_PARAMETER_TEXTURE(Texture2D<float2>, VelocityInput)
		SHADER_PARAMETER_TEXTURE(Texture2D<float>, PressureInput)
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
		SHADER_PARAMETER_TEXTURE(Texture2D<float2>, VelocityInput)
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
		SHADER_PARAMETER_TEXTURE(Texture2D<float2>, VelocityInput)
		SHADER_PARAMETER_TEXTURE(Texture2D<float>, VorticityInput)
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