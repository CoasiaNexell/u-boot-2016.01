/*
 * (C) Copyright 2016 Nexell
 * Hyunseok, Jung <hsjung@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <config.h>
#include <common.h>
#ifdef CONFIG_PWM_NX
#include <pwm.h>
#endif
#include <asm/io.h>

#include <asm/arch/nexell.h>
#include <asm/arch/nx_gpio.h>


#ifdef CONFIG_DM_PMIC
#include <dm.h>
#include <dm/uclass-internal.h>
#include <power/pmic.h>
#endif

#ifdef CONFIG_DM_REGULATOR
#include <power/regulator.h>
#endif

#ifdef CONFIG_DM_CHARGER
#include <power/charger.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

/*------------------------------------------------------------------------------
 * intialize nexell soc and board status.
 */

/* call from u-boot */
int board_early_init_f(void)
{
	return 0;
}

void board_gpio_init(void)
{
	nx_gpio_initialize();
	nx_gpio_set_base_address(0, (void *)PHY_BASEADDR_GPIOA);
	nx_gpio_set_base_address(1, (void *)PHY_BASEADDR_GPIOB);
	nx_gpio_set_base_address(2, (void *)PHY_BASEADDR_GPIOC);
	nx_gpio_set_base_address(3, (void *)PHY_BASEADDR_GPIOD);
	nx_gpio_set_base_address(4, (void *)PHY_BASEADDR_GPIOE);
}

#ifdef CONFIG_PWM_NX
void board_backlight_init(void)
{
	pwm_init(CONFIG_BACKLIGHT_CH, CONFIG_BACKLIGHT_DIV,
		 CONFIG_BACKLIGHT_INV);
	pwm_config(CONFIG_BACKLIGHT_CH, TO_DUTY_NS(CONFIG_BACKLIGHT_DUTY,
						   CONFIG_BACKLIGHT_HZ),
		   TO_PERIOD_NS(CONFIG_BACKLIGHT_HZ));
}
#endif

int mmc_get_env_dev(void)
{
	return 0;
}

int board_init(void)
{
	board_gpio_init();
#ifdef CONFIG_PWM_NX
	board_backlight_init();
#endif
	return 0;
}

#ifdef CONFIG_DM_CHARGER_AXP228
void charger_init_board(void)
{
	struct udevice *dev;
	struct dm_charger_uclass_platdata *chg_uc_pdata;
	struct udevice *charger;
	int chg_type, chg_curr;
	int chg_adp_limit, chg_usb_limit;
	int vbatt, gauge;
	int ret = -ENODEV;

	for (ret = uclass_find_first_device(UCLASS_CHARGER, &dev);
		dev;
		ret = uclass_find_next_device(&dev)) {
		if (ret)
			continue;

		chg_uc_pdata = dev_get_uclass_platdata(dev);
		if (!chg_uc_pdata)
			continue;

		ret = uclass_get_device_tail(dev, 0, &charger);
		if (!ret)
			break;
	}

	chg_type = charger_get_charge_type(charger);
	printf("AXP228 Charger: Type             : %s\n"
		, chg_type == CHARGER_USB ? "USB" :
		(chg_type == CHARGER_TA ? "ADP" : "NONE"));

	vbatt = charger_get_value_vbatt(charger) / 1000;
	printf("AXP228 Charger: Vbatt            : %dmV\n", vbatt);

	gauge = charger_get_value_gauge(charger);
	printf("AXP228 Charger: Gauge            : %d%%\n", gauge);

	chg_curr = charger_get_charge_current(charger) / 1000;
	printf("AXP228 Charger: Charge Current   : %dmA\n", chg_curr);

	chg_adp_limit = charger_get_limit_current(charger, CHARGER_TA) / 1000;
	printf("AXP228 Charger: ADP Limit        : %dmA\n", chg_adp_limit);

	chg_usb_limit = charger_get_limit_current(charger, CHARGER_USB) / 1000;
	if (chg_usb_limit == 0)
		printf("AXP228 Charger: USB Limit        : No limit\n");
	else
		printf("AXP228 Charger: USB Limit        : %dmA\n"
			, chg_usb_limit);
}
#endif

int board_late_init(void)
{
	setenv("board_rev", "0");

#ifdef CONFIG_SILENT_CONSOLE
	gd->flags &= ~GD_FLG_SILENT;
#endif

#ifdef CONFIG_RECOVERY_BOOT
#define ALIVE_SCRATCH1_READ_REGISTER (0xc00108b4)
#define ALIVE_SCRATCH1_RESET_REGISTER (0xc00108ac)
#define RECOVERY_SIGNATURE (0x52455343) /* (ASCII) : R.E.S.C */
#ifndef QUICKBOOT
	printf("signature --> 0x%x\n", readl(ALIVE_SCRATCH1_READ_REGISTER));
#endif
	if (readl(ALIVE_SCRATCH1_READ_REGISTER) == RECOVERY_SIGNATURE) {
		printf("reboot recovery!!!!\n");
		writel(0xffffffff, ALIVE_SCRATCH1_RESET_REGISTER);
		setenv("bootcmd", "run recoveryboot");
	}
#endif

#ifdef CONFIG_DM_CHARGER_AXP228
	charger_init_board();
#endif

	return 0;
}

#ifdef CONFIG_SPLASH_SOURCE
#include <splash.h>
struct splash_location splash_locations[] = {
	{
	.name = "mmc_fs",
	.storage = SPLASH_STORAGE_MMC,
	.flags = SPLASH_STORAGE_FS,
	.devpart = "0:1",
	},
	{
	.name = "mmc",
	.storage = SPLASH_STORAGE_MMC,
	.flags = SPLASH_STORAGE_RAW,
	.offset = CONFIG_SPLASH_MMC_OFFSET,
	},
};

int splash_screen_prepare(void)
{
	int err = splash_source_load(splash_locations,
		sizeof(splash_locations)/sizeof(struct splash_location));
	if (!err) {
		char addr[64];

		sprintf(addr, "0x%lx", gd->fb_base);
		setenv("fb_addr", addr);
	}

	return err;
}
#endif

/* u-boot dram initialize  */
int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
	return 0;
}

/* u-boot dram board specific */
void dram_init_banksize(void)
{
	/* set global data memory */
	gd->bd->bi_arch_number = machine_arch_type;
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x00000100;

	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size  = CONFIG_SYS_SDRAM_SIZE;
}

#ifdef CONFIG_DM_PMIC_AXP228
void power_init_board(void)
{
	struct udevice *pmic;
	struct udevice *dev;
#ifdef CONFIG_DM_REGULATOR
	struct dm_regulator_uclass_platdata *reg_uc_pdata;
	struct udevice *regulator;
#endif
	int ret = -ENODEV;

	ret = pmic_get("axp228_gpio@34", &pmic);
	if (ret)
		printf("Can't get PMIC: %s!\n", "axp228_gpio@32");

	if (device_has_children(pmic)) {
#ifdef CONFIG_DM_REGULATOR
		for (ret = uclass_find_first_device(UCLASS_REGULATOR, &dev);
			dev;
			ret = uclass_find_next_device(&dev)) {
			if (ret)
				continue;

			reg_uc_pdata = dev_get_uclass_platdata(dev);
			if (!reg_uc_pdata)
				continue;

			uclass_get_device_tail(dev, 0, &regulator);
		}
#endif
	}
}
#endif
