#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Texture2D.h"
#include "FogSceneViewExtension.h"
#include "FluidSimulationComponent.generated.h"

#ifndef MAX_FLUID_INTERACTION_FORCE_SOURCE
#define MAX_FLUID_INTERACTION_FORCE_SOURCE 8
#endif

class UCurveFloat;
class ADirectionalLight;
class ULightComponent;
class UPrimitiveComponent; 

class FRDGBuilder;

// 시뮬레이션에 필요한 RTs
struct FFluidResources
{
	FTextureRHIRef Velocity[2];
	FTextureRHIRef Density[2];
	FTextureRHIRef Pressure[2];
	FTextureRHIRef Divergence; 
	FTextureRHIRef Vorticity;
	FTextureRHIRef TempVelocity; 

	/** Cached Texture */
	TRefCountPtr<IPooledRenderTarget> VelocityPooledRT[2];
	TRefCountPtr<IPooledRenderTarget> DensityPooledRT[2];
	TRefCountPtr<IPooledRenderTarget> PressurePooledRT[2];
	TRefCountPtr<IPooledRenderTarget> DivergencePooledRT;
	TRefCountPtr<IPooledRenderTarget> VorticityPooledRT;
	TRefCountPtr<IPooledRenderTarget> TempVelocityPooledRT; 
    
	int32 Resolution = 0;
	bool bInitialize = false;
	bool bBaseDensityInitialized = false;

	int32 VelocityIndex = 0;
	int32 DensityIndex = 0;
	int32 PressureIndex = 0; 
	
	void Init(int32 Res, FRHICommandListImmediate& RHICmdList);
};

UENUM(BlueprintType)
enum class EFluidFogDebugMode: uint8
{
	Sim_2D UMETA(DisplayName = "2D Simulation"),
	Sim_3D UMETA(DisplayName = "3D Simulation"),
	DebugSelfShadow UMETA(DisplayName = "Debug Self Shadow")
};

UENUM(BlueprintType)
enum class EFluidHeightAttenuationMode : uint8
{
	LegacyExp UMETA(DisplayName = "Legacy Exponential"),
	AdaptiveNormalCurveAttenuationized UMETA(DisplayName = "Adaptive Normalized(Linear)"),
	CurveAttenuation UMETA(DisplayName = "Curve Attenuation"),
};

struct FFluidInteractionForceSource
{
	FVector4f PositionRadius = FVector4f(0.0f, 0.0f, 1.0f, 1.0f);
	FVector4f ForceDensity = FVector4f(0.0f, 0.0f, 0.0f, 1.0f);
};

struct FTrackedFluidInteractionActor
{
	TWeakObjectPtr<AActor> InteractionActor;
	TWeakObjectPtr<UPrimitiveComponent> Comp;
	FVector LastLocation = FVector::ZeroVector; 
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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")
	bool bEnableFog = true;
	 
	/** Simulation Params */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Simulation", meta = (ClampMin = "16", ClampMax = "2048"))
	int32 SimResolution = 1024;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Simulation", meta = (ClampMin = "0.9", ClampMax = "1.0"))
	float Dissipation = 0.993f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Density", meta =
	(ClampMin = "0.0"))
	float FogDensityMultiplier = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Rendering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AbsorptionScale = 0.1f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Rendering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScatteringScale = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Rendering")
	FLinearColor FogColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Rendering", meta =
	(ClampMin = "8", ClampMax = "128"))
	int32 NumSteps = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Rendering")
	float MaxRayDistance = 5000.f;
 
	/** Fog Rendering using Directional Light */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Rendering")
	bool bUseWorldDirectionalLight = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Rendering", meta = (EditCondition = "bUseWorldDirectionalLight"))
	TObjectPtr<ADirectionalLight> DirectionalLightActor = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Rendering", meta = (EditCondition = "bUseWorldDirectionalLight"))
	bool bUseDirectionalLightColor = false;
	
	// Shadowing
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Rendering|SelfShadow")
	FVector FogLightIntensity = FVector(0.4f, 0.2f, 1.0f);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Rendering|SelfShadow")
	float SelfShadowLightIntensity = 10.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Rendering|SelfShadow", meta = (ClampMin = "0.0"))
	float SelfShadowDensityScale = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Rendering|SelfShadow", meta = (ClampMin = "1", ClampMax = "64"))
	int32 SelfShadowStepCount = 15;
	
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Rendering|SelfShadow", meta = (ClampMin = "0.0", ClampMax = "2000.0"))
	float SelfShadowMaxDistance = 2000.0f;
	
	/** Vorticity has not been applied yet */
	float VorticityStrengthParam = 5.0f;
	int32 PressureIterations = 20;
	float Viscosity = 0.001f;
	
	/** Interaction Params */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Interaction")
	bool bEnableActorInteraction = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Interaction", meta = (ClampMin = "1.0"))
	float ActorInteractionRadiusMultiplier = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Interaction", meta = (ClampMin = "1.0"))
	float ActorInteractionForceMultiplier = 2.0f;
	
	/** Maintenance Params*/ 
	bool bEnableDensityMaintenance = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Maintenance")
	TObjectPtr<UTexture2D> BaseDensityNoiseTexture = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Maintenance", meta = (ClampMin = "0.0"))
	float BaseDensityTarget = 500.0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Maintenance", meta = (ClampMin = "0.0"))
	float BaseDensityRecoverySpeed = 0.4f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Maintenance", meta = (ClampMin = "0.0"))
	float BaseDensityDeadbandRatio = 0.8f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Maintenance", meta = (ClampMin = "0.0"))
	float BaseDensityNoiseRepeat = 1.0f;
	
	/** Fog Height Params*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Height")
	EFluidHeightAttenuationMode HeightAttenuationMode = EFluidHeightAttenuationMode::CurveAttenuation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Height", meta = (EditCondition = "HeightAttenuationMode == EFluidHeightAttenuationMode::CurveAttenuation"))
	TObjectPtr<UCurveFloat> HeightAttenuationCurve = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Height", meta = (ClampMin = "32", ClampMax = "1024", EditCondition = "HeightAttenuationMode == EFluidHeightAttenuationMode::CurveAttenuation"))
	int32 HeightCurveLUTResolution = 256;
 
	/** HeightFadeStartRatio 부터 감쇠적용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Height", meta = (EditCondition = "HeightAttenuationMode == EFluidHeightAttenuationMode::AdaptiveNormalCurveAttenuationized"))
	float HeightFadeStartRatio = 0.5f;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Height", meta = (EditCondition = "HeightAttenuationMode == EFluidHeightAttenuationMode::AdaptiveNormalCurveAttenuationized"))
	float HeightFadeStrength = 1.0f;
	
	/** Fog의 바닥부터 바로 감쇠 적용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Height", meta = (EditCondition = "HeightAttenuationMode == EFluidHeightAttenuationMode::LegacyExp"))
	float HeightFalloff = 200.f;
	
	UPROPERTY()
	bool bHeightCurveDirty = false;
	
	// Phase Function
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog|Phase", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float GOfHG = 0.1f;
	
private: 
	/**=================== Helper Function ===================*/
	
	/** Interaction function & Data */
	UFUNCTION()
	void HandleInteractionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	
	UFUNCTION()
	void HandleInteractionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	void AddInteractionActor(AActor* Actor, UPrimitiveComponent* Comp);
	void RemoveInteractionActor(AActor* Actor, UPrimitiveComponent* Comp); 
	bool WorldLocationToSimulationUV(const FVector& WorldLocation, const FVector& BoundsOrigin, const FVector& BoundsExtents, FVector2f& OutUV);
	
	TArray<FFluidInteractionForceSource> BuildInteractionForceSources(float DeltaTime);
	
	TWeakObjectPtr<UPrimitiveComponent> InteractionBoundsComponent;
	TArray<FTrackedFluidInteractionActor>  ActiveInteractionActors;
	
	/**Directional Light 관련 함수들 */
	bool TryResolveDirectionalLight(class ADirectionalLight*& OutLightActor) const;
	bool TryGetDirectionalLightSampleToLight(FVector& OutSampleToLight) const;
	
	/** Fog에 대한 인자들을 FFluidFogRenderState 로 묶기 */
	bool ResolveSimulationBounds(FVector& OutOrigin, FVector& OutExtent) const;
	FFluidFogRenderState BuildFogRenderStateSnapShot() const;
	
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
	
	static void ExecuteSimulationRDG(
	FRHICommandListImmediate& RHICmdList,
	TSharedPtr<FFluidResources, ESPMode::ThreadSafe> FluidResources,
	float DeltaTime,
	 int32 InVelIndex, int32 InDenIndex, int32 InPresIndex,
	 // 시뮬레이션 파라미터들
	 float InDissipation,  
	 float InVorticityStrength,
	 float InVisc,
	 int32 InPressureIterations,
	 // Fog Generation Use Noise
	 bool bInEnableDensityMaintenance,
	FTextureRHIRef InBaseDensityNoiseTexture,
	float InBaseDensityTarget,
	float InBaseDensityRecoverySpeed,
	float InBaseDensityDeadbandRatio,
	float InBaseDensityNoiseRepeat,
	// Interaction Force
	const TArray<FFluidInteractionForceSource>& InInteractionForceSources,
	 // 반환용
	 int32& OutVelIndex, int32& OutDenIndex, int32& OutPresIndex
	);
	
	static void AddSimulationPasses(
	FRDGBuilder& GraphBuilder,
	TSharedPtr<FFluidResources, ESPMode::ThreadSafe> FluidResources,
	float DeltaTime,
	 int32 InVelIndex, int32 InDenIndex, int32 InPresIndex,
	 // 시뮬레이션 파라미터들
	 float InDissipation,  
	 float InVorticityStrength,
	 float InVisc,
	 int32 InPressureIterations,
	 // Fog Generation Use Noise
	 bool bInEnableDensityMaintenance,
	FTextureRHIRef InBaseDensityNoiseTexture,
	float InBaseDensityTarget,
	float InBaseDensityRecoverySpeed,
	float InBaseDensityDeadbandRatio,
	float InBaseDensityNoiseRepeat,
	// Interaction Force
	const TArray<FFluidInteractionForceSource>& InInteractionForceSources,
	 // 반환용
	 int32& OutVelIndex, int32& OutDenIndex, int32& OutPresIndex
	 );
	
};
