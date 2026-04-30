// Fill out your copyright notice in the Description page of Project Settings.


#include "VolumetricFluidFog.h"
#include "Components/BoxComponent.h"
#include "FluidSimulationComponent.h"

// Sets default values
AVolumetricFluidFog::AVolumetricFluidFog()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BoundsComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundsComponent"));
	SetRootComponent(BoundsComponent);

	BoundsComponent->InitBoxExtent(FVector(3.0f, 3.0f, 3.0f));
	BoundsComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BoundsComponent->SetGenerateOverlapEvents(true);
	BoundsComponent->SetCollisionObjectType(ECC_WorldDynamic);
	BoundsComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	
	BoundsComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BoundsComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	BoundsComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	
	BoundsComponent->SetCanEverAffectNavigation(false);
	BoundsComponent->SetHiddenInGame(true);

	FluidSimulationComponent = CreateDefaultSubobject<UFluidSimulationComponent>(TEXT("FluidSimulationComponent"));
}

// Called when the game starts or when spawned
void AVolumetricFluidFog::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AVolumetricFluidFog::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

