/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_REPORTER_H_
#define _RED_MEMORY_REPORTER_H_

namespace red
{
namespace memory
{
	class PoolRegistry;
	class MetricsRegistry;
	class SystemAllocator;

	struct ReporterParameter
	{
		const PoolRegistry * poolRegistry;
		const MetricsRegistry * metricsRegistry;
		const SystemAllocator * systemAllocator;
	};

	class Reporter
	{
	public:
		Reporter();
		~Reporter();

		void Initialize( const ReporterParameter & param );

		void WriteReportToLog() const;

	private:

		ReporterParameter m_parameter;
	};
}
}

#endif
