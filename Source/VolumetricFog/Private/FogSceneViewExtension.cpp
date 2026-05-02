#include "FogSceneViewExtension.h"
#include "PixelShaderUtils.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "FogSceneViewExtension.h"
#include "SceneRendering.h" 
#include "SystemTextures.h"

IMPLEMENT_GLOBAL_SHADER(FFogFullscreenPS, "/VolumetricFog/Rendering/FogFullscreen.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FFogRayMarchingPS, "/VolumetricFog/Rendering/FogRayMarch.usf", "MainPS", SF_Pixel);

FFogSceneViewExtension::FFogSceneViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister)
{	
	// PostOpaque 델리게이트 등록
	IRendererModule& RendererModule = FModuleManager::GetModuleChecked<IRendererModule>("Renderer");
	
	PostOpaqueDelegateHandle = RendererModule.RegisterPostOpaqueRenderDelegate(FPostOpaqueRenderDelegate::CreateRaw(this, &FFogSceneViewExtension::RenderFog_RenderThread));
}

FFogSceneViewExtension::~FFogSceneViewExtension()
{
	if (PostOpaqueDelegateHandle.IsValid())
	{
		IRendererModule& RendererModule = FModuleManager::GetModuleChecked<IRendererModule>("Renderer");
		RendererModule.RemovePostOpaqueRenderDelegate(PostOpaqueDelegateHandle);
		PostOpaqueDelegateHandle.Reset();
	}
}

void FFogSceneViewExtension::RenderFog_RenderThread(FPostOpaqueRenderParameters& InParameters)
{
	const FFluidFogRenderState& State = RenderState;
	
	if (!State.bEnable || !State.DensityTexture || !DensityPooledRT || !VolumeNoisePooledRT)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RenderFog_RenderThread]: Need Texture!!"));
		return;
	}
	
	FRDGTextureRef SceneColor = InParameters.ColorTexture;
	FRDGTextureRef SceneDepth = InParameters.DepthTexture;

	// 현재까지 그려진(PostOpaque) SceneColor, SceneDepth 를 가져온다.
	const FScreenPassTexture SceneColorInput(SceneColor, InParameters.ViewportRect);
	const FScreenPassTexture SceneDepthInput(SceneDepth, InParameters.ViewportRect);

	FRDGBuilder& GraphBuilder = *InParameters.GraphBuilder;
	const FViewInfo& View = *InParameters.View;

	// Read Write를 동시에 할 수 없으니, SceneColor 복사
	FScreenPassRenderTarget FogOutput =
		FScreenPassRenderTarget::CreateFromInput(
			GraphBuilder,
			SceneColorInput,
			ERenderTargetLoadAction::ENoAction,
			TEXT("FogOutput")
		); 
	 
	// 캐싱된 PooledRT를 RDG에 등록
	// DensityTexture를 RGD pass에서 샘플하려면 RDG에 external texture로 등록을 해야 된다. 
	FRDGTextureRef DensityRDG = GraphBuilder.RegisterExternalTexture(DensityPooledRT);
	
	const FRDGSystemTextures& SystemTextures = FRDGSystemTextures::Get(GraphBuilder);
	
	/** HeightCurve가 전송되기 전이면 Fallback texture 사용 */
	FRDGTextureRef HeightCurveRDG;
	if (HeightCurvePooledRT)
	{
		HeightCurveRDG = GraphBuilder.RegisterExternalTexture(HeightCurvePooledRT);
	}
	else
	{
		HeightCurveRDG = SystemTextures.White;
	} 
	
	/** Volume Noise Texture For Fog Modeling*/
	FRDGTextureRef VolumeNoiseRDG;
	VolumeNoiseRDG = GraphBuilder.RegisterExternalTexture(VolumeNoisePooledRT);
	//if (VolumeNoisePooledRT)
	//{
	//	VolumeNoiseRDG = GraphBuilder.RegisterExternalTexture(VolumeNoisePooledRT);
	//}
	//else
	//{
	//	//TODO
	//}
	
	// Shader Params 
	auto* Params = GraphBuilder.AllocParameters<FFogRayMarchingPS::FParameters>();
	Params->SceneColorTexture = SceneColor;
	Params->SceneDepthTexture = SceneDepth;
	Params->HeightCurveTexture = HeightCurveRDG;

	Params->SceneColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	Params->SceneDepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	Params->HeightCurveSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(); 
	
	Params->SceneColorViewport = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(SceneColorInput));
	Params->SceneDepthViewport = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(SceneDepthInput));
	Params->OutputViewport = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(FogOutput));

	Params->DensityTexture  = DensityRDG;
	Params->BilinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp>::GetRHI();

	Params->InvViewProjectionMatrix = FMatrix44f(View.ViewMatrices.GetInvViewProjectionMatrix());
	Params->CameraPosition = FVector3f(View.ViewMatrices.GetViewOrigin());

	Params->HeightAttenuationMode = State.HeightAttenuationMode;
	Params->HeightFadeStartRatio = State.HeightFadeStartRatio;
	Params->HeightFadeStrength = State.HeightFadeStrength;

	Params->FogBaseHeight        = State.FogBaseHeight;
	Params->FogMaxHeight         = State.FogMaxHeight;
	Params->HeightFalloff        = State.HeightFalloff;

	Params->FogDensityMultiplier = State.FogDensityMultiplier; 
	Params->AbsorptionScale      = State.AbsorptionScale;
	Params->ScatteringScale      = State.ScatteringScale;
	Params->FogColor             = State.FogColor;
	Params->NumSteps             = State.NumSteps;
	Params->MaxRayDistance       = State.MaxRayDistance;
 
	Params->SimulationCenter	= State.SimulationCenter;
	Params->SimulationExtents	= State.SimulationExtents;

	
	Params->FogDebugMode = State.FogDebugMode;
	
	// Self Shadow   
	Params->SelfShadowLightDirection = State.SelfShadowLightDirection;
	Params->SelfShadowLightColor = State.SelfShadowLightColor;
	Params->SelfShadowLightIntensity = State.SelfShadowLightIntensity;
	Params->SelfShadowDensityScale = State.SelfShadowDensityScale;
	Params->SelfShadowStepCount = State.SelfShadowStepCount;
	Params->SelfShadowMaxDistance = State.SelfShadowMaxDistance;
	
	// Phase Function
	Params->GOfHG = State.GOfHG;
	
	// Modeling
	Params->VolumeNoiseTexture = VolumeNoiseRDG;
	Params->VolumeNoiseSampler = TStaticSamplerState<SF_Trilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
	
	// Read: Scene Color
	// Write: FogOutput
	Params->RenderTargets[0] = FogOutput.GetRenderTargetBinding();
	
	TShaderMapRef<FFogRayMarchingPS> PS(GetGlobalShaderMap(View.GetFeatureLevel()));
	
	FPixelShaderUtils::AddFullscreenPass(
			 GraphBuilder,
			 GetGlobalShaderMap(View.GetFeatureLevel()),
			 RDG_EVENT_NAME("FogRayMarch"),
			 PS, Params,
		FogOutput.ViewRect);

	FRHICopyTextureInfo CopyInfo;
	
	CopyInfo.SourcePosition = FIntVector(InParameters.ViewportRect.Min.X,
		InParameters.ViewportRect.Min.Y, 0);
	
	CopyInfo.DestPosition = CopyInfo.SourcePosition;
	
	CopyInfo.Size = FIntVector(InParameters.ViewportRect.Width(),
		InParameters.ViewportRect.Height(), 1);
	
	AddCopyTexturePass(GraphBuilder, FogOutput.Texture, SceneColor, CopyInfo);

}

void FFogSceneViewExtension::ApplyRenderState_RenderThread(const FFluidFogRenderState& InState)
{
	check(IsInRenderingThread());
	
	const bool bDensityChanged = (RenderState.DensityTexture != InState.DensityTexture);
	const bool bNoiseChanged = (RenderState.VolumeNoiseTexture != InState.VolumeNoiseTexture);
	
	RenderState = InState;
	
	if (RenderState.DensityTexture)
	{
		if (bDensityChanged || !DensityPooledRT)
		{
			DensityPooledRT = CreateRenderTarget(RenderState.DensityTexture, TEXT("FogDensity"));
		}
	}
	else
	{
		DensityPooledRT.SafeRelease();
	}
	
	// Volume Noise Texture  
	if (RenderState.VolumeNoiseTexture)
	{
		if (bNoiseChanged || !VolumeNoisePooledRT)
		{
			VolumeNoisePooledRT = CreateRenderTarget(RenderState.VolumeNoiseTexture, TEXT("VolumeNoise"));
		}
	}
	else
	{
		VolumeNoisePooledRT.SafeRelease();
	}
}

void FFogSceneViewExtension::UpdateHeightCurveLUT_RenderThread(FRHICommandListImmediate& RHICmdList,
                                                               TConstArrayView<float> Samples)
{ 
	// RenderThread가 아니면 assert 
	check(IsInRenderingThread());
	
	// Sampling할게 없으면 Release
	if (Samples.IsEmpty())
	{
		ReleaseHeightCurveLUT_RenderThread();
		return;
	}
	
	const uint32 NewSizeX = static_cast<uint32>(Samples.Num());
	
	// Height Curve Resource에 문제가 있으면 Release 후,
	// Height Curve Resource 재생성 및 초기화
	if (!HeightCurveResource || HeightCurveResource->GetSizeX() != NewSizeX)
	{
		ReleaseHeightCurveLUT_RenderThread();
		
		HeightCurveResource = MakeUnique<FHeightCurveLUTResource>(NewSizeX);
		HeightCurveResource->InitRHI(RHICmdList);
		HeightCurvePooledRT = CreateRenderTarget(HeightCurveResource->GetRHI(), TEXT("FogHeightCurve"));
	}
	
	HeightCurveResource->Update(RHICmdList, Samples);
}

void FFogSceneViewExtension::ReleaseHeightCurveLUT_RenderThread()
{
	check(IsInRenderingThread());
	
	HeightCurvePooledRT.SafeRelease();
	
	if (HeightCurveResource)
	{
		HeightCurveResource->ReleaseResource();
		HeightCurveResource.Reset();
	}
}
 
