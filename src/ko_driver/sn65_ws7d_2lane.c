/*
** Copyright (C) 2022 CNflysky. All rights reserved.
** Kernel DRM driver for Multiple Panels in DSI interface.
*/

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <video/mipi_display.h>

struct dsi_panel_desc {
  const struct drm_display_mode *mode;
  unsigned int lanes;
  unsigned long flags;
  enum mipi_dsi_pixel_format format;
  void (*init_sequence)(struct mipi_dsi_device *dsi);
};

struct dsi_panel {
  struct drm_panel panel;
  struct mipi_dsi_device *dsi;
  const struct dsi_panel_desc *desc;
  //struct gpio_desc *reset;
};

static inline struct dsi_panel *to_dsi_panel(struct drm_panel *panel) {
  return container_of(panel, struct dsi_panel, panel);
}

static inline int panel_dsi_write(struct mipi_dsi_device *dsi, const void *seq,
                                  size_t len) {
  return mipi_dsi_dcs_write_buffer(dsi, seq, len);
}

#define panel_command(dsi, seq...)          \
  {                                         \
    const uint8_t d[] = {seq};              \
    panel_dsi_write(dsi, d, ARRAY_SIZE(d)); \
  }

static void sn65_ws7d_init_sequence(struct mipi_dsi_device *dsi) {
}

/*
static void w280bf036i_init_sequence(struct mipi_dsi_device *dsi) {
}


static void w500ie020_init_sequence(struct mipi_dsi_device *dsi) {
}
*/

static int dsi_panel_prepare(struct drm_panel *panel) {
  struct dsi_panel *dsi_panel = to_dsi_panel(panel);
  //gpiod_set_value_cansleep(dsi_panel->reset, 0);
  //msleep(30);
  //gpiod_set_value_cansleep(dsi_panel->reset, 1);
  //msleep(150);

  mipi_dsi_dcs_soft_reset(dsi_panel->dsi);
  msleep(30);

  dsi_panel->desc->init_sequence(dsi_panel->dsi);

  mipi_dsi_dcs_set_tear_on(dsi_panel->dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
  mipi_dsi_dcs_exit_sleep_mode(dsi_panel->dsi);
  msleep(120);
  return 0;
}

static int dsi_panel_enable(struct drm_panel *panel) {
  return mipi_dsi_dcs_set_display_on(to_dsi_panel(panel)->dsi);
}

static int dsi_panel_disable(struct drm_panel *panel) {
  return mipi_dsi_dcs_set_display_off(to_dsi_panel(panel)->dsi);
}

static int dsi_panel_unprepare(struct drm_panel *panel) {
  struct dsi_panel *dsi_panel = to_dsi_panel(panel);

  mipi_dsi_dcs_enter_sleep_mode(dsi_panel->dsi);

  //gpiod_set_value_cansleep(dsi_panel->reset, 0);

  return 0;
}

static int dsi_panel_get_modes(struct drm_panel *panel,
                               struct drm_connector *connector) {
  struct dsi_panel *dsi_panel = to_dsi_panel(panel);
  const struct drm_display_mode *desc_mode = dsi_panel->desc->mode;
  struct drm_display_mode *mode;

  mode = drm_mode_duplicate(connector->dev, desc_mode);
  if (!mode) {
    dev_err(&dsi_panel->dsi->dev, "failed to add mode %ux%u@%u\n",
            desc_mode->hdisplay, desc_mode->vdisplay,
            drm_mode_vrefresh(desc_mode));
    return -ENOMEM;
  }

  drm_mode_set_name(mode);
  drm_mode_probed_add(connector, mode);

  connector->display_info.width_mm = desc_mode->width_mm;
  connector->display_info.height_mm = desc_mode->height_mm;

  return 1;
}

static const struct drm_panel_funcs dsi_panel_funcs = {
    .disable = dsi_panel_disable,
    .unprepare = dsi_panel_unprepare,
    .prepare = dsi_panel_prepare,
    .enable = dsi_panel_enable,
    .get_modes = dsi_panel_get_modes,
};

static const struct drm_display_mode sn65_ws7d_mode = {
    .clock = 51000,

    .hdisplay = 1024,
    .hsync_start = 1024 + /* HFP */ 10,
    .hsync_end = 1024 + 10 + /* HSync */ 4,
    .htotal = 1024 + 10 + 4 + /* HBP */ 20,

    .vdisplay = 600,
    .vsync_start = 600 + /* VFP */ 8,
    .vsync_end = 600 + 8 + /* VSync */ 4,
    .vtotal = 600 + 8 + 4 + /* VBP */ 14,

    .width_mm = 43,
    .height_mm = 57,

    .type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

//static const struct drm_display_mode w500ie020_mode = {
//    .clock = 20500,
//
//    .hdisplay = 480,
//    .hsync_start = 480 + /* HFP */ 10,
//    .hsync_end = 480 + 10 + /* HSync */ 4,
//    .htotal = 480 + 10 + 4 + /* HBP */ 20,
//
//    .vdisplay = 854,
//    .vsync_start = 854 + /* VFP */ 8,
//    .vsync_end = 854 + 8 + /* VSync */ 4,
//    .vtotal = 854 + 8 + 4 + /* VBP */ 14,
//
//    .width_mm = 43,
//    .height_mm = 58,
//
//    .type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
//};

static const struct dsi_panel_desc sn65_ws7d_desc = {
    .mode = &sn65_ws7d_mode,
    .lanes = 2,
    .flags =
        MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM,
    .format = MIPI_DSI_FMT_RGB888,
    .init_sequence = sn65_ws7d_init_sequence};

/*
static const struct dsi_panel_desc w500ie020_desc = {
    .mode = &w500ie020_mode,
    .lanes = 2,
    .flags =
        MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM,
    .format = MIPI_DSI_FMT_RGB888,
    .init_sequence = w500ie020_init_sequence};
*/
static int dsi_panel_probe(struct mipi_dsi_device *dsi) {
  struct dsi_panel *dsi_panel;
  const struct dsi_panel_desc *desc;
  int ret;

  
  printk("dsi_panel_probe 11111111111\n");
	

  dsi_panel =
      devm_kzalloc(&dsi->dev, sizeof(*dsi_panel), GFP_KERNEL);
  if (!dsi_panel) return -ENOMEM;

  desc = of_device_get_match_data(&dsi->dev);
  dsi->mode_flags = desc->flags;
  dsi->format = desc->format;
  dsi->lanes = desc->lanes;

  //#dsi_panel->panel.prepare_upstream_first = true;
  //dsi_panel->reset = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_LOW);
  //if (IS_ERR(dsi_panel->reset)) {
  //  dev_err(&dsi->dev, "Couldn't get our reset GPIO\n");
  //  return PTR_ERR(dsi_panel->reset);
  //} else
    drm_panel_init(&dsi_panel->panel, &dsi->dev, &dsi_panel_funcs,
                   DRM_MODE_CONNECTOR_DSI);

  ret = drm_panel_of_backlight(&dsi_panel->panel);
  if (ret) return ret;

  drm_panel_add(&dsi_panel->panel);

  mipi_dsi_set_drvdata(dsi, dsi_panel);
  dsi_panel->dsi = dsi;
  dsi_panel->desc = desc;
  if ((ret = mipi_dsi_attach(dsi))) drm_panel_remove(&dsi_panel->panel);

  printk("dsi_panel_probe 22222222222\n");

  return ret;
}

static int dsi_panel_remove(struct mipi_dsi_device *dsi) {
  struct dsi_panel *dsi_panel = mipi_dsi_get_drvdata(dsi);

  mipi_dsi_detach(dsi);
  drm_panel_remove(&dsi_panel->panel);

  return 0;
}

static const struct of_device_id dsi_panel_of_match[] = {
    {.compatible = "gcat,sn65_ws7d", .data = &sn65_ws7d_desc},	
    //{.compatible = "wlk,w280bf036i", .data = &w280bf036i_desc},
    //{.compatible = "wlk,w500ie020", .data = &w500ie020_desc},
    {}};

MODULE_DEVICE_TABLE(of, dsi_panel_of_match);

static struct mipi_dsi_driver dsi_panel_driver = {
    .probe = dsi_panel_probe,
    .remove = dsi_panel_remove,
    .driver =
        {
            .name = "dsi_panel_driver",
            .of_match_table = dsi_panel_of_match,
        },
};
module_mipi_dsi_driver(dsi_panel_driver);

//MODULE_SOFTDEP("pre: gpio_pca953x");
//MODULE_AUTHOR("CNflysky@qq.com");
MODULE_AUTHOR("greatcat@ms18.hinet.net");
MODULE_DESCRIPTION("Raspberry Pi DSI Panels Driver for sn65_ws7d");
MODULE_LICENSE("GPL");
