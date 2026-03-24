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
	if (!bEnable || !DensityRHI || !VelocityRHI || !DensityPooledRT || !VelocityPooledRT || !CurlNoisePooledRT)
	{
		return;
	}

	FRDGTextureRef SceneColor = InParameters.ColorTexture;
	FRDGTextureRef SceneDepth = InParameters.DepthTexture;

	const FScreenPassTexture SceneColorInput(SceneColor, InParameters.ViewportRect);
	const FScreenPassTexture SceneDepthInput(SceneDepth, InParameters.ViewportRect);

	FRDGBuilder& GraphBuilder = *InParameters.GraphBuilder;
	const FViewInfo& View = *InParameters.View;

	FScreenPassRenderTarget FogOutput =
		FScreenPassRenderTarget::CreateFromInput(
			GraphBuilder,
			SceneColorInput,
			ERenderTargetLoadAction::ENoAction,
			TEXT("FogOutput")
		); 
	 
	// 캐싱된 PooledRT를 RDG에 등록
	FRDGTextureRef DensityRDG = GraphBuilder.RegisterExternalTexture(DensityPooledRT);
	FRDGTextureRef VelocityRDG = GraphBuilder.RegisterExternalTexture(VelocityPooledRT); 
	FRDGTextureRef CurlNoiseRDG = GraphBuilder.RegisterExternalTexture(CurlNoisePooledRT);
	
	const FRDGSystemTextures& SystemTextures = FRDGSystemTextures::Create(GraphBuilder);
	/** HeightCurve가 전송되기 전이면 Fallback texture 사용 */
	FRDGTextureRef HeightCurveRDG = HeightCurvePooledRT ?  GraphBuilder.RegisterExternalTexture(HeightCurvePooledRT) : SystemTextures.White;
	 
	
	// Shader Params 
	auto* Params = GraphBuilder.AllocParameters<FFogRayMarchingPS::FParameters>();
	Params->SceneColorTexture = SceneColor;
	Params->SceneDepthTexture = SceneDepth;
	Params->CurlNoiseTexture = CurlNoiseRDG;
	Params->HeightCurveTexture = HeightCurveRDG;

	Params->SceneColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	Params->SceneDepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	Params->CurlNoiseSampler = TStaticSamplerState< SF_Bilinear>::GetRHI();
	Params->HeightCurveSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(); 
	
	Params->SceneColorViewport = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(SceneColorInput));
	Params->SceneDepthViewport = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(SceneDepthInput));
	Params->OutputViewport = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(FogOutput));

	Params->DensityTexture  = DensityRDG;
	Params->VelocityTexture = VelocityRDG;
	Params->BilinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp>::GetRHI();

	Params->InvViewProjectionMatrix = FMatrix44f(View.ViewMatrices.GetInvViewProjectionMatrix());
	Params->CameraPosition = FVector3f(View.ViewMatrices.GetViewOrigin());

	Params->HeightAttenuationMode = HeightAttenuationMode;
	Params->HeightFadeStartRatio = HeightFadeStartRatio;
	Params->HeightFadeStrength = HeightFadeStrength;

	Params->FogBaseHeight        = FogBaseHeight;
	Params->FogMaxHeight         = FogMaxHeight;
	Params->HeightFalloff        = HeightFalloff;


	Params->FogDensityMultiplier = FogDensityMultiplier;
	Params->Absorption           = Absorption;
	Params->FogColor             = FogColor;
	Params->NumSteps             = NumSteps;
	Params->MaxRayDistance       = MaxRayDistance;

	Params->CurlTexScale = CurlTexScale;
	Params->CurlTexSpeed = CurlTexSpeed;
	Params->CurlTexStrength = CurlTexStrength;

	Params->CurlNoiseScale         = CurlNoiseScale;
	Params->CurlNoiseSpeed         = CurlNoiseSpeed;
	Params->CurlDistortStrength    = CurlDistortStrength;
	Params->VelocityDistortStrength = VelocityDistortStrength;
	Params->BaseNoiseScale         = BaseNoiseScale;
	Params->Time                   = AccumulatedTime;
 
	Params->SimulationCenter = SimulationCenter;
	Params->SimulationExtents = SimulationExtents;

	
	Params->FogDebugMode = FogDebugMode;
	
	// Scene Color에 직접 합성 
	//Params->RenderTargets[0] = FRenderTargetBinding(SceneColor, ERenderTargetLoadAction::ELoad);
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

void FFogSceneViewExtension::UpdateHeightCurveLUT_RenderThread(FRHICommandListImmediate& RHICmdList,
	TConstArrayView<float> Samples)
{
	check(IsInRenderingThread());
	
	if (Samples.IsEmpty())
	{
		ReleaseHeightCurveLUT_RenderThread();
		return;
	}
	
	const uint32 NewSizeX = static_cast<uint32>(Samples.Num());
	
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
 
