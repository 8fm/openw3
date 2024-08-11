/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_CPUID_H_
#define _RED_CPUID_H_

#include "types.h"
#include "utility.h"

namespace Red
{
	namespace System
	{
		enum ECPUCapabilities
		{
			CPU_MMX = 0,
			CPU_SSE1,
			CPU_SSE2,
			CPU_SSE3,
			CPU_SSE4_1,
			CPU_SSE4_2,
			CPU_Hyperthreading,
			CPU_3DNow,
			CPU_3DNowExt
		};

		class CpuId
		{
		public:
			CpuId();

		public:
			static CpuId& GetInstance()
			{
				static CpuId instance;
				return instance;
			}

			RED_INLINE const AnsiChar* GetVendor() const				{ return m_vendor; }
			RED_INLINE Uint32 GetNumberOfLogicalCores() const			{ return m_logicalCores; }
			RED_INLINE Uint32 GetNumberOfPhysicalCores() const			{ return m_physicalCores; }
			RED_INLINE Bool GetCPUCapability( ECPUCapabilities flag )	{ return ( m_cpuCapabilityFlags & FLAG( flag ) )? true : false; }

		private:
			Uint32 m_physicalCores;
			Uint32 m_logicalCores;
			Uint32 m_cpuCapabilityFlags;

			AnsiChar m_vendor[ 16 ];
		};
	}
}

#endif // _RED_CPUID_H_
