/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GMAMEUI
 *
 * Copyright 2007-2008 Andrew Burton <adb@iinet.net.au>
 * based on GXMame code
 * 2002-2005 Stephane Pontier <shadow_walker@users.sourceforge.net>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "common.h"

#include <gtk/gtkcolorseldialog.h>
#include <gtk/gtkfontsel.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtktreestore.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "callbacks.h"
#include "interface.h"
#include "about.h"
#include "directories.h"
#include "gmameui.h"
#include "audit.h"
#include "gui.h"
#include "properties.h"
#include "progression_window.h"
#include "io.h"
#include "gui_prefs_dialog.h"

void update_favourites_list (gboolean add);

/* Main window menu: File */
void
on_play_activate (GtkAction *action,
		  gpointer  user_data)
{
	g_return_if_fail (current_exec != NULL);

	play_game (gui_prefs.current_game);
}

void
on_play_and_record_input_activate (GtkAction *action,
				   gpointer  user_data)
{
	select_inp (FALSE);
}


void
on_playback_input_activate (GtkAction *action,
			    gpointer  user_data)
{
	select_inp (TRUE);
}

void
on_select_random_game_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gint random_game;

	g_return_if_fail (visible_games > 0);

	random_game = (gint) g_random_int_range (0, visible_games);
	GMAMEUI_DEBUG ("random game#%i", random_game);

	gtk_tree_model_foreach (GTK_TREE_MODEL (main_gui.tree_model),
				foreach_find_random_rom_in_store,
				(gpointer *) random_game);
}

void update_favourites_list (gboolean add) {
	Columns_type type;

	gui_prefs.current_game->favourite = add;

	gmameui_ui_set_favourites_sensitive (add);
	
	g_object_get (selected_filter,
		      "type", &type,
		      NULL);
	/* problems because the row values are completly changed, I redisplay the complete game list */
	if (type == FAVORITE)
		create_gamelist_content ();
	else
		update_game_in_list (gui_prefs.current_game);
}

void
on_add_to_favorites_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	update_favourites_list (TRUE);
}


void
on_remove_from_favorites_activate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	update_favourites_list (FALSE);
}

/* If rom_name is NULL, then the default options are used and loaded, otherwise
   the rom-specific options are used */
static void
show_properties_dialog (gchar *rom_name)
{
	g_return_if_fail (current_exec != NULL);
	
	/* SDLMAME uses a different set of options to XMAME. If we are running
	   XMAME, then use the legacy GXMAME method of maintaining the options */
	if (current_exec->type == XMAME_EXEC_WIN32) {
		/* SDLMAME */
		GtkWidget *options_dialog = mame_options_get_dialog (main_gui.options);

		GladeXML *xml = glade_xml_new (GLADEDIR "options.glade", NULL, NULL);
		mame_options_add_page (main_gui.options, xml, "Display", "Display",
		                       "gmameui-display-toolbar");
		mame_options_add_page (main_gui.options, xml, "OpenGL", "OpenGL",
		                       "gmameui-display-toolbar");
		mame_options_add_page (main_gui.options, xml, "Sound", "Sound",
		                       "gmameui-sound-toolbar");
		mame_options_add_page (main_gui.options, xml, "Input", "Input",
		                       "gmameui-joystick-toolbar");
		mame_options_add_page (main_gui.options, xml, "performance_vbox", "Performance",
		                       "gmameui-general-toolbar");
		mame_options_add_page (main_gui.options, xml, "misc_vbox", "Miscellaneous",
		                       "gmameui-general-toolbar");
		mame_options_add_page (main_gui.options, xml, "debugging_vbox", "Debugging",
		                       "gmameui-rom");

		
		gtk_dialog_run (GTK_WIDGET (options_dialog));
		gtk_widget_destroy (GTK_WIDGET (options_dialog));

	} else {
		/* XMAME options */
		GtkWidget *properties_window;

		/* have to test if a game is selected or not
		   then only after launch the properties window */
		/* joystick focus turned off, will be turned on again in properties.c (exit_properties_window) */
		joy_focus_off ();
		if (rom_name)
			properties_window = create_properties_windows (gui_prefs.current_game);
		else
			properties_window = create_properties_windows (NULL);
		gtk_widget_show (properties_window);
	}
}

void
on_properties_activate (GtkAction *action,
			gpointer  user_data)
{
	show_rom_properties ();
}

void
on_options_activate (GtkAction *action,
		     gpointer  user_data)
{
	gchar *current_rom;
	g_object_get (main_gui.gui_prefs, "current-rom", &current_rom, NULL);
	
	show_properties_dialog (current_rom);
	
	g_free (current_rom);
}

void
on_options_default_activate (GtkAction *action,
			     gpointer  user_data)
{
	show_properties_dialog (NULL);
}

void
on_audit_all_games_activate (GtkMenuItem     *menuitem,
			     gpointer         user_data)
{
	if (!current_exec) {
		gmameui_message (ERROR, NULL, _("No xmame executables defined"));
		/* reenable joystick */
		joy_focus_on ();
		return;
	}

	gamelist_check (current_exec);

	create_checking_games_window ();
	UPDATE_GUI;
	launch_checking_games_window ();
}


void
on_exit_activate (GtkMenuItem     *menuitem,
		  gpointer         user_data)
{
	exit_gmameui ();
}

static void
quick_refresh_list (void)
{
	static gboolean quick_check_running;
	GList *list_pointer;
	RomEntry *rom;
	
	if (quick_check_running) {
		GMAMEUI_DEBUG ("Quick check already running");
		return;
	}

	quick_check_running = 1;
// FIXME TODO	gtk_widget_set_sensitive (GTK_WIDGET (main_gui.refresh_menu), FALSE);
	/* remove all information concerning the presence of roms */
	for (list_pointer = g_list_first (game_list.roms); list_pointer; list_pointer = g_list_next (list_pointer)) {
		rom = (RomEntry *)list_pointer->data;
		rom->has_roms = UNKNOWN;
	}
	/* refresh the display */
	create_gamelist_content ();

	game_list.not_checked_list = g_list_copy (game_list.roms);

	quick_check ();
	/* final refresh only if we are in AVAILABLE or UNAVAILABLE Folder*/
/* DELETE Not using FolderID in new prefs
	if ((gui_prefs.FolderID == AVAILABLE) || (gui_prefs.FolderID == UNAVAILABLE)) {
		create_gamelist_content ();
		GMAMEUI_DEBUG ("Final Refresh");
	}*/
	quick_check_running = 0;
// FIXME TODO	gtk_widget_set_sensitive (GTK_WIDGET (main_gui.refresh_menu), TRUE);
}

void
on_refresh_activate (GtkAction *action,
		     gpointer  user_data)
{
	quick_refresh_list ();
}

void
on_rebuild_game_list_menu_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_widget_set_sensitive (main_gui.scrolled_window_games, FALSE);
	UPDATE_GUI;
	
	GMAMEUI_DEBUG ("recreate game list");
	gamelist_parse (current_exec);
	GMAMEUI_DEBUG ("reload everything");
	gamelist_save ();
	load_games_ini ();
	load_catver_ini ();
	create_gamelist_content ();
	gtk_widget_set_sensitive (main_gui.scrolled_window_games, TRUE);
}

void
on_directories_menu_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *directory_window;
	directory_window = create_directories_selection ();
	gtk_widget_show (directory_window);
}

void
on_preferences_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	/*
	GtkWidget *gui_prefs_window;
	gui_prefs_window = create_gui_prefs_window ();
	gtk_widget_show (gui_prefs_window);
	*/
	
	MameGuiPrefsDialog *prefs_dialog;
	prefs_dialog = mame_gui_prefs_dialog_new ();
GMAMEUI_DEBUG("Running dialog");
//	gtk_dialog_run (prefs_dialog);
GMAMEUI_DEBUG("Done running dialog");
//	gtk_widget_destroy (prefs_dialog);
}


/* Main window menu: Help */
void
on_about_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	about_window_show ();
}

