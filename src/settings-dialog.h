/*  settings-dialog.h
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

#ifndef _SETTINGS_DIALOG_H
#define _SETTINGS_DIALOG_H

#include <gtk/gtk.h>
#include "settings.h"

G_BEGIN_DECLS

#define SETTINGS_DIALOG_TYPE            (settings_dialog_get_type ())
#define SETTINGS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SETTINGS_DIALOG_TYPE, SettingsDialog))
#define SETTINGS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SETTINGS_DIALOG_TYPE, SettingsDialogClass))
#define IS_SETTINGS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SETTINGS_DIALOG_TYPE))
#define IS_SETTINGS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SETTINGS_DIALOG_TYPE))
#define SETTINGS_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SETTINGS_DIALOG_TYPE, SettingsDialogClass))

typedef struct _SettingsDialog        SettingsDialog;
typedef struct _SettingsDialogClass   SettingsDialogClass;
typedef struct _SettingsDialogPrivate SettingsDialogPrivate;

struct _SettingsDialogClass
{
	GtkDialogClass parent_class;
};

struct _SettingsDialog
{
	GtkDialog parent;
	SettingsDialogPrivate *priv;
	
};

GType      settings_dialog_get_type (void);

GtkWidget* settings_dialog_new (Settings*);

G_END_DECLS

#endif
