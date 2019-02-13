/******************************************************************************

  Copyright (c) 2012 Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  3. Neither the name of the Intel Corporation nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************/

#include "ieee1588.hpp"
#include "avbts_clock.hpp"
#include "avbts_osnet.hpp"
#include "avbts_oslock.hpp"
#include "avbts_persist.hpp"
#include "gptp_cfg.hpp"
#include "ether_port.hpp"

#ifdef ARCH_INTELCE
#include "linux_hal_intelce.hpp"
#else
#include "linux_hal_generic.hpp"
#endif

#include "linux_hal_persist_file.hpp"
#include <ctype.h>
#include <inttypes.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#ifdef SYSTEMD_WATCHDOG
#include <watchdog.hpp>
#endif

#define PHY_DELAY_GB_TX_I20 184 //1G delay
#define PHY_DELAY_GB_RX_I20 382 //1G delay
#define PHY_DELAY_MB_TX_I20 1044//100M delay
#define PHY_DELAY_MB_RX_I20 2133//100M delay

void gPTPPersistWriteCB(char *bufPtr, uint32_t bufSize);

void print_usage( char *arg0 ) {
	fprintf( stderr,
			"%s <network interface> [-S] [-P] [-M <filename>] "
			"[-G <group>] [-R <priority 1>] "
			"[-D <gb_tx_delay,gb_rx_delay,mb_tx_delay,mb_rx_delay>] "
			"[-T] [-L] [-E] [-GM] [-N] [-INITSYNC <value>] [-OPERSYNC <value>] "
			"[-INITPDELAY <value>] [-OPERPDELAY <value>] "
			"[-F <path to gptp_cfg.ini file>] "
			"\n",
			arg0 );
	fprintf
		( stderr,
		  "\t-S start syntonization\n"
		  "\t-P pulse per second\n"
		  "\t-M <filename> save/restore state\n"
		  "\t-G <group> group id for shared memory\n"
		  "\t-R <priority 1> priority 1 value\n"
		  "\t-D Phy Delay <gb_tx_delay,gb_rx_delay,mb_tx_delay,mb_rx_delay>\n"
		  "\t-T force master (ignored when Automotive Profile set)\n"
		  "\t-L force slave (ignored when Automotive Profile set)\n"
		  "\t-E enable test mode (as defined in AVnu automotive profile)\n"
		  "\t-V enable AVnu Automotive Profile\n"
		  "\t-N allow processing SyncFollowUp with negative correction field\n"
		  "\t-GM set grandmaster for Automotive Profile\n"
		  "\t-INITSYNC <value> initial sync interval (Log base 2. 0 = 1 second)\n"
		  "\t-OPERSYNC <value> operational sync interval (Log base 2. 0 = 1 second)\n"
		  "\t-INITPDELAY <value> initial pdelay interval (Log base 2. 0 = 1 second)\n"
		  "\t-OPERPDELAY <value> operational pdelay interval (Log base 2. 0 = 1 sec)\n"
		  "\t-F <path-to-ini-file>\n"
		);
}

int watchdog_setup(OSThreadFactory *thread_factory)
{
#ifdef SYSTEMD_WATCHDOG
	SystemdWatchdogHandler *watchdog = new SystemdWatchdogHandler();
	OSThread *watchdog_thread = thread_factory->createThread();
	int watchdog_result;
	long unsigned int watchdog_interval;
	watchdog_interval = watchdog->getSystemdWatchdogInterval(&watchdog_result);
	if (watchdog_result) {
		GPTP_LOG_INFO("Watchtog interval read from service file: %lu us", watchdog_interval);
		watchdog->update_interval = watchdog_interval / 2;
		GPTP_LOG_STATUS("Starting watchdog handler (Update every: %lu us)", watchdog->update_interval);
		watchdog_thread->start(watchdogUpdateThreadFunction, watchdog);
		return 0;
	} else if (watchdog_result < 0) {
		GPTP_LOG_ERROR("Watchdog settings read error.");
		return -1;
	} else {
		GPTP_LOG_STATUS("Watchdog disabled");
		return 0;
	}
#else
	return 0;
#endif
}

static IEEE1588Clock *pClock = NULL;
static EtherPort *pPort = NULL;

int main(int argc, char **argv)
{
	PortInit_t portInit;

	sigset_t set;
	InterfaceName *ifname;
	int sig;

	bool syntonize = false;
	int i;
	bool pps = false;
	uint8_t priority1 = 248;
	bool override_portstate = false;
	PortState port_state = PTP_SLAVE;
	char *restoredata = NULL;
	char *restoredataptr = NULL;
	off_t restoredatalength = 0;
	off_t restoredatacount = 0;
	bool restorefailed = false;
	LinuxIPCArg *ipc_arg = NULL;
	bool use_config_file = false;
	char config_file_path[512];
	memset(config_file_path, 0, 512);

	GPTPPersist *pGPTPPersist = NULL;
	LinuxThreadFactory *thread_factory = new LinuxThreadFactory();

	// Block SIGUSR1
	{
		sigset_t block;
		sigemptyset( &block );
		sigaddset( &block, SIGUSR1 );
		if( pthread_sigmask( SIG_BLOCK, &block, NULL ) != 0 ) {
			GPTP_LOG_ERROR("Failed to block SIGUSR1");
			return -1;
		}
	}

	GPTP_LOG_REGISTER();
	GPTP_LOG_INFO("gPTP starting");
	if (watchdog_setup(thread_factory) != 0) {
		GPTP_LOG_ERROR("Watchdog handler setup error");
		return -1;
	}
	phy_delay_map_t ether_phy_delay;
	bool input_delay=false;

	portInit.clock = NULL;
	portInit.index = 0;
	portInit.timestamper = NULL;
	portInit.net_label = NULL;
	portInit.automotive_profile = false;
	portInit.isGM = false;
	portInit.testMode = false;
	portInit.linkUp = false;
	portInit.allowNegativeCorrField = false;
	portInit.initialLogSyncInterval = LOG2_INTERVAL_INVALID;
	portInit.initialLogPdelayReqInterval = LOG2_INTERVAL_INVALID;
	portInit.operLogPdelayReqInterval = LOG2_INTERVAL_INVALID;
	portInit.operLogSyncInterval = LOG2_INTERVAL_INVALID;
	portInit.condition_factory = NULL;
	portInit.thread_factory = NULL;
	portInit.timer_factory = NULL;
	portInit.lock_factory = NULL;
	portInit.syncReceiptThreshold =
		CommonPort::DEFAULT_SYNC_RECEIPT_THRESH;
	portInit.neighborPropDelayThreshold =
		CommonPort::NEIGHBOR_PROP_DELAY_THRESH;

	LinuxNetworkInterfaceFactory *default_factory =
		new LinuxNetworkInterfaceFactory;
	OSNetworkInterfaceFactory::registerFactory
		(factory_name_t("default"), default_factory);
	LinuxTimerQueueFactory *timerq_factory = new LinuxTimerQueueFactory();
	LinuxLockFactory *lock_factory = new LinuxLockFactory();
	LinuxTimerFactory *timer_factory = new LinuxTimerFactory();
	LinuxConditionFactory *condition_factory = new LinuxConditionFactory();
	LinuxSharedMemoryIPC *ipc = new LinuxSharedMemoryIPC();
	/* Create Low level network interface object */
	if( argc < 2 ) {
		printf( "Interface name required\n" );
		print_usage( argv[0] );
		return -1;
	}
	ifname = new InterfaceName( argv[1], strlen(argv[1]) );

	/* Process optional arguments */
	for( i = 2; i < argc; ++i ) {

		if( argv[i][0] == '-' ) {
			if( strcmp(argv[i] + 1,  "S") == 0 ) {
				// Get syntonize directive from command line
				syntonize = true;
			}
			else if( strcmp(argv[i] + 1,  "T" ) == 0 ) {
				override_portstate = true;
				port_state = PTP_MASTER;
			}
			else if( strcmp(argv[i] + 1,  "L" ) == 0 ) {
				override_portstate = true;
				port_state = PTP_SLAVE;
			}
			else if( strcmp(argv[i] + 1,  "M" )  == 0 ) {
				// Open file
				if( i+1 < argc ) {
					pGPTPPersist = makeLinuxGPTPPersistFile();
					if (pGPTPPersist) {
						pGPTPPersist->initStorage(argv[i + 1]);
				  }
				}
				else {
					printf( "Restore file must be specified on "
							"command line\n" );
				}
			}
			else if( strcmp(argv[i] + 1,  "G") == 0 ) {
				if( i+1 < argc ) {
					ipc_arg = new LinuxIPCArg(argv[++i]);
				} else {
					printf( "Must specify group name on the command line\n" );
				}
			}
			else if( strcmp(argv[i] + 1,  "P") == 0 ) {
				pps = true;
			}
			else if( strcmp(argv[i] + 1,  "H") == 0 ) {
				print_usage( argv[0] );
				GPTP_LOG_UNREGISTER();
				return 0;
			}
			else if( strcmp(argv[i] + 1,  "R") == 0 ) {
				if( i+1 >= argc ) {
					printf( "Priority 1 value must be specified on "
							"command line, using default value\n" );
				} else {
					unsigned long tmp = strtoul( argv[i+1], NULL, 0 ); ++i;
					if( tmp == 0 ) {
						printf( "Invalid priority 1 value, using "
								"default value\n" );
					} else {
						priority1 = (uint8_t) tmp;
					}
				}
			}
			else if (strcmp(argv[i] + 1, "D") == 0) {
				int phy_delay[4];
				input_delay=true;
				int delay_count=0;
				char *cli_inp_delay = strtok(argv[i+1],",");
				while (cli_inp_delay != NULL)
				{
					if(delay_count>3)
					{
						printf("Too many values\n");
						print_usage( argv[0] );
						GPTP_LOG_UNREGISTER();
						return 0;
					}
					phy_delay[delay_count]=atoi(cli_inp_delay);
					delay_count++;
					cli_inp_delay = strtok(NULL,",");
				}
				if (delay_count != 4)
				{
					printf("All four delay values must be specified\n");
					print_usage( argv[0] );
					GPTP_LOG_UNREGISTER();
					return 0;
				}
				ether_phy_delay[LINKSPEED_1G].set_delay
					( phy_delay[0], phy_delay[1] );
				ether_phy_delay[LINKSPEED_100MB].set_delay
					( phy_delay[2], phy_delay[3] );
			}
			else if (strcmp(argv[i] + 1, "V") == 0) {
				portInit.automotive_profile = true;
			}
			else if (strcmp(argv[i] + 1, "GM") == 0) {
				portInit.isGM = true;
			}
			else if (strcmp(argv[i] + 1, "E") == 0) {
				portInit.testMode = true;
			}
			else if (strcmp(argv[i] + 1, "N") == 0) {
				portInit.allowNegativeCorrField = true;
			}
			else if (strcmp(argv[i] + 1, "INITSYNC") == 0) {
				portInit.initialLogSyncInterval = atoi(argv[++i]);
			}
			else if (strcmp(argv[i] + 1, "OPERSYNC") == 0) {
				portInit.operLogSyncInterval = atoi(argv[++i]);
			}
			else if (strcmp(argv[i] + 1, "INITPDELAY") == 0) {
				portInit.initialLogPdelayReqInterval = atoi(argv[++i]);
			}
			else if (strcmp(argv[i] + 1, "OPERPDELAY") == 0) {
				portInit.operLogPdelayReqInterval = atoi(argv[++i]);
			}
			else if (strcmp(argv[i] + 1, "F") == 0)
			{
				if( i+1 < argc ) {
					use_config_file = true;
					strcpy(config_file_path, argv[i+1]);
				} else {
					fprintf(stderr, "config file must be specified.\n");
				}
			}
		}
	}

	if (!input_delay)
	{
		ether_phy_delay[LINKSPEED_1G].set_delay
			( PHY_DELAY_GB_TX_I20, PHY_DELAY_GB_RX_I20 );
		ether_phy_delay[LINKSPEED_100MB].set_delay
			( PHY_DELAY_MB_TX_I20, PHY_DELAY_MB_RX_I20 );
	}
	portInit.phy_delay = &ether_phy_delay;

	if( !ipc->init( ipc_arg ) ) {
		delete ipc;
		ipc = NULL;
	}
	if( ipc_arg != NULL ) delete ipc_arg;

	if( pGPTPPersist ) {
		uint32_t bufSize = 0;
		if (!pGPTPPersist->readStorage(&restoredata, &bufSize))
			GPTP_LOG_ERROR("Failed to stat restore file");
		restoredatalength = bufSize;
		restoredatacount = restoredatalength;
		restoredataptr = (char *)restoredata;
	}

#ifdef ARCH_INTELCE
	EtherTimestamper *timestamper = new LinuxTimestamperIntelCE();
#else
	EtherTimestamper *timestamper = new LinuxTimestamperGeneric();
#endif

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset( &set, SIGTERM );
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGUSR2);
	if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
		perror("pthread_sigmask()");
		GPTP_LOG_UNREGISTER();
		return -1;
	}

	pClock = new IEEE1588Clock
		( false, syntonize, priority1, timerq_factory, ipc,
		  lock_factory );

	if( restoredataptr != NULL ) {
		if( !restorefailed )
			restorefailed =
				!pClock->restoreSerializedState( restoredataptr, &restoredatacount );
		restoredataptr = ((char *)restoredata) + (restoredatalength - restoredatacount);
	}

	// TODO: The setting of values into temporary variables should be changed to
	// just set directly into the portInit struct.
	portInit.clock = pClock;
	portInit.index = 1;
	portInit.timestamper = timestamper;
	portInit.net_label = ifname;
	portInit.condition_factory = condition_factory;
	portInit.thread_factory = thread_factory;
	portInit.timer_factory = timer_factory;
	portInit.lock_factory = lock_factory;

	if(use_config_file)
	{
		GptpIniParser iniParser(config_file_path);

		if (iniParser.parserError() < 0) {
			GPTP_LOG_ERROR("Cant parse ini file. Aborting file reading.");
		}
		else
		{
			GPTP_LOG_INFO("priority1 = %d", iniParser.getPriority1());
			GPTP_LOG_INFO("announceReceiptTimeout: %d", iniParser.getAnnounceReceiptTimeout());
			GPTP_LOG_INFO("syncReceiptTimeout: %d", iniParser.getSyncReceiptTimeout());
			iniParser.print_phy_delay();
			GPTP_LOG_INFO("neighborPropDelayThresh: %ld", iniParser.getNeighborPropDelayThresh());
			GPTP_LOG_INFO("syncReceiptThreshold: %d", iniParser.getSyncReceiptThresh());

			/* If using config file, set the neighborPropDelayThresh.
			 * Otherwise it will use its default value (800ns) */
			portInit.neighborPropDelayThreshold =
				iniParser.getNeighborPropDelayThresh();

			/* If using config file, set the syncReceiptThreshold, otherwise
			 * it will use the default value (SYNC_RECEIPT_THRESH)
			 */
			portInit.syncReceiptThreshold =
				iniParser.getSyncReceiptThresh();

			/*Only overwrites phy_delay default values if not input_delay switch enabled*/
			if(!input_delay)
			{
				ether_phy_delay = iniParser.getPhyDelay();
			}

			portInit.allowNegativeCorrField = iniParser.getAllowNegativeCorrField();
			GPTP_LOG_INFO("SyncFollowUp with negative correction field: %s",
						  portInit.allowNegativeCorrField ? "permitted" : "forbidden");
		}

	}

	pPort = new EtherPort(&portInit);

	if (!pPort->init_port()) {
		GPTP_LOG_ERROR("failed to initialize port");
		GPTP_LOG_UNREGISTER();
		return -1;
	}

	if( restoredataptr != NULL ) {
		if( !restorefailed ) {
			restorefailed = !pPort->restoreSerializedState( restoredataptr, &restoredatacount );
			GPTP_LOG_INFO("Persistent port data restored: asCapable:%d, port_state:%d, one_way_delay:%lld",
						  pPort->getAsCapable(), pPort->getPortState(), pPort->getLinkDelay());
		}
		restoredataptr = ((char *)restoredata) + (restoredatalength - restoredatacount);
	}

	if (portInit.automotive_profile) {
		if (portInit.isGM) {
			port_state = PTP_MASTER;
		}
		else {
			port_state = PTP_SLAVE;
		}
		override_portstate = true;
	}

	if( override_portstate ) {
		pPort->setPortState( port_state );
	}

	// Start PPS if requested
	if( pps ) {
		if( !timestamper->HWTimestamper_PPS_start()) {
			GPTP_LOG_ERROR("Failed to start pulse per second I/O");
		}
	}

	// Configure persistent write
	if (pGPTPPersist) {
		off_t len = 0;
		restoredatacount = 0;
		pClock->serializeState(NULL, &len);
		restoredatacount += len;
		pPort->serializeState(NULL, &len);
		restoredatacount += len;
		pGPTPPersist->setWriteSize((uint32_t)restoredatacount);
		pGPTPPersist->registerWriteCB(gPTPPersistWriteCB);
	}

	pPort->processEvent(POWERUP);

	do {
		sig = 0;

		if (sigwait(&set, &sig) != 0) {
			perror("sigwait()");
			GPTP_LOG_UNREGISTER();
			return -1;
		}

		if (sig == SIGHUP) {
			if (pGPTPPersist) {
			  // If port is either master or slave, save clock and then port state
			  if (pPort->getPortState() == PTP_MASTER || pPort->getPortState() == PTP_SLAVE) {
				pGPTPPersist->triggerWriteStorage();
			  }
			}
		}

		if (sig == SIGUSR2) {
			pPort->logIEEEPortCounters();
		}
	} while (sig == SIGHUP || sig == SIGUSR2);

	GPTP_LOG_ERROR("Exiting on %d", sig);

	if (pGPTPPersist) {
		pGPTPPersist->closeStorage();
	}

	// Stop PPS if previously started
	if( pps ) {
		if( !timestamper->HWTimestamper_PPS_stop()) {
			GPTP_LOG_ERROR("Failed to stop pulse per second I/O");
		}
	}

	if( ipc ) delete ipc;

	GPTP_LOG_UNREGISTER();
	return 0;
}

void gPTPPersistWriteCB(char *bufPtr, uint32_t bufSize)
{
	off_t restoredatalength = bufSize;
	off_t restoredatacount = restoredatalength;
	char *restoredataptr = NULL;

	GPTP_LOG_INFO("Signal received to write restore data");

	restoredataptr = (char *)bufPtr;
	pClock->serializeState(restoredataptr, &restoredatacount);
	restoredataptr = ((char *)bufPtr) + (restoredatalength - restoredatacount);
	pPort->serializeState(restoredataptr, &restoredatacount);
	restoredataptr = ((char *)bufPtr) + (restoredatalength - restoredatacount);
}
