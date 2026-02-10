#include "FluidShaders.h"

IMPLEMENT_GLOBAL_SHADER(FFluidAdvectVelocityCS, "/VolumetricFog/FluidAdvectVelocity.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FFluidAdvectCS, "/VolumetricFog/FluidAdvect.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FFluidDiffuseCS, "/VolumetricFog/FluidDiffuse.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FFluidForceCS, "/VolumetricFog/FluidForce.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FFluidDivergenceCS, "/VolumetricFog/FluidDivergence.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FFluidGradientSubtractCS, "/VolumetricFog/FluidGradientSubtract.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FFluidVorticityCS, "/VolumetricFog/FluidVorticity.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FFluidVorticityForceCS, "/VolumetricFog/FluidVorticityForce.usf", "MainCS", SF_Compute);
