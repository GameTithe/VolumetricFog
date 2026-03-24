#include  "HeightCurveLUTResource.h"

#include "Field/FieldSystemNoiseAlgo.h"


void FHeightCurveLUTResource::InitRHI(FRHICommandListBase& RHICmdList)
{
	const FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create2D(TEXT("FogHeightCurveLUT"), SizeX, 1, PF_R32_FLOAT);
		
	TextureRHI = RHICmdList.CreateTexture(Desc);
	SamplerStateRHI = GetOrCreateSamplerState(
		FSamplerStateInitializerRHI(SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp));
	
	ShaderResourceViewRHI = RHICmdList.CreateShaderResourceView(TextureRHI, FRHIViewDesc::CreateTextureSRV().SetDimensionFromTexture(TextureRHI));
}

void FHeightCurveLUTResource::Update(FRHICommandListImmediate& RHICmdList, TConstArrayView<float> Samples)
{
	check(Samples.Num() == static_cast<int32>(SizeX));
	
	const FUpdateTextureRegion2D Region(0, 0, 0, 0, SizeX, 1);
	RHICmdList.UpdateTexture2D(TextureRHI, 0, Region, SizeX * sizeof(float), reinterpret_cast<const uint8*>(Samples.GetData()));
}


