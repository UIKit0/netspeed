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

#include "netspeed.h"

typedef struct _NetspeedPrivate NetspeedPrivate;

struct _NetspeedPrivate
{
	int dummy;
};

#define NETSPEED_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), NETSPEED_TYPE, NetspeedPrivate))

static void netspeed_class_init (NetspeedClass *klass);
static void netspeed_init       (Netspeed *self);
static void netspeed_dispose    (GObject *object);
static void netspeed_finalize   (GObject *object);

G_DEFINE_TYPE (Netspeed, netspeed, PANEL_TYPE_APPLET);

static void
netspeed_class_init (NetspeedClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (NetspeedPrivate));

	object_class->dispose = netspeed_dispose;
	object_class->finalize = netspeed_finalize;
}

static void
netspeed_init (Netspeed *self)
{
}

static void
netspeed_dispose (GObject *object)
{
	G_OBJECT_CLASS (netspeed_parent_class)->dispose (object);
}

static void
netspeed_finalize (GObject *object)
{
	G_OBJECT_CLASS (netspeed_parent_class)->finalize (object);
}
