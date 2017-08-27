/*
 * mux/usb.h - definitions for USB multiplexers
 *
 * Copyright (C) 2017 Hans de Goede <hdegoede@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _LINUX_MUX_USB_H
#define _LINUX_MUX_USB_H

/* Mux state values for USB device/host role muxes */
#define MUX_USB_DEVICE		(0) /* USB device mode */
#define MUX_USB_HOST		(1) /* USB host mode */
#define MUX_USB_STATES		(2)

/*
 * Mux state values for Type-C polarity/role/altmode muxes.
 *
 * MUX_TYPEC_POLARITY_INV may be or-ed together with any other mux-state as
 * inverted-polarity (Type-C plugged in upside down) can happen with any
 * other mux-state.
 */
#define MUX_TYPEC_POLARITY_INV		BIT(0)   /* Polarity inverted bit */
#define MUX_TYPEC_DEVICE		(0 << 1) /* USB device mode */
#define MUX_TYPEC_HOST			(1 << 1) /* USB host mode */
#define MUX_TYPEC_HOST_AND_DP_SRC	(2 << 1) /* USB host + 2 lanes DP src */
#define MUX_TYPEC_DP_SRC		(3 << 1) /* 4 lanes Display Port src */
#define MUX_TYPEC_STATES		(4 << 1)

#endif /* _LINUX_MUX_TYPEC_H */
