/*************************************************************************************************************
Copyright (c) 2015, Coveloz Consulting Ltda
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS LISTED "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS LISTED BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Attributions: The inih library portion of the source code is licensed from
Brush Technology and Ben Hoyt - Copyright (c) 2009, Brush Technology and Copyright (c) 2009, Ben Hoyt.
Complete license and copyright information can be found at
https://github.com/benhoyt/inih/commit/74d2ca064fb293bc60a77b0bd068075b293cf175.
*************************************************************************************************************/

/**
 * @file
 * MODULE SUMMARY : Reads the .ini file and parses it into information
 * to be used on daemon_cl
 */

#include <string>

#include "ini.h"
#include <limits.h>
#include <common_port.hpp>

const uint32_t LINKSPEED_10G =		10000000;
const uint32_t LINKSPEED_2_5G =		2500000;
const uint32_t LINKSPEED_1G =		1000000;
const uint32_t LINKSPEED_100MB =	100000;
const uint32_t INVALID_LINKSPEED =	UINT_MAX;

/**
 * @brief Returns name given numeric link speed
 * @return NULL if speed/name isn't found
 */
const char *findNameBySpeed( uint32_t speed );

/**
 * @brief Provides the gptp interface for
 * the iniParser external module
 */
class GptpIniParser
{
    public:

        /**
         * @brief Container with the information to get from the .ini file
         */
        typedef struct
        {
            /*ptp data set*/
            unsigned char priority1;

            /*port data set*/
            unsigned int announceReceiptTimeout;
            unsigned int syncReceiptTimeout;
            unsigned int syncReceiptThresh;		//!< Number of wrong sync messages that will trigger a switch to master
            int64_t neighborPropDelayThresh;
            unsigned int seqIdAsCapableThresh;
            uint16_t lostPdelayRespThresh;
            PortState port_state;
            bool allowNegativeCorrField;

            /*ethernet adapter data set*/
	    std::string ifname;
		phy_delay_map_t phy_delay;
        } gptp_cfg_t;

        /*public methods*/
        GptpIniParser(std::string ini_path);
        ~GptpIniParser();

        /**
         * @brief  Reads the parser Error value
         * @param  void
         * @return Parser Error
         */
        int parserError(void);

        /**
         * @brief  Reads priority1 config value
         * @param  void
         * @return priority1
         */
        unsigned char getPriority1(void)
        {
            return _config.priority1;
        }

        /**
         * @brief  Reads the announceReceiptTimeout configuration value
         * @param  void
         * @return announceRecepitTimeout value from .ini file
         */
        unsigned int getAnnounceReceiptTimeout(void)
        {
            return _config.announceReceiptTimeout;
        }

        /**
         * @brief  Reads the syncRecepitTimeout configuration value
         * @param  void
         * @return syncRecepitTimeout value from the .ini file
         */
        unsigned int getSyncReceiptTimeout(void)
        {
            return _config.syncReceiptTimeout;
        }

        /**
         * @brief  Reads the PHY DELAY values from the configuration file
         * @param  void
         * @return PHY delay map structure
         */
        const phy_delay_map_t getPhyDelay(void)
        {
            return _config.phy_delay;
        }

        /**
         * @brief  Reads the neighbohr propagation delay threshold from the configuration file
         * @param  void
         * @return neighborPropDelayThresh value from the .ini file
         */
        int64_t getNeighborPropDelayThresh(void)
        {
            return _config.neighborPropDelayThresh;
        }

        /**
         * @brief  Reads the sync receipt threshold from the configuration file
         * @return syncRecepitThresh value from the .ini file
         */
        unsigned int getSyncReceiptThresh(void)
        {
            return _config.syncReceiptThresh;
        }

        /**
         * @brief  Reads the allowNegativeCorrectionField flag from the configuration file
         * @return allowNegativeCorrectionField from the .ini file
         */
        bool getAllowNegativeCorrField(void)
        {
            return _config.allowNegativeCorrField;
        }

	/**
	 * @brief Dump PHY delays to screen
	 */
	void print_phy_delay( void );

    private:
        int _error;
        gptp_cfg_t _config;

        static int iniCallBack(void *user, const char *section, const char *name, const char *value);
        static bool parseMatch(const char *s1, const char *s2);
};

