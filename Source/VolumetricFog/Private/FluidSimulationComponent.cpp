#include "FluidSimulationComponent.h"
#include "FluidShaders.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHICommandList.h"
#include "RHIStaticStates.h"
#include "GlobalShader.h"
#include "VolumetricFluidFog.h"
#include "Components/BoxComponent.h"
#include "Curves/CurveFloat.h"
#include "RHIBreadcrumbs.h"
#include "Stats/StatsHierarchical.h"

// ======== Fluid Resource ========
void FFluidResources::Init(int32 Res, FRHICommandListImmediate& RHICmdList)
{
	Resolution = Res;

	auto CreateUAVTexForCS = [&](const TCHAR* Name, EPixelFormat Format) -> FTextureRHIRef
	{
		FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create2D(Name)
		.SetExtent(Res, Res)
		.SetFormat(Format)
		.SetNumMips(1)
		.SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV)
		.SetInitialState(ERHIAccess::UAVCompute);

		return RHICreateTexture(Desc);
	};

	Velocity[0] = CreateUAVTexForCS(TEXT("FluidVelocityA"), PF_G32R32F);
	Velocity[1] = CreateUAVTexForCS(TEXT("FluidVelocityB"), PF_G32R32F);

	Density[0] = CreateUAVTexForCS(TEXT("FluidDensityA"), PF_R32_FLOAT);
	Density[1] = CreateUAVTexForCS(TEXT("FluidDensityB"), PF_R32_FLOAT);
	
	Pressure[0] = CreateUAVTexForCS(TEXT("FluidPressureA"), PF_R32_FLOAT);
	Pressure[1] = CreateUAVTexForCS(TEXT("FluidPressureB"), PF_R32_FLOAT);

	Divergence = CreateUAVTexForCS(TEXT("FluidDivergence"), PF_R32_FLOAT);
	Vorticity = CreateUAVTexForCS(TEXT("FluidVorticity"), PF_R32_FLOAT);

	TempVelocity = CreateUAVTexForCS(TEXT("FluidTempVelocity"), PF_G32R32F);
	// SRV/UAV 캐싱
	VelocitySRV[0] = RHICmdList.CreateShaderResourceView(Velocity[0], 0);
	VelocitySRV[1] = RHICmdList.CreateShaderResourceView(Velocity[1], 0);
	DensitySRV[0]  = RHICmdList.CreateShaderResourceView(Density[0], 0);
	DensitySRV[1]  = RHICmdList.CreateShaderResourceView(Density[1], 0);
	PressureSRV[0] = RHICmdList.CreateShaderResourceView(Pressure[0], 0);
	PressureSRV[1] = RHICmdList.CreateShaderResourceView(Pressure[1], 0);
	DivergenceSRV  = RHICmdList.CreateShaderResourceView(Divergence, 0);
	VorticitySRV   = RHICmdList.CreateShaderResourceView(Vorticity, 0);
	TempVelocitySRV = RHICmdList.CreateShaderResourceView(TempVelocity, 0);

	VelocityUAV[0] = RHICmdList.CreateUnorderedAccessView(Velocity[0], 0);
	VelocityUAV[1] = RHICmdList.CreateUnorderedAccessView(Velocity[1], 0);
	DensityUAV[0]  = RHICmdList.CreateUnorderedAccessView(Density[0], 0);
	DensityUAV[1]  = RHICmdList.CreateUnorderedAccessView(Density[1], 0);
	PressureUAV[0] = RHICmdList.CreateUnorderedAccessView(Pressure[0], 0);
	PressureUAV[1] = RHICmdList.CreateUnorderedAccessView(Pressure[1], 0);
	DivergenceUAV  = RHICmdList.CreateUnorderedAccessView(Divergence, 0);
	VorticityUAV   = RHICmdList.CreateUnorderedAccessView(Vorticity, 0);
	TempVelocityUAV = RHICmdList.CreateUnorderedAccessView(TempVelocity, 0);

	// 캐싱된 UAV로 클리어
	RHICmdList.ClearUAVFloat(VelocityUAV[0], FVector4f(0, 0, 0, 0));
	RHICmdList.ClearUAVFloat(VelocityUAV[1], FVector4f(0, 0, 0, 0));
	RHICmdList.ClearUAVFloat(DensityUAV[0], FVector4f(0, 0, 0, 0));
	RHICmdList.ClearUAVFloat(DensityUAV[1], FVector4f(0, 0, 0, 0));
	RHICmdList.ClearUAVFloat(PressureUAV[0], FVector4f(0, 0, 0, 0));
	RHICmdList.ClearUAVFloat(PressureUAV[1], FVector4f(0, 0, 0, 0));
	RHICmdList.ClearUAVFloat(DivergenceUAV, FVector4f(0, 0, 0, 0));
	RHICmdList.ClearUAVFloat(VorticityUAV, FVector4f(0, 0, 0, 0));
	RHICmdList.ClearUAVFloat(TempVelocityUAV, FVector4f(0, 0, 0, 0));

	bInitialize = true;
}

// ======== Fluid Simulation Component ========

UFluidSimulationComponent::UFluidSimulationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFluidSimulationComponent::BeginPlay()
{
	Super::BeginPlay();

	// PIE에 들어갈 때, 한 번 GPU에 올리기
	FluidResources = MakeShared<FFluidResources, ESPMode::ThreadSafe>();
	int32 Res = SimResolution;
	auto Resources = FluidResources;
	
	// Render thread에서 resources 초기화
	ENQUEUE_RENDER_COMMAND(FInitFluidResource)
	(
		[Resources, Res](FRHICommandListImmediate& RHICmdList)
		{
			Resources->Init(Res, RHICmdList);
		}
	);
	
	// Curve Data 
	const bool bUseCurveAttenuation = HeightAttenuationMode == EFluidHeightAttenuationMode::CurveAttenuation;
	
	bWasUsingCurveAttenuation = bUseCurveAttenuation;
	LastHeightCurveAsset = HeightAttenuationCurve;
	LastHeightCurveLUTResolution = HeightCurveLUTResolution;
	bHeightCurveDirty = bUseCurveAttenuation;
	
		
	// Fog Data Snapshot을 Render Thread에 전달 
	FogExtension = FSceneViewExtensions::NewExtension<FFogSceneViewExtension>();
	
	if (FogExtension.IsValid())
	{
		const FFluidFogRenderState InitialState = BuildFogRenderStateSnapShot();
		auto Ext = FogExtension;
		
		ENQUEUE_RENDER_COMMAND(InitFogExtension) 
		([Ext, InitialState](FRHICommandListImmediate& RHUCmdList)
		{
			if (Ext.IsValid())
			{
				Ext->ApplyRenderState_RenderThread(InitialState);
			}
		});
	} 
	
	if (bUseCurveAttenuation)
	{
		PushHeightCurveSamplesToFogExtension();
		bHeightCurveDirty = false;
	}
	
}


void UFluidSimulationComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// Resources가 초기화되지 않았으면 Early Return
	if (!FluidResources)
	{
		UE_LOG(LogTemp, Warning, TEXT("TickComponent: Fluid Resources is Null"));
		return;
	}
	
	// 게임스레드에서 설정해둔 Fog관련 값들 캡처
	auto Resources = FluidResources;
	float DT = DeltaTime;
	FVector2f FP = FVector2f(ForcePosition);
	FVector2f FD = FVector2f(ForceDirection);
	float FR = ForceRadius;
	float FS = ForceStrength;
	float DA = DensityAmount;
	float Diss = Dissipation;
	float Vortiy = VorticityStrengthParam;
	float Visc = Viscosity;
	int32 PresItr = PressureIterations; 

    float SimTime = AccumulatedTime + DeltaTime;
    float CurlTiling = CurlSimulationTiling;
    float CurlSpeed = CurlSimulationSpeed;
    float CurlStrength = CurlVelocityStrength;
    float CurlMaskScale = CurlDensityMaskScale;

    FTextureRHIRef CurlSimulationTexRHI = nullptr;
    if (CurlNoiseTexture && CurlNoiseTexture->GetResource())
    {
        CurlSimulationTexRHI = CurlNoiseTexture->GetResource()->TextureRHI;
    }

	FFluidFogRenderState Snapshot  =  BuildFogRenderStateSnapShot();
	auto Ext = FogExtension;
	
	ENQUEUE_RENDER_COMMAND(FFluidSimluationStep)(
	[ Resources, Ext, Snapshot, 
		DT, FP, FD, FR, FS, DA, Diss, 
        CurlSimulationTexRHI, SimTime, CurlTiling, CurlSpeed, CurlStrength, CurlMaskScale,
        Vortiy, Visc, PresItr](FRHICommandListImmediate& RHICmdList) mutable 
	{
		if (!Resources->bInitialize)
		{
			if (Ext.IsValid())
			{
				FFluidFogRenderState DisabledState = Snapshot;
				DisabledState.bEnable = false;
				Ext->ApplyRenderState_RenderThread(DisabledState);
			}
			return;
		}

		int32 InVelIdx = Resources->VelocityIndex;
		int32 InDenIdx = Resources->DensityIndex;
		int32 InPressIdx = Resources->PressureIndex;
		
		int32 OutVelIdx = InVelIdx, OutDenIdx = InDenIdx, OutPrsIdx = InPressIdx;
         
		// 시뮬레이션
		UFluidSimulationComponent::ExecuteSimulation(RHICmdList, Resources,
			DT,InVelIdx, InDenIdx, InPressIdx,
					   FP, FD, FR, FS, DA, Diss, 
                       CurlSimulationTexRHI, SimTime, CurlTiling, CurlSpeed, CurlStrength, CurlMaskScale, 
                       Vortiy, Visc, PresItr,
					   OutVelIdx, OutDenIdx, OutPrsIdx); 
	
		Resources->VelocityIndex = OutVelIdx;
		Resources->DensityIndex = OutDenIdx;
		Resources->PressureIndex = OutPrsIdx; 
		
		// 시뮬레이션 결과를 바탕으로 렌더링 (ping-pong buffer)
		Snapshot.DensityTexture = Resources->Density[OutDenIdx];
		
		if (Ext.IsValid())
		{	
			Ext->ApplyRenderState_RenderThread(Snapshot);
		}
	}
	);
	
	AccumulatedTime += DeltaTime;
 
}
 
bool UFluidSimulationComponent::ResolveSimulationBounds(FVector& OutOrigin, FVector& OutExtent) const
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    if (const AVolumetricFluidFog* FogActor = Cast<AVolumetricFluidFog>(Owner))
    {
        if (const UBoxComponent* Box = FogActor->GetBoundsComponents())
        {
            OutOrigin = Box->GetComponentLocation();
            OutExtent = Box->GetScaledBoxExtent();
            return true;
        }
    }

    Owner->GetActorBounds(false, OutOrigin, OutExtent);
    return true;
}


FFluidFogRenderState UFluidSimulationComponent::BuildFogRenderStateSnapShot() const
{
	FFluidFogRenderState State;
	State.bEnable = bEnableFog;
	State.HeightAttenuationMode = static_cast<int32>(HeightAttenuationMode);
	State.HeightFalloff = HeightFalloff;
	State.HeightFadeStartRatio = HeightFadeStartRatio;
	State.HeightFadeStrength = HeightFadeStrength;
	State.FogDensityMultiplier = FogDensityMultiplier; 
	State.AbsorptionScale = AbsorptionScale;
	State.ScatteringScale = ScatteringScale;
	State.FogColor = FVector3f(FogColor.R, FogColor.G, FogColor.B);
	State.NumSteps = NumSteps;
	State.MaxRayDistance = MaxRayDistance;
	State.FogDebugMode = static_cast<int32>(FogDebugMode);
	FVector BoundsOrigin, BoundsExtents;
	
	if (ResolveSimulationBounds(BoundsOrigin, BoundsExtents))
	{
		State.SimulationCenter = FVector3f(BoundsOrigin);
		State.SimulationExtents = FVector3f(BoundsExtents);
		State.FogBaseHeight = BoundsOrigin.Z - BoundsExtents.Z;
		State.FogMaxHeight = BoundsOrigin.Z + BoundsExtents.Z;
	}
	
	// Density Texture의 인덱스를 여기서는 알 수 없으니 따로 채워줘야 함
	
	
	//Self Shadow  
	const FVector SafeToLight = SelfShadowLightDirection.GetSafeNormal();
	State.SelfShadowLightDirection = FVector3f(SafeToLight);
	State.SelfShadowLightColor = FVector3f(SelfShadowLightColor.R, SelfShadowLightColor.G, SelfShadowLightColor.B);
	State.SelfShadowLightIntensity = SelfShadowLightIntensity;
	State.SelfShadowDensityScale = SelfShadowDensityScale;
	State.SelfShadowStepCount = SelfShadowStepCount;
	State.SelfShadowMaxDistance = SelfShadowMaxDistance; 
	
	// Phase Function
	State.GOfHG = GOfHG;
	return State;
}

TArray<float> UFluidSimulationComponent::BuildHeightCurveSamples() const
{
	const int32 LUTWidth = FMath::Max(HeightCurveLUTResolution, 2);
	
	TArray<float> Samples;
	Samples.SetNumUninitialized(LUTWidth);
	
	float MinTime = 0.0f;
	float MaxTime = 1.0f;
	
	// CurveData의 시간 보정
	if (HeightAttenuationCurve)
	{
		HeightAttenuationCurve->FloatCurve.GetTimeRange(MinTime, MaxTime);
		
		if (FMath::IsNearlyEqual( MinTime,  MaxTime))
		{
			MaxTime = MinTime + 1.0f;
		}
	}
	
	// CurveData의 시간에 맞게 값 보정 
	for (int32 X = 0; X < LUTWidth; ++X)
	{ 
		const float U = LUTWidth > 1 ? static_cast<float>(X) / static_cast<float>(LUTWidth - 1)  : 0.0f;
		const float CurveTime = FMath::Lerp(MinTime, MaxTime, U);
		
		Samples[X] = HeightAttenuationCurve ? HeightAttenuationCurve->GetFloatValue(CurveTime) : (1.0f - U); 
	}
	
	return Samples;
}

void UFluidSimulationComponent::PushHeightCurveSamplesToFogExtension()
{
	if (!FogExtension.IsValid())
	{
		return;
	}

	// Curve Data를 Array<Float>로 받아오기
	TArray<float> Samples = BuildHeightCurveSamples();
	TSharedPtr<FFogSceneViewExtension, ESPMode::ThreadSafe> LocalFogExtension = FogExtension;

	ENQUEUE_RENDER_COMMAND(FPushFogHeightCurveSamples)(
			[LocalFogExtension, Samples = MoveTemp(Samples)](FRHICommandListImmediate& RHICmdList) mutable
			{
					if (LocalFogExtension.IsValid())
					{
						LocalFogExtension->UpdateHeightCurveLUT_RenderThread(RHICmdList, MakeArrayView(Samples));
					}
			});
}

void UFluidSimulationComponent::ReleaseHeightCurveFromFogExtension()
{ 
	if (!FogExtension.IsValid())
	{
		return;
	}

	TSharedPtr<FFogSceneViewExtension, ESPMode::ThreadSafe> LocalFogExtension = FogExtension;

	ENQUEUE_RENDER_COMMAND(FReleaseFogHeightCurve)(
			[LocalFogExtension](FRHICommandListImmediate& RHICmdList)
			{
					if (LocalFogExtension.IsValid())
					{
							LocalFogExtension->ReleaseHeightCurveLUT_RenderThread();
					}
			});
}


void UFluidSimulationComponent::ExecuteSimulation(FRHICommandListImmediate& RHICmdList,
												  TSharedPtr<FFluidResources, ESPMode::ThreadSafe> InFluidResources,
												  float DeltaTime, int32 InVelIndex, int32 InDenIndex, int32 InPresIndex, FVector2f InForcePosition,
												  FVector2f InForceDirection, float InForceRadius, float InForceStrength, float InDensityAmount, float InDissipation, 
												  FTextureRHIRef InCurlNoiseTexture, float InSimulationTime, float InCurlSimulationTiling, float InCurlSimulationSpeed, float InCurlVelocityStrength, float InCurlDensityMaskScale,
                                                  float InVorticityStrength, float InVisc, int32 InPressureIterations, int32& OutVelIndex, int32& OutDenIndex, int32& OutPresIndex)
 {
 	 check(IsInRenderingThread());
  
 	 const int32 Resolution = InFluidResources->Resolution;
 	 const int32 VelWriteIdx = 1 - InVelIndex;
 	 const int32 DenWriteIdx = 1 - InDenIndex;
 	 const int32 PresWriteIdx = 1 - InPresIndex;
      
 	 const float Dx = 1.0f / (float)Resolution;
 	 const float HalfInvDx = 0.5f * (float)Resolution;
 	 const FVector2f InvResolution(Dx, Dx);
 
 	 const FIntPoint ResolutionPt(Resolution, Resolution);
 	 const FIntVector GroupCount(
 	 	FMath::DivideAndRoundUp(Resolution, 8), // 8: Num of Computer Shader Threads 
 	 	FMath::DivideAndRoundUp(Resolution, 8), // 8: Num of Computer Shader Threads 
 	 	1
 	 );
  
 	 auto ToSRV = [&](FRHITexture* Tex)
 	 {
 	 	RHICmdList.Transition(FRHITransitionInfo(Tex, ERHIAccess::Unknown, ERHIAccess::SRVCompute));
 	 };
 	 auto ToUAV = [&](FRHITexture* Tex)
 	 {
 	 	RHICmdList.Transition(FRHITransitionInfo(Tex, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
 	 };
 	
 	 int CurVelIdx = InVelIndex;
 	 int CurDenIdx = InDenIndex;
 	 int CurPresIdx = InPresIndex;
  
 	 // Step 1: Advect Velocity
 	 {
         RHI_BREADCRUMB_EVENT(RHICmdList, "Fluid.AdvectVelocity");

 	 	int32 NextVelIdx = 1 - CurVelIdx;
  
 	 	ToSRV(InFluidResources->Velocity[CurVelIdx]);
 	 	ToUAV(InFluidResources->Velocity[NextVelIdx]);
  
 	 	TShaderMapRef<FFluidAdvectVelocityCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
 	 	FFluidAdvectVelocityCS::FParameters Params;
 	 	Params.VelocityInput = InFluidResources->VelocitySRV[CurVelIdx];
 	 	Params.VelocityOutput = InFluidResources->VelocityUAV[NextVelIdx];
 	 	Params.BilinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
 	 	Params.DeltaTime = DeltaTime;
 	 	Params.InvResolution  = InvResolution;
 	 	Params.Resolution     = ResolutionPt;
  
 	 	FComputeShaderUtils::Dispatch(RHICmdList, Shader, Params, GroupCount);
         CurVelIdx = NextVelIdx; 
 	 }
 	 
 	  //// Step2: Vorticity
 	  //{
 	  //	ToSRV(InFluidResources->Velocity[CurVelIdx]);
 	  //	ToUAV(InFluidResources->Vorticity);
      //
 	  //	TShaderMapRef<FFluidVorticityCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
 	  //	FFluidVorticityCS::FParameters Params;
      //
 	  //	Params.VelocityInput = InFluidResources->VelocitySRV[CurVelIdx];
 	  //	Params.VorticityOutput = MakeUAV(InFluidResources->Vorticity);
 	  //	Params.Resolution = ResolutionPt;
 	  //	Params.HalfInvDx = HalfInvDx;
      //
 	  //	FComputeShaderUtils::Dispatch(RHICmdList, Shader, Params, GroupCount);
 	  //}
 	  //
 	  //// Step3: Vorticity Confinement Force
 	  //{
 	  //	int NextVelIdx = 1 - CurVelIdx;
 	  //	ToSRV(InFluidResources->Velocity[CurVelIdx]);
 	  //	ToSRV(InFluidResources->Vorticity);
 	  //	ToUAV(InFluidResources->Velocity[NextVelIdx]);
 	  //
 	  //	TShaderMapRef<FFluidVorticityForceCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
 	  //	FFluidVorticityForceCS::FParameters Params;
      //
 	  //	Params.VelocityInput = InFluidResources->VelocitySRV[CurVelIdx];
 	  //	Params.VorticityInput = MakeSRV(InFluidResources->Vorticity);
 	  //	Params.VelocityOutput = InFluidResources->VelocityUAV[NextVelIdx];
 	  //	Params.Resolution = ResolutionPt;
 	  //	Params.VorticityStrength = InVorticityStrength;
 	  //	Params.DeltaTime = DeltaTime;
 	  //	Params.HalfInvDx = HalfInvDx;
      //
 	  //	FComputeShaderUtils::Dispatch(RHICmdList, Shader, Params, GroupCount);
 	  //	CurVelIdx = NextVelIdx;
 	  //}
	    if (InVisc > 0.0f)
	    {
	    	// 원본 속도를 TempVelocity에 보관 (Jacobi "b" 항, 반복 내내 고정)
	    	RHICmdList.Transition(FRHITransitionInfo(InFluidResources->Velocity[CurVelIdx], ERHIAccess::Unknown, ERHIAccess::CopySrc));
	    	RHICmdList.Transition(FRHITransitionInfo(InFluidResources->TempVelocity, ERHIAccess::Unknown, ERHIAccess::CopyDest));
	    	FRHICopyTextureInfo CopyInfo;
	    	RHICmdList.CopyTexture(InFluidResources->Velocity[CurVelIdx], InFluidResources->TempVelocity, CopyInfo);

	    	// Dx=1 이므로: a = dt * visc, Alpha = 1/a, InvBeta = 1/(4 + Alpha)
	    	//const float a = DeltaTime * InVisc;
	    	const float a = DeltaTime * InVisc / (Dx * Dx);
	    	const float ViscAlpha = 1.0f / a;
	    	const float ViscInvBeta = 1.0f / (4.0f + ViscAlpha);

	    	for (int32 i = 0; i < 5; ++i)
	    	{
	    		int32 NextVelIdx = 1 - CurVelIdx;

	    		ToSRV(InFluidResources->Velocity[CurVelIdx]);
	    		ToSRV(InFluidResources->TempVelocity);
	    		ToUAV(InFluidResources->Velocity[NextVelIdx]);

	    		TShaderMapRef<FFluidDiffuseVelocityCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	    		FFluidDiffuseVelocityCS::FParameters Params;
	    		Params.InputTexture  = InFluidResources->VelocitySRV[CurVelIdx];
	    		Params.PrevTexture   = InFluidResources->TempVelocitySRV;
	    		Params.OutputTexture = InFluidResources->VelocityUAV[NextVelIdx];
	    		Params.Alpha         = ViscAlpha;
	    		Params.InvBeta       = ViscInvBeta;
	    		Params.Resolution    = ResolutionPt;

	    		FComputeShaderUtils::Dispatch(RHICmdList, Shader, Params, GroupCount);
	    		CurVelIdx = NextVelIdx;
	    	}
	    }
 	 // Step 4: Force
 	 {

        RHI_BREADCRUMB_EVENT(RHICmdList, "Fluid.Force");

 	 	int32 NextVelIdx = 1 - CurVelIdx;
 	 	int32 NextDenIdx = 1 - CurDenIdx;
 	 	ToSRV(InFluidResources->Velocity[CurVelIdx]);
 	 	ToSRV(InFluidResources->Density[CurDenIdx]);
 	 	ToUAV(InFluidResources->Velocity[NextVelIdx]);
 	 	ToUAV(InFluidResources->Density[NextDenIdx]);
  
 	 	TShaderMapRef<FFluidForceCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
 	 	FFluidForceCS::FParameters Params;
 	 	Params.VelocityInput  = InFluidResources->VelocitySRV[CurVelIdx];
 	 	Params.DensityInput   = InFluidResources->DensitySRV[CurDenIdx];
 	 	Params.VelocityOutput = InFluidResources->VelocityUAV[NextVelIdx];
 	 	Params.DensityOutput  = InFluidResources->DensityUAV[NextDenIdx];
 	 	Params.ForcePosition  = InForcePosition;
 	 	Params.ForceDirection = InForceDirection;
 	 	Params.ForceRadius    = InForceRadius;
 	 	Params.ForceStrength  = InForceStrength;
 	 	Params.DensityAmount  = InDensityAmount;
 	 	Params.DeltaTime      = DeltaTime;
 	 	Params.Dissipation    = InDissipation;
        Params.CurlNoiseTexture = InCurlNoiseTexture;
        Params.CurlNoiseSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Clamp>::GetRHI();
        Params.Time = InSimulationTime;
        Params.CurlSimulationTiling = InCurlSimulationTiling;
        Params.CurlSimulationSpeed = InCurlSimulationSpeed;
        Params.CurlVelocityStrength = InCurlVelocityStrength;
        Params.CurlDensityMaskScale = InCurlDensityMaskScale;
 	 	Params.InvResolution  = InvResolution;
 	 	Params.Resolution     = ResolutionPt;
  
 	 	FComputeShaderUtils::Dispatch(RHICmdList, Shader, Params, GroupCount);
 	 	CurVelIdx = NextVelIdx;
 	 	CurDenIdx = NextDenIdx;
 	 } 
	
 	 // Step 5: Divergence
 	 {
         RHI_BREADCRUMB_EVENT(RHICmdList, "Fluid.Divergence");

 	 	ToSRV(InFluidResources->Velocity[CurVelIdx]);
 	 	ToUAV(InFluidResources->Divergence);
  
 	 	TShaderMapRef<FFluidDivergenceCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
 	 	FFluidDivergenceCS::FParameters Params;
 	 	Params.VelocityInput    = InFluidResources->VelocitySRV[CurVelIdx];
 	 	Params.DivergenceOutput = InFluidResources->DivergenceUAV;
 	 	Params.Resolution       = ResolutionPt;
 	 	Params.HalfInvDx        = HalfInvDx;
  
 	 	FComputeShaderUtils::Dispatch(RHICmdList, Shader, Params, GroupCount); 
 	 }
  
 	 // Step 6: Pressure Solve (Jacobi iteration)
 	 {

 		 {
 		 	RHICmdList.ClearUAVFloat(InFluidResources->PressureUAV[0], FVector4f(0, 0, 0, 0));
 		 	RHICmdList.ClearUAVFloat(InFluidResources->PressureUAV[1], FVector4f(0, 0, 0, 0));
 		 }
 	 	CurPresIdx = 0;
 	 	
 	 	const float Alpha = -(Dx * Dx);
 	 	const float InvBeta = 0.25f;
 	 	
 	 	for (int32 i = 0; i < InPressureIterations; ++i)
 	 	{
 	 		int32 NextPresIdx = 1 - CurPresIdx;
  
 	 		ToSRV(InFluidResources->Pressure[CurPresIdx]);
 	 		ToSRV(InFluidResources->Divergence);
 	 		ToUAV(InFluidResources->Pressure[NextPresIdx]);
  
 	 		TShaderMapRef<FFluidDiffuseCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
 	 		FFluidDiffuseCS::FParameters Params;
 	 		Params.InputTexture  = InFluidResources->PressureSRV[CurPresIdx];
 	 		Params.PrevTexture   = InFluidResources->DivergenceSRV;
 	 		Params.OutputTexture = InFluidResources->PressureUAV[NextPresIdx];
 	 		Params.Alpha         = Alpha;
 	 		Params.InvBeta       = InvBeta;
 	 		Params.Resolution    = ResolutionPt;
  
 	 		FComputeShaderUtils::Dispatch(RHICmdList, Shader, Params, GroupCount);
 	 		CurPresIdx = NextPresIdx;
 	 	} 
 	 }
 	
 	 // Step 7: Gradient Subtract — velocity
 	 {
         RHI_BREADCRUMB_EVENT(RHICmdList, "Fluid.GradientSubtract");

 	 	int32 NextVelIdx = 1 - CurVelIdx;
 	 	ToSRV(InFluidResources->Velocity[CurVelIdx]);
 	 	ToSRV(InFluidResources->Pressure[CurPresIdx]);
 	 	ToUAV(InFluidResources->Velocity[NextVelIdx]);
  
 	 	TShaderMapRef<FFluidGradientSubtractCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
 	 	FFluidGradientSubtractCS::FParameters Params;
 	 	Params.VelocityInput  = InFluidResources->VelocitySRV[CurVelIdx];
 	 	Params.PressureInput  = InFluidResources->PressureSRV[CurPresIdx];
 	 	Params.VelocityOutput = InFluidResources->VelocityUAV[NextVelIdx];
 	 	Params.Resolution     = ResolutionPt;
 	 	Params.HalfInvDx      = HalfInvDx;
  
 	 	FComputeShaderUtils::Dispatch(RHICmdList, Shader, Params, GroupCount);
 	 	CurVelIdx = NextVelIdx;
 	 }

 	 // Step 8 : Diffuse
 	//{
 	// 	// Divergence 텍스처를 임시 버퍼로 사용 (이 시점에서 안 쓰니까)
 	// 	// 확산 전 원본 밀도를 보관
 	// 	RHICmdList.Transition(FRHITransitionInfo(InFluidResources->Density[CurDenIdx], ERHIAccess::Unknown, ERHIAccess::CopySrc));
 	// 	RHICmdList.Transition(FRHITransitionInfo(InFluidResources->Divergence, ERHIAccess::Unknown, ERHIAccess::CopyDest));
 	// 	FRHICopyTextureInfo CopyInfo;
 	// 	RHICmdList.CopyTexture(InFluidResources->Density[CurDenIdx], InFluidResources->Divergence, CopyInfo);
    //
 	// 	// Jacobi 확산 파라미터
 	// 	// DensityDiffusion: 클수록 더 많이 퍼짐 (0.5 ~ 50.0 범위에서 조절)
 	// 	const float DensityDiffusion = 10.0f;
 	// 	//const float DiffAlpha = 1.0f / (DensityDiffusion * DeltaTime);
 	// 	const float DiffAlpha = (Dx * Dx) / (DensityDiffusion * DeltaTime);
 	// 	const float DiffInvBeta = 1.0f / (4.0f + DiffAlpha);
 	// 	const int32 DiffIterations = 5;
    //
 	// 	for (int32 i = 0; i < DiffIterations; ++i)
 	// 	{
 	// 		int32 NextDenIdx = 1 - CurDenIdx;
    //
 	// 		ToSRV(InFluidResources->Density[CurDenIdx]);
 	// 		ToSRV(InFluidResources->Divergence);
 	// 		ToUAV(InFluidResources->Density[NextDenIdx]);
    //
 	// 		TShaderMapRef<FFluidDiffuseCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
 	// 		FFluidDiffuseCS::FParameters Params;
 	// 		Params.InputTexture  = InFluidResources->DensitySRV[CurDenIdx];   // 이웃 (현재 추정)
 	// 		Params.PrevTexture   = InFluidResources->DivergenceSRV;           // 원본 밀도 (b)
 	// 		Params.OutputTexture = InFluidResources->DensityUAV[NextDenIdx];
 	// 		Params.Alpha         = DiffAlpha;
 	// 		Params.InvBeta       = DiffInvBeta;
 	// 		Params.Resolution    = ResolutionPt;
    //
 	// 		FComputeShaderUtils::Dispatch(RHICmdList, Shader, Params, GroupCount);
 	// 		CurDenIdx = NextDenIdx;
 	// 	}
 	//}
 	 // Step 9: Advect Density
 	 {
         RHI_BREADCRUMB_EVENT(RHICmdList, "Fluid.AdvectDensity");
 	 	int32 NextDenIdx = 1 - CurDenIdx;
 	 	ToSRV(InFluidResources->Velocity[CurVelIdx]);
 	 	ToSRV(InFluidResources->Density[CurDenIdx]);
 	 	ToUAV(InFluidResources->Density[NextDenIdx]);
  
 	 	TShaderMapRef<FFluidAdvectCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
 	 	FFluidAdvectCS::FParameters Params;
 	 	Params.VelocityInput  = InFluidResources->VelocitySRV[CurVelIdx];
 	 	Params.DensityInput   = InFluidResources->DensitySRV[CurDenIdx];
 	 	Params.DensityOutput  = InFluidResources->DensityUAV[NextDenIdx];
 	 	Params.BilinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
 	 	Params.DeltaTime      = DeltaTime;
 	 	Params.InvResolution  = InvResolution;
 	 	Params.Resolution     = ResolutionPt;
  
 	 	FComputeShaderUtils::Dispatch(RHICmdList, Shader, Params, GroupCount);
 	 	CurDenIdx = NextDenIdx;
 	 }
 	
  
	 OutVelIndex = CurVelIdx;
	 OutDenIndex = CurDenIdx;
	 OutPresIndex = CurPresIdx;
 
 }
  
void UFluidSimulationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	FluidResources.Reset();
	
	if (FogExtension.IsValid())
	{
		TSharedPtr<FFogSceneViewExtension, ESPMode::ThreadSafe> LocalFogExtension= FogExtension; 
		
		ReleaseHeightCurveFromFogExtension();
		
		ENQUEUE_RENDER_COMMAND(FDisableFogExtension)
		(
			[LocalFogExtension](FRHICommandListImmediate& RHICmdList)
			{
				if (LocalFogExtension.IsValid())
				{
					FFluidFogRenderState DisabledState;
					DisabledState.bEnable = false;
					LocalFogExtension->ApplyRenderState_RenderThread((DisabledState));
				}
				
			}	
		);
		FogExtension.Reset();
	}
	
}
