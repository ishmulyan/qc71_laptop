// SPDX-License-Identifier: GPL-2.0
#include "pr.h"

#include <linux/init.h>
/* #include <linux/led-class-multicolor.h> */
#include <linux/leds.h>
#include <linux/moduleparam.h>
#include <linux/types.h>

#include "util.h"
#include "ec.h"
#include "features.h"
#include "led_keyboard.h"
#include "pdev.h"

static bool keyboard_led_registered;

static ssize_t keyboard_color_show(struct device *dev,
                                   struct device_attribute *attr, char *buf)
{
    uint32_t color = 0;

    int status = ec_read_byte(KBD_BACKLIGHT_RGB_RED_ADDR);

    if (status < 0)
        return status;

    color = status;

    status = ec_read_byte(KBD_BACKLIGHT_RGB_GREEN_ADDR);

    if (status < 0)
        return status;

    color = color | (status<<8);

    status = ec_read_byte(KBD_BACKLIGHT_RGB_BLUE_ADDR);

    if (status < 0)
        return status;

    color = color | (status<<16);

    return sprintf(buf, "%03u\n", color);
}

static ssize_t keyboard_color_store(struct device *dev, struct device_attribute *attr,
                                    const char *buf, size_t count)
{
    uint32_t color;
    int status;

    if (kstrtouint(buf, 0, &color))
        return -EINVAL;

    status = ec_write_byte(KBD_BACKLIGHT_RGB_RED_ADDR, color & 0x000000ff);
    if (status < 0)
        return status;

    status = ec_write_byte(KBD_BACKLIGHT_RGB_GREEN_ADDR, (color & 0x0000ff00) >> 8);
    if (status < 0)
        return status;

    status = ec_write_byte(KBD_BACKLIGHT_RGB_BLUE_ADDR, (color & 0x00ff0000) >> 16);
    if (status < 0)
        return status;

    return count;
}

static enum led_brightness qc71_keyboard_led_get_brightness(struct led_classdev *led_cdev)
{
    return LED_FULL;
}

static int qc71_keyboard_led_set_brightness(struct led_classdev *led_cdev,
                                            enum led_brightness value)
{
    return 0;
}

/* ========================================================================== */

static DEVICE_ATTR(color,         0644, keyboard_color_show,   keyboard_color_store);

static struct attribute *qc71_keyboard_led_attrs[] = {
    &dev_attr_color.attr,
    NULL
};

ATTRIBUTE_GROUPS(qc71_keyboard_led);

static struct led_classdev qc71_keyboard_led = {
    .name                    = KBUILD_MODNAME "::kbd_backlight",
    .max_brightness          = 1,
    .brightness_get          = qc71_keyboard_led_get_brightness,
    .brightness_set_blocking = qc71_keyboard_led_set_brightness,
    .groups                  = qc71_keyboard_led_groups,
};

/* ========================================================================== */

int __init qc71_led_keyboard_setup(void)
{
    int err;

    if (!qc71_features.kbd_backlight_rgb)
        return -ENODEV;

    err = led_classdev_register(&qc71_platform_dev->dev, &qc71_keyboard_led);

    if (!err)
        keyboard_led_registered = true;

    return err;
}

void qc71_led_keyboard_cleanup(void)
{
    if (keyboard_led_registered) {
        led_classdev_unregister(&qc71_keyboard_led);
    }
}
