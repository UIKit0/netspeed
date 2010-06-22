/*  info-dialog.h
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

#include <glib/gi18n.h>
#include "info-dialog.h"
#include "backend.h"
#include "utils.h"

#define GRAPH_VALUES 180
#define GRAPH_LINES 4

enum
{
	PROP_0,
	PROP_SETTINGS,
	PROP_DEV_INFO
};

struct _InfoDialogPrivate
{
	GtkWidget *drawingarea;
	GtkWidget *signalbar;
	GtkWidget *inbytes_text;
	GtkWidget *outbytes_text;
	
	DevInfo devinfo;

	double max_graph, in_graph[GRAPH_VALUES], out_graph[GRAPH_VALUES];
	int index_graph;

	Settings *settings;
	GtkWidget *incolor_selector;
	GtkWidget *outcolor_selector;
};

#define INFO_DIALOG_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), INFO_DIALOG_TYPE, InfoDialogPrivate))

static void info_dialog_class_init (InfoDialogClass *klass);
static void info_dialog_init       (InfoDialog *self);
static void info_dialog_constructed(GObject *object);
static void info_dialog_dispose    (GObject *object);
static void info_dialog_finalize   (GObject *object);
static void info_dialog_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void info_dialog_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);

G_DEFINE_TYPE (InfoDialog, info_dialog, GTK_TYPE_DIALOG);

#if 0
/* Redraws the graph drawingarea
 * Some really black magic is going on in here ;-)
 */
static void
redraw_graph(NetspeedApplet *applet)
{
	GdkGC *gc;
	GdkColor color;
	GtkWidget *da = GTK_WIDGET(applet->drawingarea);
	GdkWindow *window, *real_window = da->window;
	GdkRectangle ra;
	GtkStateType state;
	GdkPoint in_points[GRAPH_VALUES], out_points[GRAPH_VALUES];
	PangoLayout *layout;
	PangoRectangle logical_rect;
	char *text; 
	int i, offset, w, h;
	double max_val;
	
	gc = gdk_gc_new (real_window);
	gdk_drawable_get_size(real_window, &w, &h);

	/* use doublebuffering to avoid flickering */
	window = gdk_pixmap_new(real_window, w, h, -1);
	
	/* the graph hight should be: hight/2 <= applet->max_graph < hight */
	for (max_val = 1; max_val < applet->max_graph; max_val *= 2) ;
	
	/* calculate the polygons (GdkPoint[]) for the graphs */ 
	offset = 0;
	for (i = (applet->index_graph + 1) % GRAPH_VALUES; applet->in_graph[i] < 0; i = (i + 1) % GRAPH_VALUES)
		offset++;
	for (i = offset + 1; i < GRAPH_VALUES; i++)
	{
		int index = (applet->index_graph + i) % GRAPH_VALUES;
		out_points[i].x = in_points[i].x = ((w - 6) * i) / GRAPH_VALUES + 4;
		in_points[i].y = h - 6 - (int)((h - 8) * applet->in_graph[index] / max_val);
		out_points[i].y = h - 6 - (int)((h - 8) * applet->out_graph[index] / max_val);
	}	
	in_points[offset].x = out_points[offset].x = ((w - 6) * offset) / GRAPH_VALUES + 4;
	in_points[offset].y = in_points[(offset + 1) % GRAPH_VALUES].y;
	out_points[offset].y = out_points[(offset + 1) % GRAPH_VALUES].y;
	
	/* draw the background */
	gdk_gc_set_rgb_fg_color (gc, &da->style->black);
	gdk_draw_rectangle (window, gc, TRUE, 0, 0, w, h);
	
	color.red = 0x3a00; color.green = 0x8000; color.blue = 0x1400;
	gdk_gc_set_rgb_fg_color(gc, &color);
	gdk_draw_rectangle (window, gc, FALSE, 2, 2, w - 6, h - 6);
	
	for (i = 0; i < GRAPH_LINES; i++) {
		int y = 2 + ((h - 6) * i) / GRAPH_LINES; 
		gdk_draw_line(window, gc, 2, y, w - 4, y);
	}
	
	/* draw the polygons */
	gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_rgb_fg_color(gc, &applet->in_color);
	gdk_draw_lines(window, gc, in_points + offset, GRAPH_VALUES - offset);
	gdk_gc_set_rgb_fg_color(gc, &applet->out_color);
	gdk_draw_lines(window, gc, out_points + offset, GRAPH_VALUES - offset);

	/* draw the 2 labels */
	state = GTK_STATE_NORMAL;
	ra.x = 0; ra.y = 0;
	ra.width = w; ra.height = h;
	
	text = bytes_to_string(max_val, TRUE, applet->show_bits);
	add_markup_fgcolor(&text, "white");
	layout = gtk_widget_create_pango_layout (da, NULL);
	pango_layout_set_markup(layout, text, -1);
	g_free (text);
	gtk_paint_layout(da->style, window, state,	FALSE, &ra, da, "max_graph", 3, 2, layout);
	g_object_unref(G_OBJECT(layout));

	text = bytes_to_string(0.0, TRUE, applet->show_bits);
	add_markup_fgcolor(&text, "white");
	layout = gtk_widget_create_pango_layout (da, NULL);
	pango_layout_set_markup(layout, text, -1);
	pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
	g_free (text);
	gtk_paint_layout(da->style, window, state,	FALSE, &ra, da, "max_graph", 3, h - 4 - logical_rect.height, layout);
	g_object_unref(G_OBJECT(layout));

	/* draw the pixmap to the real window */	
	gdk_draw_drawable(real_window, gc, window, 0, 0, 0, 0, w, h);
	
	g_object_unref(G_OBJECT(gc));
	g_object_unref(G_OBJECT(window));
}

static gboolean
da_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	NetspeedApplet *applet = (NetspeedApplet*)data;
	
	redraw_graph(applet);
	
	return FALSE;
}	
#endif

static void
incolor_changed_cb (GtkColorButton *cb, gpointer data)
{
	InfoDialogPrivate *priv = INFO_DIALOG (data)->priv;
	gchar *color;
	GdkColor clr;
	
	gtk_color_button_get_color (cb, &clr);
	color = gdk_color_to_string (&clr);
	g_object_set (priv->settings,
			"graph-color-in", color,
			NULL);
	g_free (color);
}

static void
outcolor_changed_cb (GtkColorButton *cb, gpointer data)
{
	InfoDialogPrivate *priv = INFO_DIALOG (data)->priv;
	gchar *color;
	GdkColor clr;
	
	gtk_color_button_get_color (cb, &clr);
	color = gdk_color_to_string (&clr);
	g_object_set (priv->settings,
			"graph-color-out", color,
			NULL);
	g_free (color);
}

static void
info_dialog_class_init (InfoDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (InfoDialogPrivate));

	object_class->constructed = info_dialog_constructed;
	object_class->dispose = info_dialog_dispose;
	object_class->finalize = info_dialog_finalize;
	object_class->set_property = info_dialog_set_property;
	object_class->get_property = info_dialog_get_property;

	g_object_class_install_property (object_class, PROP_SETTINGS,
		g_param_spec_object ("settings",
							 "Settings",
							 "The netspeed settings.",
							 SETTINGS_TYPE,
							 G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void
info_dialog_init (InfoDialog *self)
{
	InfoDialogPrivate *priv;
	GtkWidget *box, *hbox;
	GtkWidget *table, *da_frame;
	GtkWidget *ip_label, *netmask_label;
	GtkWidget *hwaddr_label, *ptpip_label;
	GtkWidget *ip_text, *netmask_text;
	GtkWidget *hwaddr_text, *ptpip_text;
	GtkWidget *inbytes_label, *outbytes_label;
	GtkWidget *incolor_sel, *incolor_label;
	GtkWidget *outcolor_sel, *outcolor_label;
	char *title;

	priv = INFO_DIALOG_GET_PRIVATE (self);
	self->priv = priv;

	title = g_strdup_printf(_("Device Details for %s"), priv->devinfo.name);
	gtk_window_set_title (GTK_WINDOW (self), title);
	g_free(title);

	gtk_dialog_add_buttons (GTK_DIALOG (self),
							GTK_STOCK_HELP, GTK_RESPONSE_HELP,
							GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT,
							NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(self),
									GTK_RESPONSE_CLOSE);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (self), TRUE);
	
	box = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(box), 12);
	
	table = gtk_table_new(4, 4, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 10);
	gtk_table_set_col_spacings(GTK_TABLE(table), 15);
	
	da_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(da_frame), GTK_SHADOW_IN);
	priv->drawingarea = gtk_drawing_area_new();
	gtk_widget_set_size_request(GTK_WIDGET(priv->drawingarea), -1, 180);
	gtk_container_add(GTK_CONTAINER(da_frame), GTK_WIDGET(priv->drawingarea));
	
	hbox = gtk_hbox_new(FALSE, 5);
	incolor_label = gtk_label_new_with_mnemonic(_("_In graph color"));
	outcolor_label = gtk_label_new_with_mnemonic(_("_Out graph color"));
	
	incolor_sel = gtk_color_button_new ();
	outcolor_sel = gtk_color_button_new ();
	priv->incolor_selector = incolor_sel;
	priv->outcolor_selector = outcolor_sel;
	
	gtk_label_set_mnemonic_widget(GTK_LABEL(incolor_label), incolor_sel);
	gtk_label_set_mnemonic_widget(GTK_LABEL(outcolor_label), outcolor_sel);
	
	gtk_box_pack_start(GTK_BOX(hbox), incolor_sel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), incolor_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), outcolor_sel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), outcolor_label, FALSE, FALSE, 0);
	
	ip_label = gtk_label_new(_("Internet Address:"));
	netmask_label = gtk_label_new(_("Netmask:"));
	hwaddr_label = gtk_label_new(_("Hardware Address:"));
	ptpip_label = gtk_label_new(_("P-t-P Address:"));
	inbytes_label = gtk_label_new(_("Bytes in:"));
	outbytes_label = gtk_label_new(_("Bytes out:"));
	
	ip_text = gtk_label_new(priv->devinfo.ip ? priv->devinfo.ip : _("none"));
	netmask_text = gtk_label_new(priv->devinfo.netmask ? priv->devinfo.netmask : _("none"));
	hwaddr_text = gtk_label_new(priv->devinfo.hwaddr ? priv->devinfo.hwaddr : _("none"));
	ptpip_text = gtk_label_new(priv->devinfo.ptpip ? priv->devinfo.ptpip : _("none"));
	priv->inbytes_text = gtk_label_new("0 byte");
	priv->outbytes_text = gtk_label_new("0 byte");

	gtk_label_set_selectable(GTK_LABEL(ip_text), TRUE);
	gtk_label_set_selectable(GTK_LABEL(netmask_text), TRUE);
	gtk_label_set_selectable(GTK_LABEL(hwaddr_text), TRUE);
	gtk_label_set_selectable(GTK_LABEL(ptpip_text), TRUE);
	
	gtk_misc_set_alignment(GTK_MISC(ip_label), 0.0f, 0.5f);
	gtk_misc_set_alignment(GTK_MISC(ip_text), 0.0f, 0.5f);
	gtk_misc_set_alignment(GTK_MISC(netmask_label), 0.0f, 0.5f);
	gtk_misc_set_alignment(GTK_MISC(netmask_text), 0.0f, 0.5f);
	gtk_misc_set_alignment(GTK_MISC(hwaddr_label), 0.0f, 0.5f);
	gtk_misc_set_alignment(GTK_MISC(hwaddr_text), 0.0f, 0.5f);
	gtk_misc_set_alignment(GTK_MISC(ptpip_label), 0.0f, 0.5f);
	gtk_misc_set_alignment(GTK_MISC(ptpip_text), 0.0f, 0.5f);
	gtk_misc_set_alignment(GTK_MISC(inbytes_label), 0.0f, 0.5f);
	gtk_misc_set_alignment(GTK_MISC(priv->inbytes_text), 0.0f, 0.5f);
	gtk_misc_set_alignment(GTK_MISC(outbytes_label), 0.0f, 0.5f);
	gtk_misc_set_alignment(GTK_MISC(priv->outbytes_text), 0.0f, 0.5f);
	
	gtk_table_attach_defaults(GTK_TABLE(table), ip_label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), ip_text, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), netmask_label, 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), netmask_text, 3, 4, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), hwaddr_label, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), hwaddr_text, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), ptpip_label, 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), ptpip_text, 3, 4, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), inbytes_label, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), priv->inbytes_text, 1, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), outbytes_label, 2, 3, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), priv->outbytes_text, 3, 4, 2, 3);
	
	/* check if we got an ipv6 address */
	if (priv->devinfo.ipv6 && (strlen (priv->devinfo.ipv6) > 2)) {
		GtkWidget *ipv6_label, *ipv6_text;

		ipv6_label = gtk_label_new (_("IPv6 Address:"));
		ipv6_text = gtk_label_new (priv->devinfo.ipv6);
		
		gtk_label_set_selectable (GTK_LABEL (ipv6_text), TRUE);
		
		gtk_misc_set_alignment (GTK_MISC (ipv6_label), 0.0f, 0.5f);
		gtk_misc_set_alignment (GTK_MISC (ipv6_text), 0.0f, 0.5f);
		
		gtk_table_attach_defaults (GTK_TABLE (table), ipv6_label, 0, 1, 3, 4);
		gtk_table_attach_defaults (GTK_TABLE (table), ipv6_text, 1, 2, 3, 4);
	}
	
	if (priv->devinfo.type == DEV_WIRELESS) {
		GtkWidget *signal_label;
		GtkWidget *essid_label;
		GtkWidget *essid_text;
		float quality;
		char *text;

		/* _maybe_ we can add the encrypted icon between the essid and the signal bar. */

		priv->signalbar = gtk_progress_bar_new ();

		quality = priv->devinfo.qual / 100.0f;
		if (quality > 1.0)
		quality = 1.0;

		text = g_strdup_printf ("%d %%", priv->devinfo.qual);
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->signalbar), quality);
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->signalbar), text);
		g_free(text);

		signal_label = gtk_label_new (_("Signal Strength:"));
		essid_label = gtk_label_new (_("ESSID:"));
		essid_text = gtk_label_new (priv->devinfo.essid);

		gtk_misc_set_alignment (GTK_MISC (signal_label), 0.0f, 0.5f);
		gtk_misc_set_alignment (GTK_MISC (essid_label), 0.0f, 0.5f);
		gtk_misc_set_alignment (GTK_MISC (essid_text), 0.0f, 0.5f);

		gtk_label_set_selectable (GTK_LABEL (essid_text), TRUE);

		gtk_table_attach_defaults (GTK_TABLE (table), signal_label, 2, 3, 4, 5);
		gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (priv->signalbar), 3, 4, 4, 5);
		gtk_table_attach_defaults (GTK_TABLE (table), essid_label, 0, 3, 4, 5);
		gtk_table_attach_defaults (GTK_TABLE (table), essid_text, 1, 4, 4, 5);
	}

#if 0
	g_signal_connect(G_OBJECT(applet->drawingarea), "expose_event",
			 GTK_SIGNAL_FUNC(da_expose_event),
			 (gpointer)applet);
#endif

	g_signal_connect (G_OBJECT (incolor_sel), "color_set", 
			 G_CALLBACK (incolor_changed_cb),
			 self);
	
	g_signal_connect (G_OBJECT (outcolor_sel), "color_set",
			 G_CALLBACK (outcolor_changed_cb),
			 self);

	gtk_box_pack_start(GTK_BOX(box), da_frame, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), table, FALSE, FALSE, 0);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), box);
	gtk_widget_show_all(box);
}

static void
info_dialog_constructed (GObject *object)
{
	InfoDialogPrivate *priv = INFO_DIALOG (object)->priv;
	char *graph_color_in, *graph_color_out;
	GdkColor in_color, out_color;

	g_object_get (priv->settings,
			"graph-color-in", &graph_color_in,
			"graph-color-out", &graph_color_out,
			NULL);
	gdk_color_parse (graph_color_in, &in_color);
	gdk_color_parse (graph_color_out, &out_color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (priv->incolor_selector),  &in_color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (priv->outcolor_selector),  &out_color);

	if (G_OBJECT_CLASS (info_dialog_parent_class)->constructed) {
		G_OBJECT_CLASS (info_dialog_parent_class)->constructed (object);
	}
}

static void
info_dialog_set_property (GObject    *object,
						guint         property_id,
						const GValue *value,
						GParamSpec   *pspec)
{
	InfoDialogPrivate *priv = INFO_DIALOG (object)->priv;

	switch (property_id) {
		case PROP_SETTINGS:
			priv->settings = g_value_dup_object (value);
			break;
	}
}

static void
info_dialog_get_property (GObject    *object,
						guint         property_id,
						GValue       *value,
						GParamSpec   *pspec)
{
	InfoDialogPrivate *priv = INFO_DIALOG (object)->priv;

	switch (property_id) {
		case PROP_SETTINGS:
			g_value_set_object (value, priv->settings);
			break;
	}
}

static void
info_dialog_dispose (GObject *object)
{
	G_OBJECT_CLASS (info_dialog_parent_class)->dispose (object);
}

static void
info_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS (info_dialog_parent_class)->finalize (object);
}

GtkWidget*
info_dialog_new (Settings *settings)
{
	return g_object_new (INFO_DIALOG_TYPE,
						"settings", settings,
						"has-separator", FALSE,
						NULL);
}

void
info_dialog_device_changed (InfoDialog *info_dialog)
{
#if 0
		for (i = 0; i < GRAPH_VALUES; i++)
		{
			priv->in_graph[i] = -1;
			priv->out_graph[i] = -1;
		}
		priv->max_graph = 0;
		priv->index_graph = 0;
#endif

}

void
info_dialog_update (InfoDialog *info_dialog)
{
	InfoDialogPrivate *priv = INFO_DIALOG (info_dialog)->priv;
	char *inbytes, *outbytes;
	gboolean show_bits;

	if (priv->signalbar) {
		float quality;
		char *text;

		quality = priv->devinfo.qual / 100.0f;
		if (quality > 1.0)
			quality = 1.0;

		text = g_strdup_printf ("%d %%", priv->devinfo.qual);
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->signalbar), quality);
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->signalbar), text);
		g_free(text);
	}

	g_object_get (priv->settings,
			"display-bits", &show_bits,
			NULL);
	/* Refresh the values of the Infodialog */
	if (priv->inbytes_text) {
		inbytes = bytes_to_string((double)priv->devinfo.rx, FALSE, show_bits);
		gtk_label_set_text(GTK_LABEL(priv->inbytes_text), inbytes);
		g_free(inbytes);
	}	
	if (priv->outbytes_text) {
		outbytes = bytes_to_string((double)priv->devinfo.tx, FALSE, show_bits);
		gtk_label_set_text(GTK_LABEL(priv->outbytes_text), outbytes);
		g_free(outbytes);
	}
#if 0
	/* Redraw the graph of the Infodialog */
	if (priv->drawingarea)
		redraw_graph(priv);
#endif
}

