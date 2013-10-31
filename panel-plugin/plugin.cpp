// Copyright (C) 2013 Graeme Gott <graeme@gottcode.org>
//
// This library is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this library.  If not, see <http://www.gnu.org/licenses/>.


#include "plugin.h"

#include "applications-page.h"
#include "configuration-dialog.h"
#include "favorites-page.h"
#include "icon-size.h"
#include "launcher.h"
#include "launcher-view.h"
#include "section-button.h"
#include "window.h"

using namespace WhiskerMenu;

//-----------------------------------------------------------------------------

extern "C" void whiskermenu_construct(XfcePanelPlugin* plugin)
{
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
	new Plugin(plugin);
}

static void whiskermenu_free(XfcePanelPlugin*, Plugin* whiskermenu)
{
	delete whiskermenu;
	whiskermenu = NULL;
}

//-----------------------------------------------------------------------------

Plugin::Plugin(XfcePanelPlugin* plugin) :
	m_plugin(plugin),
	m_window(NULL),
	m_button_title(get_button_title_default()),
	m_button_icon_name("xfce4-whiskermenu"),
	m_button_title_visible(false),
	m_button_icon_visible(true)
{
	// Load settings
	gchar* file = xfce_panel_plugin_lookup_rc_file(m_plugin);
	if (file)
	{
		XfceRc* settings = xfce_rc_simple_open(file, true);
		g_free(file);

		m_button_title = xfce_rc_read_entry(settings, "button-title", m_button_title.c_str());
		m_button_icon_name = xfce_rc_read_entry(settings, "button-icon", m_button_icon_name.c_str());
		m_button_title_visible = xfce_rc_read_bool_entry(settings, "show-button-title", m_button_title_visible);
		m_button_icon_visible = xfce_rc_read_bool_entry(settings, "show-button-icon", m_button_icon_visible);
		Launcher::set_show_name(xfce_rc_read_bool_entry(settings, "launcher-show-name", Launcher::get_show_name()));
		Launcher::set_show_description(xfce_rc_read_bool_entry(settings, "launcher-show-description", Launcher::get_show_description()));
		SectionButton::set_hover_activate(xfce_rc_read_bool_entry(settings, "hover-switch-category", SectionButton::get_hover_activate()));
		SectionButton::set_icon_size(xfce_rc_read_int_entry(settings, "category-icon-size", SectionButton::get_icon_size()));
		LauncherView::set_icon_size(xfce_rc_read_int_entry(settings, "item-icon-size", LauncherView::get_icon_size()));
		ApplicationsPage::set_load_hierarchy(xfce_rc_read_bool_entry(settings, "load-hierarchy", ApplicationsPage::get_load_hierarchy()));
		FavoritesPage::set_remember_favorites(xfce_rc_read_bool_entry(settings, "favorites-in-recent", FavoritesPage::get_remember_favorites()));
		Window::set_display_recent(xfce_rc_read_bool_entry(settings, "display-recent-default", Window::get_display_recent()));
		Window::set_position_search_alternate(xfce_rc_read_bool_entry(settings, "position-search-alternate", Window::get_position_search_alternate()));
		Window::set_position_commands_alternate(xfce_rc_read_bool_entry(settings, "position-commands-alternate", Window::get_position_commands_alternate()));

		m_window = new Window(settings);
		m_window->set_settings_command(xfce_rc_read_entry(settings, "command-settings", m_window->get_settings_command().c_str()));
		m_window->set_lockscreen_command(xfce_rc_read_entry(settings, "command-lockscreen", m_window->get_lockscreen_command().c_str()));
		m_window->set_logout_command(xfce_rc_read_entry(settings, "command-logout", m_window->get_logout_command().c_str()));

		xfce_rc_close(settings);
	}
	else
	{
		m_window = new Window(NULL);
	}
	g_signal_connect(m_window->get_widget(), "unmap", G_CALLBACK(Plugin::menu_hidden_slot), this);

	// Prevent empty panel button
	if (!m_button_icon_visible)
	{
		if (!m_button_title_visible)
		{
			m_button_icon_visible = true;
		}
		else if (m_button_title.empty())
		{
			m_button_title = get_button_title_default();
		}
	}

	// Create toggle button
	m_button = xfce_create_panel_toggle_button();
	gtk_button_set_relief(GTK_BUTTON(m_button), GTK_RELIEF_NONE);
	g_signal_connect(m_button, "button-press-event", G_CALLBACK(Plugin::button_clicked_slot), this);
	gtk_widget_show(m_button);

	m_button_box = GTK_BOX(gtk_hbox_new(false, 1));
	gtk_container_add(GTK_CONTAINER(m_button), GTK_WIDGET(m_button_box));
	gtk_widget_show(GTK_WIDGET(m_button_box));

	m_button_icon = XFCE_PANEL_IMAGE(xfce_panel_image_new_from_source(m_button_icon_name.c_str()));
	gtk_box_pack_start(m_button_box, GTK_WIDGET(m_button_icon), false, false, 0);
	if (m_button_icon_visible)
	{
		gtk_widget_show(GTK_WIDGET(m_button_icon));
	}

	m_button_label = GTK_LABEL(gtk_label_new(NULL));
	gtk_label_set_markup(m_button_label, m_button_title.c_str());
	gtk_box_pack_start(m_button_box, GTK_WIDGET(m_button_label), false, false, 0);
	if (m_button_title_visible)
	{
		gtk_widget_show(GTK_WIDGET(m_button_label));
	}

	// Add plugin to panel
	gtk_container_add(GTK_CONTAINER(plugin), m_button);
	xfce_panel_plugin_add_action_widget(plugin, m_button);

	// Connect plugin signals to functions
	g_signal_connect(plugin, "free-data", G_CALLBACK(whiskermenu_free), this);
	g_signal_connect(plugin, "configure-plugin", G_CALLBACK(Plugin::configure_slot), this);
#if (LIBXFCE4PANEL_CHECK_VERSION(4,10,0))
	g_signal_connect(plugin, "mode-changed", G_CALLBACK(Plugin::mode_changed_slot), this);
#else
	g_signal_connect(plugin, "orientation-changed", G_CALLBACK(Plugin::orientation_changed_slot), this);
#endif
	g_signal_connect(plugin, "remote-event", G_CALLBACK(Plugin::remote_event_slot), this);
	g_signal_connect_swapped(plugin, "save", G_CALLBACK(Plugin::save_slot), this);
	g_signal_connect(plugin, "size-changed", G_CALLBACK(Plugin::size_changed_slot), this);
	xfce_panel_plugin_menu_show_configure(plugin);
}

//-----------------------------------------------------------------------------

Plugin::~Plugin()
{
	save();

	delete m_window;
	m_window = NULL;

	gtk_widget_destroy(m_button);
}

//-----------------------------------------------------------------------------

std::string Plugin::get_button_title_default()
{
	return _("Applications Menu");
}

//-----------------------------------------------------------------------------

void Plugin::reload()
{
	m_window->hide();
	m_window->get_applications()->invalidate_applications();
}

//-----------------------------------------------------------------------------

void Plugin::set_button_style(ButtonStyle style)
{
	m_button_icon_visible = style & ShowIcon;
	if (m_button_icon_visible)
	{
		gtk_widget_show(GTK_WIDGET(m_button_icon));
	}
	else
	{
		gtk_widget_hide(GTK_WIDGET(m_button_icon));
	}

	m_button_title_visible = style & ShowText;
	if (m_button_title_visible)
	{
		gtk_widget_show(GTK_WIDGET(m_button_label));
	}
	else
	{
		gtk_widget_hide(GTK_WIDGET(m_button_label));
	}

	size_changed(xfce_panel_plugin_get_size(m_plugin));
}

//-----------------------------------------------------------------------------

void Plugin::set_button_title(const std::string& title)
{
	m_button_title = title;
	gtk_label_set_markup(m_button_label, m_button_title.c_str());
	size_changed(xfce_panel_plugin_get_size(m_plugin));
}

//-----------------------------------------------------------------------------

void Plugin::set_button_icon_name(const std::string& icon)
{
	m_button_icon_name = icon;
	xfce_panel_image_set_from_source(m_button_icon, icon.c_str());
	size_changed(xfce_panel_plugin_get_size(m_plugin));
}

//-----------------------------------------------------------------------------

void Plugin::set_configure_enabled(bool enabled)
{
	if (enabled)
	{
		xfce_panel_plugin_unblock_menu(m_plugin);
	}
	else
	{
		xfce_panel_plugin_block_menu(m_plugin);
	}
}

//-----------------------------------------------------------------------------

bool Plugin::button_clicked(GdkEventButton* event)
{
	if (event->button != 1 || event->state & GDK_CONTROL_MASK)
	{
		return false;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_button)) == true)
	{
		m_window->hide();
	}
	else
	{
		popup_menu(false);
	}

	return true;
}

//-----------------------------------------------------------------------------

void Plugin::menu_hidden()
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_button), false);
	xfce_panel_plugin_block_autohide(m_plugin, false);
	if (m_window->get_modified())
	{
		save();
	}
}

//-----------------------------------------------------------------------------

void Plugin::configure()
{
	ConfigurationDialog* dialog = new ConfigurationDialog(this);
	g_signal_connect_swapped(dialog->get_widget(), "destroy", G_CALLBACK(Plugin::save_slot), this);
}

//-----------------------------------------------------------------------------

void Plugin::orientation_changed(bool vertical)
{
	gtk_label_set_angle(m_button_label, vertical ? 270: 0);

	size_changed(xfce_panel_plugin_get_size(m_plugin));
}

//-----------------------------------------------------------------------------

bool Plugin::remote_event(gchar* name, GValue* value)
{
	if (strcmp(name, "popup"))
	{
		return false;
	}

	if (gtk_widget_get_visible(m_window->get_widget()))
	{
		m_window->hide();
	}
	else
	{
		popup_menu(value && G_VALUE_HOLDS_BOOLEAN(value) && g_value_get_boolean(value));
	}

	return true;
}

//-----------------------------------------------------------------------------

void Plugin::save()
{
	gchar* file = xfce_panel_plugin_save_location(m_plugin, true);
	if (!file)
	{
		return;
	}
	XfceRc* settings = xfce_rc_simple_open(file, false);
	g_free(file);

	xfce_rc_write_entry(settings, "button-title", m_button_title.c_str());
	xfce_rc_write_entry(settings, "button-icon", m_button_icon_name.c_str());
	xfce_rc_write_bool_entry(settings, "show-button-title", m_button_title_visible);
	xfce_rc_write_bool_entry(settings, "show-button-icon", m_button_icon_visible);
	xfce_rc_write_bool_entry(settings, "launcher-show-name", Launcher::get_show_name());
	xfce_rc_write_bool_entry(settings, "launcher-show-description", Launcher::get_show_description());
	xfce_rc_write_bool_entry(settings, "hover-switch-category", SectionButton::get_hover_activate());
	xfce_rc_write_int_entry(settings, "category-icon-size", SectionButton::get_icon_size());
	xfce_rc_write_int_entry(settings, "item-icon-size", LauncherView::get_icon_size());
	xfce_rc_write_bool_entry(settings, "load-hierarchy", ApplicationsPage::get_load_hierarchy());
	xfce_rc_write_bool_entry(settings, "favorites-in-recent", FavoritesPage::get_remember_favorites());
	xfce_rc_write_bool_entry(settings, "display-recent-default", Window::get_display_recent());
	xfce_rc_write_bool_entry(settings, "position-search-alternate", Window::get_position_search_alternate());
	xfce_rc_write_bool_entry(settings, "position-commands-alternate", Window::get_position_commands_alternate());
	xfce_rc_write_entry(settings, "command-settings", m_window->get_settings_command().c_str());
	xfce_rc_write_entry(settings, "command-lockscreen", m_window->get_lockscreen_command().c_str());
	xfce_rc_write_entry(settings, "command-logout", m_window->get_logout_command().c_str());
	m_window->save(settings);

	xfce_rc_close(settings);
}

//-----------------------------------------------------------------------------

bool Plugin::size_changed(int size)
{
#if (LIBXFCE4PANEL_CHECK_VERSION(4,10,0))
	gint row_size = size / xfce_panel_plugin_get_nrows(m_plugin);
#else
	gint row_size = size;
#endif

	GtkOrientation orientation = xfce_panel_plugin_get_orientation(m_plugin);

	// Make icon expand to fill button if title is not visible
	gtk_box_set_child_packing(GTK_BOX(m_button_box), GTK_WIDGET(m_button_icon),
			!m_button_title_visible,
			!m_button_title_visible,
			0, GTK_PACK_START);

	if (!m_button_title_visible)
	{
		xfce_panel_image_set_size(m_button_icon, -1);
		if (orientation == GTK_ORIENTATION_HORIZONTAL)
		{
			gtk_widget_set_size_request(GTK_WIDGET(m_plugin), row_size, size);
		}
		else
		{
			gtk_widget_set_size_request(GTK_WIDGET(m_plugin), size, row_size);
		}
	}
	else
	{
		GtkStyle* style = gtk_widget_get_style(m_button);
		gint border = (2 * std::max(style->xthickness, style->ythickness)) + 2;
		xfce_panel_image_set_size(m_button_icon, row_size - border);
		gtk_widget_set_size_request(GTK_WIDGET(m_plugin), -1, -1);

#if (LIBXFCE4PANEL_CHECK_VERSION(4,10,0))
		// Put title next to icon if panel is wide enough
		if (xfce_panel_plugin_get_mode(m_plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
		{
			GtkRequisition label_size;
			gtk_widget_size_request(GTK_WIDGET(m_button_label), &label_size);
			if (label_size.width <= (size - row_size))
			{
				orientation = GTK_ORIENTATION_HORIZONTAL;
			}
		}
#endif
	}

	gtk_orientable_set_orientation(GTK_ORIENTABLE(m_button_box), orientation);

	return true;
}

//-----------------------------------------------------------------------------

void Plugin::popup_menu(bool at_cursor)
{
	if (!at_cursor)
	{
		xfce_panel_plugin_block_autohide(m_plugin, true);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_button), true);
		m_window->show(m_button, xfce_panel_plugin_get_orientation(m_plugin) == GTK_ORIENTATION_HORIZONTAL);
	}
	else
	{
		m_window->show(NULL, true);
	}
}

//-----------------------------------------------------------------------------