#pragma  once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RendererInterface.h"
#include "ScreenPass.h"
#include "HeightCurveLUTResource.h"

// Ray Marching PS
class FFogRayMarchingPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFogRayMarchingPS);
	SHADER_USE_PARAMETER_STRUCT(FFogRayMarchingPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, HeightCurveTexture)

		SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, SceneDepthSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, HeightCurveSampler)
		

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DensityTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, BilinearSampler)

		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, SceneColorViewport)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, SceneDepthViewport)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, OutputViewport)

		SHADER_PARAMETER(FMatrix44f, InvViewProjectionMatrix)
		SHADER_PARAMETER(FVector3f, CameraPosition)

		SHADER_PARAMETER(float, FogBaseHeight)
		SHADER_PARAMETER(float, FogMaxHeight)

		//Height Attenuation Mode
		SHADER_PARAMETER(int32, HeightAttenuationMode)
		// Legacy
		SHADER_PARAMETER(float, HeightFalloff)

		//Adaptive
		SHADER_PARAMETER(float, HeightFadeStartRatio)
		SHADER_PARAMETER(float, HeightFadeStrength)

		SHADER_PARAMETER(float, FogDensityMultiplier)
        //SHADER_PARAMETER(float, Absorption)
        SHADER_PARAMETER(float, AbsorptionScale)
        SHADER_PARAMETER(float, ScatteringScale)
	
        SHADER_PARAMETER(FVector3f, FogColor)
        SHADER_PARAMETER(int, NumSteps)
        SHADER_PARAMETER(float, MaxRayDistance)
 
        SHADER_PARAMETER(FVector3f, SimulationCenter)
        SHADER_PARAMETER(FVector3f, SimulationExtents)
		
		//Debug Parameter 
		SHADER_PARAMETER(int32, FogDebugMode)
		
		//Self Shadow  
		SHADER_PARAMETER(FVector3f, SelfShadowLightDirection)
		SHADER_PARAMETER(FVector3f, SelfShadowLightColor)
		SHADER_PARAMETER(float, SelfShadowLightIntensity)
		SHADER_PARAMETER(float, SelfShadowDensityScale)
		SHADER_PARAMETER(int32, SelfShadowStepCount)
		SHADER_PARAMETER(float, SelfShadowMaxDistance)
	
		//Phase Function
		SHADER_PARAMETER(float, GOfHG)
			
        RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

 
// 테스트용 Full Screen PS
class FFogFullscreenPS: public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFogFullscreenPS);
	SHADER_USE_PARAMETER_STRUCT(FFogFullscreenPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

struct FFluidFogRenderState
{
	bool bEnable = false;
	
	// Fog Parameters
	float FogBaseHeight = 0.f;
	float FogMaxHeight = 500.f;
	float FogDensityMultiplier = 2.f;
	//float Absorption          = 0.5f;
	float AbsorptionScale = 0.1f;
	float ScatteringScale = 0.5f;
	
	FVector3f FogColor        = FVector3f(0.8f, 0.85f, 0.9f);
	int32 NumSteps            = 64;
	float MaxRayDistance       = 5000.f;
	
	// Height Attenuation Mode 
	int32 HeightAttenuationMode = 1;
	
	// Legacy
	float HeightFalloff       = 200.f;
	
	// Adaptive Height Attenuation
	float HeightFadeStartRatio = 0.5f;
	float HeightFadeStrength = 1.0f; 
	
	FVector3f SimulationCenter	= FVector3f::ZeroVector;
	FVector3f SimulationExtents = FVector3f(3.0f, 3.0f, 3.0f);

	// Debug Parameter 
	int32 FogDebugMode = 1; 
	
	// Self Shadow 
	FVector3f SelfShadowLightDirection = FVector3f(0.4f, 0.2f, 1.0f);
	FVector3f SelfShadowLightColor = FVector3f(1.0f, 1.0f, 1.0f);
	 
	float SelfShadowLightIntensity = 1.0f;
  	float SelfShadowDensityScale;
	int32 SelfShadowStepCount = 6;
	float SelfShadowMaxDistance = 2000.0f;
	
	// Phase Function
	float GOfHG = 0.0f;
	
	FTextureRHIRef DensityTexture;
};
// SceneViewExtension
class FFogSceneViewExtension : public FSceneViewExtensionBase
{
public:
	FFogSceneViewExtension(const FAutoRegister& AutoRegister);
	virtual ~FFogSceneViewExtension();

	// 필수 오버라이드
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
	
	/** Render Thread Helper Function */
	
	/** density texture가 바뀌었으면, RDG 등록용 및 PooledRT 갱신 */
	void ApplyRenderState_RenderThread(const FFluidFogRenderState& InState);
	
	void UpdateHeightCurveLUT_RenderThread(FRHICommandListImmediate& RHICmdList, TConstArrayView<float> Samples);
	void ReleaseHeightCurveLUT_RenderThread();

private:
	void RenderFog_RenderThread(FPostOpaqueRenderParameters& InParameters);

	FFluidFogRenderState RenderState;
	TRefCountPtr<IPooledRenderTarget> DensityPooledRT; 

	FDelegateHandle PostOpaqueDelegateHandle; 
	
	// Height Atteunation Resource 
	TUniquePtr<FHeightCurveLUTResource> HeightCurveResource;
	TRefCountPtr<IPooledRenderTarget> HeightCurvePooledRT;
};
