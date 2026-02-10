#include "NoiseGenerateComponent.h"
#include "NoiseComputeShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHICommandList.h"
#include "GlobalShader.h"

UNoiseGenerateComponent::UNoiseGenerateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNoiseGenerateComponent::BeginPlay()
{
	Super::BeginPlay();

	if (OutputRT)
	{
		OutputRT->bSupportsUAV = true;
		OutputRT->bCanCreateUAV = true;
		OutputRT->UpdateResourceImmediate(true);
	}
}


void UNoiseGenerateComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AccumulatedTime += DeltaTime;

	if (!OutputRT)
	{
		return;
	}

	FTextureRenderTargetResource* RTResource = OutputRT->GameThread_GetRenderTargetResource();
	if (!RTResource)
	{
		return;
	}

	float CurrentTime = AccumulatedTime;;

	ENQUEUE_RENDER_COMMAND(FDispatchNoiseShader)(
		[this, RTResource, CurrentTime](FRHICommandListImmediate& RHICmdList)
		{
			ExectureNoiseShaderByRenderThread(RHICmdList, RTResource, CurrentTime);
		}
	);
	
	
}

void UNoiseGenerateComponent::ExectureNoiseShaderByRenderThread(FRHICommandListImmediate& RHICmdList,
	FTextureRenderTargetResource* RTResource, float Time)
{
	check(IsInRenderingThread());

	// UAV 생성
	FTexture2DRHIRef TextureRHI = RTResource->GetRenderTargetTexture();
	if (!TextureRHI)
	{
		return;
	}


	//SRV → UAV 전환 (쓰기 전)
      RHICmdList.Transition(
          FRHITransitionInfo(TextureRHI, ERHIAccess::Unknown, ERHIAccess::UAVCompute)
      );

	FUnorderedAccessViewRHIRef UAV = RHICmdList.CreateUnorderedAccessView(TextureRHI, 0);
		
	// shader 가져오기
	TShaderMapRef<FGenerateNoiseCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	
	// Setting Params
	FGenerateNoiseCS::FParameters Parameters;
	Parameters.Time = Time;
	Parameters.OutputTexture = UAV;
	
	// Dispatch
	FComputeShaderUtils::Dispatch(
		RHICmdList,
		ComputeShader,
		Parameters,
		FIntVector(64,64,1) // 512/8 = 64
		);
	
	
	// UAV -> SRV
	RHICmdList.Transition(
		FRHITransitionInfo(TextureRHI, ERHIAccess::UAVCompute, ERHIAccess::SRVMask)
		); 
}

