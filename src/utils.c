/*  utils.c
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

#include <glib/gi18n.h>
#include "utils.h"

/* Converts a number of bytes into a human
 * readable string - in [M/k]bytes[/s]
 * The string has to be freed
 */
char* 
bytes_to_string(double bytes, gboolean per_sec, gboolean bits)
{
	const char *format;
	const char *unit;
	guint kilo; /* no really a kilo : a kilo or kibi */

	if (bits) {
		bytes *= 8;
		kilo = 1000;
	} else
		kilo = 1024;

	if (bytes < kilo) {

		format = "%.0f %s";

		if (per_sec)
			unit = bits ? N_("b/s")   : N_("B/s");
		else
			unit = bits ? N_("bits")  : N_("bytes");

	} else if (bytes < (kilo * kilo)) {
		format = (bytes < (100 * kilo)) ? "%.1f %s" : "%.0f %s";
		bytes /= kilo;

		if (per_sec)
			unit = bits ? N_("kb/s") : N_("KiB/s");
		else
			unit = bits ? N_("kb")   : N_("KiB");

	} else {

		format = "%.1f %s";

		bytes /= kilo * kilo;

		if (per_sec)
			unit = bits ? N_("Mb/s") : N_("MiB/s");
		else
			unit = bits ? N_("Mb")   : N_("MiB");
	}

	return g_strdup_printf(format, bytes, gettext(unit));
}


gboolean
open_uri (GtkWidget *parent, const char *url, GError **error)
{
	gboolean ret;
	char *cmdline;
	GdkScreen *screen;

	screen = gtk_widget_get_screen (parent);
	cmdline = g_strconcat ("xdg-open ", url, NULL);
	ret = gdk_spawn_command_line_on_screen (screen, cmdline, error);
	g_free (cmdline);

	return ret;
}

/* Adds a Pango markup "size" to a bytestring
 */
static void
add_markup_size(char **string, int size)
{
	char *tmp = *string;
	*string = g_strdup_printf("<span size=\"%d\">%s</span>", size * 1000, tmp);
	g_free(tmp);
}

/* Adds a Pango markup "foreground" to a bytestring
 */
static void
add_markup_fgcolor(char **string, const char *color)
{
	char *tmp = *string;
	*string = g_strdup_printf("<span foreground=\"%s\">%s</span>", color, tmp);
	g_free(tmp);
}

