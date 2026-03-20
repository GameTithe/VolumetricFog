#pragma  once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RendererInterface.h"
#include "ScreenPass.h"

// Ray Marching PS
class FFogRayMarchingPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFogRayMarchingPS);
	SHADER_USE_PARAMETER_STRUCT(FFogRayMarchingPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, CurlNoiseTexture)

		SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, SceneDepthSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, CurlNoiseSampler)

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DensityTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, VelocityTexture)
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
        SHADER_PARAMETER(float, Absorption)
        SHADER_PARAMETER(FVector3f, FogColor)
        SHADER_PARAMETER(int, NumSteps)
        SHADER_PARAMETER(float, MaxRayDistance)

        SHADER_PARAMETER(float, CurlNoiseScale)
        SHADER_PARAMETER(float, CurlNoiseSpeed)
        SHADER_PARAMETER(float, CurlDistortStrength)
        SHADER_PARAMETER(float, VelocityDistortStrength)
        SHADER_PARAMETER(float, BaseNoiseScale)
        SHADER_PARAMETER(float, Time)

		SHADER_PARAMETER(float, CurlTexScale)
		SHADER_PARAMETER(float, CurlTexSpeed)
		SHADER_PARAMETER(float, CurlTexStrength)
 
        SHADER_PARAMETER(FVector3f, SimulationCenter)
        SHADER_PARAMETER(FVector3f, SimulationExtents)
		
		//Debug Parameter 
		SHADER_PARAMETER(int32, FogDebugMode)
	
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

	// 외부에서 Render Thread 호출
	void SetDensityRHI(FTextureRHIRef InTex)
	{
		if (DensityRHI != InTex)
		{
			DensityRHI = InTex;
			DensityPooledRT = InTex ? CreateRenderTarget(InTex, TEXT("FogDensity")) : nullptr;
		}
	}
	void SetVelocityRHI(FTextureRHIRef InTex)
	{
		if (VelocityRHI != InTex)
		{
			VelocityRHI = InTex;
			VelocityPooledRT = InTex ? CreateRenderTarget(InTex, TEXT("FogVelocity")) : nullptr;
		}
	}
	void SetCurlNoiseRHI(FTextureRHIRef InTex)
	{
		if (CurlNoiseRHI != InTex)
		{
			CurlNoiseRHI = InTex;
			CurlNoisePooledRT = InTex ? CreateRenderTarget(InTex, TEXT("CurlNoise")) : nullptr;
		}
	}
	
	bool bEnable = false;
	
	// Fog Parameters
	float FogBaseHeight = 0.f;
	float FogMaxHeight = 500.f;
	float FogDensityMultiplier = 2.f;
	float Absorption          = 0.5f;
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
	 
	// Curl Noise Parameters 
	float CurlNoiseScale         = 0.003f;
	float CurlNoiseSpeed         = 0.1f;
	float CurlDistortStrength    = 1.0f;
	float VelocityDistortStrength = 0.5f;
	float BaseNoiseScale         = 0.01f;
	float AccumulatedTime        = 0.f;
	
	float CurlTexScale = 0.006f;
	float CurlTexSpeed = 0.12f;
	float CurlTexStrength = 40.0f;

	FVector3f SimulationCenter	= FVector3f::ZeroVector;
	FVector3f SimulationExtents = FVector3f(3.0f, 3.0f, 3.0f);

	// Debug Parameter 
	int32 FogDebugMode = 1;
private:
	void RenderFog_RenderThread(FPostOpaqueRenderParameters& InParameters);

	
	FTextureRHIRef DensityRHI;
	FTextureRHIRef VelocityRHI;

	TRefCountPtr<IPooledRenderTarget> DensityPooledRT;
	TRefCountPtr<IPooledRenderTarget> VelocityPooledRT;

	FDelegateHandle PostOpaqueDelegateHandle;
	 
	// Curl Noise Using Texture
	FTextureRHIRef CurlNoiseRHI;
	TRefCountPtr<IPooledRenderTarget> CurlNoisePooledRT;

};
