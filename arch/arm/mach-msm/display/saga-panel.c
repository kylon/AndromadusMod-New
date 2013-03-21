/* linux/arch/arm/mach-msm/board-saga-panel.c
 *
 * Copyright (C) 2008 HTC Corporation.
 * Author: Jay Tu <jay_tu@htc.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio.h>

#include <asm/io.h>
#include <asm/mach-types.h>
#include <mach/msm_fb.h>
#include <mach/msm_iomap-7x30.h>
#include <mach/vreg.h>
#include <mach/msm_panel.h>
#include <mach/panel_id.h>


#include "../board-saga.h"
#include "../devices.h"
#include "../proc_comm.h"
#include "../../../../drivers/video/msm/mdp_hw.h"

#if 1
#define B(s...) printk(s)
#else
#define B(s...) do {} while (0)
#endif
#define LCM_GPIO_CFG(gpio, func) \
PCOM_GPIO_CFG(gpio, func, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_4MA)
extern int panel_type;
struct vreg *vreg_ldo19, *vreg_ldo20;
struct mddi_cmd {
        unsigned char cmd;
        unsigned delay;
        unsigned char *vals;
        unsigned len;
};
#define prm_size 20
#define LCM_CMD(_cmd, _delay, ...)                              \
{                                                               \
        .cmd = _cmd,                                            \
        .delay = _delay,                                        \
        .vals = (u8 []){__VA_ARGS__},                           \
        .len = sizeof((u8 []){__VA_ARGS__}) / sizeof(u8)        \
}
#define DEFAULT_BRIGHTNESS 255
#define PWM_USER_DEF	 		143
#define PWM_USER_MIN			30
#define PWM_USER_DIM			 9
#define PWM_USER_MAX			255

#define PWM_HITACHI_DEF			174
#define PWM_HITACHI_MIN			 10
#define PWM_HITACHI_MAX			255
enum {
	GATE_ON = 1 << 0,
};

static uint32_t display_on_gpio_table[] = {
	LCM_GPIO_CFG(SAGA_LCD_PCLK_1, 1),
	LCM_GPIO_CFG(SAGA_LCD_DE, 1),
	LCM_GPIO_CFG(SAGA_LCD_VSYNC, 1),
	LCM_GPIO_CFG(SAGA_LCD_HSYNC, 1),
	LCM_GPIO_CFG(SAGA_LCD_G0, 1),
	LCM_GPIO_CFG(SAGA_LCD_G1, 1),
	LCM_GPIO_CFG(SAGA_LCD_G2, 1),
	LCM_GPIO_CFG(SAGA_LCD_G3, 1),
	LCM_GPIO_CFG(SAGA_LCD_G4, 1),
	LCM_GPIO_CFG(SAGA_LCD_G5, 1),
	LCM_GPIO_CFG(SAGA_LCD_B0, 1),
	LCM_GPIO_CFG(SAGA_LCD_B1, 1),
	LCM_GPIO_CFG(SAGA_LCD_B2, 1),
	LCM_GPIO_CFG(SAGA_LCD_B3, 1),
	LCM_GPIO_CFG(SAGA_LCD_B4, 1),
	LCM_GPIO_CFG(SAGA_LCD_B5, 1),
	LCM_GPIO_CFG(SAGA_LCD_R0, 1),
	LCM_GPIO_CFG(SAGA_LCD_R1, 1),
	LCM_GPIO_CFG(SAGA_LCD_R2, 1),
	LCM_GPIO_CFG(SAGA_LCD_R3, 1),
	LCM_GPIO_CFG(SAGA_LCD_R4, 1),
	LCM_GPIO_CFG(SAGA_LCD_R5, 1),
};

static uint32_t display_off_gpio_table[] = {
	LCM_GPIO_CFG(SAGA_LCD_PCLK_1, 0),
	LCM_GPIO_CFG(SAGA_LCD_DE, 0),
	LCM_GPIO_CFG(SAGA_LCD_VSYNC, 0),
	LCM_GPIO_CFG(SAGA_LCD_HSYNC, 0),
	LCM_GPIO_CFG(SAGA_LCD_G0, 0),
	LCM_GPIO_CFG(SAGA_LCD_G1, 0),
	LCM_GPIO_CFG(SAGA_LCD_G2, 0),
	LCM_GPIO_CFG(SAGA_LCD_G3, 0),
	LCM_GPIO_CFG(SAGA_LCD_G4, 0),
	LCM_GPIO_CFG(SAGA_LCD_G5, 0),
	LCM_GPIO_CFG(SAGA_LCD_B0, 0),
	LCM_GPIO_CFG(SAGA_LCD_B1, 0),
	LCM_GPIO_CFG(SAGA_LCD_B2, 0),
	LCM_GPIO_CFG(SAGA_LCD_B3, 0),
	LCM_GPIO_CFG(SAGA_LCD_B4, 0),
	LCM_GPIO_CFG(SAGA_LCD_B5, 0),
	LCM_GPIO_CFG(SAGA_LCD_R0, 0),
	LCM_GPIO_CFG(SAGA_LCD_R1, 0),
	LCM_GPIO_CFG(SAGA_LCD_R2, 0),
	LCM_GPIO_CFG(SAGA_LCD_R3, 0),
	LCM_GPIO_CFG(SAGA_LCD_R4, 0),
	LCM_GPIO_CFG(SAGA_LCD_R5, 0),
};

static int panel_gpio_switch(int on)
{
	config_gpio_table(
		!!on ? display_on_gpio_table : display_off_gpio_table,
		!!on ? ARRAY_SIZE(display_on_gpio_table) : ARRAY_SIZE(display_off_gpio_table));

	return 0;
}

static inline int is_sony_panel(void){
	return (panel_type == PANEL_ID_SAG_SONY)? 1 : 0;
}
static inline int is_hitachi_panel(void){
	return (panel_type == PANEL_ID_SAG_HITACHI)? 1 : 0;
}

static int panel_init_power(void)
{
  int rc;

  vreg_ldo19 = vreg_get(NULL, "wlan2");
  
  if (IS_ERR(vreg_ldo19)) {
    pr_err("%s: wlan2 vreg get failed (%ld)\n",
           __func__, PTR_ERR(vreg_ldo19));
    return -1;
  }
  
  /* lcd panel power */
  /* 2.85V -- LDO20 */
  vreg_ldo20 = vreg_get(NULL, "gp13");
  
  if (IS_ERR(vreg_ldo20)) {
    pr_err("%s: gp13 vreg get failed (%ld)\n",
           __func__, PTR_ERR(vreg_ldo20));
    return -1;
  }
  
  rc = vreg_set_level(vreg_ldo19, 1800);
  if (rc) {
    pr_err("%s: vreg LDO19 set level failed (%d)\n",
           __func__, rc);
    return -1;
  }
  return 0;
}

static int panel_sony_power(int on)
{
	int rc = 0;

	printk(KERN_INFO "%s: %d\n", __func__, on);

	if (on) {
		rc = vreg_enable(vreg_ldo19);
	}
	if (rc) {
		pr_err("%s: LDO19 vreg enable failed (%d)\n",
		__func__, rc);
		return -1;
	}

	if (on)
		rc = vreg_enable(vreg_ldo20);
	if (rc) {
		pr_err("%s: LDO20 vreg enable failed (%d)\n",
			__func__, rc);
		return -1;
	}

	if (on) {
		hr_msleep(10);
		gpio_set_value(SAGA_LCD_RSTz_ID1, 1);
		hr_msleep(10);
		gpio_set_value(SAGA_LCD_RSTz_ID1, 0);
		udelay(500);
		gpio_set_value(SAGA_LCD_RSTz_ID1, 1);
		hr_msleep(10);
	} else if (!on) {
		hr_msleep(10);
		gpio_set_value(SAGA_LCD_RSTz_ID1, 0);
		hr_msleep(120);
	}

	if(!on) {
		rc = vreg_disable(vreg_ldo19);
		rc = vreg_disable(vreg_ldo20);
	}

	if (rc) {
		pr_err("%s: LDO19, 20 vreg disable failed (%d)\n",
		__func__, rc);
		return -1;
	}

	return 0;
}

static void lcdc_config_gpios(int on)
{
	printk(KERN_INFO "%s: power goes to %d\n", __func__, on);

	if (panel_sony_power(on))
		printk(KERN_ERR "%s: panel_power failed!\n", __func__);
	if (panel_gpio_switch(on))
		printk(KERN_ERR "%s: panel_gpio_switch failed!\n", __func__);
}

static struct msm_panel_common_pdata lcdc_panel_data = {
	.panel_config_gpio = lcdc_config_gpios,
};

struct platform_device lcdc_sonywvga_panel_device = {
	.name   = "lcdc_s6d16a0x21_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_panel_data,
	}
};

static struct msm_panel_common_pdata mddi_renesas_pdata;
static struct platform_device mddi_renesas_device = {
	.name   = "mddi_renesas_R61408_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &mddi_renesas_pdata,
	}
};

static int msm_fb_mddi_sel_clk(u32 *clk_rate)
{
  *clk_rate *= 2;
	return 0;
}

static int
mddi_hitachi_power(u32 on)
{
  printk(KERN_ERR "%s: %d\n", __func__, on);
	if (panel_type == PANEL_ID_SAG_HITACHI) {
          vreg_enable(vreg_ldo19);
          gpio_set_value(SAGA_MDDI_RSTz,0);
          vreg_enable(vreg_ldo20);
          hr_msleep(1);
          gpio_set_value(SAGA_MDDI_RSTz,1);
          hr_msleep(5);
        }
        return 1;
}

static struct mddi_platform_data mddi_pdata = {
  //.mddi_power_save = mddi_hitachi_power,
	.mddi_sel_clk = msm_fb_mddi_sel_clk,
	.mddi_client_power = mddi_hitachi_power,
};

static struct msm_panel_common_pdata mdp_pdata = {
  .hw_revision_addr = 0xac001270,
  .gpio = 30,
  .mdp_max_clk = 192000000,
  .mdp_rev = MDP_REV_40,
};

static int lcdc_panel_power(int on)
{
	int flag_on = !!on;
	static int lcdc_power_save_on;

	if (lcdc_power_save_on == flag_on)
		return 0;

	lcdc_power_save_on = flag_on;

	return panel_sony_power(on);
}

static struct lcdc_platform_data lcdc_pdata = {
	.lcdc_power_save = lcdc_panel_power,
};

struct msm_list_device saga_fb_devices[] = {
  { "mdp", &mdp_pdata },
  { "pmdh", &mddi_pdata },
  { "lcdc", &lcdc_pdata }
};

int device_fb_detect_panel(const char *name)
{
  if (!strcmp(name, "lcdc_s6d16a0x21_wvga") && is_sony_panel())
      return 0;
  if (!strcmp(name, "mddi_renesas_R61408_wvga") && is_hitachi_panel())
    return 0;
}

int __init saga_init_panel(void)
{
  int ret = 0;

  ret = panel_init_power();
  if (ret)
    return ret;

  msm_fb_add_devices(
                     saga_fb_devices, ARRAY_SIZE(saga_fb_devices));
  if (is_sony_panel())
    {
      //    msm_fb_register_device("lcdc", &lcdc_pdata);
      ret = platform_device_register(&lcdc_sonywvga_panel_device);
      printk(KERN_ERR "%s is sony panel: %d\n", __func__, panel_type);
    }
  else
    {
      //      msm_fb_register_device("mddi", &mddi_pdata);
      //      ret = platform_device_register(&mddi_renesas_device);
      printk(KERN_ERR "%s: Panel not yet supported (%d)\n", __func__, panel_type);
    }
  return ret;
}

