/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for display drivers and applications
 */

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

/**
 * @brief Display Interface
 * @defgroup display_interface display Interface
 * @ingroup io_interfaces
 * @{
 */

#include <device.h>
#include <stddef.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum display_pixel_format {
	PIXEL_FORMAT_RGB_888		= BIT(0),
	PIXEL_FORMAT_MONO01		= BIT(1), /* 0=Black 1=White */
	PIXEL_FORMAT_MONO10		= BIT(2), /* 1=Black 0=White */
};

enum display_screen_info {
	/**
	 * If selected, one octet represents 8 pixels ordered vertically,
	 * otherwise ordered horizontally.
	 */
	SCREEN_INFO_MONO_VTILED		= BIT(0),
	/**
	 * If selected, the MSB represents the first pixel,
	 * otherwise MSB represents the last pixel.
	 */
	SCREEN_INFO_MONO_MSB_FIRST	= BIT(1),
	/**
	 * Electrophoretic Display.
	 */
	SCREEN_INFO_EPD			= BIT(2),
};

/**
 * @struct display_capabilities
 * @brief Structure holding display capabilities
 *
 * @var display_capabilities::x_resolution
 * Display resolution in the X direction
 *
 * @var display_capabilities::y_resolution
 * Display resolution in the Y direction
 *
 * @var display_capabilities::supported_pixel_formats
 * Bitwise or of pixel formats supported by the display
 *
 * @var display_capabilities::screen_info
 * Information about display panel
 *
 * @var display_capabilities::current_pixel_format
 * Currently active pixel format for the display
 *
 */
struct display_capabilities {
	u16_t x_resolution;
	u16_t y_resolution;
	u32_t supported_pixel_formats;
	u32_t screen_info;
	enum display_pixel_format current_pixel_format;
};

/**
 * @typedef display_on_api
 * @brief Callback API to turn display on
 * See display_on() for argument description
 */
typedef int (*display_on_api)(const struct device *dev);

/**
 * @typedef display_off_api
 * @brief Callback API to turn display off
 * See display_off() for argument description
 */
typedef int (*display_off_api)(const struct device *dev);

/**
 * @typedef display_write_bitmap_api
 * @brief Callback API for writing bitmap
 * See display_write_bitmap() for argument description
 */
typedef int (*display_write_bitmap_api)(const struct device *dev, const u16_t x,
					const u16_t y, const u16_t w,
					const u16_t h, const u16_t p,
					const void *data);

/**
 * @typedef display_read_bitmap_api
 * @brief Callback API for reading bitmap
 * See display_read_bitmap() for argument description
 */
typedef int (*display_read_bitmap_api)(const struct device *dev, const u16_t x,
				       const u16_t y, const u16_t w,
				       const u16_t h, const u16_t p,
				       void *data);

/**
 * @typedef display_get_framebuffer_api
 * @brief Callback API to get framebuffer pointer
 * See display_get_framebuffer() for argument description
 */
typedef void *(*display_get_framebuffer_api)(const struct device *dev);

/**
 * @typedef display_set_brightness_api
 * @brief Callback API to set display brightness
 * See display_set_brightness() for argument description
 */
typedef int (*display_set_brightness_api)(const struct device *dev,
					  const u8_t brightness);

/**
 * @typedef display_set_contrast_api
 * @brief Callback API to set display contrast
 * See display_set_contrast() for argument description
 */
typedef int (*display_set_contrast_api)(const struct device *dev,
					const u8_t contrast);

/**
 * @typedef display_get_capabilities_api
 * @brief Callback API to get display capabilities
 * See display_get_capabilities() for argument description
 */
typedef void (*display_get_capabilities_api)(const struct device *dev,
		struct display_capabilities *capabilities);

/**
 * @typedef display_set_pixel_format_api
 * @brief Callback API to set pixel format used by the display
 * See display_set_pixel_format() for argument description
 */
typedef int (*display_set_pixel_format_api)(const struct device *dev,
		const enum display_pixel_format pixel_format);

/**
 * @brief Display driver API
 * API which a display driver should expose
 */
struct display_driver_api {
	display_on_api display_on;
	display_off_api display_off;
	display_write_bitmap_api write_bitmap;
	display_read_bitmap_api read_bitmap;
	display_get_framebuffer_api get_framebuffer;
	display_set_brightness_api set_brightness;
	display_set_contrast_api set_contrast;
	display_get_capabilities_api get_capabilities;
	display_set_pixel_format_api set_pixel_format;
};

/**
 * @brief Write bitmap
 *
 * @param dev Pointer to device structure
 * @param x x coordinate of the upper left corner
 * @param y y coordinate of the upper left corner
 * @param w width of the bitmap in pixels
 * @param h height of the bitmap in pixels
 * @param p pitch between lines in pixels
 * @param data pointer to the data array, the data array should be at
 * minimum p * h * bytes per pixel
 *
 * @retval 0 on success else negative errno code.
 */
inline int display_write_bitmap(const struct device *dev, const u16_t x,
				const u16_t y, const u16_t w, const u16_t h,
				const u16_t p, const void *data)
{
	struct display_driver_api *api =
	    (struct display_driver_api *)dev->driver_api;
	return api->write_bitmap(dev, x, y, w, h, p, data);
}

/**
 * @brief Read bitmap
 *
 * @param dev Pointer to device structure
 * @param x x coordinate of the upper left corner
 * @param y y coordinate of the upper left corner
 * @param w width of the bitmap in pixels
 * @param h height of the bitmap in pixels
 * @param p pitch between lines in pixels
 * @param data pointer to the data array, the data array should be at
 * minimum p * h * bytes per pixel
 *
 * @retval 0 on success else negative errno code.
 */
inline int display_read_bitmap(const struct device *dev, const u16_t x,
			       const u16_t y, const u16_t w, const u16_t h,
			       const u16_t p, void *data)
{
	struct display_driver_api *api =
	    (struct display_driver_api *)dev->driver_api;
	return api->read_bitmap(dev, x, y, w, h, p, data);
}

/**
 * @brief Get pointer to framebuffer for direct access
 *
 * @param dev Pointer to device structure
 *
 * @retval Pointer to frame buffer or NULL if direct framebuffer access
 * is not supported
 *
 */
inline void *display_get_framebuffer(const struct device *dev)
{
	struct display_driver_api *api =
	    (struct display_driver_api *)dev->driver_api;
	return api->get_framebuffer(dev);
}

/**
 * @brief Turn display on
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success else negative errno code.
 */
inline int display_on(const struct device *dev)
{
	struct display_driver_api *api =
	    (struct display_driver_api *)dev->driver_api;
	return api->display_on(dev);
}

/**
 * @brief Turn display off
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success else negative errno code.
 */
inline int display_off(const struct device *dev)
{
	struct display_driver_api *api =
	    (struct display_driver_api *)dev->driver_api;
	return api->display_off(dev);
}

/**
 * @brief Set the brightness of the display
 *
 * Set the brightness of the display in steps of 1/256, where 255 is full
 * brightness and 0 is minimal.
 *
 * @param dev Pointer to device structure
 * @param brightness Brightness in steps of 1/256
 *
 * @retval 0 on success else negative errno code.
 */
inline int display_set_brightness(const struct device *dev, u8_t brightness)
{
	struct display_driver_api *api =
	    (struct display_driver_api *)dev->driver_api;
	return api->set_brightness(dev, brightness);
}

/**
 * @brief Set the contrast of the display
 *
 * Set the contrast of the display in steps of 1/256, where 255 is maximum
 * difference and 0 is minimal.
 *
 * @param dev Pointer to device structure
 * @param contrast Contrast in steps of 1/256
 *
 * @retval 0 on success else negative errno code.
 */
inline int display_set_contrast(const struct device *dev, u8_t contrast)
{
	struct display_driver_api *api =
	    (struct display_driver_api *)dev->driver_api;
	return api->set_contrast(dev, contrast);
}

/**
 * @brief Get display capabilities
 *
 * @param dev Pointer to device structure
 * @param capabilities Pointer to capabilities structure to populate
 */
inline void display_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	struct display_driver_api *api =
	    (struct display_driver_api *)dev->driver_api;
	api->get_capabilities(dev, capabilities);
}

/**
 * @brief Set pixel format used by the display
 *
 * @param dev Pointer to device structure
 * @param pixel_format Pixel format to be used by display
 *
 * @retval 0 on success else negative errno code.
 */
inline int
display_set_pixel_format(const struct device *dev,
			 const enum display_pixel_format pixel_format)
{
	struct display_driver_api *api =
	    (struct display_driver_api *)dev->driver_api;
	api->set_pixel_format(dev, pixel_format);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __DISPLAY_H__*/
