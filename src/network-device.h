/*  network-device.h
 *  vim:ts=4:sw=4:noexpandtab:cindent
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Netspeed Applet was writen by Jörgen Scheibengruber <mfcn@gmx.de>
 */

#ifndef __NETWORK_DEVICE_H__
#define __NETWORK_DEVICE_H__

#include <glib.h>
#include <glib-object.h>

#include "backend.h"

G_BEGIN_DECLS

#define NETWORK_DEVICE_TYPE            (network_device_get_type ())
#define NETWORK_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NETWORK_DEVICE_TYPE, NetworkDevice))
#define NETWORK_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NETWORK_DEVICE_TYPE, NetworkDeviceClass))
#define IS_NETWORK_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NETWORK_DEVICE_TYPE))
#define IS_NETWORK_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NETWORK_DEVICE_TYPE))
#define NETWORK_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NETWORK_DEVICE_TYPE, NetworkDeviceClass))

typedef struct _NetworkDevice        NetworkDevice;
typedef struct _NetworkDeviceClass   NetworkDeviceClass;
typedef struct _NetworkDevicePrivate NetworkDevicePrivate;

/* Different types of interfaces */
typedef enum
{
	NETWORK_DEVICE_TYPE_LO,
	NETWORK_DEVICE_TYPE_ETHERNET,
	NETWORK_DEVICE_TYPE_WIRELESS,
	NETWORK_DEVICE_TYPE_PPP,
	NETWORK_DEVICE_TYPE_PLIP,
	NETWORK_DEVICE_TYPE_SLIP,
	NETWORK_DEVICE_TYPE_UNKNOWN
} NetworkDeviceType;

typedef enum
{
	NETWORK_DEVICE_STATE_UP = 1 << 0,
	NETWORK_DEVICE_STATE_RUNNING = 1 << 1
} NetworkDeviceState;

struct _NetworkDeviceClass
{
	GObjectClass parent_class;

	void (*changed)(void);
};

struct _NetworkDevice
{
	GObject parent;
	NetworkDevicePrivate *priv;
};

GType network_device_get_type (void);

NetworkDevice* network_device_new (const char *name);
const char* network_device_name (NetworkDevice* device);

G_END_DECLS

#endif
