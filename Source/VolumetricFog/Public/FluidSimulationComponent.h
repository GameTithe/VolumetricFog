#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "FluidSimulationComponent.generated.h"


// 시뮬레이션에 필요한 RTs
struct FFluidResources
{
	FTextureRHIRef Velocity[2];
	FTextureRHIRef Density[2];
	FTextureRHIRef Pressure[2];
	FTextureRHIRef Divergence; 
	FTextureRHIRef Vorticity;

	int32 Resolution = 0;
	bool bInitialize = false;

	void Init(int32 Res, FRHICommandListImmediate& RHICmdList);
};

UCLASS(ClassGroup=(VolumetricFog), meta=(BlueprintSpawnableComponent))
class VOLUMETRICFOG_API UFluidSimulationComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UFluidSimulationComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ======== Editer Setting ========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid")
	UTextureRenderTarget2D* OutputRT;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid", meta = (ClampMin = "16", ClampMax = "2048"))
	int32 SimResolution = 256;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Force")
	FVector2D ForcePosition = FVector2D(0.5, 0.8);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Force")
	FVector2D ForceDirection = FVector2D(0.0f, -1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Force")
	float ForceRadius = 0.05f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Force")
	float ForceStrength = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Force")
	float DensityAmount = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid", meta = (ClampMin = "0.9", ClampMax = "1.0"))
	float Dissipation = 0.995f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid", meta = (ClampMin = "0.0"))
	float VorticityStrengthParam = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid", meta = (ClampMin = "1", ClampMax = "100"))
	int32 PressureIterations = 20;

private:
	TSharedPtr<FFluidResources, ESPMode::ThreadSafe> FluidResources;

	// ping-pong Index
	int32 VelIndex = 0;
	int32 DenIndex = 0;
	int32 PresIndex = 0;

	void ExecuteSimulation(
		FRHICommandListImmediate& RHICmdList,
		TSharedPtr<FFluidResources, ESPMode::ThreadSafe> FluidResources,
		FTextureRenderTargetResource* RTResource,
		float DeltaTime,
	 	int32 InVelIndex, int32 InDenIndex, int32 InPresIndex,
	 	// 시뮬레이션 파라미터들
	 	FVector2f InForcePosition,
	 	FVector2f InForceDirection,
	 	float InForceRadius,
	 	float InForceStrength,
	 	float InDensityAmount,
	 	float InDissipation,
	 	float InVorticityStrength,
	 	int32 InPressureIterations,
	 	// 반환용
	 	int32& OutVelIndex, int32& OutDenIndex, int32& OutPresIndex
	 	);
	
};