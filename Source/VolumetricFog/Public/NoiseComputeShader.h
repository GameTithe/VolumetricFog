#include "CoreMinimal.h"
#include "GlobalShader.h"
#include  "ShaderParameterStruct.h"
#include "RenderGraphUtils.h"

// 캄푸트 쉐이더 클래스 선언
class FGenerateNoiseCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FGenerateNoiseCS);
	SHADER_USE_PARAMETER_STRUCT(FGenerateNoiseCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(float, Time)
		SHADER_PARAMETER_UAV(RWTexture2D<float4>, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	} 
}; 