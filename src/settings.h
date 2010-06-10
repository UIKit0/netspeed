/*  settings.h
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

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define SETTINGS_TYPE            (settings_get_type ())
#define SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SETTINGS_TYPE, Settings))
#define SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SETTINGS_TYPE, SettingsClass))
#define IS_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SETTINGS_TYPE))
#define IS_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SETTINGS_TYPE))
#define SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SETTINGS_TYPE, SettingsClass))

typedef struct _Settings        Settings;
typedef struct _SettingsClass   SettingsClass;
typedef struct _SettingsPrivate SettingsPrivate;

struct _SettingsClass
{
	GObjectClass parent_class;
};

struct _Settings
{
	GObject parent;
	SettingsPrivate *priv;
};

GType     settings_get_type (void);

Settings* settings_new (void);

G_END_DECLS

#endif
