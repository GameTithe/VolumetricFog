#include "FogSceneViewExtension.h"
#include "PixelShaderUtils.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "FogSceneViewExtension.h"
#include "SceneRendering.h"

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
	if (!bEnable || !DensityRHI || !VelocityRHI || !DensityPooledRT || !VelocityPooledRT)
	{
		return;
	}

	FRDGBuilder& GraphBuilder = *InParameters.GraphBuilder;
	const FViewInfo& View = *InParameters.View;

	FRDGTextureRef SceneColor = InParameters.ColorTexture;
	FRDGTextureRef SceneDepth = InParameters.DepthTexture;

	// 캐싱된 PooledRT를 RDG에 등록
	FRDGTextureRef DensityRDG = GraphBuilder.RegisterExternalTexture(DensityPooledRT);
	FRDGTextureRef VelocityRDG = GraphBuilder.RegisterExternalTexture(VelocityPooledRT);
	
	// Shader Params 
	auto* Params = GraphBuilder.AllocParameters<FFogRayMarchingPS::FParameters>();
	Params->SceneColorTexture = SceneColor;
	Params->SceneDepthTexture = SceneDepth;
	Params->SceneColorSampler =
TStaticSamplerState<SF_Bilinear>::GetRHI();
	Params->SceneDepthSampler =
TStaticSamplerState<SF_Point>::GetRHI();

	Params->DensityTexture  = DensityRDG;
	Params->VelocityTexture = VelocityRDG;
	Params->BilinearSampler = TStaticSamplerState<SF_Bilinear,
AM_Clamp, AM_Clamp>::GetRHI();

	Params->InvViewProjectionMatrix =
FMatrix44f(View.ViewMatrices.GetInvViewProjectionMatrix());
	Params->CameraPosition =
FVector3f(View.ViewMatrices.GetViewOrigin());
	Params->ViewportSize   =
FVector2f(InParameters.ViewportRect.Width(),

InParameters.ViewportRect.Height());

	Params->FogBaseHeight        = FogBaseHeight;
	Params->FogMaxHeight         = FogMaxHeight;
	Params->HeightFalloff        = HeightFalloff;
	Params->FogDensityMultiplier = FogDensityMultiplier;
	Params->Absorption           = Absorption;
	Params->FogColor             = FogColor;
	Params->NumSteps             = NumSteps;
	Params->MaxRayDistance       = MaxRayDistance;

	Params->CurlNoiseScale         = CurlNoiseScale;
	Params->CurlNoiseSpeed         = CurlNoiseSpeed;
	Params->CurlDistortStrength    = CurlDistortStrength;
	Params->VelocityDistortStrength = VelocityDistortStrength;
	Params->BaseNoiseScale         = BaseNoiseScale;
	Params->Time                   = AccumulatedTime;

	Params->SimulationCenter = SimulationCenter;
	Params->SimulationSize   = SimulationSize;
	
	// Scene Color에 직접 합성 
	Params->RenderTargets[0] = FRenderTargetBinding(SceneColor, ERenderTargetLoadAction::ELoad);
	
	TShaderMapRef<FFogRayMarchingPS> PS(GetGlobalShaderMap(View.GetFeatureLevel()));
	
	FPixelShaderUtils::AddFullscreenPass(
			 GraphBuilder,
			 GetGlobalShaderMap(View.GetFeatureLevel()),
			 RDG_EVENT_NAME("FogRayMarch"),
			 PS, Params,
			 InParameters.ViewportRect);
}
 