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
#include "settings.h"

enum
{
	PROP_0,
	PROP_DEVICE,
	PROP_SHOW_SUM,
	PROP_SHOW_BITS,
	PROP_CHANGE_ICON,
	PROP_AUTO_CHANGE_DEVICE,
	PROP_IN_COLOR,
	PROP_OUT_COLOR,
	PROP_UP_CMD,
	PROP_DOWN_CMD
};

struct _SettingsPrivate
{
	char *device;
};

#define SETTINGS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), SETTINGS_TYPE, SettingsPrivate))

static void settings_class_init (SettingsClass *klass);
static void settings_init       (Settings *self);
static void settings_finalize   (GObject *object);
static void settings_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void settings_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);

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
}

static void
settings_init (Settings *self)
{
	SettingsPrivate *priv = SETTINGS_GET_PRIVATE (self);
	self->priv = priv;

	priv->device = g_strdup ("lo");
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
	}
}

static void
settings_finalize (GObject *object)
{
	SettingsPrivate *priv = SETTINGS (object)->priv;

	g_free (priv->device);

	G_OBJECT_CLASS (settings_parent_class)->finalize (object);
}

Settings* settings_new (void)
{
	return g_object_new (SETTINGS_TYPE, NULL);
}
