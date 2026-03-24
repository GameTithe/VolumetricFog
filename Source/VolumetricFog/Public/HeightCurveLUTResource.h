#pragma once

#include "RenderResource.h"

class FHeightCurveLUTResource : public FTextureWithSRV
{
public:
	explicit FHeightCurveLUTResource(uint32 InSizeX) : SizeX (InSizeX) { }
	
	uint32 GetSizeX() const { return SizeX; }
	FTextureRHIRef GetRHI() const { return TextureRHI; }

	virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
	void Update(FRHICommandListImmediate& RHICmdList, TConstArrayView<float> Samples);
	 
private:
	uint32 SizeX = 0;
};
