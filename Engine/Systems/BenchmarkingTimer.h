#pragma once
#include <chrono>
#include "..\Editor\Editor.h"
namespace Debug
{
	class ScopedTimer
	{
	public:
		ScopedTimer()
		{
			m_startTimepoint = std::chrono::high_resolution_clock::now();
		}

		~ScopedTimer()
		{
			Stop();
		}

		void Stop()
		{
			auto endTimepoint = std::chrono::high_resolution_clock::now();

			auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTimepoint).time_since_epoch().count();
			auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

			auto duration = end - start;

			double ms = duration * 0.001;
			m_timeInMilis = ms;
		}

		double GetTimeInMilis() { return m_timeInMilis; }
		double GetTimeInSeconds() { return m_timeInMilis / 1000.0; }

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_startTimepoint;
		double m_timeInMilis = 0.0;
	};
}
