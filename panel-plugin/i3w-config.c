/*  xfce4-netspeed-plugin
 *
 *  Copyright (c) 2011 Calin Crisan <ccrisan@gmail.com>
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
 */


#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "i3w-config.h"

typedef struct {
    i3WorkspacesConfig *config;
    XfcePanelPlugin *plugin;
} ConfigDialogClosedParam;

guint32
serialize_gdkcolor(GdkColor *gdkcolor);
GdkColor *
unserialize_gdkcolor(guint32 color);

void
normal_color_changed(GtkWidget *widget, i3WorkspacesConfig *config);
void
urgent_color_changed(GtkWidget *widget, i3WorkspacesConfig *config);
void
strip_workspace_numbers_changed(GtkWidget *widget, i3WorkspacesConfig *config);

void
config_dialog_closed(GtkWidget *dialog, int response, ConfigDialogClosedParam *param);

/* Function Implementations */

i3WorkspacesConfig *
i3_workspaces_config_new()
{
    i3WorkspacesConfig *config = g_new0(i3WorkspacesConfig, 1);
    
    return config;
}

void
i3_workspaces_config_free(i3WorkspacesConfig *config)
{
    if (config->normal_color) g_free(config->normal_color);
    if (config->urgent_color) g_free(config->urgent_color);
    
    g_free(config);
}

gboolean
i3_workspaces_config_load(i3WorkspacesConfig *config, XfcePanelPlugin *plugin)
{
    gchar *file = xfce_panel_plugin_save_location(plugin, FALSE);
    if (G_UNLIKELY(!file))
        return FALSE;

    XfceRc *rc = xfce_rc_simple_open(file, FALSE);
    g_free(file);

    guint32 normal_color = xfce_rc_read_int_entry(rc, "normal_color", 0x000000);
    config->normal_color = unserialize_gdkcolor(normal_color);

    guint32 urgent_color = xfce_rc_read_int_entry(rc, "urgent_color", 0xff0000);
    config->urgent_color = unserialize_gdkcolor(urgent_color);

    config->strip_workspace_numbers = xfce_rc_read_bool_entry(rc, "strip_workspace_numbers", FALSE);

    xfce_rc_close(rc);
    
    return TRUE;
}

gboolean
i3_workspaces_config_save(i3WorkspacesConfig *config, XfcePanelPlugin *plugin)
{
    gchar *file = xfce_panel_plugin_save_location(plugin, TRUE);
    if (G_UNLIKELY(!file))
        return FALSE;

    XfceRc *rc = xfce_rc_simple_open(file, FALSE);
    g_free(file);

    xfce_rc_write_int_entry(rc, "normal_color", serialize_gdkcolor(config->normal_color));
    xfce_rc_write_int_entry(rc, "urgent_color", serialize_gdkcolor(config->urgent_color));
    xfce_rc_write_bool_entry(rc, "strip_workspace_numbers", config->strip_workspace_numbers);

    xfce_rc_close(rc);
    
    return TRUE;
}

void
i3_workspaces_config_show(i3WorkspacesConfig *config, XfcePanelPlugin *plugin)
{
    GtkWidget *dialog, *dialog_vbox, *hbox, *button, *label;

    xfce_panel_plugin_block_menu(plugin);

    dialog = xfce_titled_dialog_new_with_buttons(_("i3 Workspaces Plugin"),
        NULL, GTK_DIALOG_NO_SEPARATOR, GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
    xfce_titled_dialog_set_subtitle(XFCE_TITLED_DIALOG(dialog), _("Configuration"));

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    gtk_window_stick(GTK_WINDOW(dialog));

    dialog_vbox = GTK_DIALOG(dialog)->vbox;

    /* normal color */
    hbox = gtk_hbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog_vbox), hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

    label = gtk_label_new(_("Normal Workspace Color:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    button = gtk_color_button_new_with_color(config->normal_color);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "color-set", G_CALLBACK(normal_color_changed), config);

    /* urgent color */
    hbox = gtk_hbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog_vbox), hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

    label = gtk_label_new(_("Urgent Workspace Color:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    button = gtk_color_button_new_with_color(config->urgent_color);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "color-set", G_CALLBACK(urgent_color_changed), config);

    /* strip workspace numbers */
    hbox = gtk_hbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog_vbox), hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

    button = gtk_check_button_new_with_mnemonic(_("Strip Workspace Numbers"));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), config->strip_workspace_numbers == TRUE);
    g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(strip_workspace_numbers_changed), config);

    /* close event */
    ConfigDialogClosedParam *param = g_new(ConfigDialogClosedParam, 1);
    param->plugin = plugin;
    param->config = config;
    g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(config_dialog_closed), param);

    gtk_widget_show_all(dialog);
}

guint32
serialize_gdkcolor(GdkColor *gdkcolor)
{
    guint32 color = 0;

    // Shift and add the color components
    color += ((guint32) gdkcolor->red) << 4;
    color += ((guint32) gdkcolor->green) << 2;
    color += ((guint32) gdkcolor->blue);

    return color;
}

GdkColor *
unserialize_gdkcolor(guint32 color)
{
    GdkColor *gdkcolor = g_new0(GdkColor, 1);

    // Mask and shift the color components
    gdkcolor->red = (guint16) ((color & 0xff0000) >> 4);
    gdkcolor->green = (guint16) ((color & 0x00ff00) >> 2);
    gdkcolor->blue = (guint16) (color & 0x0000ff);

    return gdkcolor;
}

void
strip_workspace_numbers_changed(GtkWidget *widget, i3WorkspacesConfig *config)
{
    g_printf("strip\n");
}

void
normal_color_changed(GtkWidget *widget, i3WorkspacesConfig *config)
{
    g_printf("normal_color\n");
}

void
urgent_color_changed(GtkWidget *widget, i3WorkspacesConfig *config)
{
    g_printf("urgent_color\n");
}

void
config_dialog_closed(GtkWidget *dialog, int response, ConfigDialogClosedParam *param)
{
    xfce_panel_plugin_unblock_menu(param->plugin);

    gtk_widget_destroy(dialog);

    i3_workspaces_config_save(param->config, param->plugin);

    g_free(param);
}
