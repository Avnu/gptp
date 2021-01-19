#include "watchdog.hpp"
#include "gptp_log.hpp"
#include <systemd/sd-daemon.h>


OSThreadExitCode watchdogUpdateThreadFunction(void *arg)
{
	SystemdWatchdogHandler *watchdog = (SystemdWatchdogHandler*) arg;
	watchdog->run_update();
	return osthread_ok;
}

bool SystemdWatchdogHandler::startSystemdWatchdogThread(OSThreadFactory *thread_factory)
{
	watchdog_thread = thread_factory->createThread();
	return watchdog_thread->start(watchdogUpdateThreadFunction, this);
}

SystemdWatchdogHandler::SystemdWatchdogHandler()
{
	GPTP_LOG_INFO("Creating Systemd watchdog handler.");
	LinuxTimerFactory timer_factory = LinuxTimerFactory();
	watchdog_timer = timer_factory.createTimer();
}

SystemdWatchdogHandler::~SystemdWatchdogHandler()
{
	delete watchdog_timer;
	delete watchdog_thread;
}

long unsigned int
SystemdWatchdogHandler::getSystemdWatchdogInterval(int *result)
{
	long unsigned int watchdog_interval; //in microseconds
	*result = sd_watchdog_enabled(0, &watchdog_interval);
	return watchdog_interval;
}

void SystemdWatchdogHandler::run_update()
{
	while(1)
	{
		GPTP_LOG_DEBUG("NOTIFYING WATCHDOG.");
		sd_notify(0, "WATCHDOG=1");
		GPTP_LOG_DEBUG("GOING TO SLEEP %lld", update_interval);
		watchdog_timer->sleep(update_interval);
		GPTP_LOG_DEBUG("WATCHDOG WAKE UP");
	}
}

int SystemdWatchdogHandler::watchdog_setup(OSThreadFactory *thread_factory)
{
	int watchdog_result;
	long unsigned int watchdog_interval;

	if (!thread_factory) {
		GPTP_LOG_ERROR("Watchog setup invalid argument (thread_factory)");
		return -1;
	}

	watchdog_interval = getSystemdWatchdogInterval(&watchdog_result);
	if (watchdog_result) {
		GPTP_LOG_INFO("Watchdog interval read from service file: %lu us", watchdog_interval);
		update_interval = watchdog_interval / 2;
		GPTP_LOG_STATUS("Starting watchdog handler (Update every: %lu us)", update_interval);
		if (startSystemdWatchdogThread(thread_factory)) {
			return 0;
		} else {
			GPTP_LOG_ERROR("Starting watchdog thread failed");
			return -1;
		}
	} else if (watchdog_result < 0) {
		GPTP_LOG_ERROR("Watchdog settings read error.");
		return -1;
	} else {
		GPTP_LOG_STATUS("Watchdog disabled");
		return 0;
	}
}
