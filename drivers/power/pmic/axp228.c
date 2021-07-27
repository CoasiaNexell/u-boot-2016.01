/*
 * axp228.c  --  PMIC driver for the X-Powers AXP228
 *
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongshin, Park <pjsin865@nexell.co.kr>
 *
 * SPDX-License-Identifier:     GPL-2.0+

 */

#include <common.h>
#include <fdtdec.h>
#include <errno.h>
#include <dm.h>
#include <i2c.h>
#include <power/pmic.h>
#include <power/regulator.h>
#include <power/axp228.h>

DECLARE_GLOBAL_DATA_PTR;

static const struct pmic_child_info pmic_reg_info[] = {
	{ .prefix = "LDO", .driver = AXP228_LDO_DRIVER },
	{ .prefix = "BUCK", .driver = AXP228_BUCK_DRIVER },
	{ },
};

static const struct pmic_child_info pmic_chg_info[] = {
	{ .prefix = "CHG", .driver = AXP228_CHG_DRIVER },
	{ },
};

#ifndef QUICKBOOT
static void axp228_print_state(struct udevice *dev)
{
	return;
}
#endif

static int axp228_reg_count(struct udevice *dev)
{
	return AXP228_NUM_OF_REGS;
}

static int axp228_write(struct udevice *dev, uint reg, const uint8_t *buff,
			  int len)
{
	if (dm_i2c_write(dev, reg, buff, len)) {
		error("write error to device: %p register: %#x!", dev, reg);
		return -EIO;
	}

	return 0;
}

static int axp228_read(struct udevice *dev, uint reg, uint8_t *buff, int len)
{
	if (dm_i2c_read(dev, reg, buff, len)) {
		error("read error from device: %p register: %#x!", dev, reg);
		return -EIO;
	}

	return 0;
}

static int axp228_set_bits(struct udevice *dev, uint reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;

	ret = axp228_read(dev, reg, &reg_val, 1);
	if (ret)
		return ret;

	if ((reg_val & bit_mask) != bit_mask) {
		reg_val |= bit_mask;
		ret = axp228_write(dev, reg, &reg_val, 1);
	}
	return ret;
}

static int axp228_clr_bits(struct udevice *dev, uint reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;

	ret = axp228_read(dev, reg, &reg_val, 1);
	if (ret)
		return ret;

	if (reg_val & bit_mask) {
		reg_val &= ~bit_mask;
		ret = axp228_write(dev, reg, &reg_val, 1);
	}
	return ret;
}

static int axp228_update(struct udevice *dev,
	uint reg, uint8_t val, uint8_t mask)
{
	u8 reg_val;
	int ret = 0;

	ret = axp228_read(dev, reg, &reg_val, 1);
	if (ret)
		return ret;

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | val;
		ret = axp228_write(dev, reg, &reg_val, 1);
	}
	return ret;
}

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
		ret = dm_i2c_read(dev, i, &value, 1);
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

static int axp228_init_chip(struct udevice *dev)
{
	uint8_t chip_id = 0;
	int err, i;
	struct int_reg v[10] = {
		{ .reg = AXP228_INTEN1, .val = 0xd8},
		{ .reg = AXP228_INTEN2, .val = 0xff},
		{ .reg = AXP228_INTEN3, .val = 0x00},
		{ .reg = AXP228_INTEN4, .val = 0x01},
		{ .reg = AXP228_INTEN5, .val = 0x00},
		{ .reg = AXP228_INTSTS1, .val = 0xff},
		{ .reg = AXP228_INTSTS2, .val = 0xff},
		{ .reg = AXP228_INTSTS3, .val = 0xff},
		{ .reg = AXP228_INTSTS4, .val = 0xff},
		{ .reg = AXP228_INTSTS5, .val = 0xff}
	};

	/*read chip id*/
	err = axp228_read(dev, AXP228_IC_TYPE, &chip_id, 1);
	if (err) {
		printf("[AXP228] read chip id failed!\n");
		return err;
	}
	printf("AXP228 CHIP ID  : 0x%02x detected\n", chip_id);

	/*enable irqs and clear*/
	for (i = 0; i < 10; i++) {
		err = axp228_write(dev, v[i].reg, &v[i].val, 1);
		if (err) {
			printf("[AXP228] clear irq failed!\n");
			return err;
		}
	}

	return 0;
}

static void axp228_device_setup(struct udevice *dev)
{
	struct dm_axp228_platdata *pdata = dev->platdata;
	uint8_t reg_val = 0x0;
	int ret = 0;

	if (
		(pdata->freq_spread_en != -ENODATA) &&
		(pdata->spread_freq != -ENODATA) &&
		(pdata->poly_phase_function != -ENODATA) &&
		(pdata->switch_freq != -ENODATA)) {
		reg_val = ((pdata->freq_spread_en << 7)
			| (pdata->spread_freq << 6)
			| (pdata->poly_phase_function << 4)
			| (pdata->switch_freq << 0));
		ret = axp228_write(dev, AXP228_DCDC_FREQSET, &reg_val, 1);
	}

	if (pdata->voff_set != -ENODATA)
		ret = axp228_update(dev, AXP228_VOFF_SET,
			pdata->freq_spread_en, 0x7);

	if (pdata->adc_control3 != -ENODATA)
		ret = axp228_set_bits(dev, AXP228_ADC_CONTROL3,
			pdata->adc_control3);

	if (
		(pdata->irq_wakeup != -ENODATA) &&
		(pdata->vbusacin_func != -ENODATA) &&
		(pdata->vbusacin_status != -ENODATA) &&
		(pdata->vbus_en != -ENODATA) &&
		(pdata->pmu_reset != -ENODATA) &&
		(pdata->overtmu_pwr_off != -ENODATA)) {
		reg_val = ((pdata->irq_wakeup << 7)
			| (pdata->vbusacin_func << 6)
			| (pdata->vbusacin_status << 5)
			| (pdata->vbus_en << 4)
			| (pdata->pmu_reset << 3)
			| (pdata->overtmu_pwr_off << 2)
			| 0x1);
		ret = axp228_write(dev, AXP228_HOTOVER_CTL, &reg_val, 1);
	}

	if (ret)
		error("%s: device init fail!!\n", __func__);
}

static int axp228_probe(struct udevice *dev)
{
#if defined(CONFIG_PMIC_REG_DUMP)
	axp228_reg_dump(dev, "PMIC Register Dump");
#endif

	axp228_device_setup(dev);

	axp228_init_chip(dev);

#if defined(CONFIG_PMIC_REG_DUMP)
	axp228_reg_dump(dev, "PMIC Setup Register Dump");
#endif

#ifndef QUICKBOOT
	axp228_print_state(dev);
#endif

	return 0;
}
static int axp228_ofdata_to_platdata(struct udevice *dev)
{
	struct dm_axp228_platdata *pdata = dev->platdata;
	const void *blob = gd->fdt_blob;
	int axp228_node;

	axp228_node = fdt_subnode_offset(blob,
		fdt_path_offset(blob, "/"), "init-axp228");

	pdata->freq_spread_en = fdtdec_get_int(blob, axp228_node,
		"axp228,freq_spread_en", -ENODATA);
	pdata->spread_freq = fdtdec_get_int(blob, axp228_node,
		"axp228,spread_freq", -ENODATA);
	pdata->poly_phase_function = fdtdec_get_int(blob, axp228_node,
		"axp228,poly_phase_function", -ENODATA);
	pdata->switch_freq = fdtdec_get_int(blob, axp228_node,
		"axp228,switch_freq", -ENODATA);

	pdata->voff_set = fdtdec_get_int(blob, axp228_node,
		"axp228,voff_set", -ENODATA);
	pdata->adc_control3 = fdtdec_get_int(blob, axp228_node,
		"axp228,adc_control3", -ENODATA);

	pdata->irq_wakeup = fdtdec_get_int(blob, axp228_node,
		"axp228,irq_wakeup", -ENODATA);
	pdata->vbusacin_func = fdtdec_get_int(blob, axp228_node,
		"axp228,vbusacin_func", -ENODATA);
	pdata->vbusacin_status = fdtdec_get_int(blob, axp228_node,
		"axp228,vbusacin_status", -ENODATA);
	pdata->vbus_en = fdtdec_get_int(blob, axp228_node,
		"axp228,vbus_en", -ENODATA);
	pdata->pmu_reset = fdtdec_get_int(blob, axp228_node,
		"axp228,pmu_reset", -ENODATA);
	pdata->overtmu_pwr_off = fdtdec_get_int(blob, axp228_node,
		"axp228,overtmu_pwr_off", -ENODATA);

	return 0;
}

static int axp228_bind(struct udevice *dev)
{
	struct dm_axp228_platdata *pdata = dev->platdata;
	const void *blob = gd->fdt_blob;

	debug("%s: dev->name:%s\n", __func__, dev->name);

	if (!strncmp(dev->name, "axp228", 6)) {
		pdata->reg_node = fdt_subnode_offset(blob,
			fdt_path_offset(blob, "/"), "voltage-regulators");
		pdata->chg_node = fdt_subnode_offset(blob,
			fdt_path_offset(blob, "/"), "init-charger");
	} else {
		pdata->reg_node = fdt_subnode_offset(blob, dev->of_offset,
				"voltage-regulators");
	}

	if (pdata->reg_node > 0) {
		debug("%s: found regulators subnode\n", __func__);
		pdata->child_reg = pmic_bind_children(dev, pdata->reg_node,
			pmic_reg_info);
		if (!pdata->child_reg)
			debug("%s: %s - no child found\n", __func__, dev->name);
	} else {
		debug("%s: regulators subnode not found!\n", __func__);
	}

	if (pdata->chg_node > 0) {
		debug("%s: found charger subnode\n", __func__);
		pdata->child_chg = pmic_bind_children(dev, pdata->chg_node,
			pmic_chg_info);
		if (!pdata->child_chg)
			debug("%s: %s - no child found\n", __func__, dev->name);
	} else {
		debug("%s: charger subnode not found!\n", __func__);
	}

	/* Always return success for this device */
	return 0;
}

static struct dm_pmic_ops axp228_ops = {
	.reg_count = axp228_reg_count,
	.read = axp228_read,
	.write = axp228_write,
};

static const struct udevice_id axp228_ids[] = {
	{ .compatible = "x-powers,axp228" },
	{ }
};

U_BOOT_DRIVER(pmic_axp228) = {
	.name = "axp228_pmic",
	.id = UCLASS_PMIC,
	.of_match = axp228_ids,
	.ofdata_to_platdata = axp228_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct dm_axp228_platdata),
	.bind = axp228_bind,
	.probe = axp228_probe,
	.ops = &axp228_ops,
};
