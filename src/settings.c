/*  settings.c
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
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "settings.h"

enum
{
	PROP_0, /* dummy */
	PROP_DEVICE,
	PROP_GCONF_PATH,
	PROP_DEFAULT_ROUTE,
	PROP_DISPLAY_SUM,
	PROP_DISPLAY_BITS,
	PROP_DISPLAY_SPECIFIC_ICON,
	PROP_GRAPH_COLOR_IN,
	PROP_GRAPH_COLOR_OUT,
	PROP_IFUP_CMD,
	PROP_IFDOWN_CMD
};

struct _SettingsPrivate
{
	char *device;
	char *gconf_path;
	gboolean follow_default_route;
	struct {
		gboolean sum;
		gboolean bits;
		gboolean specific_icon;
	} display;
	struct {
		char* in;
		char* out;
	} graph_colors;
	struct {
		char *ifup;
		char *ifdown;
	} commands;
};

#define SETTINGS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), SETTINGS_TYPE, SettingsPrivate))

static void  settings_class_init (SettingsClass *klass);
static void  settings_init       (Settings *self);
static void  settings_finalize   (GObject *object);
static void  settings_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void  settings_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void  settings_store_property (Settings *settings, int property_id, const GValue *value);
static void  settings_load_all       (Settings *settings);
static char* settings_gconf_key_from_property_id (Settings *settings, int property_id);

G_DEFINE_TYPE (Settings, settings, G_TYPE_OBJECT);

static void
settings_class_init (SettingsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (SettingsPrivate));

	object_class->finalize = settings_finalize;
	object_class->set_property = settings_set_property;
	object_class->get_property = settings_get_property;

	g_object_class_install_property (object_class, PROP_DEVICE,
		g_param_spec_string ("device",
							"Device",
							"The device that is monitored.",
							"lo",
							G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_GCONF_PATH,
		g_param_spec_string ("gconf-path",
							"GConf path",
							"The applet instance' gconf path.",
							NULL,
							G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_DEFAULT_ROUTE,
		g_param_spec_boolean ("default-route",
							"Default route",
							"Track the device which has the default route set.",
							TRUE,
							G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_DISPLAY_SUM,
		g_param_spec_boolean ("display-sum",
							"Display sum",
							"Display the sum of incoming and outgoing traffic, instead of both individually.",
							TRUE,
							G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_DISPLAY_BITS,
		g_param_spec_boolean ("display-bits",
							"Display Bits",
							"Display traffic in bits instead of bytes.",
							FALSE,
							G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_DISPLAY_SPECIFIC_ICON,
		g_param_spec_boolean ("display-specific-icon",
							"Display specific icon",
							"Display a device specific icon.",
							TRUE,
							G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_GRAPH_COLOR_IN,
		g_param_spec_string ("graph-color-in",
							"Graph color in",
							"GdkColor for incoming traffic graph.",
							"#00FF00",
							G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_GRAPH_COLOR_OUT,
		g_param_spec_string ("graph-color-out",
							"Graph color out",
							"GdkColor for outgoing traffic graph.",
							"#FF0000",
							G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_IFUP_CMD,
		g_param_spec_string ("ifup-command",
							"Ifup Command",
							"Command to bring the specified device up.",
							"/sbin/ifup",
							G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_IFDOWN_CMD,
		g_param_spec_string ("ifdown-command",
							"Ifdown Command",
							"Command to bring the specified device down.",
							"/sbin/ifdown",
							G_PARAM_READWRITE));
}

static void
settings_init (Settings *self)
{
	SettingsPrivate *priv = SETTINGS_GET_PRIVATE (self);
	self->priv = priv;

	priv->device = g_strdup ("lo");
	priv->gconf_path = NULL;
	priv->follow_default_route = TRUE;
	priv->display.sum = FALSE;
	priv->display.bits = FALSE;
	priv->display.specific_icon = TRUE;
	priv->graph_colors.in = g_strdup ("#00FF00");
	priv->graph_colors.out = g_strdup ("#FF0000");
	priv->commands.ifup = g_strdup ("/sbin/ifup");
	priv->commands.ifdown = g_strdup ("/sbin/ifdown");
}

static void
settings_set_property (GObject       *object,
						guint         property_id,
						const GValue *value,
						GParamSpec   *pspec)
{
	SettingsPrivate *priv = SETTINGS (object)->priv;

	switch (property_id) {
		case PROP_DEVICE:
			g_free (priv->device);
			priv->device = g_value_dup_string (value);
			break;
		case PROP_GCONF_PATH:
			g_free (priv->gconf_path);
			priv->gconf_path = g_value_dup_string (value);
			settings_load_all (SETTINGS (object));
			return;
		case PROP_DEFAULT_ROUTE:
			priv->follow_default_route = g_value_get_boolean (value);
			break;
		case PROP_DISPLAY_SUM:
			priv->display.sum = g_value_get_boolean (value);
			break;
		case PROP_DISPLAY_BITS:
			priv->display.bits = g_value_get_boolean (value);
			break;
		case PROP_DISPLAY_SPECIFIC_ICON:
			priv->display.specific_icon = g_value_get_boolean (value);
			break;
		case PROP_GRAPH_COLOR_IN:
			g_free (priv->graph_colors.in);
			priv->graph_colors.in = g_value_dup_string (value);
			break;
		case PROP_GRAPH_COLOR_OUT:
			g_free (priv->graph_colors.out);
			priv->graph_colors.out = g_value_dup_string (value);
			break;
		case PROP_IFUP_CMD:
			g_free (priv->commands.ifup);
			priv->commands.ifup = g_value_dup_string (value);
			break;
		case PROP_IFDOWN_CMD:
			g_free (priv->commands.ifdown);
			priv->commands.ifdown = g_value_dup_string (value);
			break;
		default:
			return;
	}
	g_object_notify (object, g_param_spec_get_name (pspec));
	if (priv->gconf_path) {
		settings_store_property (SETTINGS (object),
								property_id,
								value);
	}
}

static void
settings_get_property (GObject       *object,
						guint         property_id,
						GValue       *value,
						GParamSpec   *pspec)
{
	SettingsPrivate *priv = SETTINGS (object)->priv;

	switch (property_id) {
		case PROP_DEVICE:
			g_value_set_string (value, priv->device);
			break;
		case PROP_GCONF_PATH:
			g_value_set_string (value, priv->gconf_path);
			break;
		case PROP_DEFAULT_ROUTE:
			g_value_set_boolean (value, priv->follow_default_route);
			break;
		case PROP_DISPLAY_SUM:
			g_value_set_boolean (value, priv->display.sum);
			break;
		case PROP_DISPLAY_BITS:
			g_value_set_boolean (value, priv->display.bits);
			break;
		case PROP_DISPLAY_SPECIFIC_ICON:
			g_value_set_boolean (value, priv->display.specific_icon);
			break;
		case PROP_GRAPH_COLOR_IN:
			g_value_set_string (value, priv->graph_colors.in);
			break;
		case PROP_GRAPH_COLOR_OUT:
			g_value_set_string (value, priv->graph_colors.out);
			break;
		case PROP_IFUP_CMD:
			g_value_set_string (value, priv->commands.ifup);
			break;
		case PROP_IFDOWN_CMD:
			g_value_set_string (value, priv->commands.ifdown);
			break;
	}
}

static char*
settings_gconf_key_from_property_id (Settings *settings, int property_id)
{
	SettingsPrivate *priv = settings->priv;
	char *gconf_path = priv->gconf_path;

	g_return_val_if_fail (gconf_path != NULL, NULL);

	switch (property_id) {
		case PROP_DEVICE:
			return gconf_concat_dir_and_key (gconf_path, "device");
		case PROP_DEFAULT_ROUTE:
			return gconf_concat_dir_and_key (gconf_path, "auto_change_device");
		case PROP_DISPLAY_SUM:
			return gconf_concat_dir_and_key (gconf_path, "show_sum");
		case PROP_DISPLAY_BITS:
			return gconf_concat_dir_and_key (gconf_path, "show_bits");
		case PROP_DISPLAY_SPECIFIC_ICON:
			return gconf_concat_dir_and_key (gconf_path, "change_icon");
		case PROP_GRAPH_COLOR_IN:
			return gconf_concat_dir_and_key (gconf_path, "in_color");
		case PROP_GRAPH_COLOR_OUT:
			return gconf_concat_dir_and_key (gconf_path, "out_color");
		case PROP_IFUP_CMD:
			return gconf_concat_dir_and_key (gconf_path, "up_command");
		case PROP_IFDOWN_CMD:
			return gconf_concat_dir_and_key (gconf_path, "down_command");
		case PROP_0:
		case PROP_GCONF_PATH:
			return NULL;
		default:
			g_warning ("No property with id %d", property_id);
			return NULL;
	}
}

static void
settings_store_property (Settings *settings, int property_id, const GValue *value)
{
	SettingsPrivate *priv = settings->priv;
	GConfClient *client;
	GError *error = NULL;
	char *key;

	g_return_if_fail (priv->gconf_path != NULL);

	client = gconf_client_get_default();
	g_return_if_fail (client != NULL);

	key = settings_gconf_key_from_property_id (settings, property_id);
	g_return_if_fail (key != NULL);

	if (G_VALUE_HOLDS_STRING (value)) {
		gconf_client_set_string (client, key, g_value_get_string (value), &error);
	} else
	if (G_VALUE_HOLDS_BOOLEAN (value)) {
		gconf_client_set_bool (client, key, g_value_get_boolean (value), &error);
	} else {
		g_warning ("Don't know how to convert property %d", property_id);
	}
	if (error) {
		char* contents = g_strdup_value_contents (value);
		g_warning ("Could not store value %s in key %s: %s", contents, key, error->message);
		g_free (contents);
		g_clear_error (&error);
	}
}

static void
settings_load_all (Settings *settings)
{
	SettingsPrivate *priv = settings->priv;
	SettingsClass* class;
	GConfClient *client;
	GParamSpec** specs;
	int i, n_specs;

	if (!priv->gconf_path)
		return;

	client = gconf_client_get_default();
	g_return_if_fail (client != NULL);

	g_object_freeze_notify (G_OBJECT (settings));

	class = SETTINGS_GET_CLASS (settings);
	specs = g_object_class_list_properties (G_OBJECT_CLASS (class), &n_specs);
	for (i = 0; i < n_specs; i++) {
		GConfValue *value;
		GError *error = NULL;
		char *key;
		const char *property_name;

		key = settings_gconf_key_from_property_id (settings, i + 1);
		if (!key) {
			continue;
		}
		value = gconf_client_get (client, key, &error);
		if (error) {
			g_warning ("Could not get value for key %s: %s", key, error->message);
			g_clear_error (&error);
		}
		property_name = g_param_spec_get_name (specs[i]);
		/* Would be nice if there was some way to convert gconf-values to gvalues *sigh* */
		if (value) {
			switch (value->type) {
				case GCONF_VALUE_STRING:
					g_object_set (G_OBJECT (settings),
								property_name,
								gconf_value_get_string (value),
								NULL);
					break;
				case GCONF_VALUE_BOOL:
					g_object_set (G_OBJECT (settings),
								property_name,
								gconf_value_get_bool (value),
								NULL);
					break;
				default:
					g_warning ("Don't know how to convert property %s", property_name);
			}
		} /* Otherwise fall-back to hard-coded default */
	}
	g_free (specs);
	g_object_thaw_notify (G_OBJECT (settings));
}

static void
settings_finalize (GObject *object)
{
	SettingsPrivate *priv = SETTINGS (object)->priv;

	g_free (priv->device);
	g_free (priv->graph_colors.in);
	g_free (priv->graph_colors.out);
	g_free (priv->commands.ifup);
	g_free (priv->commands.ifdown);

	G_OBJECT_CLASS (settings_parent_class)->finalize (object);
}

Settings* settings_new (void)
{
	return g_object_new (SETTINGS_TYPE, NULL);
}

Settings* settings_new_with_gconf_path (const char *gconf_path)
{
	return g_object_new (SETTINGS_TYPE,
						"gconf-path", gconf_path,
						NULL);
}
