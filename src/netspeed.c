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
#include "backend.h"
#include "utils.h"

/* Icons for the interfaces */
static const char* const dev_type_icon[DEV_UNKNOWN + 1] = {
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

static const char IN_ICON[] = "stock_navigate-next";
static const char OUT_ICON[] = "stock_navigate-prev";
static const char ERROR_ICON[] = "gtk-dialog-error";
static const char LOGO_ICON[] = "netspeed-applet";

/* How many old in out values do we store?
 * The value actually shown in the applet is the average
 * of these values -> prevents the value from
 * "jumping around like crazy"
 */
#define OLD_VALUES 5
#define GRAPH_VALUES 180
#define GRAPH_LINES 4

typedef struct _NetspeedApplet NetspeedApplet;

struct _NetspeedPrivate
{
	NetspeedApplet *stuff;

	Settings *settings;
	guint timeout_id;

	GtkWidget *settings_dialog;
	GtkWidget *info_dialog;
	GtkWidget *connect_dialog;
};

#define NETSPEED_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), NETSPEED_TYPE, NetspeedPrivate))

static void netspeed_class_init (NetspeedClass *klass);
static void netspeed_init       (Netspeed *self);
static void netspeed_dispose    (GObject *object);
static void netspeed_finalize   (GObject *object);

G_DEFINE_TYPE (Netspeed, netspeed, PANEL_TYPE_APPLET);


/* A struct containing all the "global" data of the 
 * applet
 * FIXME: This is old stuff and should be moved into NetspeedPrivate
 */
struct _NetspeedApplet
{
	PanelApplet *applet;
	GtkWidget *box, *pix_box,
	*in_box, *in_label, *in_pix,
	*out_box, *out_label, *out_pix,
	*sum_box, *sum_label, *dev_pix, *qual_pix;
	GdkPixbuf *qual_pixbufs[4];
	
	int refresh_time;
	gboolean show_sum, show_bits;
	gboolean change_icon, auto_change_device;
	gboolean labels_dont_shrink;
	
	DevInfo devinfo;
	gboolean device_has_changed;
		
	int width;
	
	guint index_old;
	guint64 in_old[OLD_VALUES], out_old[OLD_VALUES];
	double max_graph, in_graph[GRAPH_VALUES], out_graph[GRAPH_VALUES];
	int index_graph;
	
	gboolean show_tooltip;
};

static const char 
netspeed_applet_menu_xml [] =
	"<popup name=\"button3\">\n"
	"   <menuitem name=\"Properties Item\" verb=\"NetspeedAppletDetails\" label=\"%s\"\n"
	"             pixtype=\"stock\" pixname=\"gtk-info\"/>\n"
	"   <separator/>\n"
	"   <menuitem name=\"Properties Item\" verb=\"NetspeedAppletProperties\" label=\"%s\"\n"
	"             pixtype=\"stock\" pixname=\"gtk-properties\"/>\n"
	"   <menuitem name=\"Help Item\" verb=\"NetspeedAppletHelp\" label=\"%s\"\n"
	"             pixtype=\"stock\" pixname=\"gtk-help\"/>\n"
	"   <menuitem name=\"About Item\" verb=\"NetspeedAppletAbout\" label=\"%s\"\n"
	"             pixtype=\"stock\" pixname=\"gtk-about\"/>\n"
	"</popup>\n";


static void
update_tooltip(NetspeedApplet* applet);

/* Here some rearangement of the icons and the labels occurs
 * according to the panelsize and wether we show in and out
 * or just the sum
 */
static void
applet_change_size_or_orient(PanelApplet *applet_widget, int arg1, NetspeedApplet *applet)
{
	int size;
	PanelAppletOrient orient;
	
	g_assert(applet);
	
	size = panel_applet_get_size(applet_widget);
	orient = panel_applet_get_orient(applet_widget);
	
	gtk_widget_ref(applet->pix_box);
	gtk_widget_ref(applet->in_pix);
	gtk_widget_ref(applet->in_label);
	gtk_widget_ref(applet->out_pix);
	gtk_widget_ref(applet->out_label);
	gtk_widget_ref(applet->sum_label);

	if (applet->in_box) {
		gtk_container_remove(GTK_CONTAINER(applet->in_box), applet->in_label);
		gtk_container_remove(GTK_CONTAINER(applet->in_box), applet->in_pix);
		gtk_widget_destroy(applet->in_box);
	}
	if (applet->out_box) {
		gtk_container_remove(GTK_CONTAINER(applet->out_box), applet->out_label);
		gtk_container_remove(GTK_CONTAINER(applet->out_box), applet->out_pix);
		gtk_widget_destroy(applet->out_box);
	}
	if (applet->sum_box) {
		gtk_container_remove(GTK_CONTAINER(applet->sum_box), applet->sum_label);
		gtk_widget_destroy(applet->sum_box);
	}
	if (applet->box) {
		gtk_container_remove(GTK_CONTAINER(applet->box), applet->pix_box);
		gtk_widget_destroy(applet->box);
	}
		
	if (orient == PANEL_APPLET_ORIENT_LEFT || orient == PANEL_APPLET_ORIENT_RIGHT) {
		applet->box = gtk_vbox_new(FALSE, 0);
		if (size > 64) {
			applet->sum_box = gtk_hbox_new(FALSE, 2);
			applet->in_box = gtk_hbox_new(FALSE, 1);
			applet->out_box = gtk_hbox_new(FALSE, 1);
		} else {	
			applet->sum_box = gtk_vbox_new(FALSE, 0);
			applet->in_box = gtk_vbox_new(FALSE, 0);
			applet->out_box = gtk_vbox_new(FALSE, 0);
		}
		applet->labels_dont_shrink = FALSE;
	} else {
		applet->in_box = gtk_hbox_new(FALSE, 1);
		applet->out_box = gtk_hbox_new(FALSE, 1);
		if (size < 48) {
			applet->sum_box = gtk_hbox_new(FALSE, 2);
			applet->box = gtk_hbox_new(FALSE, 1);
			applet->labels_dont_shrink = TRUE;
		} else {
			applet->sum_box = gtk_vbox_new(FALSE, 0);
			applet->box = gtk_vbox_new(FALSE, 0);
			applet->labels_dont_shrink = !applet->show_sum;
		}
	}		
	
	gtk_box_pack_start(GTK_BOX(applet->in_box), applet->in_pix, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(applet->in_box), applet->in_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(applet->out_box), applet->out_pix, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(applet->out_box), applet->out_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(applet->sum_box), applet->sum_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(applet->box), applet->pix_box, FALSE, FALSE, 0);
	
	gtk_widget_unref(applet->pix_box);
	gtk_widget_unref(applet->in_pix);
	gtk_widget_unref(applet->in_label);
	gtk_widget_unref(applet->out_pix);
	gtk_widget_unref(applet->out_label);
	gtk_widget_unref(applet->sum_label);

	if (applet->show_sum) {
		gtk_box_pack_start(GTK_BOX(applet->box), applet->sum_box, TRUE, TRUE, 0);
	} else {
		gtk_box_pack_start(GTK_BOX(applet->box), applet->in_box, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(applet->box), applet->out_box, TRUE, TRUE, 0);
	}		
	
	gtk_widget_show_all(applet->box);
	gtk_container_add(GTK_CONTAINER(applet->applet), applet->box);
}

/* Change the background of the applet according to
 * the panel background.
 */
static void 
change_background_cb(PanelApplet *applet_widget, 
				PanelAppletBackgroundType type,
				GdkColor *color, GdkPixmap *pixmap, 
				NetspeedApplet *applet)
{
	GtkStyle *style;
	GtkRcStyle *rc_style = gtk_rc_style_new ();
	gtk_widget_set_style (GTK_WIDGET (applet_widget), NULL);
	gtk_widget_modify_style (GTK_WIDGET (applet_widget), rc_style);
	gtk_rc_style_unref (rc_style);

	switch (type) {
		case PANEL_PIXMAP_BACKGROUND:
			style = gtk_style_copy (GTK_WIDGET (applet_widget)->style);
			if(style->bg_pixmap[GTK_STATE_NORMAL])
				g_object_unref (style->bg_pixmap[GTK_STATE_NORMAL]);
			style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref (pixmap);
			gtk_widget_set_style (GTK_WIDGET(applet_widget), style);
			g_object_unref (style);
			break;

		case PANEL_COLOR_BACKGROUND:
			gtk_widget_modify_bg(GTK_WIDGET(applet_widget), GTK_STATE_NORMAL, color);
			break;

		case PANEL_NO_BACKGROUND:
			break;
	}
}


/* Change the icons according to the selected device
 */
static void
change_icons(NetspeedApplet *applet)
{
	GdkPixbuf *dev, *down;
	GdkPixbuf *in_arrow, *out_arrow;
	GtkIconTheme *icon_theme;
	
	icon_theme = gtk_icon_theme_get_default();
	/* If the user wants a different icon then the eth0, we load it */
	if (applet->change_icon) {
		dev = gtk_icon_theme_load_icon(icon_theme, 
                        dev_type_icon[applet->devinfo.type], 16, 0, NULL);
	} else {
        	dev = gtk_icon_theme_load_icon(icon_theme, 
					       dev_type_icon[DEV_UNKNOWN], 
					       16, 0, NULL);
	}
    
    	/* We need a fallback */
    	if (dev == NULL) 
		dev = gtk_icon_theme_load_icon(icon_theme, 
					       dev_type_icon[DEV_UNKNOWN],
					       16, 0, NULL);
        
    
	in_arrow = gtk_icon_theme_load_icon(icon_theme, IN_ICON, 16, 0, NULL);
	out_arrow = gtk_icon_theme_load_icon(icon_theme, OUT_ICON, 16, 0, NULL);

	/* Set the windowmanager icon for the applet */
	gtk_window_set_default_icon_name(LOGO_ICON);

	gtk_image_set_from_pixbuf(GTK_IMAGE(applet->out_pix), out_arrow);
	gtk_image_set_from_pixbuf(GTK_IMAGE(applet->in_pix), in_arrow);
	gdk_pixbuf_unref(in_arrow);
	gdk_pixbuf_unref(out_arrow);
	
	if (applet->devinfo.running) {
		gtk_widget_show(applet->in_box);
		gtk_widget_show(applet->out_box);
	} else {
		GdkPixbuf *copy;
		gtk_widget_hide(applet->in_box);
		gtk_widget_hide(applet->out_box);

		/* We're not allowed to modify "dev" */
        	copy = gdk_pixbuf_copy(dev);
        
        	down = gtk_icon_theme_load_icon(icon_theme, ERROR_ICON, 16, 0, NULL);	
		gdk_pixbuf_composite(down, copy, 8, 8, 8, 8, 8, 8, 0.5, 0.5, GDK_INTERP_BILINEAR, 0xFF);
		g_object_unref(down);
	      	g_object_unref(dev);
		dev = copy;
	}		

	gtk_image_set_from_pixbuf(GTK_IMAGE(applet->dev_pix), dev);
	g_object_unref(dev);
}

static void
update_quality_icon(NetspeedApplet *applet)
{
	unsigned int q;
	
	q = (applet->devinfo.qual);
	q /= 25;
	q = CLAMP(q, 0, 3); /* q out of range would crash when accessing qual_pixbufs[q] */
	gtk_image_set_from_pixbuf (GTK_IMAGE(applet->qual_pix), applet->qual_pixbufs[q]);
}

static void
init_quality_pixbufs(NetspeedApplet *applet)
{
	GtkIconTheme *icon_theme;
	int i;
	
	icon_theme = gtk_icon_theme_get_default();

	for (i = 0; i < 4; i++) {
		if (applet->qual_pixbufs[i])
			g_object_unref(applet->qual_pixbufs[i]);
		applet->qual_pixbufs[i] = gtk_icon_theme_load_icon(icon_theme, 
			wireless_quality_icon[i], 24, 0, NULL);
	}
}


static void
icon_theme_changed_cb(GtkIconTheme *icon_theme, gpointer user_data)
{
    NetspeedApplet *applet = (NetspeedApplet*)user_data;
    init_quality_pixbufs(user_data);
    if (applet->devinfo.type == DEV_WIRELESS && applet->devinfo.up)
        update_quality_icon(user_data);
    change_icons(user_data);
}    

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

/* Here happens the really interesting stuff */
static void
update_applet(NetspeedApplet *applet)
{
	NetspeedPrivate *priv = NETSPEED (applet->applet)->priv;
	guint64 indiff, outdiff;
	double inrate, outrate;
	int i;
	DevInfo oldinfo;
	
	if (!applet)	return;
	
	/* First we try to figure out if the device has changed */
	oldinfo = applet->devinfo;
	get_device_info(oldinfo.name, &applet->devinfo);
	if (compare_device_info(&applet->devinfo, &oldinfo))
		applet->device_has_changed = TRUE;
	free_device_info(&oldinfo);

	/* If the device has changed, reintialize stuff */	
	if (applet->device_has_changed) {
		change_icons(applet);
		if (applet->devinfo.type == DEV_WIRELESS &&
			applet->devinfo.up) {
			gtk_widget_show(applet->qual_pix);
		} else {
			gtk_widget_hide(applet->qual_pix);
		}	
		for (i = 0; i < OLD_VALUES; i++)
		{
			applet->in_old[i] = applet->devinfo.rx;
			applet->out_old[i] = applet->devinfo.tx;
		}
		for (i = 0; i < GRAPH_VALUES; i++)
		{
			applet->in_graph[i] = -1;
			applet->out_graph[i] = -1;
		}
		applet->max_graph = 0;
		applet->index_graph = 0;
		applet->device_has_changed = FALSE;
	}
		
	/* create the strings for the labels and tooltips */
	if (applet->devinfo.running)
	{	
		if (applet->devinfo.rx < applet->in_old[applet->index_old]) indiff = 0;
		else indiff = applet->devinfo.rx - applet->in_old[applet->index_old];
		if (applet->devinfo.tx < applet->out_old[applet->index_old]) outdiff = 0;
		else outdiff = applet->devinfo.tx - applet->out_old[applet->index_old];
		
		inrate = indiff * 1000.0;
		inrate /= (double)(applet->refresh_time * OLD_VALUES);
		outrate = outdiff * 1000.0;
		outrate /= (double)(applet->refresh_time * OLD_VALUES);
		
		applet->in_graph[applet->index_graph] = inrate;
		applet->out_graph[applet->index_graph] = outrate;
		applet->max_graph = MAX(inrate, applet->max_graph);
		applet->max_graph = MAX(outrate, applet->max_graph);
		
		applet->devinfo.rx_rate = bytes_to_string(inrate, TRUE, applet->show_bits);
		applet->devinfo.tx_rate = bytes_to_string(outrate, TRUE, applet->show_bits);
		applet->devinfo.sum_rate = bytes_to_string(inrate + outrate, TRUE, applet->show_bits);
	} else {
		applet->devinfo.rx_rate = g_strdup("");
		applet->devinfo.tx_rate = g_strdup("");
		applet->devinfo.sum_rate = g_strdup("");
		applet->in_graph[applet->index_graph] = 0;
		applet->out_graph[applet->index_graph] = 0;
	}
	
	if (applet->devinfo.type == DEV_WIRELESS) {
		if (applet->devinfo.up)
			update_quality_icon(applet);
		
	}

	if (priv->info_dialog) {
		info_dialog_update (INFO_DIALOG (priv->info_dialog));
	}

	update_tooltip(applet);

	/* Refresh the text of the labels and tooltip */
	if (applet->show_sum) {
		gtk_label_set_markup(GTK_LABEL(applet->sum_label), applet->devinfo.sum_rate);
	} else {
		gtk_label_set_markup(GTK_LABEL(applet->in_label), applet->devinfo.rx_rate);
		gtk_label_set_markup(GTK_LABEL(applet->out_label), applet->devinfo.tx_rate);
	}

	/* Save old values... */
	applet->in_old[applet->index_old] = applet->devinfo.rx;
	applet->out_old[applet->index_old] = applet->devinfo.tx;
	applet->index_old = (applet->index_old + 1) % OLD_VALUES;

	/* Move the graphindex. Check if we can scale down again */
	applet->index_graph = (applet->index_graph + 1) % GRAPH_VALUES; 
	if (applet->index_graph % 20 == 0)
	{
		double max = 0;
		for (i = 0; i < GRAPH_VALUES; i++)
		{
			max = MAX(max, applet->in_graph[i]);
			max = MAX(max, applet->out_graph[i]);
		}
		applet->max_graph = max;
	}

	/* Always follow the default route */
	if (applet->auto_change_device) {
		gboolean change_device_now = !applet->devinfo.running;
		if (!change_device_now) {
			const gchar *default_route;
			default_route = get_default_route();
			change_device_now = (default_route != NULL
						&& strcmp(default_route,
							applet->devinfo.name));
		}
		if (change_device_now) {
			search_for_up_if(applet);
		}
	}
}

static gboolean
timeout_function(gpointer user_data)
{
	NetspeedApplet *applet = user_data;
	
	update_applet(applet);
	return TRUE;
}

/* Display a section of netspeed help
 */
static void
display_help (GtkWidget *dialog, const gchar *section)
{
	GError *error = NULL;
	gboolean ret;
	char *uri;

	if (section)
		uri = g_strdup_printf ("ghelp:netspeed_applet?%s", section);
	else
		uri = g_strdup ("ghelp:netspeed_applet");

	ret = open_uri (dialog, uri, &error);
	g_free (uri);
	if (ret == FALSE) {
		GtkWidget *error_dialog = gtk_message_dialog_new (NULL,
								  GTK_DIALOG_MODAL,
								  GTK_MESSAGE_ERROR,
								  GTK_BUTTONS_OK,
								  _("There was an error displaying help:\n%s"),
								  error->message);
		g_signal_connect (error_dialog, "response",
				  G_CALLBACK (gtk_widget_destroy), NULL);
	       
		gtk_window_set_resizable (GTK_WINDOW (error_dialog), FALSE);
		gtk_window_set_screen  (GTK_WINDOW (error_dialog), gtk_widget_get_screen (dialog));
		gtk_widget_show (error_dialog);
		g_error_free (error);
	}
}

/* Opens gnome help application
 */
static void
help_cb (BonoboUIComponent *uic, gpointer data, const gchar *verbname)
{
	Netspeed *applet = NETSPEED (data);

	display_help (GTK_WIDGET (applet), NULL);
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
about_cb(BonoboUIComponent *uic, gpointer data, const gchar *verbname)
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

/* Handle preference dialog response event
 */
static void
pref_response_cb (GtkDialog *dialog, gint id, gpointer data)
{
	NetspeedPrivate *priv = NETSPEED (data)->priv;

    if (id == GTK_RESPONSE_HELP) {
		display_help (GTK_WIDGET (dialog), "netspeed_applet-settings");
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
settings_cb(BonoboUIComponent *uic, gpointer data, const gchar *verbname)
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
			 G_CALLBACK (pref_response_cb), applet);

	gtk_widget_show_all (priv->settings_dialog);
}

/* Handle info dialog response event
 */
static void
info_response_cb (GtkDialog *dialog, gint id, gpointer data)
{
	NetspeedPrivate *priv = NETSPEED (data)->priv;

	if (id == GTK_RESPONSE_HELP){
		display_help (GTK_WIDGET (dialog), "netspeed_applet-details");
		return;
	}
	
	gtk_widget_destroy (priv->info_dialog);
	priv->info_dialog = NULL;

#if 0
	applet->inbytes_text = NULL;
	applet->outbytes_text = NULL;
	applet->drawingarea = NULL;
	applet->signalbar = NULL;
#endif
}

/* Creates the details dialog
 */
static void
showinfo_cb(BonoboUIComponent *uic, gpointer data, const gchar *verbname)
{
	Netspeed *applet = NETSPEED (data);
	NetspeedPrivate *priv = applet->priv;
	
	if (priv->info_dialog)
	{
		gtk_window_present (GTK_WINDOW (priv->info_dialog));
		return;
	}
	
	priv->info_dialog = info_dialog_new (priv->settings);
	g_signal_connect (G_OBJECT (priv->info_dialog), "response",
			 G_CALLBACK (info_response_cb), applet);

	gtk_widget_show_all (priv->info_dialog);
}

static const BonoboUIVerb
netspeed_applet_menu_verbs [] = 
{
		BONOBO_UI_VERB("NetspeedAppletDetails", showinfo_cb),
		BONOBO_UI_VERB("NetspeedAppletProperties", settings_cb),
		BONOBO_UI_UNSAFE_VERB("NetspeedAppletHelp", help_cb),
		BONOBO_UI_VERB("NetspeedAppletAbout", about_cb),
	
		BONOBO_UI_VERB_END
};

/* Block the size_request signal emit by the label if the
 * text changes. Only if the label wants to grow, we give
 * permission. This will eventually result in the maximal
 * size of the applet and prevents the icons and labels from
 * "jumping around" in the panel which looks uggly
 */
static void
label_size_request_cb(GtkWidget *widget, GtkRequisition *requisition, NetspeedApplet *applet)
{
	if (applet->labels_dont_shrink) {
		if (requisition->width <= applet->width)
			requisition->width = applet->width;
		else
			applet->width = requisition->width;
	}
}	

static gboolean
applet_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	NetspeedPrivate *priv = NETSPEED (widget)->priv;
	char *up_cmd, *down_cmd;

	if (event->button == 1)
	{
		GError *error = NULL;
		
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
			
			if (priv->stuff->devinfo.up)
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
					priv->stuff->devinfo.name);
			response = gtk_dialog_run(GTK_DIALOG(priv->connect_dialog));
			gtk_widget_destroy (priv->connect_dialog);
			priv->connect_dialog = NULL;
			
			if (response == GTK_RESPONSE_YES)
			{
				GtkWidget *dialog;
				char *command;
				
				command = g_strdup_printf("%s %s", 
					priv->stuff->devinfo.up ? down_cmd : up_cmd,
					priv->stuff->devinfo.name);

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
	
	return FALSE;
}	

static void
update_tooltip(NetspeedApplet* applet)
{
  GString* tooltip;

  if (!applet->show_tooltip)
    return;

  tooltip = g_string_new("");

  if (!applet->devinfo.running)
    g_string_printf(tooltip, _("%s is down"), applet->devinfo.name);
  else {
    if (applet->show_sum) {
      g_string_printf(
		      tooltip,
		      _("%s: %s\nin: %s out: %s"),
		      applet->devinfo.name,
		      applet->devinfo.ip ? applet->devinfo.ip : _("has no ip"),
		      applet->devinfo.rx_rate,
		      applet->devinfo.tx_rate
		      );
    } else {
      g_string_printf(
		      tooltip,
		      _("%s: %s\nsum: %s"),
		      applet->devinfo.name,
		      applet->devinfo.ip ? applet->devinfo.ip : _("has no ip"),
		      applet->devinfo.sum_rate
		      );
    }
    if (applet->devinfo.type == DEV_WIRELESS)
      g_string_append_printf(
			     tooltip,
			     _("\nESSID: %s\nStrength: %d %%"),
			     applet->devinfo.essid ? applet->devinfo.essid : _("unknown"),
			     applet->devinfo.qual
			     );

  }

  gtk_widget_set_tooltip_text(GTK_WIDGET(applet->applet), tooltip->str);
  gtk_widget_trigger_tooltip_query(GTK_WIDGET(applet->applet));
  g_string_free(tooltip, TRUE);
}


static gboolean
netspeed_enter_cb(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	NetspeedApplet *applet = data;

	applet->show_tooltip = TRUE;
	update_tooltip(applet);

	return TRUE;
}

static gboolean
netspeed_leave_cb(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	NetspeedApplet *applet = data;

	applet->show_tooltip = FALSE;
	return TRUE;
}

static void
netspeed_class_init (NetspeedClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (NetspeedPrivate));

	object_class->dispose = netspeed_dispose;
	object_class->finalize = netspeed_finalize;

	glibtop_init();
	g_set_application_name (_("Netspeed"));
}

static void
netspeed_init (Netspeed *self)
{
	NetspeedPrivate *priv = NETSPEED_GET_PRIVATE (self);
	NetspeedApplet *applet;
	int i;
	GtkIconTheme *icon_theme;
	GtkWidget *spacer, *spacer_box;
	
	/* Install shortcut */
	self->priv = priv;
	icon_theme = gtk_icon_theme_get_default();

	gtk_widget_set_name (GTK_WIDGET(self), "PanelApplet");
	
	/* Alloc the applet. The "NULL-setting" is really redudant
 	 * but aren't we paranoid?
	 */
	applet = g_malloc0(sizeof(NetspeedApplet));
	applet->applet = PANEL_APPLET(self);
	priv->stuff = applet;
	memset(&applet->devinfo, 0, sizeof(DevInfo));
	applet->refresh_time = 1000.0;
	applet->show_sum = FALSE;
	applet->show_bits = FALSE;
	applet->change_icon = TRUE;
	applet->auto_change_device = TRUE;

	for (i = 0; i < GRAPH_VALUES; i++)
	{
		applet->in_graph[i] = -1;
		applet->out_graph[i] = -1;
	}	
	
	applet->in_label = gtk_label_new("");
	applet->out_label = gtk_label_new("");
	applet->sum_label = gtk_label_new("");
	
	applet->in_pix = gtk_image_new();
	applet->out_pix = gtk_image_new();
	applet->dev_pix = gtk_image_new();
	applet->qual_pix = gtk_image_new();
	
	applet->pix_box = gtk_hbox_new(FALSE, 0);
	spacer = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(applet->pix_box), spacer, TRUE, TRUE, 0);
	spacer = gtk_label_new("");
	gtk_box_pack_end(GTK_BOX(applet->pix_box), spacer, TRUE, TRUE, 0);

	spacer_box = gtk_hbox_new(FALSE, 2);	
	gtk_box_pack_start(GTK_BOX(applet->pix_box), spacer_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(spacer_box), applet->qual_pix, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(spacer_box), applet->dev_pix, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(self), "change_size",
                           G_CALLBACK(applet_change_size_or_orient),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(icon_theme), "changed",
                           G_CALLBACK(icon_theme_changed_cb),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(self), "change_orient",
                           G_CALLBACK(applet_change_size_or_orient),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(self), "change_background",
                           G_CALLBACK(change_background_cb),
			   (gpointer)applet);
		       
	g_signal_connect(G_OBJECT(applet->in_label), "size_request",
                           G_CALLBACK(label_size_request_cb),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(applet->out_label), "size_request",
                           G_CALLBACK(label_size_request_cb),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(applet->sum_label), "size_request",
                           G_CALLBACK(label_size_request_cb),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(self), "button-press-event",
                           G_CALLBACK(applet_button_press),
                           NULL);

	g_signal_connect(G_OBJECT(self), "leave_notify_event",
			 G_CALLBACK(netspeed_leave_cb),
			 (gpointer)applet);

	g_signal_connect(G_OBJECT(self), "enter_notify_event",
			 G_CALLBACK(netspeed_enter_cb),
			 (gpointer)applet);
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

	G_OBJECT_CLASS (netspeed_parent_class)->dispose (object);
}

static void
netspeed_finalize (GObject *object)
{
	NetspeedPrivate *priv;
	NetspeedApplet *applet;
	GtkIconTheme *icon_theme;

	priv = NETSPEED (object)->priv;
	applet = priv->stuff;

	icon_theme = gtk_icon_theme_get_default();
	g_object_disconnect(G_OBJECT(icon_theme), "any_signal::changed",
						G_CALLBACK(icon_theme_changed_cb), (gpointer)priv->stuff,
						NULL);

	g_source_remove(priv->timeout_id);

	free_device_info(&applet->devinfo);
	g_free(applet);

	G_OBJECT_CLASS (netspeed_parent_class)->finalize (object);
}

static gboolean
netspeed_factory (PanelApplet *applet, const gchar *iid, gpointer data)
{
	NetspeedPrivate *priv;
	char* menu_string;
	char* dummy_key, *dummy;
	char* gconf_path;

	g_return_val_if_fail (IS_NETSPEED (applet), FALSE);
	priv = NETSPEED (applet)->priv;

	if (strcmp (iid, "OAFIID:GNOME_NetspeedApplet"))
		return FALSE;

	menu_string = g_strdup_printf(netspeed_applet_menu_xml, _("Device _Details"), _("_Preferences..."), _("_Help"), _("_About..."));
	panel_applet_setup_menu(applet, menu_string,
                            netspeed_applet_menu_verbs,
                            applet);
	g_free (menu_string);

	/* Get stored settings from the gconf database
	 */
	if (panel_applet_gconf_get_bool(applet, "have_settings", NULL))
	{	
		char *tmp = NULL;
		
		priv->stuff->show_sum = panel_applet_gconf_get_bool(applet, "show_sum", NULL);
		priv->stuff->show_bits = panel_applet_gconf_get_bool(applet, "show_bits", NULL);
		priv->stuff->change_icon = panel_applet_gconf_get_bool(applet, "change_icon", NULL);
		priv->stuff->auto_change_device = panel_applet_gconf_get_bool(applet, "auto_change_device", NULL);
		
		tmp = panel_applet_gconf_get_string(applet, "device", NULL);
		if (tmp && strcmp(tmp, "")) 
		{
			get_device_info(tmp, &priv->stuff->devinfo);
			g_free(tmp);
		}
	}

	dummy_key = panel_applet_gconf_get_full_key (applet, "dummy");
	dummy = dummy_key ? strstr (dummy_key, "dummy") : NULL;
	if (dummy) {
		dummy[0] = 0;
		gconf_path = dummy_key;
		priv->settings = settings_new_with_gconf_path (gconf_path);
	} else {
		gconf_path = NULL;
		g_warning ("Could not figure out gconf-path from dummy-key %s", dummy_key);
		priv->settings = settings_new ();
	}

	g_free (dummy_key);

	if (!priv->stuff->devinfo.name) {
		GList *ptr, *devices = get_available_devices();
		ptr = devices;
		while (ptr) { 
			if (!g_str_equal(ptr->data, "lo")) {
				get_device_info(ptr->data, &priv->stuff->devinfo);
				break;
			}
			ptr = g_list_next(ptr);
		}
		free_devices_list(devices);		
	}
	if (!priv->stuff->devinfo.name)
		get_device_info("lo", &priv->stuff->devinfo);	
	priv->stuff->device_has_changed = TRUE;	
	
	init_quality_pixbufs(priv->stuff);
	
	applet_change_size_or_orient(applet, -1, (gpointer)priv->stuff);
	update_applet(priv->stuff);

	panel_applet_set_flags(applet, PANEL_APPLET_EXPAND_MINOR);

	gtk_widget_show_all(GTK_WIDGET(applet));

	priv->timeout_id = g_timeout_add (priv->stuff->refresh_time,
									timeout_function,
									(gpointer)priv->stuff);

	return TRUE;
}

PANEL_APPLET_BONOBO_FACTORY("OAFIID:GNOME_NetspeedApplet_Factory",
							NETSPEED_TYPE, PACKAGE, VERSION,
							netspeed_factory, NULL)

