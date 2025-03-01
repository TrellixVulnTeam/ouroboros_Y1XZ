// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com

#ifndef OURO_NETWORKERROR_REPORTER_H
#define OURO_NETWORKERROR_REPORTER_H

#include "common/common.h"
#include "common/timer.h"
#include "helper/debug_helper.h"
#include "network/common.h"
#include "network/network_exception.h"

namespace Ouroboros { 
namespace Network
{
class EventDispatcher;
class Address;

struct ErrorReportAndCount
{
	uint64 lastReportStamps;
	uint64 lastRaisedStamps;
	uint count;
};

typedef std::pair< Address, std::string > AddressAndErrorString;

typedef std::map< AddressAndErrorString, ErrorReportAndCount > ErrorsAndCounts;

class ErrorReporter : public TimerHandler
{
public:
	ErrorReporter(EventDispatcher & dispatcher);
	~ErrorReporter();

	void reportException(Reason reason, const Address & addr = Address::NONE,
		const char * prefix = NULL, const char* suffix = NULL);

	void reportPendingExceptions(bool reportBelowThreshold = false);

private:
	static const uint ERROR_REPORT_MIN_PERIOD_MS;
	static const uint ERROR_REPORT_COUNT_MAX_LIFETIME_MS;

	void reportException(const NetworkException & ne, 
		const char * prefix = NULL, const char* suffix = NULL);

	void reportError(const Address & address, const char* format, ...);
	void addReport(const Address & address, const std::string & error);

	static std::string addressErrorToString(
			const Address & address,
			const std::string & errorString);

	static std::string addressErrorToString(
			const Address & address,
			const std::string & errorString,
			const ErrorReportAndCount & reportAndCount,
			const uint64 & now);

	virtual void handleTimeout(TimerHandle handle, void * arg);

	TimerHandle reportLimitTimerHandle_;
	ErrorsAndCounts errorsAndCounts_;
};


}
}

#ifdef CODE_INLINE
#include "error_reporter.inl"
#endif
#endif // OURO_NETWORKERROR_REPORTER_H
