#pragma once

#include <map>
#include <ctime>
#include <chrono>
#include <sstream>
#include <iostream>

#include <windows.h>


namespace SkinCut
{
	enum CLOCKTYPE
	{
		CLOCK_QPC_MS,
		CLOCK_QPC_US,
		CLOCK_CHRONO_MS,
		CLOCK_CHRONO_US
	};


	class Stopwatch
	{
	private:
		struct Split
		{
			long long start;
			long long elapsed;

			Split()
			{
				start = 0L;
				elapsed = 0L;
			}

			Split(long long start_time)
			{
				start = start_time;
				elapsed = 0L;
			}
		};

		CLOCKTYPE mClockType;
		LARGE_INTEGER mFrequency;
		std::map<std::string, Split>* mSplits;

	public:
		Stopwatch(CLOCKTYPE ct = CLOCK_QPC_US);
		Stopwatch(std::string id, CLOCKTYPE ct = CLOCK_QPC_US);
		~Stopwatch();

		void Start(std::string id);
		void Stop(std::string id);
		void Reset();
		void Reset(std::string id, bool start = true);

		long long ElapsedTime(std::string id);

		void Report(bool terse = false, bool totalonly = false);
		void Report(std::string id, bool terse = false);

	private:
		long long GetTime();
	};
}

