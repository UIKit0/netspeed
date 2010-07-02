/*  netspeed.c
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

#include <math.h>
#include <gtk/gtk.h>
#include <panel-applet-gconf.h>
#include <gconf/gconf-client.h>
#include "netspeed.h"
#include "settings.h"
#include "settings-dialog.h"
#include "info-dialog.h"
#include "network-device.h"
#include "utils.h"

/* Icons for the interfaces */
static const char* const dev_type_icon[NETWORK_DEVICE_TYPE_UNKNOWN + 1] = {
	"netspeed-loopback",         /* DEV_LO */
	"network-wired",             /* DEV_ETHERNET */
	"network-wireless",          /* DEV_WIRELESS */
	"netspeed-ppp",              /* DEV_PPP */
	"netspeed-plip",             /* DEV_PLIP */
	"netspeed-plip",             /* DEV_SLIP */
	"network-workgroup",         /* DEV_UNKNOWN */
};

static const char* wireless_quality_icon[] = {
	"netspeed-wireless-25",
	"netspeed-wireless-50",
	"netspeed-wireless-75",
	"netspeed-wireless-100"
};

static const char IN_ICON[] = "gtk-go-up";
static const char OUT_ICON[] = "gtk-go-down";
static const char ERROR_ICON[] = "gtk-dialog-error";
static const char LOGO_ICON[] = "netspeed-applet";

/* How many old in out values do we store?
 * The value actually shown in the applet is the average
 * of these values -> prevents the value from
 * "jumping around like crazy"
 */
#define OLD_VALUES 5

struct _NetspeedPrivate
{
	Settings *settings;

	NetworkDevice *device;

	GtkWidget *settings_dialog;
	GtkWidget *info_dialog;
	GtkWidget *connect_dialog;

	GtkWidget *box;
	GtkWidget *pix_box;
	GtkWidget *dev_pix;
	GtkWidget *qual_pix;
	GtkWidget *in_box, *in_label, *in_pix;
	GtkWidget *out_box, *out_label, *out_pix;
	GtkWidget *sum_box, *sum_label;

	GdkPixbuf *qual_pixbufs[4];

	guint index_old;
	guint64 in_old[OLD_VALUES], out_old[OLD_VALUES];

	gboolean labels_dont_shrink;
	int width;
	
	gboolean show_tooltip;
};

#define NETSPEED_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), NETSPEED_TYPE, NetspeedPrivate))

static void netspeed_class_init (NetspeedClass *klass);
static void netspeed_init       (Netspeed *self);
static void netspeed_dispose    (GObject *object);
static void netspeed_finalize   (GObject *object);
static void netspeed_relayout   (Netspeed *applet);
static gboolean netspeed_enter_notify_event (GtkWidget *widget, GdkEventCrossing *event);
static gboolean netspeed_leave_notify_event (GtkWidget *widget, GdkEventCrossing *event);
static gboolean netspeed_button_press_event (GtkWidget *widget, GdkEventButton   *event);
static void netspeed_change_orient (PanelApplet *applet, PanelAppletOrient orient);
static void netspeed_change_size   (PanelApplet *applet, guint size);
static void netspeed_change_background
		(PanelApplet *applet,
		PanelAppletBackgroundType type,
		GdkColor *color,
		GdkPixmap *pixmap);
static void netspeed_menu_show_help (BonoboUIComponent *uic, gpointer data, const gchar *verbname);
static void netspeed_menu_show_about (BonoboUIComponent *uic, gpointer data, const gchar *verbname);
static void netspeed_menu_show_settings_dialog (BonoboUIComponent *uic, gpointer data, const gchar *verbname);
static void netspeed_menu_show_info_dialog (BonoboUIComponent *uic, gpointer data, const gchar *verbname);
static void netspeed_display_help_section (GtkWidget *parent, const gchar *section);
static void netspeed_set_network_device (Netspeed *netspeed, NetworkDevice *device);

G_DEFINE_TYPE (Netspeed, netspeed, PANEL_TYPE_APPLET);

static void
update_tooltip(Netspeed* applet);

/* Change the icons according to the selected device
 */
static void
change_icons(Netspeed *applet)
{
	NetspeedPrivate *priv = NETSPEED (applet)->priv;
	GdkPixbuf *dev, *down;
	GdkPixbuf *in_arrow, *out_arrow;
	GtkIconTheme *icon_theme;
	gboolean change_icon;
	NetworkDeviceType type;
	NetworkDeviceState state;

	icon_theme = gtk_icon_theme_get_default();
	/* If the user wants a different icon then the eth0, we load it */
	g_object_get (priv->settings, "display-specific-icon", &change_icon, NULL);
	g_object_get (priv->device, "type", &type, "state", &state, NULL);
	if (change_icon) {
		dev = gtk_icon_theme_load_icon(icon_theme,
                        dev_type_icon[type], 16, 0, NULL);
	} else {
        	dev = gtk_icon_theme_load_icon(icon_theme, 
					       dev_type_icon[NETWORK_DEVICE_TYPE_UNKNOWN], 
					       16, 0, NULL);
	}
    
	/* We need a fallback */
	if (dev == NULL)
	dev = gtk_icon_theme_load_icon(icon_theme, 
					   dev_type_icon[NETWORK_DEVICE_TYPE_UNKNOWN],
					   16, 0, NULL);
	
	in_arrow = gtk_icon_theme_load_icon(icon_theme, IN_ICON, 16, 0, NULL);
	out_arrow = gtk_icon_theme_load_icon(icon_theme, OUT_ICON, 16, 0, NULL);

	/* Set the windowmanager icon for the applet */
	gtk_window_set_default_icon_name(LOGO_ICON);

	gtk_image_set_from_pixbuf(GTK_IMAGE(priv->out_pix), out_arrow);
	gtk_image_set_from_pixbuf(GTK_IMAGE(priv->in_pix), in_arrow);
	gdk_pixbuf_unref(in_arrow);
	gdk_pixbuf_unref(out_arrow);
	
	if (state & NETWORK_DEVICE_STATE_RUNNING) {
		gtk_widget_show(priv->in_box);
		gtk_widget_show(priv->out_box);
	} else {
		GdkPixbuf *copy;
		gtk_widget_hide(priv->in_box);
		gtk_widget_hide(priv->out_box);

		/* We're not allowed to modify "dev" */
        	copy = gdk_pixbuf_copy(dev);
        
        	down = gtk_icon_theme_load_icon(icon_theme, ERROR_ICON, 16, 0, NULL);	
		gdk_pixbuf_composite(down, copy, 8, 8, 8, 8, 8, 8, 0.5, 0.5, GDK_INTERP_BILINEAR, 0xFF);
		g_object_unref(down);
		g_object_unref(dev);
		dev = copy;
	}

	gtk_image_set_from_pixbuf(GTK_IMAGE(priv->dev_pix), dev);
	g_object_unref(dev);
}

static void
update_quality_icon(Netspeed *applet)
{
	NetspeedPrivate *priv = applet->priv;
	unsigned int q;
	int wifi_quality;

	g_object_get (priv->device, "wifi-quality", &wifi_quality, NULL);
	q = wifi_quality / 25;
	q = CLAMP(q, 0, 3); /* q out of range would crash when accessing qual_pixbufs[q] */
	gtk_image_set_from_pixbuf (GTK_IMAGE(priv->qual_pix), priv->qual_pixbufs[q]);
}

static void
init_quality_pixbufs(Netspeed *applet)
{
	NetspeedPrivate *priv = applet->priv;
	GtkIconTheme *icon_theme;
	int i;
	
	icon_theme = gtk_icon_theme_get_default();

	for (i = 0; i < 4; i++) {
		if (priv->qual_pixbufs[i]) {
			g_object_unref(priv->qual_pixbufs[i]);
		}
		priv->qual_pixbufs[i] =
			gtk_icon_theme_load_icon (icon_theme,
				wireless_quality_icon[i], 24, 0, NULL);
	}
}

static void
icon_theme_changed_cb(GtkIconTheme *icon_theme, gpointer user_data)
{
	Netspeed *applet = NETSPEED (user_data);
	NetspeedPrivate *priv = applet->priv;
	NetworkDeviceType type;
	NetworkDeviceState state;

	init_quality_pixbufs (applet);
	g_object_get (priv->device, "type", &type, "state", &state, NULL);
	if (type == NETWORK_DEVICE_TYPE_WIRELESS &&
		(state & NETWORK_DEVICE_STATE_UP)) {
		update_quality_icon (applet);
	}
	change_icons (applet);
}


static void
netspeed_set_network_device (Netspeed *applet, NetworkDevice *device)
{
	NetspeedPrivate *priv = applet->priv;
	g_return_if_fail (IS_NETWORK_DEVICE (device));

	if (priv->device) {
		g_object_unref (priv->device);
	}
	priv->device = g_object_ref (device);
}

#if 0
static gboolean
set_applet_devinfo(NetspeedApplet* applet, const char* iface)
{
	DevInfo info;

	get_device_info(iface, &info);

	if (info.running) {
		free_device_info(&applet->devinfo);
		applet->devinfo = info;
		applet->device_has_changed = TRUE;
		return TRUE;
	}

	free_device_info(&info);
	return FALSE;
}

/* Find the first available device, that is running and != lo */
static void
search_for_up_if(NetspeedApplet *applet)
{
	const gchar *default_route;
	GList *devices, *tmp;
	DevInfo info;
	
	default_route = get_default_route();
    
	if (default_route != NULL) {
		if (set_applet_devinfo(applet, default_route))
			return;
	}

	devices = get_available_devices();
	for (tmp = devices; tmp; tmp = g_list_next(tmp)) {
		if (is_dummy_device(tmp->data))
			continue;
		if (set_applet_devinfo(applet, tmp->data))
			break;
	}
	free_devices_list(devices);
}
#endif

/* Here happens the really interesting stuff */
static void
update_applet(Netspeed *applet)
{
#if 0
	NetspeedPrivate *priv = NETSPEED (applet)->priv;
	guint64 indiff, outdiff;
	double inrate, outrate;
	int i;
	DevInfo oldinfo;
	gboolean show_sum, show_bits, auto_change_device;

	g_object_get (priv->settings,
			"display-bits", &show_bits,
			"display-sum", &show_sum,
			"default-route", &auto_change_device,
			NULL);
	
	/* First we try to figure out if the device has changed */
	oldinfo = priv->stuff->devinfo;
	get_device_info(oldinfo.name, &priv->stuff->devinfo);
	if (compare_device_info(&priv->stuff->devinfo, &oldinfo))
		priv->stuff->device_has_changed = TRUE;
	free_device_info(&oldinfo);

	/* If the device has changed, reintialize stuff */	
	if (priv->stuff->device_has_changed) {
		change_icons(applet);
		if (priv->stuff->devinfo.type == DEV_WIRELESS &&
			priv->stuff->devinfo.up) {
			gtk_widget_show(priv->qual_pix);
		} else {
			gtk_widget_hide(priv->qual_pix);
		}	
		for (i = 0; i < OLD_VALUES; i++)
		{
			priv->in_old[i] = priv->stuff->devinfo.rx;
			priv->out_old[i] = priv->stuff->devinfo.tx;
		}
		if (priv->info_dialog) {
			info_dialog_device_changed (priv->info_dialog);
		}
		priv->stuff->device_has_changed = FALSE;
	}
		
	/* create the strings for the labels and tooltips */
	if (priv->stuff->devinfo.running)
	{	
		if (priv->stuff->devinfo.rx < priv->in_old[priv->index_old]) indiff = 0;
		else indiff = priv->stuff->devinfo.rx - priv->in_old[priv->index_old];
		if (priv->stuff->devinfo.tx < priv->out_old[priv->index_old]) outdiff = 0;
		else outdiff = priv->stuff->devinfo.tx - priv->out_old[priv->index_old];
		
		inrate = indiff * 1000.0;
		inrate /= (double)(priv->stuff->refresh_time * OLD_VALUES);
		outrate = outdiff * 1000.0;
		outrate /= (double)(priv->stuff->refresh_time * OLD_VALUES);

#if 0
		priv->in_graph[priv->index_graph] = inrate;
		priv->out_graph[priv->index_graph] = outrate;
		priv->max_graph = MAX(inrate, priv->max_graph);
		priv->max_graph = MAX(outrate, priv->max_graph);
#endif

		priv->stuff->devinfo.rx_rate = bytes_to_string(inrate, TRUE, show_bits);
		priv->stuff->devinfo.tx_rate = bytes_to_string(outrate, TRUE, show_bits);
		priv->stuff->devinfo.sum_rate = bytes_to_string(inrate + outrate, TRUE, show_bits);
	} else {
		priv->stuff->devinfo.rx_rate = g_strdup("");
		priv->stuff->devinfo.tx_rate = g_strdup("");
		priv->stuff->devinfo.sum_rate = g_strdup("");
#if 0
		priv->in_graph[priv->index_graph] = 0;
		priv->out_graph[priv->index_graph] = 0;
#endif
	}
	
	if (priv->stuff->devinfo.type == DEV_WIRELESS) {
		if (priv->stuff->devinfo.up)
			update_quality_icon(applet);
		
	}

	if (priv->info_dialog) {
		info_dialog_update (INFO_DIALOG (priv->info_dialog));
	}

	update_tooltip(applet);

	/* Refresh the text of the labels and tooltip */
	if (show_sum) {
		gtk_label_set_markup(GTK_LABEL(priv->sum_label), priv->stuff->devinfo.sum_rate);
	} else {
		gtk_label_set_markup(GTK_LABEL(priv->in_label), priv->stuff->devinfo.rx_rate);
		gtk_label_set_markup(GTK_LABEL(priv->out_label), priv->stuff->devinfo.tx_rate);
	}

	/* Save old values... */
	priv->in_old[priv->index_old] = priv->stuff->devinfo.rx;
	priv->out_old[priv->index_old] = priv->stuff->devinfo.tx;
	priv->index_old = (priv->index_old + 1) % OLD_VALUES;

#if 0
	/* Move the graphindex. Check if we can scale down again */
	priv->index_graph = (priv->index_graph + 1) % GRAPH_VALUES; 
	if (priv->index_graph % 20 == 0)
	{
		double max = 0;
		for (i = 0; i < GRAPH_VALUES; i++)
		{
			max = MAX(max, priv->in_graph[i]);
			max = MAX(max, priv->out_graph[i]);
		}
		priv->max_graph = max;
	}
#endif

	/* Always follow the default route */
	if (auto_change_device) {
		gboolean change_device_now = !priv->stuff->devinfo.running;
		if (!change_device_now) {
			const gchar *default_route;
			default_route = get_default_route();
			change_device_now = (default_route != NULL
						&& strcmp(default_route,
							priv->stuff->devinfo.name));
		}
		if (change_device_now) {
			search_for_up_if(priv->stuff);
		}
	}
#endif
}

/* Block the size_request signal emit by the label if the
 * text changes. Only if the label wants to grow, we give
 * permission. This will eventually result in the maximal
 * size of the applet and prevents the icons and labels from
 * "jumping around" in the panel which looks uggly
 */
static void
label_size_request_cb(GtkWidget *widget, GtkRequisition *requisition, gpointer user_data)
{
	NetspeedPrivate *priv = NETSPEED (user_data)->priv;
	if (priv->labels_dont_shrink) {
		if (requisition->width <= priv->width)
			requisition->width = priv->width;
		else
			priv->width = requisition->width;
	}
}	

static void
update_tooltip(Netspeed* applet)
{
	NetspeedPrivate *priv = NETSPEED (applet)->priv;
	GString* tooltip;
	gboolean show_sum;
	char *ipv4_addr;
	char *netmask;
	char *hw_addr;
	char *ptp_addr;
	char *name;
	NetworkDeviceState state;
	NetworkDeviceType type;

	if (!priv->show_tooltip) {
		return;
	}

	g_object_get (priv->settings, "display-sum", &show_sum, NULL);
	g_object_get (priv->device,
			"state", &state,
			"name", &name,
			"ipv4-addr", &ipv4_addr,
			"netmask", &netmask,
			"hw-addr", &hw_addr,
			"ptp-addr", &ptp_addr,
			NULL);

	tooltip = g_string_new("");
#if 0
	if (state & NETWORK_DEVICE_STATE_RUNNING) {
	if (!priv->stuff->devinfo.running) {
		if (show_sum) {
			g_string_printf(
				  tooltip,
				  _("%s: %s\nin: %s out: %s"),
				  name,
				  ipv4_addr ? ipv4_addr : _("has no ip"),
				  priv->stuff->devinfo.rx_rate,
				  priv->stuff->devinfo.tx_rate
				  );
		} else {
			g_string_printf(
				  tooltip,
				  _("%s: %s\nsum: %s"),
				  name,
				  ipv4_addr ? ipv4_addr : _("has no ip"),
				  priv->stuff->devinfo.sum_rate
				  );
		}
		if (type == NETWORK_DEVICE_TYPE_WIRELESS) {
			g_string_append_printf(
					 tooltip,
					 _("\nESSID: %s\nStrength: %d %%"),
					 priv->stuff->devinfo.essid ? priv->stuff->devinfo.essid : _("unknown"),
					 priv->stuff->devinfo.qual
					 );
		}
	} else {
		g_string_printf(tooltip, _("%s is down"), priv->stuff->devinfo.name);
	}
#endif
	g_free (name);
	g_free (ipv4_addr);
	g_free (netmask);
	g_free (hw_addr);
	g_free (ptp_addr);

	gtk_widget_set_tooltip_text(GTK_WIDGET(applet), tooltip->str);
	gtk_widget_trigger_tooltip_query(GTK_WIDGET(applet));
	g_string_free(tooltip, TRUE);
}

static void
network_device_changed_cb(NetworkDevice *device, gpointer user_data)
{
	Netspeed *applet = NETSPEED (user_data);
	
	update_applet (applet);
}

static void
settings_display_sum_changed_cb (GObject *object,
	GParamSpec *pspec,
	gpointer    user_data)
{
	Netspeed *applet = NETSPEED (user_data);
	netspeed_relayout (applet);
	change_icons (applet);
}

static void
settings_display_specific_icon_changed_cb (GObject *object,
	GParamSpec *pspec,
	gpointer    user_data)
{
	Netspeed *applet = NETSPEED (user_data);
	change_icons (applet);
}

static void
settings_device_changed_cb (GObject *object,
	GParamSpec *pspec,
	gpointer    user_data)
{
	Netspeed *applet = NETSPEED (user_data);
	NetspeedPrivate *priv = applet->priv;
	char *device;

#if 0
	g_object_get (object, "device", &device, NULL);
	get_device_info (device, &priv->stuff->devinfo);
	priv->stuff->device_has_changed = TRUE;
#endif
}

static void
netspeed_class_init (NetspeedClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	PanelAppletClass *applet_class = PANEL_APPLET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (NetspeedPrivate));

	object_class->dispose = netspeed_dispose;
	object_class->finalize = netspeed_finalize;
	widget_class->enter_notify_event = netspeed_enter_notify_event;
	widget_class->leave_notify_event = netspeed_leave_notify_event;
	widget_class->button_press_event = netspeed_button_press_event;
	applet_class->change_background = netspeed_change_background;
	applet_class->change_orient = netspeed_change_orient;
	applet_class->change_size = netspeed_change_size;

	glibtop_init();
	g_set_application_name (_("Netspeed"));
}

static void
netspeed_init (Netspeed *self)
{
	NetspeedPrivate *priv = NETSPEED_GET_PRIVATE (self);
	int i;
	GtkIconTheme *icon_theme;
	GtkWidget *spacer, *spacer_box;
	
	/* Install shortcut */
	self->priv = priv;
	gtk_widget_set_name (GTK_WIDGET(self), "PanelApplet");

	panel_applet_set_flags (PANEL_APPLET (self),
			PANEL_APPLET_EXPAND_MINOR);
	
#if 0
	for (i = 0; i < GRAPH_VALUES; i++)
	{
		applet->in_graph[i] = -1;
		applet->out_graph[i] = -1;
	}	
#endif
	priv->in_label = gtk_label_new("test");
	priv->out_label = gtk_label_new("test");
	priv->sum_label = gtk_label_new("test");
	
	priv->in_pix = gtk_image_new();
	priv->out_pix = gtk_image_new();
	priv->dev_pix = gtk_image_new();
	priv->qual_pix = gtk_image_new();
	
	priv->pix_box = gtk_hbox_new(FALSE, 0);
	spacer = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(priv->pix_box), spacer, TRUE, TRUE, 0);
	spacer = gtk_label_new("");
	gtk_box_pack_end(GTK_BOX(priv->pix_box), spacer, TRUE, TRUE, 0);

	spacer_box = gtk_hbox_new(FALSE, 2);	
	gtk_box_pack_start(GTK_BOX(priv->pix_box), spacer_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(spacer_box), priv->qual_pix, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(spacer_box), priv->dev_pix, FALSE, FALSE, 0);

	icon_theme = gtk_icon_theme_get_default();
	g_signal_connect(G_OBJECT(icon_theme), "changed",
                           G_CALLBACK(icon_theme_changed_cb),
                           self);

	g_signal_connect(G_OBJECT(priv->in_label), "size_request",
                           G_CALLBACK(label_size_request_cb),
                           self);

	g_signal_connect(G_OBJECT(priv->out_label), "size_request",
                           G_CALLBACK(label_size_request_cb),
                           self);

	g_signal_connect(G_OBJECT(priv->sum_label), "size_request",
                           G_CALLBACK(label_size_request_cb),
                           self);
}

static gboolean
netspeed_factory (PanelApplet *applet, const gchar *iid, gpointer data)
{
	NetspeedPrivate *priv;
	char* gconf_path;
	char* device;
	static const BonoboUIVerb netspeed_applet_menu_verbs[] = {
		BONOBO_UI_VERB("NetspeedAppletDetails", netspeed_menu_show_info_dialog),
		BONOBO_UI_VERB("NetspeedAppletProperties", netspeed_menu_show_settings_dialog),
		BONOBO_UI_UNSAFE_VERB("NetspeedAppletHelp", netspeed_menu_show_help),
		BONOBO_UI_VERB("NetspeedAppletAbout", netspeed_menu_show_about),
		BONOBO_UI_VERB_END
	};

	g_return_val_if_fail (IS_NETSPEED (applet), FALSE);
	priv = NETSPEED (applet)->priv;

	if (strcmp (iid, "OAFIID:GNOME_NetspeedApplet"))
		return FALSE;

	panel_applet_setup_menu_from_file (applet,
			DATADIR"/netspeed/",
			"menu.xml", NULL,
			netspeed_applet_menu_verbs,
			applet);

	gconf_path = panel_applet_get_preferences_key (applet);
	priv->settings = settings_new_with_gconf_path (gconf_path);
	g_free (gconf_path);

	g_signal_connect (G_OBJECT (priv->settings),
					"notify::display-sum",
					G_CALLBACK (settings_display_sum_changed_cb),
					applet);
	g_signal_connect (G_OBJECT (priv->settings),
					"notify::display-specific-icon",
					G_CALLBACK (settings_display_specific_icon_changed_cb),
					applet);
	g_signal_connect (G_OBJECT (priv->settings),
					"notify::device",
					G_CALLBACK (settings_device_changed_cb),
					applet);

	g_object_get (priv->settings, "device", &device, NULL);
	priv->device = network_device_new (device);
	g_free (device);

	g_signal_connect (G_OBJECT (priv->device),
					"changed",
					G_CALLBACK (network_device_changed_cb),
					applet);

	init_quality_pixbufs (NETSPEED (applet));
	netspeed_relayout (NETSPEED (applet));

	gtk_widget_show_all(GTK_WIDGET(applet));


	return TRUE;
}

/* Here some rearangement of the icons and the labels occurs
 * according to the panelsize and wether we show in and out
 * or just the sum
 */
static void
netspeed_relayout (Netspeed *applet)
{
	NetspeedPrivate *priv = applet->priv;
	int size;
	PanelAppletOrient orient;
	gboolean show_sum;

	if (!priv->settings) {
		return;
	}
	size = panel_applet_get_size (PANEL_APPLET (applet));
	orient = panel_applet_get_orient (PANEL_APPLET (applet));

	g_object_get (priv->settings, "display-sum", &show_sum, NULL);

	gtk_widget_ref(priv->pix_box);
	gtk_widget_ref(priv->in_pix);
	gtk_widget_ref(priv->in_label);
	gtk_widget_ref(priv->out_pix);
	gtk_widget_ref(priv->out_label);
	gtk_widget_ref(priv->sum_label);

	if (priv->in_box) {
		gtk_container_remove(GTK_CONTAINER(priv->in_box), priv->in_label);
		gtk_container_remove(GTK_CONTAINER(priv->in_box), priv->in_pix);
		gtk_widget_destroy(priv->in_box);
	}
	if (priv->out_box) {
		gtk_container_remove(GTK_CONTAINER(priv->out_box), priv->out_label);
		gtk_container_remove(GTK_CONTAINER(priv->out_box), priv->out_pix);
		gtk_widget_destroy(priv->out_box);
	}
	if (priv->sum_box) {
		gtk_container_remove(GTK_CONTAINER(priv->sum_box), priv->sum_label);
		gtk_widget_destroy(priv->sum_box);
	}
	if (priv->box) {
		gtk_container_remove(GTK_CONTAINER(priv->box), priv->pix_box);
		gtk_widget_destroy(priv->box);
	}
		
	if (orient == PANEL_APPLET_ORIENT_LEFT || orient == PANEL_APPLET_ORIENT_RIGHT) {
		priv->box = gtk_vbox_new(FALSE, 0);
		if (size > 64) {
			priv->sum_box = gtk_hbox_new(FALSE, 2);
			priv->in_box = gtk_hbox_new(FALSE, 1);
			priv->out_box = gtk_hbox_new(FALSE, 1);
		} else {	
			priv->sum_box = gtk_vbox_new(FALSE, 0);
			priv->in_box = gtk_vbox_new(FALSE, 0);
			priv->out_box = gtk_vbox_new(FALSE, 0);
		}
		priv->labels_dont_shrink = FALSE;
	} else {
		priv->in_box = gtk_hbox_new(FALSE, 1);
		priv->out_box = gtk_hbox_new(FALSE, 1);
		if (size < 48) {
			priv->sum_box = gtk_hbox_new(FALSE, 2);
			priv->box = gtk_hbox_new(FALSE, 1);
			priv->labels_dont_shrink = TRUE;
		} else {
			priv->sum_box = gtk_vbox_new(FALSE, 0);
			priv->box = gtk_vbox_new(FALSE, 0);
			priv->labels_dont_shrink = !show_sum;
		}
	}		
	
	gtk_box_pack_start(GTK_BOX(priv->in_box), priv->in_pix, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(priv->in_box), priv->in_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(priv->out_box), priv->out_pix, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(priv->out_box), priv->out_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(priv->sum_box), priv->sum_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(priv->box), priv->pix_box, FALSE, FALSE, 0);
	
	gtk_widget_unref(priv->pix_box);
	gtk_widget_unref(priv->in_pix);
	gtk_widget_unref(priv->in_label);
	gtk_widget_unref(priv->out_pix);
	gtk_widget_unref(priv->out_label);
	gtk_widget_unref(priv->sum_label);

	if (show_sum) {
		gtk_box_pack_start(GTK_BOX(priv->box), priv->sum_box, TRUE, TRUE, 0);
	} else {
		gtk_box_pack_start(GTK_BOX(priv->box), priv->in_box, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(priv->box), priv->out_box, TRUE, TRUE, 0);
	}		
	
	gtk_widget_show_all(priv->box);
	gtk_container_add(GTK_CONTAINER(applet), priv->box);
}

static gboolean
netspeed_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	NetspeedPrivate *priv = NETSPEED (widget)->priv;

	if (event->button == 1)
	{
		GError *error = NULL;
		char *up_cmd, *down_cmd;
		
		if (priv->connect_dialog)
		{	
			gtk_window_present (GTK_WINDOW (priv->connect_dialog));
			return FALSE;
		}
		
		g_object_get (priv->settings,
				"ifup-command", &up_cmd,
				"ifdown-command", &down_cmd,
				NULL);
		if (up_cmd && down_cmd)
		{
			const char *question;
			int response;
			NetworkDeviceState state;
			char *name;
			
			g_object_get (priv->device,
					"state", &state,
					"name", &name,
					NULL);
			if (state & NETWORK_DEVICE_STATE_UP)
			{
				question = _("Do you want to disconnect %s now?");
			} 
			else
			{
				question = _("Do you want to connect %s now?");
			}
			
			priv->connect_dialog = gtk_message_dialog_new(NULL,
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
					question,
					name);
			response = gtk_dialog_run(GTK_DIALOG(priv->connect_dialog));
			gtk_widget_destroy (priv->connect_dialog);
			priv->connect_dialog = NULL;
			g_free (name);

			if (response == GTK_RESPONSE_YES)
			{
				GtkWidget *dialog;
				char *command;
				
				command = g_strdup_printf("%s %s", 
					state & NETWORK_DEVICE_STATE_UP ? down_cmd : up_cmd,
					network_device_name (priv->device));

				if (!g_spawn_command_line_async(command, &error))
				{
					dialog = gtk_message_dialog_new_with_markup(NULL, 
							GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
							_("<b>Running command %s failed</b>\n%s"),
							command,
							error->message);
					gtk_dialog_run (GTK_DIALOG (dialog));
					gtk_widget_destroy (dialog);
					g_error_free (error);
				}
				g_free(command);
			} 
		}	
		g_free (up_cmd);
		g_free (down_cmd);
	}
	
	return GTK_WIDGET_CLASS (netspeed_parent_class)->button_press_event (widget, event);
}

static gboolean
netspeed_enter_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
	Netspeed *applet = NETSPEED (widget);
	NetspeedPrivate *priv = applet->priv;

	priv->show_tooltip = TRUE;
	update_tooltip(applet);

	return TRUE;
}

static gboolean
netspeed_leave_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
	Netspeed *applet = NETSPEED (widget);
	NetspeedPrivate *priv = applet->priv;

	priv->show_tooltip = FALSE;
	
	return TRUE;
}

/* Change the background of the applet according to
 * the panel background.
 */
static void 
netspeed_change_background (PanelApplet *applet,
		PanelAppletBackgroundType type,
		GdkColor *color,
		GdkPixmap *pixmap)
{
	GtkStyle *style;
	GtkRcStyle *rc_style = gtk_rc_style_new ();
	gtk_widget_set_style (GTK_WIDGET (applet), NULL);
	gtk_widget_modify_style (GTK_WIDGET (applet), rc_style);
	gtk_rc_style_unref (rc_style);

	switch (type) {
		case PANEL_PIXMAP_BACKGROUND:
			style = gtk_style_copy (GTK_WIDGET (applet)->style);
			if(style->bg_pixmap[GTK_STATE_NORMAL])
				g_object_unref (style->bg_pixmap[GTK_STATE_NORMAL]);
			style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref (pixmap);
			gtk_widget_set_style (GTK_WIDGET(applet), style);
			g_object_unref (style);
			break;

		case PANEL_COLOR_BACKGROUND:
			gtk_widget_modify_bg(GTK_WIDGET(applet), GTK_STATE_NORMAL, color);
			break;

		case PANEL_NO_BACKGROUND:
			break;
	}
}

static void
netspeed_change_orient (PanelApplet *applet, PanelAppletOrient orient)
{
	netspeed_relayout (NETSPEED (applet));
}

static void
netspeed_change_size (PanelApplet *applet, guint size)
{
	netspeed_relayout (NETSPEED (applet));
}

/* Handle preference dialog response event
 */
static void
settings_dialog_response_cb (GtkDialog *dialog, gint id, gpointer data)
{
	NetspeedPrivate *priv = NETSPEED (data)->priv;

    if (id == GTK_RESPONSE_HELP) {
		netspeed_display_help_section (GTK_WIDGET (dialog), "netspeed_applet-settings");
		return;
	}

	gtk_widget_destroy (priv->settings_dialog);
	priv->settings_dialog = NULL;
}


/* Creates the settings dialog
 * After its been closed, take the new values and store
 * them in the gconf database
 */
static void
netspeed_menu_show_settings_dialog (BonoboUIComponent *uic, gpointer data, const gchar *verbname)
{
	Netspeed *applet = NETSPEED (data);
	NetspeedPrivate *priv = applet->priv;

	if (priv->settings_dialog)
	{
		gtk_window_present(GTK_WINDOW(priv->settings_dialog));
		return;
	}
	priv->settings_dialog = settings_dialog_new (priv->settings);
	g_signal_connect (G_OBJECT (priv->settings_dialog), "response",
			 G_CALLBACK (settings_dialog_response_cb), applet);

	gtk_widget_show_all (priv->settings_dialog);
}

/* Handle info dialog response event
 */
static void
info_dialog_response_cb (GtkDialog *dialog, gint id, gpointer data)
{
	NetspeedPrivate *priv = NETSPEED (data)->priv;

	if (id == GTK_RESPONSE_HELP){
		netspeed_display_help_section (GTK_WIDGET (dialog), "netspeed_applet-details");
		return;
	}
	
	gtk_widget_destroy (priv->info_dialog);
	priv->info_dialog = NULL;
}

/* Creates the details dialog
 */
static void
netspeed_menu_show_info_dialog (BonoboUIComponent *uic, gpointer data, const gchar *verbname)
{
	Netspeed *applet = NETSPEED (data);
	NetspeedPrivate *priv = applet->priv;
	
	if (priv->info_dialog)
	{
		gtk_window_present (GTK_WINDOW (priv->info_dialog));
		return;
	}
	
	priv->info_dialog = info_dialog_new (priv->settings, priv->device);
	g_signal_connect (G_OBJECT (priv->info_dialog), "response",
			 G_CALLBACK (info_dialog_response_cb), applet);

	gtk_widget_show_all (priv->info_dialog);
}


enum {
	LINK_TYPE_EMAIL,
	LINK_TYPE_URL
};

/* handle the links of the about dialog */
static void
handle_links (GtkAboutDialog *about, const gchar *link, gpointer data)
{
	gchar *new_link;
	GError *error = NULL;
	gboolean ret;
	GtkWidget *dialog;

	switch (GPOINTER_TO_INT (data)){
	case LINK_TYPE_EMAIL:
		new_link = g_strdup_printf ("mailto:%s", link);
		break;
	case LINK_TYPE_URL:
		new_link = g_strdup (link);
		break;
	default:
		g_assert_not_reached ();
	}

	ret = open_uri (GTK_WIDGET (about), new_link, &error);

	if (ret == FALSE) {
    		dialog = gtk_message_dialog_new (GTK_WINDOW (dialog), 
						 GTK_DIALOG_DESTROY_WITH_PARENT, 
						 GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
				                 _("Failed to show:\n%s"), new_link); 
    		gtk_dialog_run (GTK_DIALOG (dialog));
    		gtk_widget_destroy (dialog);
		g_error_free(error);
	}
	g_free (new_link);
}

/* Just the about window... If it's already open, just fokus it
 */
static void
netspeed_menu_show_about (BonoboUIComponent *uic, gpointer data, const gchar *verbname)
{
	const char *authors[] = 
	{
		"Jörgen Scheibengruber <mfcn@gmx.de>", 
		"Dennis Cranston <dennis_cranston@yahoo.com>",
		"Pedro Villavicencio Garrido <pvillavi@gnome.org>",
		"Benoît Dejean <benoit@placenet.org>",
		NULL
	};

	gtk_about_dialog_set_email_hook ((GtkAboutDialogActivateLinkFunc) handle_links,
					 GINT_TO_POINTER (LINK_TYPE_EMAIL), NULL);
	
	gtk_about_dialog_set_url_hook ((GtkAboutDialogActivateLinkFunc) handle_links,
				       GINT_TO_POINTER (LINK_TYPE_URL), NULL);
	
	gtk_show_about_dialog (NULL, 
			       "version", VERSION, 
			       "copyright", "Copyright 2002 - 2010 Jörgen Scheibengruber",
			       "comments", _("A little applet that displays some information on the traffic on the specified network device"),
			       "authors", authors, 
			       "documenters", NULL, 
			       "translator-credits", _("translator-credits"),
			       "website", "http://www.gnome.org/projects/netspeed/",
			       "website-label", _("Netspeed Website"),
			       "logo-icon-name", LOGO_ICON,
			       NULL);
}

/* Opens gnome help application
 */
static void
netspeed_menu_show_help (BonoboUIComponent *uic, gpointer data, const gchar *verbname)
{
	Netspeed *applet = NETSPEED (data);

	netspeed_display_help_section (GTK_WIDGET (applet), NULL);
}

/* Display a section of netspeed help
 */
static void
netspeed_display_help_section (GtkWidget *parent, const gchar *section)
{
	GError *error = NULL;
	gboolean ret;
	char *uri;

	if (section)
		uri = g_strdup_printf ("ghelp:netspeed_applet?%s", section);
	else
		uri = g_strdup ("ghelp:netspeed_applet");

	ret = open_uri (parent, uri, &error);
	g_free (uri);
	if (ret == FALSE) {
		GtkWidget *error_dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
								  GTK_DIALOG_MODAL,
								  GTK_MESSAGE_ERROR,
								  GTK_BUTTONS_OK,
								  _("There was an error displaying help:\n%s"),
								  error->message);
		g_signal_connect (error_dialog, "response",
				  G_CALLBACK (gtk_widget_destroy), NULL);
	       
		gtk_window_set_resizable (GTK_WINDOW (error_dialog), FALSE);
		gtk_window_set_screen  (GTK_WINDOW (error_dialog), gtk_widget_get_screen (parent));
		gtk_widget_show (error_dialog);
		g_error_free (error);
	}
}

static void
netspeed_dispose (GObject *object)
{
	NetspeedPrivate *priv;
	priv = NETSPEED (object)->priv;

	if (priv->settings) {
		g_object_unref (priv->settings);
		priv->settings = NULL;
	}

	if (priv->device) {
		g_object_unref (priv->device);
		priv->device = NULL;
	}

	G_OBJECT_CLASS (netspeed_parent_class)->dispose (object);
}

static void
netspeed_finalize (GObject *object)
{
	NetspeedPrivate *priv;
	GtkIconTheme *icon_theme;

	priv = NETSPEED (object)->priv;

	icon_theme = gtk_icon_theme_get_default();
	g_object_disconnect(G_OBJECT(icon_theme), "any_signal::changed",
						G_CALLBACK(icon_theme_changed_cb), object,
						NULL);

	G_OBJECT_CLASS (netspeed_parent_class)->finalize (object);
}

PANEL_APPLET_BONOBO_FACTORY("OAFIID:GNOME_NetspeedApplet_Factory",
							NETSPEED_TYPE, PACKAGE, VERSION,
							netspeed_factory, NULL)

