/*  netspeed.h
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

#ifndef _NETSPEED_H
#define _NETSPEED_H

#include <panel-applet.h>

G_BEGIN_DECLS

#define NETSPEED_TYPE            (netspeed_get_type ())
#define NETSPEED(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NETSPEED_TYPE, Netspeed))
#define NETSPEED_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NETSPEED_TYPE, NetspeedClass))
#define IS_NETSPEED(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NETSPEED_TYPE))
#define IS_NETSPEED_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NETSPEED_TYPE))
#define NETSPEED_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NETSPEED_TYPE, NetspeedClass))

typedef struct _Netspeed      Netspeed;
typedef struct _NetspeedClass NetspeedClass;

struct _NetspeedClass
{
	PanelAppletClass parent_class;
};

struct _Netspeed
{
	PanelApplet parent;
};

GType netspeed_get_type (void);

G_END_DECLS

#endif
