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

#ifndef _INFO_DIALOG_H
#define _INFO_DIALOG_H

#include <gtk/gtk.h>
#include "settings.h"
#include "network-device.h"

G_BEGIN_DECLS

#define INFO_DIALOG_TYPE            (info_dialog_get_type ())
#define INFO_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INFO_DIALOG_TYPE, InfoDialog))
#define INFO_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INFO_DIALOG_TYPE, InfoDialogClass))
#define IS_INFO_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INFO_DIALOG_TYPE))
#define IS_INFO_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INFO_DIALOG_TYPE))
#define INFO_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INFO_DIALOG_TYPE, InfoDialogClass))

typedef struct _InfoDialog        InfoDialog;
typedef struct _InfoDialogClass   InfoDialogClass;
typedef struct _InfoDialogPrivate InfoDialogPrivate;

struct _InfoDialogClass
{
	GtkDialogClass parent_class;
};

struct _InfoDialog
{
	GtkDialog parent;
	InfoDialogPrivate *priv;
};

GType info_dialog_get_type (void);

GtkWidget* info_dialog_new (Settings *settings, NetworkDevice *device);

void info_dialog_set_device (InfoDialog *info_dialog, NetworkDevice *device);

void info_dialog_update (InfoDialog *info_dialog);

G_END_DECLS

#endif
