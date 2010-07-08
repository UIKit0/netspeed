/*  utils.h
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

#ifndef _UTILS_H
#define _UTILS_H

#include <gtk/gtk.h>

char* bytes_to_string (double bytes, gboolean per_sec, gboolean bits);

gboolean open_uri (GtkWidget *parent, const char *url, GError **error);

typedef struct _RingBuffer RingBuffer;
struct _RingBuffer {
	guint64 *rx;
	guint64 *tx;
	int oldest, values, size;
};

RingBuffer *ring_buffer_new (int size);

void ring_buffer_free (RingBuffer *buffer);

void ring_buffer_reset (RingBuffer *buffer);

void ring_buffer_append (RingBuffer *buffer, guint64 rx, guint64 tx);

void ring_buffer_average (RingBuffer *buffer, float *rx, float *tx);

#endif
