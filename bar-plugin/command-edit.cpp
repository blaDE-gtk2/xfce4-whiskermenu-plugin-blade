/*
 * Copyright (C) 2013 Graeme Gott <graeme@gottcode.org>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "command-edit.h"

#include "command.h"
#include "slot.h"

#include <glib/gi18n-lib.h>

using namespace BladeMenu;

//-----------------------------------------------------------------------------

CommandEdit::CommandEdit(Command* command, GtkSizeGroup* label_size_group) :
	m_command(command)
{
	m_widget = gtk_hbox_new(false, 6);

	bool active = m_command->get_shown();

	m_shown = GTK_TOGGLE_BUTTON(gtk_check_button_new_with_mnemonic(m_command->get_text()));
	gtk_toggle_button_set_active(m_shown, active);
	gtk_box_pack_start(GTK_BOX(m_widget), GTK_WIDGET(m_shown), false, false, 0);
	gtk_size_group_add_widget(label_size_group, GTK_WIDGET(m_shown));
	g_signal_connect_slot<GtkToggleButton*>(m_shown, "toggled", &CommandEdit::shown_toggled, this);

	m_entry = GTK_ENTRY(gtk_entry_new());
	gtk_entry_set_text(m_entry, m_command->get());
	gtk_widget_set_sensitive(GTK_WIDGET(m_entry), active);
	gtk_box_pack_start(GTK_BOX(m_widget), GTK_WIDGET(m_entry), true, true, 0);
	g_signal_connect_slot<GtkEditable*>(m_entry, "changed", &CommandEdit::command_changed, this);

	m_browse_button = gtk_button_new();
	gtk_widget_set_tooltip_text(m_browse_button, _("Browse the file system to choose a custom command."));
	gtk_widget_set_sensitive(GTK_WIDGET(m_browse_button), active);
	gtk_box_pack_start(GTK_BOX(m_widget), m_browse_button, false, false, 0);
	gtk_widget_show(m_browse_button);

	GtkWidget* image = gtk_image_new_from_icon_name("document-open", GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(m_browse_button), image);
	gtk_widget_show(image);
	g_signal_connect_slot<GtkButton*>(m_browse_button, "clicked", &CommandEdit::browse_clicked, this);
}

//-----------------------------------------------------------------------------

void CommandEdit::browse_clicked()
{
	GtkFileChooser* chooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new(_("Select Command"),
			GTK_WINDOW(gtk_widget_get_toplevel(m_widget)),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			NULL));
	gtk_file_chooser_set_local_only(chooser, true);
	gtk_file_chooser_set_current_folder(chooser, BINDIR);

	// Select current command
	gchar* filename = g_strdup(m_command->get());
	if (filename != NULL)
	{
		// Make sure command is absolute path
		if (!g_path_is_absolute(filename))
		{
			gchar* absolute_path = g_find_program_in_path(filename);
			if (absolute_path != NULL)
			{
				g_free(filename);
				filename = absolute_path;
			}
		}

		if (g_path_is_absolute(filename))
		{
			gtk_file_chooser_set_filename(chooser, filename);
		}
		g_free(filename);
	}

	// Set new command
	if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename(chooser);
		gtk_entry_set_text(m_entry, filename);
		g_free(filename);
	}

	gtk_widget_destroy(GTK_WIDGET(chooser));
}

//-----------------------------------------------------------------------------

void CommandEdit::command_changed()
{
	m_command->set(gtk_entry_get_text(m_entry));
}

//-----------------------------------------------------------------------------

void CommandEdit::shown_toggled()
{
	bool active = gtk_toggle_button_get_active(m_shown);
	m_command->set_shown(active);
	gtk_widget_set_sensitive(GTK_WIDGET(m_entry), active);
	gtk_widget_set_sensitive(GTK_WIDGET(m_browse_button), active);
}

//-----------------------------------------------------------------------------
