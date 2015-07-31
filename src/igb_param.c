/*******************************************************************************

  Intel(R) Gigabit Ethernet Linux driver
  Copyright(c) 2007 Intel Corporation.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

*******************************************************************************/


#include <linux/netdevice.h>

#include "igb.h"

/* This is the only thing that needs to be changed to adjust the
 * maximum number of ports that the driver can manage.
 */

#define IGB_MAX_NIC 32

#define OPTION_UNSET   -1
#define OPTION_DISABLED 0
#define OPTION_ENABLED  1

/* All parameters are treated the same, as an integer array of values.
 * This macro just reduces the need to repeat the same declaration code
 * over and over (plus this helps to avoid typo bugs).
 */

#define IGB_PARAM_INIT { [0 ... IGB_MAX_NIC] = OPTION_UNSET }
#ifndef module_param_array
/* Module Parameters are always initialized to -1, so that the driver
 * can tell the difference between no user specified value or the
 * user asking for the default value.
 * The true default values are loaded in when igb_check_options is called.
 *
 * This is a GCC extension to ANSI C.
 * See the item "Labeled Elements in Initializers" in the section
 * "Extensions to the C Language Family" of the GCC documentation.
 */

#define IGB_PARAM(X, desc) \
	static const int __devinitdata X[IGB_MAX_NIC+1] = IGB_PARAM_INIT; \
	MODULE_PARM(X, "1-" __MODULE_STRING(IGB_MAX_NIC) "i"); \
	MODULE_PARM_DESC(X, desc);
#else
#define IGB_PARAM(X, desc) \
	static int __devinitdata X[IGB_MAX_NIC+1] = IGB_PARAM_INIT; \
	static int num_##X = 0; \
	module_param_array_named(X, X, int, &num_##X, 0); \
	MODULE_PARM_DESC(X, desc);
#endif

/* Transmit Descriptor Count
 *
 * Valid Range: 80-4096
 *
 * Default Value: 256
 */
IGB_PARAM(TxDescriptors, "Number of transmit descriptors");

/* Receive Descriptor Count
 *
 * Valid Range: 80-4096
 *
 * Default Value: 256
 */
IGB_PARAM(RxDescriptors, "Number of receive descriptors");

/* User Specified Speed Override
 *
 * Valid Range: 0, 10, 100, 1000
 *  - 0    - auto-negotiate at all supported speeds
 *  - 10   - only link at 10 Mbps
 *  - 100  - only link at 100 Mbps
 *  - 1000 - only link at 1000 Mbps
 *
 * Default Value: 0
 */
IGB_PARAM(Speed, "Speed setting");

/* User Specified Duplex Override
 *
 * Valid Range: 0-2
 *  - 0 - auto-negotiate for duplex
 *  - 1 - only link at half duplex
 *  - 2 - only link at full duplex
 *
 * Default Value: 0
 */
IGB_PARAM(Duplex, "Duplex setting");

/* Auto-negotiation Advertisement Override
 *
 * Valid Range: 0x01-0x0F, 0x20-0x2F (copper); 0x20 (fiber)
 *
 * The AutoNeg value is a bit mask describing which speed and duplex
 * combinations should be advertised during auto-negotiation.
 * The supported speed and duplex modes are listed below
 *
 * Bit           7     6     5      4      3     2     1      0
 * Speed (Mbps)  N/A   N/A   1000   N/A    100   100   10     10
 * Duplex                    Full          Full  Half  Full   Half
 *
 * Default Value: 0x2F (copper); 0x20 (fiber)
 */
IGB_PARAM(AutoNeg, "Advertised auto-negotiation setting");
#define AUTONEG_ADV_DEFAULT  0x2F
#define AUTONEG_ADV_MASK     0x2F

/* User Specified Flow Control Override
 *
 * Valid Range: 0-3
 *  - 0 - No Flow Control
 *  - 1 - Rx only, respond to PAUSE frames but do not generate them
 *  - 2 - Tx only, generate PAUSE frames but ignore them on receive
 *  - 3 - Full Flow Control Support
 *
 * Default Value: Read flow control settings from the EEPROM
 */
IGB_PARAM(FlowControl, "Flow Control setting");
#define FLOW_CONTROL_DEFAULT FLOW_CONTROL_FULL

/* XsumRX - Receive Checksum Offload Enable/Disable
 *
 * Valid Range: 0, 1
 *  - 0 - disables all checksum offload
 *  - 1 - enables receive IP/TCP/UDP checksum offload
 *
 * Default Value: 1
 */
IGB_PARAM(XsumRX, "Disable or enable Receive Checksum offload");

/* Interrupt Throttle Rate (interrupts/sec)
 *
 * Valid Range: 100-100000 (0=off, 1=dynamic, 3=dynamic conservative)
 */
IGB_PARAM(InterruptThrottleRate, "Interrupt Throttling Rate");
#define DEFAULT_ITR                    3
#define MAX_ITR                   100000
#define MIN_ITR                      100

/* LLIPort (Low Latency Interrupt TCP Port)
 *
 * Valid Range: 0 - 65535
 *
 * Default Value: 0 (disabled)
 */
IGB_PARAM(LLIPort, "Low Latency Interrupt TCP Port");

#define DEFAULT_LLIPORT                0
#define MAX_LLIPORT               0xFFFF
#define MIN_LLIPORT                    0

/* LLIPush (Low Latency Interrupt on TCP Push flag)
 *
 * Valid Range: 0,1
 *
 * Default Value: 0 (disabled)
 */
IGB_PARAM(LLIPush, "Low Latency Interrupt on TCP Push flag");

#define DEFAULT_LLIPUSH                0
#define MAX_LLIPUSH                    1
#define MIN_LLIPUSH                    0

/* LLISize (Low Latency Interrupt on Packet Size)
 *
 * Valid Range: 0 - 1500
 *
 * Default Value: 0 (disabled)
 */
IGB_PARAM(LLISize, "Low Latency Interrupt on Packet Size");

#define DEFAULT_LLISIZE                0
#define MAX_LLISIZE                 1500
#define MIN_LLISIZE                    0

struct igb_option {
	enum { enable_option, range_option, list_option } type;
	char *name;
	char *err;
	int  def;
	union {
		struct { /* range_option info */
			int min;
			int max;
		} r;
		struct { /* list_option info */
			int nr;
			struct igb_opt_list { int i; char *str; } *p;
		} l;
	} arg;
};

static int __devinit igb_validate_option(int *value, struct igb_option *opt,
                                         struct igb_adapter *adapter)
{
	if (*value == OPTION_UNSET) {
		*value = opt->def;
		return 0;
	}

	switch (opt->type) {
	case enable_option:
		switch (*value) {
		case OPTION_ENABLED:
			DPRINTK(PROBE, INFO, "%s Enabled\n", opt->name);
			return 0;
		case OPTION_DISABLED:
			DPRINTK(PROBE, INFO, "%s Disabled\n", opt->name);
			return 0;
		}
		break;
	case range_option:
		if (*value >= opt->arg.r.min && *value <= opt->arg.r.max) {
			DPRINTK(PROBE, INFO,
					"%s set to %i\n", opt->name, *value);
			return 0;
		}
		break;
	case list_option: {
		int i;
		struct igb_opt_list *ent;

		for (i = 0; i < opt->arg.l.nr; i++) {
			ent = &opt->arg.l.p[i];
			if (*value == ent->i) {
				if (ent->str[0] != '\0')
					DPRINTK(PROBE, INFO, "%s\n", ent->str);
				return 0;
			}
		}
	}
		break;
	default:
		BUG();
	}

	DPRINTK(PROBE, INFO, "Invalid %s value specified (%i) %s\n",
	       opt->name, *value, opt->err);
	*value = opt->def;
	return -1;
}

static void igb_check_fiber_options(struct igb_adapter *adapter);
static void igb_check_copper_options(struct igb_adapter *adapter);

/**
 * igb_check_options - Range Checking for Command Line Parameters
 * @adapter: board private structure
 *
 * This routine checks all command line parameters for valid user
 * input.  If an invalid value is given, or if no user specified
 * value exists, a default value is used.  The final value is stored
 * in a variable in the adapter structure.
 **/

void __devinit igb_check_options(struct igb_adapter *adapter)
{
	struct e1000_hw *hw = &adapter->hw;
	int bd = adapter->bd_number;
	if (bd >= IGB_MAX_NIC) {
		DPRINTK(PROBE, NOTICE,
		       "Warning: no configuration for board #%i\n", bd);
		DPRINTK(PROBE, NOTICE, "Using defaults for all values\n");
#ifndef module_param_array
		bd = IGB_MAX_NIC;
#endif
	}

	{ /* Transmit Descriptor Count */
		struct igb_option opt = {
			.type = range_option,
			.name = "Transmit Descriptors",
			.err  = "using default of "
				__MODULE_STRING(IGB_DEFAULT_TXD),
			.def  = IGB_DEFAULT_TXD,
			.arg  = { .r = { .min = IGB_MIN_TXD }}
		};
		struct igb_tx_ring *tx_ring = adapter->tx_ring;
		int i;
		opt.arg.r.max = IGB_MAX_TXD;

#ifdef module_param_array
		if (num_TxDescriptors > bd) {
#endif
			tx_ring->count = TxDescriptors[bd];
			igb_validate_option(&tx_ring->count, &opt, adapter);
			tx_ring->count = ALIGN(tx_ring->count,
			                       REQ_TX_DESCRIPTOR_MULTIPLE);
#ifdef module_param_array
		} else {
			tx_ring->count = opt.def;
		}
#endif
		for (i = 0; i < adapter->num_tx_queues; i++)
			tx_ring[i].count = tx_ring->count;
	}
	{ /* Receive Descriptor Count */
		struct igb_option opt = {
			.type = range_option,
			.name = "Receive Descriptors",
			.err  = "using default of "
				__MODULE_STRING(IGB_DEFAULT_RXD),
			.def  = IGB_DEFAULT_RXD,
			.arg  = { .r = { .min = IGB_MIN_RXD }}
		};
		struct igb_rx_ring *rx_ring = adapter->rx_ring;
		int i;
		opt.arg.r.max = IGB_MAX_RXD;

#ifdef module_param_array
		if (num_RxDescriptors > bd) {
#endif
			rx_ring->count = RxDescriptors[bd];
			igb_validate_option(&rx_ring->count, &opt, adapter);
			rx_ring->count = ALIGN(rx_ring->count,
			                       REQ_RX_DESCRIPTOR_MULTIPLE);
#ifdef module_param_array
		} else {
			rx_ring->count = opt.def;
		}
#endif
		for (i = 0; i < adapter->num_rx_queues; i++)
			rx_ring[i].count = rx_ring->count;
	}
	{ /* Checksum Offload Enable/Disable */
		struct igb_option opt = {
			.type = enable_option,
			.name = "Checksum Offload",
			.err  = "defaulting to Enabled",
			.def  = OPTION_ENABLED
		};

#ifdef module_param_array
		if (num_XsumRX > bd) {
#endif
			int rx_csum = XsumRX[bd];
			igb_validate_option(&rx_csum, &opt, adapter);
			adapter->rx_csum = rx_csum;
#ifdef module_param_array
		} else {
			adapter->rx_csum = opt.def;
		}
#endif
	}
	{ /* Flow Control */

		struct igb_opt_list fc_list[] =
			{{ e1000_fc_none,    "Flow Control Disabled" },
			 { e1000_fc_rx_pause,"Flow Control Receive Only" },
			 { e1000_fc_tx_pause,"Flow Control Transmit Only" },
			 { e1000_fc_full,    "Flow Control Enabled" },
			 { e1000_fc_default, "Flow Control Hardware Default" }};

		struct igb_option opt = {
			.type = list_option,
			.name = "Flow Control",
			.err  = "reading default settings from EEPROM",
			.def  = e1000_fc_default,
			.arg  = { .l = { .nr = ARRAY_SIZE(fc_list),
					 .p = fc_list }}
		};

#ifdef module_param_array
		if (num_FlowControl > bd) {
#endif
			int fc = FlowControl[bd];
			igb_validate_option(&fc, &opt, adapter);
			hw->mac.original_fc = fc;
			hw->mac.fc = fc;
#ifdef module_param_array
		} else {
			hw->mac.original_fc = opt.def;
			hw->mac.fc = opt.def;
		}
#endif
	}
	{ /* Interrupt Throttling Rate */
		struct igb_option opt = {
			.type = range_option,
			.name = "Interrupt Throttling Rate (ints/sec)",
			.err  = "using default of " __MODULE_STRING(DEFAULT_ITR),
			.def  = DEFAULT_ITR,
			.arg  = { .r = { .min = MIN_ITR,
					 .max = MAX_ITR }}
		};

#ifdef module_param_array
		if (num_InterruptThrottleRate > bd) {
#endif
			adapter->itr = InterruptThrottleRate[bd];
			switch (adapter->itr) {
			case 0:
				DPRINTK(PROBE, INFO, "%s turned off\n",
				        opt.name);
				break;
			case 1:
				DPRINTK(PROBE, INFO, "%s set to dynamic mode\n",
					opt.name);
				adapter->itr_setting = adapter->itr;
				adapter->itr = IGB_START_ITR;
				break;
			case 3:
				DPRINTK(PROBE, INFO,
				        "%s set to dynamic conservative mode\n",
					opt.name);
				adapter->itr_setting = adapter->itr;
				adapter->itr = IGB_START_ITR;
				break;
			default:
				igb_validate_option(&adapter->itr, &opt,
				        adapter);
				/* save the setting, because the dynamic bits change itr */
				/* clear the lower two bits because they are
				 * used as control */
				adapter->itr_setting = adapter->itr & ~3;
				break;
			}
#ifdef module_param_array
		} else {
			adapter->itr_setting = opt.def;
			adapter->itr = 8000;
		}
#endif
	}
	{ /* Low Latency Interrupt TCP Port*/
		struct igb_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt TCP Port",
			.err  = "using default of " __MODULE_STRING(DEFAULT_LLIPORT),
			.def  = DEFAULT_LLIPORT,
			.arg  = { .r = { .min = MIN_LLIPORT,
					 .max = MAX_LLIPORT }}
		};

#ifdef module_param_array
		if (num_LLIPort > bd) {
#endif
			adapter->lli_port = LLIPort[bd];
			if (adapter->lli_port) {
				igb_validate_option(&adapter->lli_port, &opt,
				        adapter);
			} else {
				DPRINTK(PROBE, INFO, "%s turned off\n",
					opt.name);
			}
#ifdef module_param_array
		} else {
			adapter->lli_port = opt.def;
		}
#endif
	}
	{ /* Low Latency Interrupt on Packet Size */
		struct igb_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt on Packet Size",
			.err  = "using default of " __MODULE_STRING(DEFAULT_LLISIZE),
			.def  = DEFAULT_LLISIZE,
			.arg  = { .r = { .min = MIN_LLISIZE,
					 .max = MAX_LLISIZE }}
		};

#ifdef module_param_array
		if (num_LLISize > bd) {
#endif
			adapter->lli_size = LLISize[bd];
			if (adapter->lli_size) {
				igb_validate_option(&adapter->lli_size, &opt,
				        adapter);
			} else {
				DPRINTK(PROBE, INFO, "%s turned off\n",
					opt.name);
			}
#ifdef module_param_array
		} else {
			adapter->lli_size = opt.def;
		}
#endif
	}
	{ /*Low Latency Interrupt on TCP Push flag*/
		struct igb_option opt = {
			.type = enable_option,
			.name = "Low Latency Interrupt on TCP Push flag",
			.err  = "defaulting to Disabled",
			.def  = OPTION_DISABLED
		};

#ifdef module_param_array
		if (num_LLIPush > bd) {
#endif
			int lli_push = LLIPush[bd];
			igb_validate_option(&lli_push, &opt, adapter);
			adapter->flags.lli_push = lli_push;
#ifdef module_param_array
		} else {
			adapter->lli_port = opt.def;
		}
#endif
	}

	switch (hw->media_type) {
	case e1000_media_type_fiber:
	case e1000_media_type_internal_serdes:
		igb_check_fiber_options(adapter);
		break;
	case e1000_media_type_copper:
		igb_check_copper_options(adapter);
		break;
	default:
		BUG();
	}
}

/**
 * igb_check_fiber_options - Range Checking for Link Options, Fiber Version
 * @adapter: board private structure
 *
 * Handles speed and duplex options on fiber adapters
 **/

static void __devinit igb_check_fiber_options(struct igb_adapter *adapter)
{
	int bd = adapter->bd_number;
#ifndef module_param_array
	bd = bd > IGB_MAX_NIC ? IGB_MAX_NIC : bd;
	if ((Speed[bd] != OPTION_UNSET)) {
#else
	if (num_Speed > bd) {
#endif
		DPRINTK(PROBE, INFO, "Speed not valid for fiber adapters, "
		       "parameter ignored\n");
	}

#ifndef module_param_array
	if ((Duplex[bd] != OPTION_UNSET)) {
#else
	if (num_Duplex > bd) {
#endif
		DPRINTK(PROBE, INFO, "Duplex not valid for fiber adapters, "
		       "parameter ignored\n");
	}

#ifndef module_param_array
	if ((AutoNeg[bd] != OPTION_UNSET) && (AutoNeg[bd] != 0x20)) {
#else
	if ((num_AutoNeg > bd) && (AutoNeg[bd] != 0x20)) {
#endif
		DPRINTK(PROBE, INFO, "AutoNeg other than 1000/Full is "
				 "not valid for fiber adapters, "
				 "parameter ignored\n");
	}
}

/**
 * igb_check_copper_options - Range Checking for Link Options, Copper Version
 * @adapter: board private structure
 *
 * Handles speed and duplex options on copper adapters
 **/

static void __devinit igb_check_copper_options(struct igb_adapter *adapter)
{
	struct e1000_hw *hw = &adapter->hw;
	int speed, dplx, an;
	int bd = adapter->bd_number;
#ifndef module_param_array
	bd = bd > IGB_MAX_NIC ? IGB_MAX_NIC : bd;
#endif

	{ /* Speed */
		struct igb_opt_list speed_list[] = {{          0, "" },
						    {   SPEED_10, "" },
						    {  SPEED_100, "" },
						    { SPEED_1000, "" }};

		struct igb_option opt = {
			.type = list_option,
			.name = "Speed",
			.err  = "parameter ignored",
			.def  = 0,
			.arg  = { .l = { .nr = ARRAY_SIZE(speed_list),
					 .p = speed_list }}
		};

#ifdef module_param_array
		if (num_Speed > bd) {
#endif
			speed = Speed[bd];
			igb_validate_option(&speed, &opt, adapter);
#ifdef module_param_array
		} else {
			speed = opt.def;
		}
#endif
	}
	{ /* Duplex */
		struct igb_opt_list dplx_list[] = {{           0, "" },
						   { HALF_DUPLEX, "" },
						   { FULL_DUPLEX, "" }};

		struct igb_option opt = {
			.type = list_option,
			.name = "Duplex",
			.err  = "parameter ignored",
			.def  = 0,
			.arg  = { .l = { .nr = ARRAY_SIZE(dplx_list),
					 .p = dplx_list }}
		};

		if (e1000_check_reset_block(hw)) {
			DPRINTK(PROBE, INFO,
				"Link active due to SoL/IDER Session. "
			        "Speed/Duplex/AutoNeg parameter ignored.\n");
			return;
		}
#ifdef module_param_array
		if (num_Duplex > bd) {
#endif
			dplx = Duplex[bd];
			igb_validate_option(&dplx, &opt, adapter);
#ifdef module_param_array
		} else {
			dplx = opt.def;
		}
#endif
	}

#ifdef module_param_array
	if ((num_AutoNeg > bd) && (speed != 0 || dplx != 0)) {
#else
	if (AutoNeg[bd] != OPTION_UNSET && (speed != 0 || dplx != 0)) {
#endif
		DPRINTK(PROBE, INFO,
		       "AutoNeg specified along with Speed or Duplex, "
		       "parameter ignored\n");
		hw->phy.autoneg_advertised = AUTONEG_ADV_DEFAULT;
	} else { /* Autoneg */
		struct igb_opt_list an_list[] =
			#define AA "AutoNeg advertising "
			{{ 0x01, AA "10/HD" },
			 { 0x02, AA "10/FD" },
			 { 0x03, AA "10/FD, 10/HD" },
			 { 0x04, AA "100/HD" },
			 { 0x05, AA "100/HD, 10/HD" },
			 { 0x06, AA "100/HD, 10/FD" },
			 { 0x07, AA "100/HD, 10/FD, 10/HD" },
			 { 0x08, AA "100/FD" },
			 { 0x09, AA "100/FD, 10/HD" },
			 { 0x0a, AA "100/FD, 10/FD" },
			 { 0x0b, AA "100/FD, 10/FD, 10/HD" },
			 { 0x0c, AA "100/FD, 100/HD" },
			 { 0x0d, AA "100/FD, 100/HD, 10/HD" },
			 { 0x0e, AA "100/FD, 100/HD, 10/FD" },
			 { 0x0f, AA "100/FD, 100/HD, 10/FD, 10/HD" },
			 { 0x20, AA "1000/FD" },
			 { 0x21, AA "1000/FD, 10/HD" },
			 { 0x22, AA "1000/FD, 10/FD" },
			 { 0x23, AA "1000/FD, 10/FD, 10/HD" },
			 { 0x24, AA "1000/FD, 100/HD" },
			 { 0x25, AA "1000/FD, 100/HD, 10/HD" },
			 { 0x26, AA "1000/FD, 100/HD, 10/FD" },
			 { 0x27, AA "1000/FD, 100/HD, 10/FD, 10/HD" },
			 { 0x28, AA "1000/FD, 100/FD" },
			 { 0x29, AA "1000/FD, 100/FD, 10/HD" },
			 { 0x2a, AA "1000/FD, 100/FD, 10/FD" },
			 { 0x2b, AA "1000/FD, 100/FD, 10/FD, 10/HD" },
			 { 0x2c, AA "1000/FD, 100/FD, 100/HD" },
			 { 0x2d, AA "1000/FD, 100/FD, 100/HD, 10/HD" },
			 { 0x2e, AA "1000/FD, 100/FD, 100/HD, 10/FD" },
			 { 0x2f, AA "1000/FD, 100/FD, 100/HD, 10/FD, 10/HD" }};

		struct igb_option opt = {
			.type = list_option,
			.name = "AutoNeg",
			.err  = "parameter ignored",
			.def  = AUTONEG_ADV_DEFAULT,
			.arg  = { .l = { .nr = ARRAY_SIZE(an_list),
					 .p = an_list }}
		};

#ifdef module_param_array
		if (num_AutoNeg > bd) {
#endif
			an = AutoNeg[bd];
			igb_validate_option(&an, &opt, adapter);
#ifdef module_param_array
		} else {
			an = opt.def;
		}
#endif
		hw->phy.autoneg_advertised = an;
	}

	switch (speed + dplx) {
	case 0:
		hw->mac.autoneg = adapter->fc_autoneg = 1;
#ifdef module_param_array
		if ((num_Speed > bd) && (speed != 0 || dplx != 0))
#else
		if (Speed[bd] != OPTION_UNSET || Duplex[bd] != OPTION_UNSET)
#endif
			DPRINTK(PROBE, INFO,
			       "Speed and duplex autonegotiation enabled\n");
		break;
	case HALF_DUPLEX:
		DPRINTK(PROBE, INFO, "Half Duplex specified without Speed\n");
		DPRINTK(PROBE, INFO, "Using Autonegotiation at "
			"Half Duplex only\n");
		hw->mac.autoneg = adapter->fc_autoneg = 1;
		hw->phy.autoneg_advertised = ADVERTISE_10_HALF |
		                             ADVERTISE_100_HALF;
		break;
	case FULL_DUPLEX:
		DPRINTK(PROBE, INFO, "Full Duplex specified without Speed\n");
		DPRINTK(PROBE, INFO, "Using Autonegotiation at "
			"Full Duplex only\n");
		hw->mac.autoneg = adapter->fc_autoneg = 1;
		hw->phy.autoneg_advertised = ADVERTISE_10_FULL |
		                             ADVERTISE_100_FULL |
		                             ADVERTISE_1000_FULL;
		break;
	case SPEED_10:
		DPRINTK(PROBE, INFO, "10 Mbps Speed specified "
			"without Duplex\n");
		DPRINTK(PROBE, INFO, "Using Autonegotiation at 10 Mbps only\n");
		hw->mac.autoneg = adapter->fc_autoneg = 1;
		hw->phy.autoneg_advertised = ADVERTISE_10_HALF |
		                             ADVERTISE_10_FULL;
		break;
	case SPEED_10 + HALF_DUPLEX:
		DPRINTK(PROBE, INFO, "Forcing to 10 Mbps Half Duplex\n");
		hw->mac.autoneg = adapter->fc_autoneg = 0;
		hw->mac.forced_speed_duplex = ADVERTISE_10_HALF;
		hw->phy.autoneg_advertised = 0;
		break;
	case SPEED_10 + FULL_DUPLEX:
		DPRINTK(PROBE, INFO, "Forcing to 10 Mbps Full Duplex\n");
		hw->mac.autoneg = adapter->fc_autoneg = 0;
		hw->mac.forced_speed_duplex = ADVERTISE_10_FULL;
		hw->phy.autoneg_advertised = 0;
		break;
	case SPEED_100:
		DPRINTK(PROBE, INFO, "100 Mbps Speed specified "
			"without Duplex\n");
		DPRINTK(PROBE, INFO, "Using Autonegotiation at "
			"100 Mbps only\n");
		hw->mac.autoneg = adapter->fc_autoneg = 1;
		hw->phy.autoneg_advertised = ADVERTISE_100_HALF |
		                             ADVERTISE_100_FULL;
		break;
	case SPEED_100 + HALF_DUPLEX:
		DPRINTK(PROBE, INFO, "Forcing to 100 Mbps Half Duplex\n");
		hw->mac.autoneg = adapter->fc_autoneg = 0;
		hw->mac.forced_speed_duplex = ADVERTISE_100_HALF;
		hw->phy.autoneg_advertised = 0;
		break;
	case SPEED_100 + FULL_DUPLEX:
		DPRINTK(PROBE, INFO, "Forcing to 100 Mbps Full Duplex\n");
		hw->mac.autoneg = adapter->fc_autoneg = 0;
		hw->mac.forced_speed_duplex = ADVERTISE_100_FULL;
		hw->phy.autoneg_advertised = 0;
		break;
	case SPEED_1000:
		DPRINTK(PROBE, INFO, "1000 Mbps Speed specified without "
			"Duplex\n");
		goto full_duplex_only;
	case SPEED_1000 + HALF_DUPLEX:
		DPRINTK(PROBE, INFO,
			"Half Duplex is not supported at 1000 Mbps\n");
		/* fall through */
	case SPEED_1000 + FULL_DUPLEX:
full_duplex_only:
		DPRINTK(PROBE, INFO,
		       "Using Autonegotiation at 1000 Mbps Full Duplex only\n");
		hw->mac.autoneg = adapter->fc_autoneg = 1;
		hw->phy.autoneg_advertised = ADVERTISE_1000_FULL;
		break;
	default:
		BUG();
	}

	/* Speed, AutoNeg and MDI/MDI-X must all play nice */
	if (e1000_validate_mdi_setting(&(adapter->hw)) < 0) {
		DPRINTK(PROBE, INFO,
			"Speed, AutoNeg and MDI-X specifications are "
			"incompatible. Setting MDI-X to a compatible value.\n");
	}
}

