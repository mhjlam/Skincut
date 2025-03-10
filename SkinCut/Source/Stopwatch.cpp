#include "Stopwatch.hpp"

#include <iomanip>
#include <functional>

#include "Utility.hpp"


using namespace SkinCut;



Stopwatch::Stopwatch(CLOCKTYPE ct) : mClockType(ct)
{
	QueryPerformanceFrequency(&mFrequency);
	mSplits = new std::map<std::string, Split>;
}

Stopwatch::Stopwatch(std::string id, CLOCKTYPE ct) : mClockType(ct)
{
	QueryPerformanceFrequency(&mFrequency);
	mSplits = new std::map<std::string, Split>;

	Start(id);
}

Stopwatch::~Stopwatch()
{
	delete mSplits;
}


void Stopwatch::Start(std::string id)
{
	// disallow empty ids
	if (id.empty()) { return; }

	// see if record exists already
	auto record = mSplits->find(id);
	if (record != mSplits->end()) { return; }

	// add new record
	long long time = GetTime();
	mSplits->insert(make_pair(id, Split(time)));
}


void Stopwatch::Stop(std::string id)
{
	// attempt to find record
	auto record = mSplits->find(id);
	if (record == mSplits->end()) { return; }

	// determine clock frequency
	long long freq = 1L;
	if (mClockType == CLOCK_QPC_MS) {
		freq = mFrequency.QuadPart / 1000L;
	}
	else if (mClockType == CLOCK_QPC_US) {
		freq = mFrequency.QuadPart / 1000000L;
	}

	// determine elapsed time
	long long time = GetTime();
	long long diff = time - record->second.start;
	record->second.elapsed = diff / freq;
}


void Stopwatch::Reset()
{
	delete mSplits;
	mSplits = new std::map<std::string, Split>;
}

void Stopwatch::Reset(std::string id, bool start)
{
	// attempt to find record
	auto record = mSplits->find(id);
	if (record == mSplits->end()) { return; }

	// reset clock
	record->second.start = (start) ? GetTime() : 0L;
	record->second.elapsed = 0L;
}


long long Stopwatch::ElapsedTime(std::string id)
{
	// attempt to find record
	auto record = mSplits->find(id);
	if (record == mSplits->end()) { return 0L; }

	// return elapsed time
	return record->second.elapsed;
}


void Stopwatch::Report(bool terse, bool totalonly)
{
	long long total = 0;

	for (auto it = mSplits->begin(); it != mSplits->end(); ++it) {
		auto record = mSplits->find(it->first);
		total += record->second.elapsed;

		if (!totalonly) { Report(it->first, terse); }
	}

	std::stringstream ss;

	if (terse) {
		ss << total;
		SkinCut::Utility::ConsoleMessage(ss.str());
		SkinCut::Utility::ConsoleMessage();
		return;
	}

	std::string unit = (mClockType == CLOCK_QPC_US || mClockType == CLOCK_CHRONO_US) ? " us" : " ms";
	ss << "Total: " << total << unit;

	SkinCut::Utility::ConsoleMessage(ss.str());
	SkinCut::Utility::ConsoleMessage();
}


void Stopwatch::Report(std::string id, bool terse)
{
	auto record = mSplits->find(id);
	if (record == mSplits->end()) { return; }

	if (terse) {
		std::stringstream ss;
		ss << record->second.elapsed;
		SkinCut::Utility::ConsoleMessage(ss.str());
		return;
	}

	std::string unit = (mClockType == CLOCK_QPC_US || mClockType == CLOCK_CHRONO_US) ? " us" : " ms";

	std::stringstream ss;
	ss << id << ": " << record->second.elapsed << unit;
	SkinCut::Utility::ConsoleMessage(ss.str());
}


long long Stopwatch::GetTime()
{
	switch (mClockType)
	{
		case CLOCK_CHRONO_MS: {
			auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
			return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
		}

		case CLOCK_CHRONO_US: {
			auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
			return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
		}

		case CLOCK_QPC_MS:
		case CLOCK_QPC_US:
		default: {
			LARGE_INTEGER qpc;
			QueryPerformanceCounter(&qpc);
			return qpc.QuadPart;
		}
	}
}

