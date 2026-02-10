#include "NoiseComputeShader.h"

// shader 등록 매크로
// 인자: C++ 클래스, .usf경로 (가상 경로), 엔트리 포인트, shader type
IMPLEMENT_GLOBAL_SHADER(
	FGenerateNoiseCS,
	"/VolumetricFog/GenerateNoise.usf", // StartupModule에서 등록한 가상 경로 
	"MainCS",
	SF_Compute);
