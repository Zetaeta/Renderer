#pragma once
#include <chrono>

namespace ch = std::chrono;
class Timer
{
public:
	void Reset()
	{
		m_StartTime = std::chrono::system_clock::now();
	}

	float ElapsedMillis()
	{
		std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
		return float(std::chrono::duration_cast<ch::milliseconds>(endTime - m_StartTime).count());
	}

	std::chrono::time_point<std::chrono::system_clock> m_StartTime;
};

