#include "build.h"
#include "cpuRecognizer.h"
#include <processthreadsapi.h>
#include <winnt.h>

namespace HardwareInstrumentation
{
	Int32 TranslateFromEnumToWinInt( ECpuFeature feature )
	{
		switch( feature )
		{
		case ECpuFeature::SSE:
			return PF_XMMI_INSTRUCTIONS_AVAILABLE;
			break;
		case ECpuFeature::SSE2:
			return PF_XMMI64_INSTRUCTIONS_AVAILABLE;
			break;
		case ECpuFeature::SSE3:
			return PF_SSE3_INSTRUCTIONS_AVAILABLE;
			break;
		default:
			break;
		}

		return 0;
	}

	CCpuRecognizer::CCpuRecognizer()
	{

	}

	Bool CCpuRecognizer::IsFeatureSupported(ECpuFeature feature) const
	{
		return ::IsProcessorFeaturePresent( TranslateFromEnumToWinInt( feature ) ) != 0;
	}

}
