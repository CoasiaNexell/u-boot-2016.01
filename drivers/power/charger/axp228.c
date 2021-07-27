/*
 * axp228.c  --  Charger driver for the AXP228
 *
 * Copyright (C) 2021  Nexell Co., Ltd.
 * Author: Jongshin, Park <pjsin865@coasia.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+

 */

#include <common.h>
#include <fdtdec.h>
#include <errno.h>
#include <dm.h>
#include <i2c.h>
#include <g_dnl.h>
#include <usb.h>
#include <usb/dwc2_udc.h>
#include <linux/time.h>
#include <power/pmic.h>
#include <power/charger.h>
#include <power/axp228.h>

DECLARE_GLOBAL_DATA_PTR;

#define DEBUG 1

#if defined(CONFIG_PMIC_REG_DUMP)
static int axp228_reg_dump(struct udevice *dev, const char *title)
{
	int i;
	int ret = 0;
	uint8_t value = 0;

	printf("############################################################\n");
	printf("##\e[31m %s \e[0m\n", title);
	printf("##       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F\n");
	for (i = 0; i <= AXP228_NUM_OF_REGS; i++) {
		if (i%16 == 0)
			printf("##  %02X:", i);
		if (i%4 == 0)
			printf(" ");
		ret = pmic_read(dev->parent, i, &value, 1);
		if (!ret)
			printf("%02x ", value);
		else
			printf("\e[31mxx\e[0m ");
		if ((i+1)%16 == 0)
			printf("\n");
	}
	printf("############################################################\n");

	return 0;
}
#endif

static int axp228_vbat_to_mV(uint16_t reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 1100 / 1000;
}

static int axp228_ocvbat_to_mV(uint16_t reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 1100 / 1000;
}


static int axp228_vdc_to_mV(uint16_t reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 1700 / 1000;
}


static int axp228_ibat_to_mA(uint16_t reg)
{
	return (int)(((reg >> 8) << 4) | (reg & 0x000F));
}

static int axp228_icharge_to_mA(uint16_t reg)
{
	return (int)(((reg >> 8) << 4) | (reg & 0x000F));
}

static int axp228_iac_to_mA(uint16_t reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 625 / 1000;
}

static int axp228_iusb_to_mA(uint16_t reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 375 / 1000;
}

static void axp228_set_usb_limit(struct udevice *dev, u8 data)
{
	uint8_t val = 0;

	pmic_read(dev->parent, AXP228_IPS_SET, &val, 1);
	val &= 0xfc;
	val |= data;
	pmic_write(dev->parent, AXP228_IPS_SET, &val, 1);

	return;
}

static int axp228_get_value_vbatt(struct udevice *dev)
{
	uint8_t reg_val[2];
	uint16_t vbat_res = 0;
	int sum_tmp = 0, i;

	for (i = 0; i < 5; i++) {
		mdelay(1);
		pmic_read(dev->parent, AXP228_VBATH_RES, &reg_val[0], 1);
		pmic_read(dev->parent, AXP228_VBATL_RES, &reg_val[1], 1);
		vbat_res = (reg_val[0] << 8) | reg_val[1];
		vbat_res = axp228_vbat_to_mV(vbat_res);
		sum_tmp += vbat_res;
	}
	sum_tmp = (sum_tmp / 5) * 1000;

	return sum_tmp;
}

static int axp228_get_value_gauge(struct udevice *dev)
{
	uint8_t reg_val;

	pmic_read(dev->parent, AXP228_CAP, &reg_val, 1);
	reg_val &= 0x7f;

	return (int)reg_val;
}

#ifdef CONFIG_SW_UBC_DETECT
static int axp228_get_usb_connect(int timeout)
{
	int controller_index = 0;
	unsigned long long start;
	unsigned long tmo;
	int ret;

	ret = board_usb_init(controller_index, USB_INIT_DEVICE);
	if (ret)
		return CMD_RET_FAILURE;

	g_dnl_clear_detach();
	ret = g_dnl_register("usb_dnl_fastboot");
	if (ret)
		return CMD_RET_FAILURE;

	if (!g_dnl_board_usb_cable_connected()) {
		ret = CMD_RET_FAILURE;
		goto exit;
	}

	start = timer_get_us();
	tmo = timeout * 1000;

	ret = CMD_RET_FAILURE;
	usb_clear_connect();

	while ((timer_get_us() - start) < tmo) {
		if (g_dnl_detach())
			break;
		usb_gadget_handle_interrupts(controller_index);

		if (usb_get_connect()) {
			ret = CMD_RET_SUCCESS;
			break;
		}
	}

exit:
	g_dnl_unregister();
	g_dnl_clear_detach();
	board_usb_cleanup(controller_index, USB_INIT_DEVICE);

	return ret;
}
#endif

static int axp228_get_charge_type(struct udevice *dev)
{
	struct dm_axp228_chg_platdata *pdata = dev->platdata;
	uint8_t val = 0;
	int ret = 0;

	pmic_read(dev->parent, AXP228_STATUS, &val, 1);
	val &= 0xff;

	if (val & AXP228_STATUS_ACEN) {
		ret = CHARGER_TA;
	} else if (val & AXP228_STATUS_USBEN) {
#ifdef CONFIG_SW_UBC_DETECT
		int timeout = 500;

		if (pdata->ubcchecktimeout != -ENODATA)
			timeout = pdata->ubcchecktimeout;

		if (axp228_get_usb_connect(timeout) == CMD_RET_SUCCESS) {
			axp228_set_usb_limit(dev, AXP228_USB_LIMIT_500);
			ret = CHARGER_USB;
		} else {
			axp228_set_usb_limit(dev, AXP228_USB_LIMIT_900);
			ret = CHARGER_TA;
		}
#else
		axp228_set_usb_limit(dev, AXP228_USB_LIMIT_500);
		ret = CHARGER_USB;
#endif
	} else {
		axp228_set_usb_limit(dev, AXP228_USB_LIMIT_500);
		ret = CHARGER_NO;
	}

	return ret;
}

static int axp228_get_charge_current(struct udevice *dev)
{
	uint8_t val = 0;
	int ret = -EINVAL;

	ret = pmic_read(dev->parent, AXP228_CHARGE1, &val, 1);
	if (ret >= 0) {
		val &= 0xf;
		ret = (val * 150) + 300;
		ret *= 1000;
	}

	return ret;
}

static int axp228_set_charge_current(struct udevice *dev, int uA)
{
	uint8_t val = 0;
	int ret = 0;

	if (uA >= 300000 && uA <= 2550000)
		val = (uA - 200001) / 150000;
	else if (uA < 300000)
		val = 0x0;
	else
		val = 0xf;

	ret = pmic_clrsetbits(dev->parent, AXP228_CHARGE1, 0x0F, val);

	return ret;
}

static int axp228_get_limit_current(struct udevice *dev, int type)
{
	uint8_t val = 0;
	int ret = -EINVAL;

	switch (type) {
	case CHARGER_TA:
		ret = pmic_read(dev->parent, AXP228_CHARGE3, &val, 1);
		if (ret >= 0) {
			val &= 0xf;
			ret = (val * 150) + 300;
			ret *= 1000;
		}
		break;

	case CHARGER_USB:
		ret = pmic_read(dev->parent, AXP228_IPS_SET, &val, 1);
		if (ret >= 0) {
			val &= 0x3;
			if (val == 0x0)
				ret = 900;
			else if (val == 0x1)
				ret = 500;
			else
				ret = 0; /* no limit */
			ret *= 1000;
		}
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int axp228_set_limit_current(struct udevice *dev, int type, int uA)
{
	uint8_t val = 0;
	int ret = -EINVAL;

	switch (type) {
	case CHARGER_TA:
		if (uA >= 300000 && uA <= 2550000)
			val = (uA - 200001) / 150000;
		else if (uA < 300000)
			val = 0x0;
		else
			val = 0xf;
		ret = pmic_clrsetbits(dev->parent, AXP228_CHARGE3, 0x0F, val);
		break;

	case CHARGER_USB:
		if (uA < 900000)
			val = 0x01; /* 0.5A */
		else if (uA < 1500000)
			val = 0x0; /* 0.9A */
		else
			val = 0x03; /* no limit */
		ret = pmic_clrsetbits(dev->parent, AXP228_IPS_SET, 0x03, val);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int axp228_chg_probe(struct udevice *dev)
{
	struct dm_axp228_chg_platdata *pdata = dev->platdata;
	uint8_t reg_val = 0, i = 0;
	uint8_t val = 0, val2 = 0, ocv_cmp = 0;
	int var = 0, tmp = 0;
	int Cur_CoulombCounter, rdc = 0;

	pmic_read(dev->parent, AXP228_POK_SET, &reg_val, 1);
	/* pok on time set */
	if (pdata->pek_on != -ENODATA) {
		if (pdata->pek_on < 1000) {
			reg_val &= 0x3f;
		} else if (pdata->pek_on < 2000) {
			reg_val &= 0x3f;
			reg_val |= 0x40;
		} else if (pdata->pek_on < 3000) {
			reg_val &= 0x3f;
			reg_val |= 0x80;
		} else {
			reg_val &= 0x3f;
			reg_val |= 0xc0;
		}
	}

	/* pok long time set*/
	if (pdata->pek_long != -ENODATA) {
		reg_val &= 0xcf;
		if (pdata->pek_long < 1000)
			tmp = 1000;
		else if (pdata->pek_long > 2500)
			tmp = 2500;
		else
			tmp = pdata->pek_long;
		reg_val |= (((tmp - 1000) / 500) << 4);
	}

	/* pek offlevel poweroff en set*/
	if (pdata->pek_off_en != -ENODATA) {
		reg_val &= 0xf7;
		if (pdata->pek_off_en)
			reg_val |= 0x08;
	}

	/*Init offlevel restart or not */
	if (pdata->pek_off_restart != -ENODATA) {
		reg_val &= 0xfb;
		if (pdata->pek_off_restart)
			reg_val |= 0x04;
	}

	/* pek offlevel time set */
	if (pdata->pek_off != -ENODATA) {
		reg_val &= 0xfc;
		if (pdata->pek_off < 4000)
			tmp = 4000;
		else if (pdata->pek_off > 10000)
			tmp = 10000;
		else
			tmp = pdata->pek_off;
		tmp = (tmp - 4000) / 2000;
		reg_val |= tmp;
	}
	pmic_write(dev->parent, AXP228_POK_SET, &reg_val, 1);

	pmic_read(dev->parent, AXP228_OFF_CTL, &reg_val, 1);
	/* pek delay set */
	if (pdata->pek_delay != -ENODATA) {
		reg_val &= 0xfc;
		reg_val |= ((pdata->pek_delay / 8) - 1);
	}

	/*Init CHGLED function*/
	if (pdata->ghcledfun != -ENODATA) {
		reg_val &= 0xf7;
		if (pdata->ghcledfun)
			reg_val |= 0x08;
	}
	pmic_write(dev->parent, AXP228_OFF_CTL, &reg_val, 1);

	/*set CHGLED Indication Type*/
	if (pdata->chgledtype != -ENODATA) {
		if (pdata->chgledtype)
			pmic_clrsetbits(dev->parent, 0x45, 0x10, 0x10);
		else
			pmic_clrsetbits(dev->parent, 0x45, 0x10, 0x0);
	}

	/*Init battery capacity correct function*/
	if (pdata->batcapcorrent != -ENODATA) {
		if (pdata->batcapcorrent)
			pmic_clrsetbits(dev->parent, AXP228_COULOMB_CTL
				, 0x20, 0x20);
		else
			pmic_clrsetbits(dev->parent, AXP228_COULOMB_CTL
				, 0x20, 0x0);
	}

	/* Init battery regulator enable or not when charge finish*/
	if (pdata->batreguen != -ENODATA) {
		if (pdata->batreguen)
			pmic_clrsetbits(dev->parent, AXP228_CHARGE2
				, 0x20, 0x20);

		else
			pmic_clrsetbits(dev->parent, AXP228_CHARGE2
				, 0x20, 0x0);
	}

	if (pdata->batdet != -ENODATA) {
		if (pdata->batdet)
			pmic_clrsetbits(dev->parent, AXP228_PDBC, 0x40, 0x40);

		else
			pmic_clrsetbits(dev->parent, AXP228_PDBC, 0x40, 0x0);
	}

	if (
		(pdata->usbvolim != -ENODATA) &&
		(pdata->usbvollimen != -ENODATA) &&
		(pdata->usbcurlim != -ENODATA) &&
		(pdata->usbcurlimen != -ENODATA)) {
		pmic_read(dev->parent, AXP228_IPS_SET, &reg_val, 1);

		/* USB voltage limit */
		if (pdata->usbvolim && pdata->usbvollimen) {
			var = pdata->usbvolim * 1000;
			if (var >= 4000000 && var <= 4700000) {
				tmp = (var - 4000000) / 100000;
				reg_val &= 0xC7;
				reg_val |= tmp << 3;
			}
		} else {
			reg_val &= ~0x40;
		}

		/* USB current limit */
		if (pdata->usbcurlim && pdata->usbcurlimen) {
			var = pdata->usbcurlim * 1000;
			reg_val &= ~0x03; /* 900mA */
			if (var == 500000) /* 500mA */
				reg_val |= 0x01;
		} else {
			reg_val |= 0x03; /* no current limit */
		}

		pmic_write(dev->parent, AXP228_IPS_SET, &reg_val, 1);
	}

	/* limit charge current */
	if (pdata->limit_current != -ENODATA) {
		val = (pdata->limit_current - 200001) / 150000;
		if (
			(pdata->limit_current >= 300000) &&
			(pdata->limit_current <= 2550000))
			pmic_clrsetbits(dev->parent, AXP228_CHARGE3
				, 0x0F, val);
		else if (pdata->limit_current < 300000)
			pmic_clrsetbits(dev->parent, AXP228_CHARGE3
				, 0x0F, 0x00);
		else
			pmic_clrsetbits(dev->parent, AXP228_CHARGE3
				, 0x0F, 0x0F);
	}

	/* charge current */
	if (pdata->charge_current != -ENODATA) {
		if (pdata->charge_current == 0)
			pmic_clrsetbits(dev->parent, AXP228_CHARGE1
				, 0x80, 0x00);
		else
			pmic_clrsetbits(dev->parent, AXP228_CHARGE1
				, 0x80, 0x80);

		pmic_read(dev->parent, AXP228_STATUS, &val, 1);
		pmic_read(dev->parent, AXP228_MODE_CHGSTATUS, &val2, 1);

		tmp = (val2 << 8) + val;
		val = (tmp & AXP228_STATUS_ACVA) ? 1 : 0;
		if (val)
			axp228_set_charge_current(dev, pdata->charge_current);
		else
			axp228_set_charge_current(dev, CHARGE_CURRENT_500);
	}

	/* set lowe power warning/shutdown level */
	if (
		(pdata->batlowlv1 != -ENODATA) &&
		(pdata->batlowlv2 != -ENODATA)) {
		reg_val = ((pdata->batlowlv1 - 5) << 4) + pdata->batlowlv2;
		pmic_write(dev->parent, AXP228_WARNING_LEVEL, &reg_val, 1);
	}

	/* OCV Table */
	if (pdata->ocvreg) {
		for (i = 0; i < pdata->ocvreg_len; i++) {
			pmic_read(dev->parent, AXP228_OCV_TABLE + i, &val, 1);
			if (val != pdata->ocv_table[i]) {
				ocv_cmp = 1;
				break;
			}
		}

		if (ocv_cmp)
			for (i = 0; i < pdata->ocvreg_len; i++)
				pmic_write(dev->parent, AXP228_OCV_TABLE + i
					, &pdata->ocv_table[i], 1);
	}

	/* RDC initial */
	if (pdata->batrdc != -ENODATA) {
		pmic_read(dev->parent, AXP228_RDC0, &reg_val, 1);
		if (ocv_cmp || ((pdata->batrdc) && (!(reg_val & 0x40)))) {
			rdc = (pdata->batrdc * 10000 + 5371) / 10742;

			reg_val = ((rdc >> 8) & 0x1F) | 0x80;
			pmic_write(dev->parent, AXP228_RDC0, &reg_val, 1);

			reg_val = rdc & 0x00FF;
			pmic_write(dev->parent, AXP228_RDC1, &reg_val, 1);
		}
	}

	/* probe RDC, OCV */
	if (pdata->batcap != -ENODATA) {
		pmic_read(dev->parent, AXP228_BATFULLCAPH_RES, &reg_val, 1);
		if (ocv_cmp || ((pdata->batcap) && (!(reg_val & 0x80)))) {
			Cur_CoulombCounter = pdata->batcap * 1000 / 1456;

			reg_val = (Cur_CoulombCounter >> 8) | 0x80;
			pmic_write(dev->parent, AXP228_BATFULLCAPH_RES, &reg_val
				, 1);

			reg_val = Cur_CoulombCounter & 0x00FF;
			pmic_write(dev->parent, AXP228_BATFULLCAPL_RES, &reg_val
				, 1);
		} else if (!pdata->batcap) {
			reg_val = 0x0;
			pmic_write(dev->parent, AXP228_BATFULLCAPH_RES, &reg_val
				, 1);
			pmic_write(dev->parent, AXP228_BATFULLCAPL_RES, &reg_val
				, 1);
		}
	}

#ifndef QUICKBOOT
	pmic_read(dev->parent, AXP228_STATUS, &val, 1);
	pmic_read(dev->parent, AXP228_MODE_CHGSTATUS, &val2, 1);
	printf("AXP228 Charger: STATUS           : 0x%02x:0x%02x, 0x%02x:0x%02x\n"
		, AXP228_STATUS, val, AXP228_MODE_CHGSTATUS, val2);
#endif

#if defined(CONFIG_PMIC_REG_DUMP)
	axp228_reg_dump(dev, "Charger Setup Register Dump");
#endif
	return 0;
}

static int axp228_chg_ofdata_to_platdata(struct udevice *dev)
{
	struct dm_axp228_chg_platdata *pdata = dev->platdata;
	const void *blob = gd->fdt_blob;
	int offset = dev->of_offset;
	int i;

	pdata->pek_on = fdtdec_get_int(blob, offset,
		"chg,pek_on", -ENODATA);
	pdata->pek_long = fdtdec_get_int(blob, offset,
		"chg,pek_long", -ENODATA);
	pdata->pek_off_en = fdtdec_get_int(blob, offset,
		"chg,pek_off_en", -ENODATA);
	pdata->pek_off_restart = fdtdec_get_int(blob, offset,
		"chg,pek_off_restart", -ENODATA);
	pdata->pek_delay = fdtdec_get_int(blob, offset,
		"chg,pek_delay", -ENODATA);
	pdata->pek_off = fdtdec_get_int(blob, offset,
		"chg,pek_off", -ENODATA);

	pdata->ghcledfun = fdtdec_get_int(blob, offset,
		"chg,ghcledfun", -ENODATA);
	pdata->chgledtype = fdtdec_get_int(blob, offset,
		"chg,chgledtype", -ENODATA);
	pdata->batcapcorrent = fdtdec_get_int(blob, offset,
		"chg,batcapcorrent", -ENODATA);
	pdata->batreguen = fdtdec_get_int(blob, offset,
		"chg,batreguen", -ENODATA);
	pdata->batdet = fdtdec_get_int(blob, offset,
		"chg,batdet", -ENODATA);

	pdata->usbvolim = fdtdec_get_int(blob, offset,
		"chg,usbvolim", -ENODATA);
	pdata->usbvollimen = fdtdec_get_int(blob, offset,
		"chg,usbvollimen", -ENODATA);
	pdata->usbcurlim = fdtdec_get_int(blob, offset,
		"chg,usbcurlim", -ENODATA);
	pdata->usbcurlimen = fdtdec_get_int(blob, offset,
		"chg,usbcurlimen", -ENODATA);

	pdata->charge_current = fdtdec_get_int(blob, offset,
		"chg,charge_current", -ENODATA);
	pdata->limit_current = fdtdec_get_int(blob, offset,
		"chg,limit_current", -ENODATA);

	pdata->batlowlv1 = fdtdec_get_int(blob, offset,
		"chg,batlowlv1", -ENODATA);
	pdata->batlowlv2 = fdtdec_get_int(blob, offset,
		"chg,batlowlv2", -ENODATA);

	pdata->ocvreg = fdt_getprop(blob, offset,
		"chg,ocvreg", &pdata->ocvreg_len);
	if (pdata->ocvreg) {
		pdata->ocvreg_len /= sizeof(*pdata->ocvreg);
		if (pdata->ocvreg_len > OCV_TABLE_SIZE)
			pdata->ocvreg_len = OCV_TABLE_SIZE;
		for (i = 0; i < pdata->ocvreg_len; i++)
			pdata->ocv_table[i] = fdt32_to_cpu(*pdata->ocvreg++);
	}

	pdata->batrdc = fdtdec_get_int(blob, offset,
		"chg,batrdc", -ENODATA);
	pdata->batcap = fdtdec_get_int(blob, offset,
		"chg,batcap", -ENODATA);
	pdata->ubcchecktimeout = fdtdec_get_int(blob, offset,
		"chg,ubcchecktimeout", -ENODATA);

	return 0;
}

static int axp228_chg_bind(struct udevice *dev)
{
	return 0;
}

static const struct dm_charger_ops axp228_chg_ops = {
	.get_value_vbatt = axp228_get_value_vbatt,
	.get_value_gauge = axp228_get_value_gauge,
	.get_charge_type = axp228_get_charge_type,
	.get_charge_current = axp228_get_charge_current,
	.set_charge_current = axp228_set_charge_current,
	.get_limit_current = axp228_get_limit_current,
	.set_limit_current = axp228_set_limit_current,
};

U_BOOT_DRIVER(axp228_chg) = {
	.name = AXP228_CHG_DRIVER,
	.id = UCLASS_CHARGER,
	.ops = &axp228_chg_ops,
	.bind = axp228_chg_bind,
	.probe = axp228_chg_probe,
	.ofdata_to_platdata = axp228_chg_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct dm_axp228_chg_platdata),
};
