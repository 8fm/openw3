#pragma once

namespace HardwareInstrumentation
{
	enum class ECpuFeature
	{
		SSE,
		SSE2,
		SSE3,
	};

	class CCpuRecognizer
	{
	public:
		CCpuRecognizer();
		Bool IsFeatureSupported( ECpuFeature feature ) const;

	};
}
