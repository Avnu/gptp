#ifndef SYSTEMDWATCHDOGHANDLER_H
#define SYSTEMDWATCHDOGHANDLER_H
#include <linux_hal_common.hpp>
#include <avbts_ostimer.hpp>
#include <avbts_osthread.hpp>

OSThreadExitCode watchdogUpdateThreadFunction(void *arg);

class SystemdWatchdogHandler
{
public:
	int watchdog_setup(OSThreadFactory *thread_factory);
	void run_update();
	SystemdWatchdogHandler();
	virtual ~SystemdWatchdogHandler();
private:
	OSTimer *watchdog_timer;
	OSThread *watchdog_thread;
	long unsigned int update_interval;
	long unsigned int getSystemdWatchdogInterval(int *result);
	bool startSystemdWatchdogThread(OSThreadFactory *thread_factory);
};

#endif // SYSTEMDWATCHDOGHANDLER_H
