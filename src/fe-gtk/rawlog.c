/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
 * Copyright (c) 2023-2024 somercet
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "fe-gtk.h"

#include <gdk/gdkkeysyms.h>

#include "../common/hexchat.h"
#include "../common/hexchatc.h"
#include "../common/cfgfiles.h"
#include "../common/server.h"
#include "gtkutil.h"
#include "palette.h"
#include "maingui.h"
#include "rawlog.h"
//#include "x text.h"
#include "xcchatview.h"
#include "fkeys.h"

//xyzzy medium how to close
static void
close_rawlog (GtkWidget *wid, server *serv)
{
	if (is_server (serv))
		serv->gui->rawlog_window = 0;
}

static void
rawlog_save (server *serv, char *file)
{
	int fh = -1;

	if (file)
	{
		if (serv->gui->rawlog_window)
			fh = hexchat_open_file (file, O_TRUNC | O_WRONLY | O_CREAT,
										 0600, XOF_DOMODE | XOF_FULLPATH);
		if (fh != -1)
		{
//wyzzy easy
			xc_chat_view_save (XC_CHAT_VIEW (serv->gui->rawlog_textlist), fh);
			close (fh);
		}
	}
}

static int
rawlog_clearbutton (GtkWidget * wid, server *serv)
{
//wyzzy easy
	xc_chat_view_clear (XC_CHAT_VIEW (serv->gui->rawlog_textlist), 0);
	return FALSE;
}

static int
rawlog_savebutton (GtkWidget * wid, server *serv)
{
	gtkutil_file_req (NULL, _("Save As..."), rawlog_save, serv, NULL, NULL, FRF_WRITE);
	return FALSE;
}

static gboolean
rawlog_key_cb (GtkWidget * wid, GdkEventKey * key, gpointer userdata)
{
	/* Copy rawlog selection to clipboard when Ctrl+Shift+C is pressed,
	 * but make sure not to copy twice, i.e. when auto-copy is enabled.
	 */
	if (!prefs.hex_text_autocopy_text &&
		(key->keyval == GDK_KEY_c || key->keyval == GDK_KEY_C) &&
		key->state & STATE_SHIFT &&
		key->state & STATE_CTRL)
	{
//wyzzy easy
		xc_chat_view_copy_selection (XC_CHAT_VIEW (userdata));
	}
	return FALSE;
}

void
open_rawlog (struct server *serv)
{
	GtkWidget *bbox, *scrolledwindow, *vbox;
	char tbuf[256];

	if (serv->gui->rawlog_window)
	{
		mg_bring_tofront (serv->gui->rawlog_window);
		return;
	}

	g_snprintf (tbuf, sizeof tbuf, _("Raw Log (%s) - %s"), serv->servername, _(DISPLAY_NAME));
	serv->gui->rawlog_window =
		mg_create_generic_tab ("RawLog", tbuf, FALSE, TRUE, close_rawlog, serv,
							 640, 320, &vbox, serv);
	gtkutil_destroy_on_esc (serv->gui->rawlog_window);

	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (vbox), scrolledwindow);

//wyzzy easy
	serv->gui->rawlog_textlist = (GObject *) xc_chat_view_new ();
	gtk_container_add (GTK_CONTAINER (scrolledwindow), GTK_WIDGET ( ((XcChatView *) serv->gui->rawlog_textlist)->tview));
	xc_chat_view_set_font (XC_CHAT_VIEW (serv->gui->rawlog_textlist), prefs.hex_text_font);
	//GTK_XTEXT (serv->gui->rawlog_textlist)->ignore_hidden = 1;

	bbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_end (GTK_BOX (vbox), bbox, 0, 0, 4);

	gtkutil_button (bbox, GTK_STOCK_CLEAR, NULL, rawlog_clearbutton,
						 serv, _("Clear Raw Log"));

	gtkutil_button (bbox, GTK_STOCK_SAVE_AS, NULL, rawlog_savebutton,
						 serv, _("Save As..."));

	/* Copy selection to clipboard when Ctrl+Shift+C is pressed AND text auto-copy is disabled */
	g_signal_connect (G_OBJECT (serv->gui->rawlog_window), "key_press_event", G_CALLBACK (rawlog_key_cb), serv->gui->rawlog_textlist);

	gtk_widget_show_all (serv->gui->rawlog_window);
}

void
fe_add_rawlog (server *serv, char *text, int len, int outbound)
{
	char **split_text;
	char *new_text;
	size_t i;

	if (!serv->gui->rawlog_window)
		return;

	split_text = g_strsplit (text, "\r\n", 0);

	for (i = 0; i < g_strv_length (split_text); i++)
	{
		if (split_text[i][0] == 0)
			break;

		if (outbound)
			new_text = g_strconcat ("\0034<<\017 ", split_text[i], NULL);
		else
			new_text = g_strconcat ("\0033>>\017 ", split_text[i], NULL);

//wyzzy easy
		xc_chat_view_append (XC_CHAT_VIEW (serv->gui->rawlog_textlist), new_text, strlen (new_text), 0);

		g_free (new_text);
	}

	g_strfreev (split_text);
}
