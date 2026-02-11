#pragma  once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

// 테스트용 Full Screen PS
class FFogFullScreenPS: public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFogFullScreenPS);
	SHADER_USE_PARAMETER_STRUCT(FFogFullScreenPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

// SceneViewExtension
class FFogSceneViewExtension : public FSceneViewExtensionBase
{
public:
	FFogSceneViewExtension(const FAutoRegister& AutoRegister);

	// 필수 오버라이드
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	
	// Post Opaque

	// 활성화 제어
};