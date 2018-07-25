/*
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <gpio.h>
#include <sensor.h>
#include <board.h>
#include <display/cfb.h>
#include <misc/printk.h>
#include <misc/util.h>

#include <stdio.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

struct channel_info {
	int chan;
	char *dev_name;
};

/* change device names if you want to use different sensors */
static struct channel_info info[] = {
	{SENSOR_CHAN_AMBIENT_TEMP, "HDC1008"},
	{SENSOR_CHAN_HUMIDITY, "HDC1008"},
	{SENSOR_CHAN_ACCEL_XYZ, CONFIG_FXOS8700_NAME},
	{SENSOR_CHAN_LIGHT, CONFIG_APDS9960_DRV_NAME},
	{SENSOR_CHAN_PROX, CONFIG_APDS9960_DRV_NAME},
};

#define DEV_HDC1008_TEMP	0
#define DEV_HDC1008_HUM		1
#define DEV_MMA8652		2
#define DEV_APDS9960_L		3
#define DEV_APDS9960_P		4

#define CH_HDC1008_TEMP		0
#define CH_HDC1008_HUM		1
#define CH_MMA8652_X		2
#define CH_MMA8652_Y		3
#define CH_MMA8652_Z		4
#define CH_APDS9960_L		5
#define CH_APDS9960_P		6

/*
 * Set Advertisement data. Based on the Eddystone specification:
 * https://github.com/google/eddystone/blob/master/protocol-specification.md
 * https://github.com/google/eddystone/tree/master/eddystone-url
 */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16,
		      0xaa, 0xfe, /* Eddystone UUID */
		      0x10, /* Eddystone-URL frame type */
		      0x00, /* Calibrated Tx power at 0m */
		      0x00, /* URL Scheme Prefix http://www. */
		      'z', 'e', 'p', 'h', 'y', 'r',
		      'p', 'r', 'o', 'j', 'e', 'c', 't',
		      0x08) /* .org */
};

/* Set Scan Response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Beacon started\n");
}
void main(void)
{
	struct device *epd_dev;
	struct device *led1_dev;
	struct device *led2_dev;
	struct device *led3_dev;
	struct device *dev[ARRAY_SIZE(info)];
	struct sensor_value val[ARRAY_SIZE(info) + 2];
	char str_buf[256];
	unsigned int i;
	int rc;
	u8_t font_height;
	int err;

	led1_dev = device_get_binding(LED1_GPIO_CONTROLLER);
	gpio_pin_configure(led1_dev, LED1_GPIO_PIN, GPIO_DIR_OUT);

	led2_dev = device_get_binding(LED2_GPIO_CONTROLLER);
	gpio_pin_configure(led2_dev, LED2_GPIO_PIN, GPIO_DIR_OUT);

	led3_dev = device_get_binding(LED3_GPIO_CONTROLLER);
	gpio_pin_configure(led3_dev, LED3_GPIO_PIN, GPIO_DIR_OUT);

	gpio_pin_write(led1_dev, LED1_GPIO_PIN, 1);
	gpio_pin_write(led2_dev, LED2_GPIO_PIN, 1);
	gpio_pin_write(led3_dev, LED3_GPIO_PIN, 1);

	gpio_pin_write(led1_dev, LED1_GPIO_PIN, 0);
	k_sleep(500);
	gpio_pin_write(led1_dev, LED1_GPIO_PIN, 1);

	gpio_pin_write(led2_dev, LED2_GPIO_PIN, 0);
	k_sleep(500);
	gpio_pin_write(led2_dev, LED2_GPIO_PIN, 1);

	gpio_pin_write(led3_dev, LED3_GPIO_PIN, 0);
	k_sleep(500);
	gpio_pin_write(led3_dev, LED3_GPIO_PIN, 1);

	for (i = 0; i < ARRAY_SIZE(info); i++) {
		dev[i] = device_get_binding(info[i].dev_name);
		if (dev[i] == NULL) {
			printk("Failed to get %s device\n", info[i].dev_name);
			return;
		}
	}

	epd_dev = device_get_binding(CONFIG_SSD1673_DEV_NAME);
	if (epd_dev == NULL) {
		printk("SSD1673 device not found\n");
		return;
	}

	if (cfb_framebuffer_init(epd_dev)) {
		printk("Framebuffer initialization failedn");
		return;
	}

	cfb_framebuffer_set_font(epd_dev, 1);
	cfb_framebuffer_clear(epd_dev, true);
	cfb_framebuffer_finalize(epd_dev);

	printk("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
		cfb_get_display_parameter(epd_dev, CFB_DISPLAY_WIDTH),
		cfb_get_display_parameter(epd_dev, CFB_DISPLAY_HEIGH),
		cfb_get_display_parameter(epd_dev, CFB_DISPLAY_PPT),
		cfb_get_display_parameter(epd_dev, CFB_DISPLAY_ROWS),
		cfb_get_display_parameter(epd_dev, CFB_DISPLAY_COLS));

	cfb_framebuffer_clear(epd_dev, false);
	if (cfb_print(epd_dev, "I am reel board.", 0, 0)) {
		printk("Failed to print a string\n");
		return;
	}

	cfb_framebuffer_finalize(epd_dev);
	cfb_get_font_size(epd_dev, 2, NULL, &font_height);
	cfb_framebuffer_set_font(epd_dev, 2);

	printk("Starting Beacon Demo\n");
	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	while (1) {
		/* fetch sensor samples */
		for (i = 0; i < ARRAY_SIZE(info); i++) {
			rc = sensor_sample_fetch(dev[i]);
			if (rc) {
				printk("Failed to fetch sample for device %s\n",
				       info[i].dev_name);
				return;
			}
		}

		if (sensor_channel_get(dev[DEV_HDC1008_TEMP],
				       info[DEV_HDC1008_TEMP].chan,
				       &val[CH_HDC1008_TEMP])) {
			goto _error_get;
		}

		if (sensor_channel_get(dev[DEV_HDC1008_HUM],
				       info[DEV_HDC1008_HUM].chan,
				       &val[CH_HDC1008_HUM])) {
			goto _error_get;
		}

		if (sensor_channel_get(dev[DEV_MMA8652],
				       info[DEV_MMA8652].chan,
				       &val[CH_MMA8652_X])) {
			goto _error_get;
		}

		if (sensor_channel_get(dev[DEV_APDS9960_L],
				       info[DEV_APDS9960_L].chan,
				       &val[CH_APDS9960_L])) {
			goto _error_get;
		}

		if (sensor_channel_get(dev[DEV_APDS9960_P],
				       info[DEV_APDS9960_P].chan,
				       &val[CH_APDS9960_P])) {
			goto _error_get;
		}

		cfb_framebuffer_clear(epd_dev, false);

		printk("Temperature: %d.%d C | RH: %d%%\n",
			val[CH_HDC1008_TEMP].val1,
			val[CH_HDC1008_TEMP].val2/100000,
			val[CH_HDC1008_HUM].val1);
		snprintf(str_buf, sizeof(str_buf),
			 "Temperature:%d.%d C\n",
			 val[CH_HDC1008_TEMP].val1,
			 val[CH_HDC1008_TEMP].val2/100000);
		if (cfb_print(epd_dev, str_buf, 0, 0)) {
			goto _error_get;
		}

		snprintf(str_buf, sizeof(str_buf),
			 "Humidity:%d%%\n",
			 val[CH_HDC1008_HUM].val1);
		if (cfb_print(epd_dev, str_buf, 0, font_height * 1)) {
			goto _error_get;
		}

		printf("AX=%10.6f AY=%10.6f AZ=%10.6f\n",
		       sensor_value_to_double(&val[CH_MMA8652_X]),
		       sensor_value_to_double(&val[CH_MMA8652_Y]),
		       sensor_value_to_double(&val[CH_MMA8652_Z]));
		snprintf(str_buf, sizeof(str_buf), "AX :%10.3f\n",
			 sensor_value_to_double(&val[CH_MMA8652_X]));
		if (cfb_print(epd_dev, str_buf, 0, font_height * 2)) {
			goto _error_get;
		}

		snprintf(str_buf, sizeof(str_buf), "AY :%10.3f\n",
			 sensor_value_to_double(&val[CH_MMA8652_Y]));
		if (cfb_print(epd_dev, str_buf, 0, font_height * 3)) {
			goto _error_get;
		}

		snprintf(str_buf, sizeof(str_buf), "AZ :%10.3f\n",
			 sensor_value_to_double(&val[CH_MMA8652_Z]));
		if (cfb_print(epd_dev, str_buf, 0, font_height * 4)) {
			goto _error_get;
		}

		printk("ambient light intensity: %d, proximity %d\n",
		       val[CH_APDS9960_L].val1, val[CH_APDS9960_P].val1);
		snprintf(str_buf, sizeof(str_buf),
			 "Light :%d, Proximity:%d\n",
			 val[CH_APDS9960_L].val1, val[CH_APDS9960_P].val1);
		if (cfb_print(epd_dev, str_buf, 0, font_height * 5)) {
			goto _error_get;
		}

		cfb_framebuffer_finalize(epd_dev);

		k_sleep(K_MSEC(1000));
	}

_error_get:
	printk("Failed to get sensor data or print a string\n");
}
