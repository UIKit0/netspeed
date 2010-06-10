#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include "backend.h"
#include "settings-dialog.h"

enum
{
	PROP_0,
	PROP_SETTINGS
};

struct _SettingsDialogPrivate
{
	GtkWidget *network_device_combo;
	GtkWidget *show_sum_checkbutton;
	GtkWidget *show_bits_checkbutton;
	GtkWidget *change_icon_checkbutton;

	Settings *settings;
};

#define SETTINGS_DIALOG_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), SETTINGS_DIALOG_TYPE, SettingsDialogPrivate))

static void settings_dialog_class_init (SettingsDialogClass *klass);
static void settings_dialog_init       (SettingsDialog *self);
static void settings_dialog_constructed(GObject *object);
static void settings_dialog_dispose    (GObject *object);
static void settings_dialog_finalize   (GObject *object);
static void settings_dialog_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void settings_dialog_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);

G_DEFINE_TYPE (SettingsDialog, settings_dialog, GTK_TYPE_DIALOG);

static void
settings_dialog_class_init (SettingsDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (SettingsDialogPrivate));

	object_class->constructed = settings_dialog_constructed;
	object_class->dispose = settings_dialog_dispose;
	object_class->finalize = settings_dialog_finalize;
	object_class->set_property = settings_dialog_set_property;
	object_class->get_property = settings_dialog_get_property;

	g_object_class_install_property (object_class, PROP_SETTINGS,
		g_param_spec_object ("settings",
							 "Settings",
							 "The netspeed settings.",
							 SETTINGS_TYPE,
							 G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

/* Handle preference dialog response event
 */
static void
pref_response_cb (GtkDialog *dialog, gint id, gpointer data)
{
#if 0
    NetspeedApplet *applet = data;
  
    if(id == GTK_RESPONSE_HELP){
        display_help (GTK_WIDGET (dialog), "netspeed_applet-settings");
	return;
    }
    panel_applet_gconf_set_string(PANEL_APPLET(applet->applet), "device", applet->devinfo.name, NULL);
    panel_applet_gconf_set_bool(PANEL_APPLET(applet->applet), "show_sum", applet->show_sum, NULL);
    panel_applet_gconf_set_bool(PANEL_APPLET(applet->applet), "show_bits", applet->show_bits, NULL);
    panel_applet_gconf_set_bool(PANEL_APPLET(applet->applet), "change_icon", applet->change_icon, NULL);
    panel_applet_gconf_set_bool(PANEL_APPLET(applet->applet), "auto_change_device", applet->auto_change_device, NULL);
    panel_applet_gconf_set_bool(PANEL_APPLET(applet->applet), "have_settings", TRUE, NULL);

    gtk_widget_destroy(GTK_WIDGET(applet->settings));
    applet->settings = NULL;
#endif
}

#if 0
/* this basically just retrieves the new devicestring 
 * and then calls applet_device_change() and change_icons()
 */
static void
device_change_cb(GtkComboBox *combo, NetspeedApplet *applet)
{
	GList *devices;
	int i, active;

	g_assert(combo);
	devices = g_object_get_data(G_OBJECT(combo), "devices");
	active = gtk_combo_box_get_active(combo);
	g_assert(active > -1);

	if (0 == active) {
		if (applet->auto_change_device)
			return;
		applet->auto_change_device = TRUE;
	} else {
		applet->auto_change_device = FALSE;
		for (i = 1; i < active; i++) {
			devices = g_list_next(devices);
		}
		if (g_str_equal(devices->data, applet->devinfo.name))
			return;
		free_device_info(&applet->devinfo);
		get_device_info(devices->data, &applet->devinfo);
	}

	applet->device_has_changed = TRUE;
	update_applet(applet);
}


/* Called when the showsum checkbutton is toggled...
 */
static void
showsum_change_cb(GtkToggleButton *togglebutton, NetspeedApplet *applet)
{
	applet->show_sum = gtk_toggle_button_get_active(togglebutton);
	applet_change_size_or_orient(applet->applet, -1, (gpointer)applet);
	change_icons(applet);
}

/* Called when the showbits checkbutton is toggled...
 */
static void
showbits_change_cb(GtkToggleButton *togglebutton, NetspeedApplet *applet)
{
	applet->show_bits = gtk_toggle_button_get_active(togglebutton);
}

/* Called when the changeicon checkbutton is toggled...
 */
static void
changeicon_change_cb(GtkToggleButton *togglebutton, NetspeedApplet *applet)
{
	applet->change_icon = gtk_toggle_button_get_active(togglebutton);
	change_icons(applet);
}
#endif

static void
settings_dialog_init (SettingsDialog *self)
{
	SettingsDialogPrivate *priv;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *categories_vbox;
	GtkWidget *category_vbox;
	GtkWidget *controls_vbox;
	GtkWidget *category_header_label;
	GtkWidget *network_device_hbox;
	GtkWidget *network_device_label;
	GtkWidget *indent_label;
	GtkSizeGroup *category_label_size_group;
	GtkSizeGroup *category_units_size_group;
	gchar *header_str;

	priv = SETTINGS_DIALOG_GET_PRIVATE (self);
	self->priv = priv;

	category_label_size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	category_units_size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	gtk_dialog_add_buttons (GTK_DIALOG (self),
							GTK_STOCK_HELP, GTK_RESPONSE_HELP,
							GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT,
							NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(self),
									GTK_RESPONSE_CLOSE);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (self), TRUE);
	gtk_window_set_title (GTK_WINDOW (self), _("Netspeed Preferences"));
	
	gtk_window_set_resizable(GTK_WINDOW(self), FALSE);
	gtk_window_set_screen(GTK_WINDOW(self), 
						gtk_widget_get_screen(GTK_WIDGET(self)));

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

	categories_vbox = gtk_vbox_new(FALSE, 18);
	gtk_box_pack_start(GTK_BOX (vbox), categories_vbox, TRUE, TRUE, 0);

	category_vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX (categories_vbox), category_vbox, TRUE, TRUE, 0);
	
	header_str = g_strconcat("<span weight=\"bold\">", _("General Settings"), "</span>", NULL);
	category_header_label = gtk_label_new(header_str);
	gtk_label_set_use_markup(GTK_LABEL(category_header_label), TRUE);
	gtk_label_set_justify(GTK_LABEL(category_header_label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC (category_header_label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX (category_vbox), category_header_label, FALSE, FALSE, 0);
	g_free(header_str);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX (category_vbox), hbox, TRUE, TRUE, 0);

	indent_label = gtk_label_new("    ");
	gtk_label_set_justify(GTK_LABEL (indent_label), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX (hbox), indent_label, FALSE, FALSE, 0);
		
	controls_vbox = gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(hbox), controls_vbox, TRUE, TRUE, 0);

	network_device_hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(controls_vbox), network_device_hbox, TRUE, TRUE, 0);
	
	network_device_label = gtk_label_new_with_mnemonic(_("Network _device:"));
	gtk_label_set_justify(GTK_LABEL(network_device_label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(network_device_label), 0.0f, 0.5f);
	gtk_size_group_add_widget(category_label_size_group, network_device_label);
	gtk_box_pack_start(GTK_BOX (network_device_hbox), network_device_label, FALSE, FALSE, 0);
	
	priv->network_device_combo = gtk_combo_box_new_text();
	gtk_label_set_mnemonic_widget(GTK_LABEL(network_device_label), priv->network_device_combo);
	gtk_box_pack_start (GTK_BOX (network_device_hbox), priv->network_device_combo, TRUE, TRUE, 0);

	priv->show_sum_checkbutton = gtk_check_button_new_with_mnemonic(_("Show _sum instead of in & out"));
	gtk_box_pack_start(GTK_BOX(controls_vbox), priv->show_sum_checkbutton, FALSE, FALSE, 0);
	
	priv->show_bits_checkbutton = gtk_check_button_new_with_mnemonic(_("Show _bits instead of bytes"));
	gtk_box_pack_start(GTK_BOX(controls_vbox), priv->show_bits_checkbutton, FALSE, FALSE, 0);
	
	priv->change_icon_checkbutton = gtk_check_button_new_with_mnemonic(_("Change _icon according to the selected device"));
	gtk_box_pack_start(GTK_BOX(controls_vbox), priv->change_icon_checkbutton, FALSE, FALSE, 0);

#if 0
	g_signal_connect(G_OBJECT (priv->network_device_combo), "changed",
			 G_CALLBACK(device_change_cb), (gpointer)applet);

	g_signal_connect(G_OBJECT (show_sum_checkbutton), "toggled",
			 G_CALLBACK(showsum_change_cb), (gpointer)applet);

	g_signal_connect(G_OBJECT (show_bits_checkbutton), "toggled",
			 G_CALLBACK(showbits_change_cb), (gpointer)applet);

	g_signal_connect(G_OBJECT (change_icon_checkbutton), "toggled",
			 G_CALLBACK(changeicon_change_cb), (gpointer)applet);

	g_signal_connect(G_OBJECT (priv->settings), "response",
			 G_CALLBACK(pref_response_cb), (gpointer)applet);
#endif

	gtk_container_add(GTK_CONTAINER (GTK_DIALOG (self)->vbox), vbox);
}

static void
settings_dialog_constructed (GObject *object)
{
	SettingsDialogPrivate *priv = SETTINGS_DIALOG (object)->priv;
	gboolean show_sum, show_bits, change_icon, auto_change_device;
	char *device;
	GList *ptr, *devices;
	int i, active = -1;

	g_object_get (priv->settings,
				"device", &device,
				"show-sum", &show_sum,
				"show-bits", &show_bits,
				"change-icon", &change_icon,
				"auto-change-device", &auto_change_device,
				NULL);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->show_sum_checkbutton), show_sum);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->show_bits_checkbutton), show_bits);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->change_icon_checkbutton), change_icon);

	/* Default means device with default route set */
	gtk_combo_box_append_text(GTK_COMBO_BOX(priv->network_device_combo), _("Default"));
	ptr = devices = get_available_devices();
	for (i = 1; ptr; ptr = g_list_next(ptr)) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(priv->network_device_combo), ptr->data);
		if (g_str_equal(ptr->data, device)) active = i;
		++i;
	}
	g_object_set_data_full(G_OBJECT(priv->network_device_combo), "devices", devices, (GDestroyNotify)free_devices_list);
	if (active < 0 || auto_change_device) {
		active = 0;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(priv->network_device_combo), active);

	if (G_OBJECT_CLASS (settings_dialog_parent_class)->constructed) {
		G_OBJECT_CLASS (settings_dialog_parent_class)->constructed (object);
	}
}


static void
settings_dialog_set_property (GObject    *object,
							guint         property_id,
							const GValue *value,
							GParamSpec   *pspec)
{
	SettingsDialogPrivate *priv = SETTINGS_DIALOG (object)->priv;

	switch (property_id) {
		case PROP_SETTINGS:
			priv->settings = g_value_get_object (value);
			break;
	}
}

static void
settings_dialog_get_property (GObject    *object,
							guint         property_id,
							GValue       *value,
							GParamSpec   *pspec)
{
	SettingsDialogPrivate *priv = SETTINGS_DIALOG (object)->priv;

	switch (property_id) {
		case PROP_SETTINGS:
			g_value_set_object (value, priv->settings);
			break;
	}
}

static void
settings_dialog_dispose (GObject *object)
{
	SettingsDialogPrivate *priv = SETTINGS_DIALOG (object)->priv;

	if (priv->settings) {
		g_object_unref (priv->settings);
		priv->settings = NULL;
	}

	G_OBJECT_CLASS (settings_dialog_parent_class)->dispose (object);
}

static void
settings_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS (settings_dialog_parent_class)->finalize (object);
}

GtkWidget* settings_dialog_new (Settings *settings)
{
	return g_object_new (SETTINGS_DIALOG_TYPE,
						"settings", settings,
						"has-separator", FALSE,
						NULL);
}
