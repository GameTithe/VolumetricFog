#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "FogSceneViewExtension.h"
#include "FluidSimulationComponent.generated.h"

class UCurveFloat;

// 시뮬레이션에 필요한 RTs
struct FFluidResources
{
	FTextureRHIRef Velocity[2];
	FTextureRHIRef Density[2];
	FTextureRHIRef Pressure[2];
	FTextureRHIRef Divergence; 
	FTextureRHIRef Vorticity;
	FTextureRHIRef TempVelocity;

    FShaderResourceViewRHIRef VelocitySRV[2];
    FShaderResourceViewRHIRef DensitySRV[2];
    FShaderResourceViewRHIRef PressureSRV[2];
    FShaderResourceViewRHIRef DivergenceSRV;
    FShaderResourceViewRHIRef VorticitySRV;
    FShaderResourceViewRHIRef TempVelocitySRV;

    FUnorderedAccessViewRHIRef VelocityUAV[2];
    FUnorderedAccessViewRHIRef DensityUAV[2];
    FUnorderedAccessViewRHIRef PressureUAV[2];
    FUnorderedAccessViewRHIRef DivergenceUAV;
    FUnorderedAccessViewRHIRef VorticityUAV;
    FUnorderedAccessViewRHIRef TempVelocityUAV;
    
	int32 Resolution = 0;
	bool bInitialize = false;

	void Init(int32 Res, FRHICommandListImmediate& RHICmdList);
};

UENUM(BlueprintType)
enum class EFluidFogDebugMode: uint8
{
	Sim_2D UMETA(DisplayName = "2D Simulation"),
	Sim_3D UMETA(DisplayName = "3D Simulation"),
	
};

UENUM(BlueprintType)
enum class EFluidHeightAttenuationMode : uint8
{
	LegacyExp UMETA(DisplayName = "Legacy Exponential"),
	AdaptiveNormalCurveAttenuationized UMETA(DisplayName = "Adaptive Normalized(Linear)"),
	CurveAttenuation UMETA(DisplayName = "Curve Attenuation"),
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

	// ======== Debug Setting ======== 
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category="Fluid|Debug")
	EFluidFogDebugMode FogDebugMode = EFluidFogDebugMode::Sim_3D;
	
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid", meta = (ClampMin = "0.0"))
	float Viscosity = 0.001f;
	
	
	// Fog Rendering
	// ======== Fog Rendering ========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")
	bool bEnableFog = true;
	 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")
	float HeightFalloff = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog", meta =
	(ClampMin = "0.0"))
	float FogDensityMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog", meta =
	(ClampMin = "0.0"))
	float Absorption = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")
	FLinearColor FogColor = FLinearColor(0.8f, 0.85f, 0.9f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog", meta =
	(ClampMin = "8", ClampMax = "128"))
	int32 NumSteps = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")
	float MaxRayDistance = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")
	float SimulationWorldSize = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Height", meta = (EditCondition = "HeightAttenuationMode == EFluidHeightAttenuationMode::CurveAttenuation"))
	TObjectPtr<UCurveFloat> HeightAttenuationCurve = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Height", meta = (ClampMin = "32", ClampMax = "1024", EditCondition = "HeightAttenuationMode == EFluidHeightAttenuationMode::CurveAttenuation"))
	int32 HeightCurveLUTResolution = 256;
 

	UPROPERTY()
	bool bHeightCurveDirty = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Height")
	EFluidHeightAttenuationMode HeightAttenuationMode = EFluidHeightAttenuationMode::CurveAttenuation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Height")
	float HeightFadeStartRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Height")
	float HeightFadeStrength = 1.0f;
	
private:
	/** Helper Function */
	bool ResolveSimulationBounds(FVector& OutOrigin, FVector& OutExtent) const;
	
	/** Curve Data를 Array<float> 데이터로 변환 */
	TArray<float> BuildHeightCurveSamples() const;
	/** GPU에 변환된 Curve Data Load */
	void PushHeightCurveSamplesToFogExtension();
	/** Release Height Curve Data */
	void ReleaseHeightCurveFromFogExtension();
	
	/** Curve Data dirty flag */
	bool bWasUsingCurveAttenuation = false;
	TWeakObjectPtr<UCurveFloat> LastHeightCurveAsset;
	int32 LastHeightCurveLUTResolution = 0;
	
	/** Resources */
	TSharedPtr<FFluidResources, ESPMode::ThreadSafe> FluidResources;
	
	TSharedPtr<FFogSceneViewExtension, ESPMode::ThreadSafe> FogExtension;
	float AccumulatedTime = 0.f;
	
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
	 	float InVisc,
	 	int32 InPressureIterations,
	 	// 반환용
	 	int32& OutVelIndex, int32& OutDenIndex, int32& OutPresIndex
	 	);
	
};