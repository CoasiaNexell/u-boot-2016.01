/*
 * axp228.h  --  PMIC driver for the X-Powers AXP228
 *
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongshin, Park <pjsin865@nexell.co.kr>
 *
 * SPDX-License-Identifier:     GPL-2.0+

 */

#ifndef __AXP228_H_
#define __AXP228_H_

/* AXP228 registers */
#define AXP228_NUM_OF_REGS 0xFF

/* Control Register List */
#define AXP228 (22)
#define AXP228_STATUS (0x00)
#define AXP228_MODE_CHGSTATUS (0x01)
#define AXP228_IC_TYPE (0x03)
#define AXP228_BUFFER1 (0x04)
#define AXP228_BUFFER2 (0x05)
#define AXP228_BUFFER3 (0x06)
#define AXP228_BUFFER4 (0x07)
#define AXP228_BUFFER5 (0x08)
#define AXP228_BUFFER6 (0x09)
#define AXP228_BUFFER7 (0x0A)
#define AXP228_BUFFER8 (0x0B)
#define AXP228_BUFFER9 (0x0C)
#define AXP228_BUFFERA (0x0D)
#define AXP228_BUFFERB (0x0E)
#define AXP228_BUFFERC (0x0F)
#define AXP228_LDO_DC_EN1 (0X10)
#define AXP228_LDO_DC_EN2 (0X12)
#define AXP228_LDO_DC_EN3 (0X13)
#define AXP228_DLDO1OUT_VOL (0x15)
#define AXP228_DLDO2OUT_VOL (0x16)
#define AXP228_DLDO3OUT_VOL (0x17)
#define AXP228_DLDO4OUT_VOL (0x18)
#define AXP228_ELDO1OUT_VOL (0x19)
#define AXP228_ELDO2OUT_VOL (0x1A)
#define AXP228_ELDO3OUT_VOL  (0x1B)
#define AXP228_DC5LDOOUT_VOL (0x1C)
#define AXP228_DC1OUT_VOL (0x21)
#define AXP228_DC2OUT_VOL (0x22)
#define AXP228_DC3OUT_VOL (0x23)
#define AXP228_DC4OUT_VOL (0x24)
#define AXP228_DC5OUT_VOL (0x25)
#define AXP228_GPIO0LDOOUT_VOL (0x91)
#define AXP228_GPIO1LDOOUT_VOL (0x93)
#define AXP228_ALDO1OUT_VOL (0x28)
#define AXP228_ALDO2OUT_VOL (0x29)
#define AXP228_ALDO3OUT_VOL (0x2A)
#define AXP228_IPS_SET (0x30)
#define AXP228_VOFF_SET (0x31)
#define AXP228_OFF_CTL (0x32)
#define AXP228_PDBC (0x32)
#define AXP228_CHARGE1 (0x33)
#define AXP228_CHARGE2 (0x34)
#define AXP228_CHARGE3 (0x35)
#define AXP228_POK_SET (0x36)
#define AXP228_DCDC_FREQSET (0x37)
#define AXP228_DCDC_MODESET (0x80)
#define AXP228_ADC_EN (0x82)
#define AXP228_ADC_CONTROL3 (0x84)
#define AXP228_PWREN_CTL1 (0x8C)
#define AXP228_PWREN_CTL2 (0x8D)
#define AXP228_HOTOVER_CTL (0x8F)

/* GPIO Set Register List */
#define AXP228_GPIO0_CTL (0x90)
#define AXP228_GPIO1_CTL (0x92)
#define AXP228_GPIO01_SIGNAL (0x94)

/* Interrupt Register List */
#define AXP228_INTEN1 (0x40)
#define AXP228_INTEN2 (0x41)
#define AXP228_INTEN3 (0x42)
#define AXP228_INTEN4 (0x43)
#define AXP228_INTEN5 (0x44)
#define AXP228_INTSTS1 (0x48)
#define AXP228_INTSTS2 (0x49)
#define AXP228_INTSTS3 (0x4A)
#define AXP228_INTSTS4 (0x4B)
#define AXP228_INTSTS5 (0x4C)

/* ADC Data Register List */
#define AXP228_VBATH_RES (0x78)
#define AXP228_VBATL_RES (0x79)
#define AXP228_ICHGH_RES (0x7A)
#define AXP228_ICHGL_RES (0x7B)
#define AXP228_DISICHGH_RES (0x7C)
#define AXP228_DISICHGL_RES (0x7D)

#define AXP228_BAT_CHGCOULOMB3 (0xB0)
#define AXP228_BAT_CHGCOULOMB2 (0xB1)
#define AXP228_BAT_CHGCOULOMB1 (0xB2)
#define AXP228_BAT_CHGCOULOMB0 (0xB3)
#define AXP228_BAT_DISCHGCOULOMB3 (0xB4)
#define AXP228_BAT_DISCHGCOULOMB2 (0xB5)
#define AXP228_BAT_DISCHGCOULOMB1 (0xB6)
#define AXP228_BAT_DISCHGCOULOMB0 (0xB7)
#define AXP228_COULOMB_CTL (0xB8)
#define AXP228_CAP (0xB9)
#define AXP228_RDC0 (0xBA)
#define AXP228_RDC1 (0xBB)
#define AXP228_OCVBATH_RES (0xBC)
#define AXP228_OCVBATL_RES (0xBD)

#define AXP228_OCV_TABLE (0xC0)

#define AXP228_BATFULLCAPH_RES (0xE0)
#define AXP228_BATFULLCAPL_RES (0xE1)
#define AXP228_WARNING_LEVEL (0xE6)
#define AXP228_ADJUST_PARA (0xE8)

#define AXP228_CHARGE_VBUS AXP228_IPS_SET


/* register bit position */

/* bit definitions for AXP events ,irq event */
/*  AXP22  */
#define AXP228_IRQ_USBLO (1 << 1)
#define AXP228_IRQ_USBRE (1 << 2)
#define AXP228_IRQ_USBIN (1 << 3)
#define AXP228_IRQ_USBOV (1 << 4)
#define AXP228_IRQ_ACRE (1 << 5)
#define AXP228_IRQ_ACIN (1 << 6)
#define AXP228_IRQ_ACOV (1 << 7)
#define AXP228_IRQ_TEMLO (1 << 8)
#define AXP228_IRQ_TEMOV (1 << 9)
#define AXP228_IRQ_CHAOV (1 << 10)
#define AXP228_IRQ_CHAST (1 << 11)
#define AXP228_IRQ_BATATOU (1 << 12)
#define AXP228_IRQ_BATATIN (1 << 13)
#define AXP228_IRQ_BATRE (1 << 14)
#define AXP228_IRQ_BATIN (1 << 15)
#define AXP228_IRQ_POKLO (1 << 16)
#define AXP228_IRQ_POKSH (1 << 17)
#define AXP228_IRQ_CHACURLO (1 << 22)
#define AXP228_IRQ_ICTEMOV (1 << 23)
#define AXP228_IRQ_EXTLOWARN2 (1 << 24)
#define AXP228_IRQ_EXTLOWARN1 (1 << 25)
#define AXP228_IRQ_GPIO0TG ((uint64_t)1 << 32)
#define AXP228_IRQ_GPIO1TG ((uint64_t)1 << 33)
#define AXP228_IRQ_GPIO2TG ((uint64_t)1 << 34)
#define AXP228_IRQ_GPIO3TG ((uint64_t)1 << 35)
#define AXP228_IRQ_PEKFE ((uint64_t)1 << 37)
#define AXP228_IRQ_PEKRE ((uint64_t)1 << 38)
#define AXP228_IRQ_TIMER ((uint64_t)1 << 39)


/* Status Query Interface */
/*  AXP22  */
#define AXP228_STATUS_SOURCE (1 << 0)
#define AXP228_STATUS_ACUSBSH (1 << 1)
#define AXP228_STATUS_BATCURDIR (1 << 2)
#define AXP228_STATUS_USBLAVHO (1 << 3)
#define AXP228_STATUS_USBVA (1 << 4)
#define AXP228_STATUS_USBEN (1 << 5)
#define AXP228_STATUS_ACVA (1 << 6)
#define AXP228_STATUS_ACEN (1 << 7)

#define AXP228_STATUS_BATINACT (1 << 3)
#define AXP228_STATUS_BATEN (1 << 5)
#define AXP228_STATUS_INCHAR (1 << 6)
#define AXP228_STATUS_ICTEMOV (1 << 7)

#define AXP228_DCDC1_MODE_BIT (0)
#define AXP228_DCDC2_MODE_BIT (1)
#define AXP228_DCDC3_MODE_BIT (2)
#define AXP228_DCDC4_MODE_BIT (3)
#define AXP228_DCDC5_MODE_BIT (4)

#define AXP228_DC5LDO_EN_BIT (0)
#define AXP228_DCDC1_EN_BIT (1)
#define AXP228_DCDC2_EN_BIT (2)
#define AXP228_DCDC3_EN_BIT (3)
#define AXP228_DCDC4_EN_BIT (4)
#define AXP228_DCDC5_EN_BIT (5)
#define AXP228_ALDO1_EN_BIT (6)
#define AXP228_ALDO2_EN_BIT (7)

#define AXP228_ELDO1_EN_BIT (0)
#define AXP228_ELDO2_EN_BIT (1)
#define AXP228_ELDO3_EN_BIT (2)
#define AXP228_DLDO1_EN_BIT (3)
#define AXP228_DLDO2_EN_BIT (4)
#define AXP228_DLDO3_EN_BIT (5)
#define AXP228_DLDO4_EN_BIT (6)
#define AXP228_DC1SW_EN_BIT (7)

#define AXP228_ALDO3_EN_BIT (7)

#define AXP228_DCDC1_BIT (7)
#define AXP228_DCDC2_BIT (6)
#define AXP228_DCDC3_BIT (5)
#define AXP228_DCDC4_BIT (4)
#define AXP228_DCDC5_BIT (3)
#define AXP228_ALDO1_BIT (2)
#define AXP228_ALDO2_BIT (1)
#define AXP228_ALDO3_BIT (0)

#define AXP228_DLDO1_BIT (7)
#define AXP228_DLDO2_BIT (6)
#define AXP228_DLDO3_BIT (5)
#define AXP228_DLDO4_BIT (4)
#define AXP228_ELDO1_BIT (3)
#define AXP228_ELDO2_BIT (2)
#define AXP228_ELDO3_BIT (1)
#define AXP228_DC5LDO_BIT (0)

/* ETC */
#define AXP228_REG_BANKSEL 0xFF

#define ABS(x)((x) > 0 ? (x) : -(x))

#define USBVOLLIM 4700 /* AXP22:4000~4700, 100/step */
#define USBVOLLIMEN 1

#define USBCURLIM 500 /* AXP22:500/900 */
#define USBCURLIMEN 1

/* charge current */
#define CHARGE_CURRENT_500 500*1000
#define CHARGE_CURRENT 1500*1000
#define POWEROFF_CHARGE_CURRENT 1500*1000

/* limit charge current */
#define LIMIT_CHARGE_CURRENT 1500*1000

/* set lowe power warning level */
#define BATLOWLV1 15 /* AXP22:5%~20% */

/* set lowe power shutdown level */
#define BATLOWLV2 0 /* AXP22:0%~15% */

/* set lowe power animation level */
#define BATLOW_ANIMATION_CAP 1 /* AXP22:0%~100% */


/* Init N_VBUSEN status*/
#define VBUSEN 1

/* Init InShort status*/
#define VBUSACINSHORT 0

/* Init CHGLED function*/
#define CHGLEDFUN 1

/* set CHGLED Indication Type*/
#define CHGLEDTYPE 0

/* Init PMU Over Temperature protection*/
#define OTPOFFEN 0

/* Init battery capacity correct function*/
#define BATCAPCORRENT 1

/* Init battery regulator enable or not when charge finish*/
#define BATREGUEN 0

#define BATDET 1

/* Unified sub device IDs for AXP */
/* LDO0 For RTCLDO ,LDO1-3 for ALDO,LDO*/
enum axp228_regnum {
	AXP228_ID_DCDC1 = 0,
	AXP228_ID_DCDC2,
	AXP228_ID_DCDC3,
	AXP228_ID_DCDC4,
	AXP228_ID_DCDC5,
	AXP228_ID_ALDO1 = 0,
	AXP228_ID_ALDO2,
	AXP228_ID_ALDO3,
	AXP228_ID_DLDO1,
	AXP228_ID_DLDO2,
	AXP228_ID_DLDO3,
	AXP228_ID_DLDO4,
	AXP228_ID_ELDO1,
	AXP228_ID_ELDO2,
	AXP228_ID_ELDO3,
	AXP228_ID_DC5LDO,
};

struct sec_voltage_desc {
	int max;
	int min;
	int step;
};

/**
 * struct axp228_para - axp228 register parameters
 * @param vol_addr      i2c address of the given buck/ldo register
 * @param vol_bitpos    bit position to be set or clear within register
 * @param vol_bitmask   bit mask value
 * @param reg_enaddr    control register address, which enable the given
 *                      given buck/ldo.
 * @param reg_enbiton   value to be written to buck/ldo to make it ON
 * @param vol           Voltage information
 */
struct axp228_para {
	enum axp228_regnum regnum;
	u8 vol_addr;
	u8 vol_bitpos;
	u8 vol_bitmask;
	u8 reg_enaddr;
	u8 reg_enbitpos;
	const struct sec_voltage_desc *vol;
};

struct dm_axp228_platdata {
	int freq_spread_en;
	int spread_freq;
	int poly_phase_function;
	int switch_freq;

	int voff_set;
	int adc_control3;

	int pek_on;
	int pek_long;
	int pek_off_en;
	int pek_off_restart;
	int pek_delay;
	int pek_off;

	int irq_wakeup;
	int vbusacin_func;
	int vbusacin_status;
	int vbus_en;
	int pmu_reset;
	int overtmu_pwr_off;
};

struct dm_axp228_buck_platdata {
	int vol;
	int sleep;
	int work_mode;
	int on;
};

struct dm_axp228_ldo_platdata {
	int vol;
	int sleep;
	int on;
};

struct dm_axp228_chg_platdata {
	unsigned int version;
	unsigned int state_of_chrg;
	unsigned int capacity;
	unsigned int voltage_uV;
	unsigned int state;

	int limit_adp_amps;
	int limit_usb_amps;
	int limit_usbdata_amps;

	int chg_adp_amps;
	int chg_usb_amps;
	int current_complete;

	int cutoff_vol;
	int lowbat_battery_vol;
	int lowbat_adp_vol;
	int lowbat_usb_vol;
	int lowbat_usbdata_vol;

	int priority;
	int complete_dis;
	int nobat_ovlim_en;
	int otg_boost;
	int suspend;
	int jeitaen;
	int usb_en;
	int adp_en;

	int rapid_ttim_80;
	int rapid_ctime;
	int rapid_rtime;

	int power_on_vol;
	int vbatov_set;
	int vweak;
	int vdead;
	int vshort;

	int vfchg;
};
/* Drivers name */
#define AXP228_LDO_DRIVER "axp228_ldo"
#define AXP228_BUCK_DRIVER "axp228_buck"
#define AXP228_CHG_DRIVER "axp228_chg"

#endif /* __AXP228_H_ */
