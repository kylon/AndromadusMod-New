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
#include <mach/msm_panel.h>
#include <mach/vreg.h>
#include <mach/panel_id.h>

#include "../board-vision.h"
#include "../devices.h"
#include "../proc_comm.h"

struct vreg *vreg_ldo19, *vreg_ldo20;
struct vreg *vreg_ldo12;

#define LCM_GPIO_CFG(gpio, func)                                        \
  PCOM_GPIO_CFG(gpio, func, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_4MA)
static uint32_t display_on_gpio_table[] = {
  LCM_GPIO_CFG(VISION_LCD_PCLK, 1),
  LCM_GPIO_CFG(VISION_LCD_DE, 1),
  LCM_GPIO_CFG(VISION_LCD_VSYNC, 1),
  LCM_GPIO_CFG(VISION_LCD_HSYNC, 1),
  LCM_GPIO_CFG(VISION_LCD_G2, 1),
  LCM_GPIO_CFG(VISION_LCD_G3, 1),
  LCM_GPIO_CFG(VISION_LCD_G4, 1),
  LCM_GPIO_CFG(VISION_LCD_G5, 1),
  LCM_GPIO_CFG(VISION_LCD_G6, 1),
  LCM_GPIO_CFG(VISION_LCD_G7, 1),
  LCM_GPIO_CFG(VISION_LCD_B3, 1),
  LCM_GPIO_CFG(VISION_LCD_B4, 1),
  LCM_GPIO_CFG(VISION_LCD_B5, 1),
  LCM_GPIO_CFG(VISION_LCD_B6, 1),
  LCM_GPIO_CFG(VISION_LCD_B7, 1),
  LCM_GPIO_CFG(VISION_LCD_R3, 1),
  LCM_GPIO_CFG(VISION_LCD_R4, 1),
  LCM_GPIO_CFG(VISION_LCD_R5, 1),
  LCM_GPIO_CFG(VISION_LCD_R6, 1),
  LCM_GPIO_CFG(VISION_LCD_R7, 1),
};

static uint32_t display_off_gpio_table[] = {
  LCM_GPIO_CFG(VISION_LCD_PCLK, 0),
  LCM_GPIO_CFG(VISION_LCD_DE, 0),
  LCM_GPIO_CFG(VISION_LCD_VSYNC, 0),
  LCM_GPIO_CFG(VISION_LCD_HSYNC, 0),
  LCM_GPIO_CFG(VISION_LCD_G2, 0),
  LCM_GPIO_CFG(VISION_LCD_G3, 0),
  LCM_GPIO_CFG(VISION_LCD_G4, 0),
  LCM_GPIO_CFG(VISION_LCD_G5, 0),
  LCM_GPIO_CFG(VISION_LCD_G6, 0),
  LCM_GPIO_CFG(VISION_LCD_G7, 0),
  LCM_GPIO_CFG(VISION_LCD_B0, 0),
  LCM_GPIO_CFG(VISION_LCD_B3, 0),
  LCM_GPIO_CFG(VISION_LCD_B4, 0),
  LCM_GPIO_CFG(VISION_LCD_B5, 0),
  LCM_GPIO_CFG(VISION_LCD_B6, 0),
  LCM_GPIO_CFG(VISION_LCD_B7, 0),
  LCM_GPIO_CFG(VISION_LCD_R0, 0),
  LCM_GPIO_CFG(VISION_LCD_R3, 0),
  LCM_GPIO_CFG(VISION_LCD_R4, 0),
  LCM_GPIO_CFG(VISION_LCD_R5, 0),
  LCM_GPIO_CFG(VISION_LCD_R6, 0),
  LCM_GPIO_CFG(VISION_LCD_R7, 0),
};

static uint32_t display_gpio_table[] = {
  VISION_LCD_PCLK,
  VISION_LCD_DE,
  VISION_LCD_VSYNC,
  VISION_LCD_HSYNC,
  VISION_LCD_G2,
  VISION_LCD_G3,
  VISION_LCD_G4,
  VISION_LCD_G5,
  VISION_LCD_G6,
  VISION_LCD_G7,
  VISION_LCD_B0,
  VISION_LCD_B3,
  VISION_LCD_B4,
  VISION_LCD_B5,
  VISION_LCD_B6,
  VISION_LCD_B7,
  VISION_LCD_R0,
  VISION_LCD_R3,
  VISION_LCD_R4,
  VISION_LCD_R5,
  VISION_LCD_R6,
  VISION_LCD_R7,
};

extern unsigned long msm_fb_base;

inline int is_samsung_panel(void)
{
  return (panel_type == SAMSUNG_PANEL || panel_type == SAMSUNG_PANELII)? 1 : 0;
}

static inline int is_sony_panel(void)
{
  return (panel_type == SONY_PANEL_SPI)? 1 : 0;
}

int panel_power_on(void)
{
  int rc;

  /* turn on L19 for OJ. Note: must before L12 */
  rc = vreg_enable(vreg_ldo19);
  if (rc) {
    pr_err("%s: LDO19 vreg enable failed (%d)\n",
           __func__, rc);
    return -1;
  }
  hr_msleep(5);
  rc = vreg_enable(vreg_ldo12);
  if (rc) {
    pr_err("%s: LDO12 vreg enable failed (%d)\n",
           __func__, rc);
    return -1;
  }
  hr_msleep(5);
  rc = vreg_enable(vreg_ldo20);
  if (rc) {
    pr_err("%s: LDO20 vreg enable failed (%d)\n",
           __func__, rc);
    return -1;
  }
  hr_msleep(5);

  if (is_samsung_panel())
    {
      hr_msleep(5);
      gpio_set_value(VISION_LCD_RSTz, 1);
      hr_msleep(25);
      gpio_set_value(VISION_LCD_RSTz, 0);
      hr_msleep(10);
      gpio_set_value(VISION_LCD_RSTz, 1);
      hr_msleep(20);
      /* XA, XB board has HW panel issue, need to set EL_EN pin */
      if(system_rev <= 1)
        gpio_set_value(VISION_EL_EN, 1);
    }
  else
    {
      hr_msleep(10);
      gpio_set_value(VISION_LCD_RSTz, 1);
      hr_msleep(10);
      gpio_set_value(VISION_LCD_RSTz, 0);
      udelay(500);
      gpio_set_value(VISION_LCD_RSTz, 1);
      hr_msleep(10);
    }

  return 0;
}

int panel_power_off(void)
{
  int rc;

  if (is_samsung_panel())
    {
      hr_msleep(5);
      if(system_rev <= 1)
        gpio_set_value(VISION_EL_EN, 0);
      gpio_set_value(VISION_LCD_RSTz, 0);
    }
  else
    {
      hr_msleep(10);
      gpio_set_value(VISION_LCD_RSTz, 0);
      hr_msleep(120);
    }

  rc = vreg_disable(vreg_ldo12);
  if (rc)
    {
      pr_err("%s: LDO12, 19, 20 vreg disable failed (%d)\n",
             __func__, rc);
      return -1;
    }
  hr_msleep(5);
  rc = vreg_disable(vreg_ldo19);
  if (rc)
    {
      pr_err("%s: LDO12, 19, 20 vreg disable failed (%d)\n",
             __func__, rc);
      return -1;
    }
  hr_msleep(5);
  rc = vreg_disable(vreg_ldo20);
  if (rc)
    {
      pr_err("%s: LDO12, 19, 20 vreg disable failed (%d)\n",
             __func__, rc);
      return -1;
    }
  hr_msleep(5);
  return 0;
}

int panel_power(int on)
{
  int rc;

  if (on)
    rc = panel_power_on();
  if (!on)
    rc = panel_power_off();

  printk(KERN_ERR "%s: Panel power %s=%d\n", __func__, (on == 1 ? "ON" : "OFF"), rc);

  return rc;
}

int device_fb_detect_panel(const char *name)
{
  if (!strcmp(name, "lcdc_s6d16a0x21_wvga") && is_sony_panel())
    return 0;
  if (!strcmp(name, "lcdc_tl2796a_wvga") && (panel_type == SAMSUNG_PANEL))
    return 0;
  if (!strcmp(name, "lcdc_s6e63m0_wvga") && (panel_type == SAMSUNG_PANELII))
    return 0;
  return -ENODEV;
}

static int panel_gpio_switch(int on)
{
  uint32_t pin, id;

  config_gpio_table(
                    !!on ? display_on_gpio_table : display_off_gpio_table,
                    ARRAY_SIZE(display_on_gpio_table));

  if (!on) {
    for (pin = 0; pin < ARRAY_SIZE(display_gpio_table); pin++) {
      gpio_set_value(display_gpio_table[pin], 0);
    }
    id = PCOM_GPIO_CFG(VISION_LCD_R6, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_4MA);
    msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &id, 0);
    id = PCOM_GPIO_CFG(VISION_LCD_R7, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_4MA);
    msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &id, 0);
    gpio_set_value(VISION_LCD_R6, 0);
    gpio_set_value(VISION_LCD_R7, 0);
  }
  return 0;
}

/* a hacky interface to control the panel power */
static void lcdc_config_gpios(int on)
{
	printk(KERN_INFO "%s: power goes to %d\n", __func__, on);

	if (panel_power(on))
		printk(KERN_ERR "%s: panel_power failed!\n", __func__);
	if (panel_gpio_switch(on))
		printk(KERN_ERR "%s: panel_gpio_switch failed!\n", __func__);
}

static struct msm_panel_common_pdata lcdc_panel_data = {
	.panel_config_gpio = lcdc_config_gpios,
};

static struct platform_device lcdc_sonywvga_panel_device = {
	.name   = "lcdc_s6d16a0x21_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_panel_data,
	}
};

static struct platform_device lcdc_tl2796a_panel_device = {
	.name   = "lcdc_tl2796a_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_panel_data,
	}
};

static struct platform_device lcdc_s6e63m0_panel_device = {
	.name   = "lcdc_s6e63m0_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_panel_data,
	}
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

	return panel_power(on);
}

static struct lcdc_platform_data lcdc_pdata = {
	.lcdc_power_save = lcdc_panel_power,
};

struct msm_list_device vision_fb_devices[] = {
  { "mdp", &mdp_pdata },
  { "lcdc", &lcdc_pdata }
};

static int panel_init_power(void)
{
  int ret = 0;

  vreg_ldo12 = vreg_get(NULL, "gp9");
  if (IS_ERR(vreg_ldo12)) {
    pr_err("%s: gp9 vreg get failed (%ld)\n",
           __func__, PTR_ERR(vreg_ldo12));
    return -1;
  }
  ret = vreg_set_level(vreg_ldo12, 2850);
  if (ret) {
    pr_err("%s: vreg LDO12(gp9) set level failed (%d)\n",
           __func__, ret);
    return -1;
  }

  vreg_ldo19 = vreg_get(NULL, "wlan2");
  if (IS_ERR(vreg_ldo19)) {
    pr_err("%s: wlan2 vreg get failed (%ld)\n",
           __func__, PTR_ERR(vreg_ldo19));
    return -1;
  }
  vreg_ldo20 = vreg_get(NULL, "gp13");

  if (IS_ERR(vreg_ldo20)) {
    pr_err("%s: gp13 vreg get failed (%ld)\n",
           __func__, PTR_ERR(vreg_ldo20));
    return -1;
  }

  ret = vreg_set_level(vreg_ldo19, 1800);
  if (ret) {
    pr_err("%s: vreg LDO19 set level failed (%d)\n",
           __func__, ret);
    return -1;
  }

  if(is_samsung_panel())
    ret = vreg_set_level(vreg_ldo20, 2850);
  else
    ret = vreg_set_level(vreg_ldo20, 2600);
  if (ret) {
    pr_err("%s: vreg LDO20 set level failed (%d)\n",
           __func__, ret);
    return -1;
  }
  return 0;
}

int vision_init_panel(void)
{
  int ret = 0;

  printk(KERN_ERR "%s: Sony=%d Samsung=%d Other=%d\n", __func__, is_sony_panel(), panel_type == SAMSUNG_PANEL, panel_type != SAMSUNG_PANEL && !is_sony_panel());

  ret = panel_init_power();
  if (ret)
    return ret;

  msm_fb_add_devices(
                     vision_fb_devices, ARRAY_SIZE(vision_fb_devices));

  if (is_sony_panel())
    {
      ret = platform_device_register(&lcdc_sonywvga_panel_device);
      printk(KERN_ERR "%s: registered sony panel: %d\n", __func__, ret);
    }
  else if (panel_type == SAMSUNG_PANEL)
    {
      ret = platform_device_register(&lcdc_tl2796a_panel_device);
      printk(KERN_ERR "%s: registered tl2796a panel: %d\n", __func__, ret);
    }
  else
    {
      ret = platform_device_register(&lcdc_s6e63m0_panel_device);
      printk(KERN_ERR "%s: registered s6e63m0 panel: %d\n", __func__, ret);
    }
  return ret;
}
