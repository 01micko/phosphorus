/* PhApplication
 *
 * Copyright (C) 2015 Jente Hidskes
 *
 * Author: Jente Hidskes <hjdskes@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "ph-application.h"
#include "ph-backend.h"
#include "ph-backend-gnome.h"
#include "ph-thumbview.h"
#include "ph-window.h"

#define SCHEMA "org.unia.phosphorus"
#define KEY_DIRECTORIES "directories"
#define KEY_RECURSE "recursion"

struct _PhApplicationPrivate {
	GSettings *settings;
	PhBackend *backend;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhApplication, ph_application, GTK_TYPE_APPLICATION);

static void
ph_application_action_preferences (GSimpleAction *action,
				   GVariant      *parameter,
				   gpointer       user_data)
{
	g_print ("Preferences\n");
}

static void
ph_application_action_about (GSimpleAction *action,
			     GVariant      *parameter,
			     gpointer       user_data)
{
	GtkWindow *window;

	window = gtk_application_get_active_window (GTK_APPLICATION (user_data));
	g_return_if_fail (PH_IS_WINDOW (window));

	ph_window_show_about_dialog (PH_WINDOW (window));
}

static void
ph_application_action_quit (GSimpleAction *action,
			    GVariant      *parameter,
			    gpointer       user_data)
{
	GList *windows;

	windows = gtk_application_get_windows (GTK_APPLICATION (user_data));

	g_list_foreach (windows, (GFunc) ph_window_close, NULL);
}

static GActionEntry app_entries[] = {
	{ "preferences", ph_application_action_preferences, NULL, NULL, NULL },
	{ "about", ph_application_action_about, NULL, NULL, NULL },
	{ "quit",  ph_application_action_quit,  NULL, NULL, NULL },
};

static void
ph_application_init_accelerators (GtkApplication *application)
{
	/* Taken from eog, which in turn has this based on a simular
	 * construct in Evince (src/ev-application.c).
	 * Setting multiple accelerators at once for an action
	 * is not very straight forward in a static way.
	 *
	 * This gchar* array simulates an action<->accels mapping.
	 * Enter the action name followed by the accelerator strings
	 * and terminate the entry with a NULL-string.*/
	static const gchar *const accelmap[] = {
		"win.apply", "<Ctrl>a", NULL,
		NULL /* Terminating NULL */
	};

	const gchar *const *it;
	for (it = accelmap; it[0]; it += g_strv_length ((gchar **) it) + 1) {
		gtk_application_set_accels_for_action (application, it[0], &it[1]);
	}
}

static void
ph_application_startup (GApplication *application)
{
	PhApplicationPrivate *priv;

	G_APPLICATION_CLASS (ph_application_parent_class)->startup (application);

	priv = ph_application_get_instance_private (PH_APPLICATION (application));

	// TODO: update live when new directory is added
	priv->settings = g_settings_new (SCHEMA);

	g_set_application_name (_("Wallpaper browser"));

	g_action_map_add_action_entries (G_ACTION_MAP (application),
					 app_entries, G_N_ELEMENTS (app_entries),
					 application);

	ph_application_init_accelerators (GTK_APPLICATION (application));
}

static void
ph_application_activate (GApplication *application)
{
	PhApplicationPrivate *priv;
	PhWindow *window;
	PhRecurseType recurse;
	gchar **directories;

	priv = ph_application_get_instance_private (PH_APPLICATION (application));

	directories = g_settings_get_strv (priv->settings, KEY_DIRECTORIES);
	recurse = g_settings_get_enum (priv->settings, KEY_RECURSE);
	window = ph_window_new (PH_APPLICATION (application), PH_BACKEND (priv->backend));
	ph_window_scan_directories (window, recurse, directories);
	g_strfreev (directories);

	gtk_window_present_with_time (GTK_WINDOW (window), GDK_CURRENT_TIME);
}

static void
ph_application_dispose (GObject *object)
{
	PhApplicationPrivate *priv;

	priv = ph_application_get_instance_private (PH_APPLICATION (object));

	if (priv->settings) {
		g_object_unref (priv->settings);
		priv->settings = NULL;
	}

	if (priv->backend) {
		g_object_unref (priv->backend);
		priv->backend = NULL;
	}

	G_OBJECT_CLASS (ph_application_parent_class)->dispose (object);
}

static void
ph_application_class_init (PhApplicationClass *ph_application_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (ph_application_class);
	GApplicationClass *application_class = G_APPLICATION_CLASS (ph_application_class);

	object_class->dispose = ph_application_dispose;

	application_class->startup = ph_application_startup;
	application_class->activate = ph_application_activate;
}

static void
ph_application_init (PhApplication *application)
{
	PhApplicationPrivate *priv;

	priv = ph_application_get_instance_private (application);

	/* TODO: decide which one when building Phosphorus? */
	priv->backend = ph_backend_gnome_new ();
}

PhApplication *
ph_application_new (void)
{
	GObject *application;

	application = g_object_new (PH_TYPE_APPLICATION,
				    "application-id", "org.unia.phosphorus",
				    "flags", G_APPLICATION_FLAGS_NONE,
				    NULL);

	return PH_APPLICATION (application);
}
