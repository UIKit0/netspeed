/*  network-device.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(sun) && defined(__SVR4)

#include <sys/sockio.h>
#endif

#include <glibtop/netlist.h>
#include <glibtop/netload.h>

#ifdef HAVE_IW
 #include <iwlib.h>
#endif /* HAVE_IW */

#include "network-device.h"

enum
{
	PROP_0, /* dummy */
	PROP_TYPE,
	PROP_NAME,
	PROP_IPV4_ADDR,
	PROP_IPV6_ADDR,
	PROP_NETMASK,
	PROP_PTP_ADDR,
	PROP_HW_ADDR,
	PROP_WIFI_ESSID,
	PROP_WIFI_QUALITY,
	PROP_STATE,
	PROP_TX,
	PROP_RX
};

enum
{
	CHANGED,
	LAST
};

struct _NetworkDevicePrivate
{
	NetworkDeviceType type;
	char *name;
	char *ipv4_addr;
	char *ipv6_addr;
	char *netmask;
	char *ptp_addr;
	char *hw_addr;
	struct {
		char *essid;
		int	quality;
	} wifi;
	guint state;
	guint64 tx, rx;

	guint timeout_id;
};

#define NETWORK_DEVICE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), NETWORK_DEVICE_TYPE, NetworkDevicePrivate))

static guint signals[LAST] = {0};

static void network_device_class_init (NetworkDeviceClass *klass);
static void network_device_init       (NetworkDevice *self);
static void network_device_finalize   (GObject *object);
static void network_device_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void network_device_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void network_device_reset (NetworkDevice *device);
static void network_device_poll_info (NetworkDevice *device);
static gboolean timeout_function (gpointer user_data);

G_DEFINE_TYPE (NetworkDevice, network_device, G_TYPE_OBJECT);

NetworkDevice*
network_device_new (const char *name)
{
	return g_object_new (NETWORK_DEVICE_TYPE,
						"name", name,
						NULL);
}

const char*
network_device_name (NetworkDevice* device)
{
	g_return_val_if_fail (IS_NETWORK_DEVICE (device), NULL);

	return device->priv->name;
}

static void
network_device_class_init (NetworkDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (NetworkDevicePrivate));

	object_class->finalize = network_device_finalize;
	object_class->get_property = network_device_get_property;
	object_class->set_property = network_device_set_property;

	signals[CHANGED] = g_signal_new ("changed",
		G_TYPE_FROM_CLASS (object_class),
		G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET (NetworkDeviceClass, changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);

	g_object_class_install_property (object_class, PROP_TYPE,
		g_param_spec_uint ("type",
							"Type",
							"The type of the device.",
							0, G_MAXUINT, 0,
							G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_NAME,
		g_param_spec_string ("name",
							"Name",
							"The name of the device that is monitored.",
							"lo",
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class, PROP_IPV4_ADDR,
		g_param_spec_string ("ipv4-addr",
							"IPv4-address",
							"The IPv4 address of the device.",
							"0.0.0.0",
							G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_IPV6_ADDR,
		g_param_spec_string ("ipv6-addr",
							"IPv6-address",
							"The IPv6 address of the device.",
							"0000:0000:0000:000:0000:0000",
							G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_NETMASK,
		g_param_spec_string ("netmask",
							"Netmask",
							"The IP network mask of the device.",
							"255.255.255.255",
							G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_PTP_ADDR,
		g_param_spec_string ("ptp-addr",
							"PTP-address",
							"The IPv4 peer address of the device.",
							"0.0.0.0",
							G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_HW_ADDR,
		g_param_spec_string ("hw-addr",
							"Hardware-address",
							"The hardware (Mac) address of the device.",
							"00:00:00:00:00:00",
							G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_WIFI_ESSID,
		g_param_spec_string ("wifi-essid",
							"WIFI Essid",
							"The currently configured wlan essid.",
							NULL,
							G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_WIFI_QUALITY,
		g_param_spec_int ("wifi-quality",
							"WIFI Quality",
							"The current wlan receptionquality.",
							0, 100, 0,
							G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_STATE,
		g_param_spec_uint ("state",
							"State",
							"A bit mask describing the current device-state.",
							0, G_MAXUINT, 0,
							G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_RX,
		g_param_spec_uint64 ("rx",
							"rx",
							"The amount of received octets.",
							0, G_MAXUINT64, 0,
							G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_TX,
		g_param_spec_uint64 ("tx",
							"tx",
							"The amount of transmitted octets.",
							0, G_MAXUINT64, 0,
							G_PARAM_READABLE));
}

static void
network_device_init (NetworkDevice *self)
{
	NetworkDevicePrivate *priv = NETWORK_DEVICE_GET_PRIVATE (self);
	self->priv = priv;

	priv->timeout_id = g_timeout_add (1000,
									timeout_function,
									self);
}

static void
network_device_get_property (GObject *object,
	guint property_id,
	GValue *value,
	GParamSpec *pspec)
{
	NetworkDevicePrivate *priv = NETWORK_DEVICE (object)->priv;

	switch (property_id) {
		case PROP_TYPE:
			g_value_set_uint (value, priv->type);
			break;
		case PROP_NAME:
			g_value_set_string (value, priv->name);
			break;
		case PROP_IPV4_ADDR:
			g_value_set_string (value, priv->ipv4_addr);
			break;
		case PROP_IPV6_ADDR:
			g_value_set_string (value, priv->ipv6_addr);
			break;
		case PROP_NETMASK:
			g_value_set_string (value, priv->netmask);
			break;
		case PROP_PTP_ADDR:
			g_value_set_string (value, priv->ptp_addr);
			break;
		case PROP_HW_ADDR:
			g_value_set_string (value, priv->hw_addr);
			break;
		case PROP_WIFI_ESSID:
			g_value_set_string (value, priv->wifi.essid);
			break;
		case PROP_WIFI_QUALITY:
			g_value_set_int (value, priv->wifi.quality);
			break;
		case PROP_STATE:
			g_value_set_uint (value, priv->state);
			break;
		case PROP_TX:
			g_value_set_uint64 (value, priv->tx);
			break;
		case PROP_RX:
			g_value_set_uint64 (value, priv->rx);
			break;
	}
}

static void
network_device_set_property (GObject *object,
	guint property_id,
	const GValue *value,
	GParamSpec *pspec)
{
	NetworkDevicePrivate *priv = NETWORK_DEVICE (object)->priv;

	switch (property_id) {
		case PROP_NAME:
			g_free (priv->name);
			priv->name = g_value_dup_string (value);
			break;
	}
}

static void
network_device_reset (NetworkDevice *device)
{
	NetworkDevicePrivate *priv = NETWORK_DEVICE (device)->priv;
	g_free (priv->ipv4_addr);
	g_free (priv->ipv6_addr);
	g_free (priv->netmask);
	g_free (priv->ptp_addr);
	g_free (priv->hw_addr);
	g_free (priv->wifi.essid);
}

static void
network_device_finalize (GObject *object)
{
	NetworkDevicePrivate *priv = NETWORK_DEVICE (object)->priv;

	g_source_remove (priv->timeout_id);
	network_device_reset (NETWORK_DEVICE (object));
	g_free (priv->name);

	G_OBJECT_CLASS (network_device_parent_class)->finalize (object);
}

static gboolean
timeout_function (gpointer user_data)
{
	NetworkDevice *device = NETWORK_DEVICE (user_data);
	
	network_device_poll_info (device);

	return TRUE;
}

static char*
format_ipv4(guint32 ip)
{
	char *str = g_malloc(INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &ip, str, INET_ADDRSTRLEN);
	return str;
}

static char*
format_ipv6(const guint8 ip[16])
{
	char *str = g_malloc(INET6_ADDRSTRLEN);
	inet_ntop(AF_INET6, ip, str, INET6_ADDRSTRLEN);
	return str;
}

/* TODO:
   these stuff are not portable because of ioctl
*/
static void
network_device_get_ptp_info (NetworkDevice *device)
{
	NetworkDevicePrivate *priv = device->priv;
	int fd = -1;
	struct ifreq request = {};

	g_strlcpy(request.ifr_name, priv->name, sizeof request.ifr_name);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return;

	if (ioctl(fd, SIOCGIFDSTADDR, &request) >= 0) {
		struct sockaddr_in* addr;
		addr = (struct sockaddr_in*)&request.ifr_dstaddr;
		priv->ptp_addr = format_ipv4(addr->sin_addr.s_addr);
	}

	close(fd);
}

#ifdef HAVE_IW
static void
network_device_get_wireless_info (NetworkDevice *device)
{
	NetworkDevicePrivate *priv = device->priv;
	int fd;
	int newqual;
	wireless_info info = {0};

	fd = iw_sockets_open ();

	if (fd < 0)
		return;

	if (iw_get_basic_config (fd, priv->name, &info.b) < 0) 
		goto out;
	
	if (info.b.has_essid) {
		if ((!priv->wifi.essid) || (strcmp (priv->wifi.essid, info.b.essid) != 0)) {
			priv->wifi.essid = g_strdup (info.b.essid);
		}
	} else {
		priv->wifi.essid = NULL;
	}

	if (iw_get_stats (fd, priv->name, &info.stats, &info.range, info.has_range) >= 0)
		info.has_stats = 1;

	if (info.has_stats) {
		if ((iw_get_range_info(fd, priv->name, &info.range) >= 0) && (info.range.max_qual.qual > 0)) {
			newqual = 0.5f + (100.0f * info.stats.qual.qual) / (1.0f * info.range.max_qual.qual);
		} else {
			newqual = info.stats.qual.qual;
		}
		
		newqual = CLAMP(newqual, 0, 100);
		if (priv->wifi.quality != newqual)
			priv->wifi.quality = newqual;
	} else {
		priv->wifi.quality = 0;
	}

	goto out;
out:
	if (fd != -1)
		close (fd);
}
#endif /* HAVE_IW */

static void
network_device_poll_info (NetworkDevice *device)
{
	NetworkDevicePrivate *priv = device->priv;
	glibtop_netload netload;
	guint8 *hw;

	g_return_if_fail (priv->name && priv->name[0]);

	priv->type = NETWORK_DEVICE_TYPE_UNKNOWN;

	network_device_reset (device);
	glibtop_get_netload(&netload, priv->name);
	priv->tx = netload.bytes_out;
	priv->rx = netload.bytes_in;

	priv->state = 0;
	priv->state |= (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_UP) ? NETWORK_DEVICE_STATE_UP : 0);
	priv->state |= (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_RUNNING) ? NETWORK_DEVICE_STATE_UP : 0);

	priv->ipv4_addr = format_ipv4(netload.address);
	priv->netmask = format_ipv4(netload.subnet);
	priv->ipv6_addr = format_ipv6(netload.address6);
	priv->wifi.quality = 0;
	priv->wifi.essid = NULL;

	hw = netload.hwaddress;
	if (hw[6] || hw[7]) {
		priv->hw_addr = g_strdup_printf(
			"%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
			hw[0], hw[1], hw[2], hw[3],
			hw[4], hw[5], hw[6], hw[7]);
	} else {
		priv->hw_addr = g_strdup_printf(
			"%02X:%02X:%02X:%02X:%02X:%02X",
			hw[0], hw[1], hw[2],
			hw[3], hw[4], hw[5]);
	}
	/* stolen from gnome-applets/multiload/linux-proc.c */

	if(netload.if_flags & (1L << GLIBTOP_IF_FLAGS_LOOPBACK)) {
		priv->type = NETWORK_DEVICE_TYPE_LO;
	}

#ifdef HAVE_IW

	else if (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_WIRELESS)) {
		priv->type = NETWORK_DEVICE_TYPE_WIRELESS;
		network_device_get_wireless_info (device);
	}

#endif /* HAVE_IW */

	else if(netload.if_flags & (1L << GLIBTOP_IF_FLAGS_POINTOPOINT)) {
		if (g_str_has_prefix(priv->name, "plip")) {
			priv->type = NETWORK_DEVICE_TYPE_PLIP;
		}
		else if (g_str_has_prefix(priv->name, "sl")) {
			priv->type = NETWORK_DEVICE_TYPE_SLIP;
		}
		else {
			priv->type = NETWORK_DEVICE_TYPE_PPP;
		}

		network_device_get_ptp_info (device);
	}
	else {
		priv->type = NETWORK_DEVICE_TYPE_ETHERNET;
	}

	g_signal_emit (device, signals[CHANGED], 0);
}

