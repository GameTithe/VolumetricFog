#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "NoiseGenerateComponent.generated.h"

UCLASS(ClassGroup =(VolumetricFog), meta = (BlueprintSpawnableComponent))
class VOLUMETRICFOG_API UNoiseGenerateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNoiseGenerateComponent();

	 virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void BeginPlay() override;

	// 에디터에서 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	UTextureRenderTarget2D* OutputRT;
	
private:
	float AccumulatedTime = 0.f;

	void ExectureNoiseShaderByRenderThread(FRHICommandListImmediate& RHICmdList,
		FTextureRenderTargetResource* RTResource, float Time);
};