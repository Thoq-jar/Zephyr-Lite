#include <sys/types.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <gio/gio.h>

static void open_file(GtkWidget* widget, gpointer text_view);
static void save_file(GtkWidget* widget, gpointer text_view);
static void quit_application(GtkWidget* widget, gpointer data);
static void show_about_dialog(GtkWidget* widget, gpointer data);
static gboolean window_clicked(GtkWidget* widget, GdkEventButton* event, gpointer data);
static gboolean window_motion(GtkWidget* widget, GdkEventMotion* event, gpointer data);
static GdkPixbuf* create_pixbuf(const gchar* filename); // Declaration

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    // Enable dark mode
    GtkSettings* settings = gtk_settings_get_default();
    g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);

    // Preload the icon
    GdkPixbuf* icon_pixbuf = create_pixbuf("icon.png");

    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 1600, 900);
    gtk_window_set_title(GTK_WINDOW(window), "Zephyr - Code at the speed of light"); // Set window title
    gtk_window_set_icon(GTK_WINDOW(window), icon_pixbuf);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "button-press-event", G_CALLBACK(window_clicked), NULL);
    g_signal_connect(window, "motion-notify-event", G_CALLBACK(window_motion), NULL);
    gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

    GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget* text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_box_pack_start(GTK_BOX(box), scrolled_window, TRUE, TRUE, 0);

    GtkWidget* menu_bar = gtk_menu_bar_new();
    GtkWidget* file_menu = gtk_menu_new();
    GtkWidget* file_menu_item = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu_item), file_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_menu_item);

    GtkWidget* open_item = gtk_menu_item_new_with_label("Open");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
    g_signal_connect(open_item, "activate", G_CALLBACK(open_file), text_view);
    gtk_widget_add_accelerator(open_item, "activate", gtk_accel_group_new(), GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    GtkWidget* save_item = gtk_menu_item_new_with_label("Save");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_item);
    g_signal_connect(save_item, "activate", G_CALLBACK(save_file), text_view);
    gtk_widget_add_accelerator(save_item, "activate", gtk_accel_group_new(), GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    GtkWidget* quit_item = gtk_menu_item_new_with_label("Quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);
    g_signal_connect(quit_item, "activate", G_CALLBACK(quit_application), NULL);
    gtk_widget_add_accelerator(quit_item, "activate", gtk_accel_group_new(), GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    GtkWidget* about_item = gtk_menu_item_new_with_label("About");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), about_item);
    g_signal_connect(about_item, "activate", G_CALLBACK(show_about_dialog), icon_pixbuf); // Pass the icon_pixbuf
    gtk_box_pack_start(GTK_BOX(box), menu_bar, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), box);

    gtk_widget_show_all(window);
    gtk_main();

    g_object_unref(icon_pixbuf); // Release the preloaded icon

    return 0;
}

static gboolean window_clicked(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    gtk_window_present(GTK_WINDOW(widget));
    return TRUE;
}

static gboolean window_motion(GtkWidget* widget, GdkEventMotion* event, gpointer data) {
    gint x, y;
    gdk_window_get_origin(gtk_widget_get_window(widget), &x, &y);
    if (event->state & GDK_BUTTON1_MASK) {
        gtk_window_move(GTK_WINDOW(widget), (gint)(event->x_root - x), (gint)(event->y_root - y));
    }
    return TRUE;
}

static void open_file(GtkWidget* widget, gpointer text_view) {
    GtkWidget* dialog;
    GtkTextBuffer* buffer;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new(
        "Open File", GTK_WINDOW(gtk_widget_get_toplevel(widget)), action,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        char* filename;
        GtkTextIter start, end;
        GtkTextBuffer* buffer;

        GtkFileChooser* chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);

        buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

        if (gtk_text_buffer_get_modified(buffer)) {
            // If text has been modified, ask user if they want to save changes
            GtkWidget* save_dialog = gtk_message_dialog_new(
                GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
                GTK_BUTTONS_YES_NO,
                "Do you want to save changes before opening a new file?");
            gint save_res = gtk_dialog_run(GTK_DIALOG(save_dialog));
            gtk_widget_destroy(save_dialog);
            if (save_res == GTK_RESPONSE_YES) {
                // If user wants to save changes, save the file
                GtkWidget* save_chooser;
                GtkFileChooserAction save_action = GTK_FILE_CHOOSER_ACTION_SAVE;
                gint save_res;

                save_chooser = gtk_file_chooser_dialog_new(
                    "Save File", GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                    save_action, "_Cancel", GTK_RESPONSE_CANCEL, "_Save",
                    GTK_RESPONSE_ACCEPT, NULL);

                save_res = gtk_dialog_run(GTK_DIALOG(save_chooser));
                if (save_res == GTK_RESPONSE_ACCEPT) {
                    char* save_filename;
                    GtkTextIter save_start, save_end;

                    GtkFileChooser* save_chooser = GTK_FILE_CHOOSER(save_chooser);
                    save_filename = gtk_file_chooser_get_filename(save_chooser);

                    gtk_text_buffer_get_start_iter(buffer, &save_start);
                    gtk_text_buffer_get_end_iter(buffer, &save_end);
                    gtk_text_buffer_delete(buffer, &save_start, &save_end);

                    gtk_text_buffer_get_bounds(buffer, &start, &end);
                    gchar* text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

                    GError* error = NULL;
                    if (!g_file_set_contents(save_filename, text, -1, &error)) {
                        g_warning("%s", error->message);
                        g_error_free(error);
                    }

                    g_free(text);
                    g_free(save_filename);
                }

                gtk_widget_destroy(save_chooser);
            }
        }

        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_delete(buffer, &start, &end);

        gchar* contents;
        gsize length;

        if (g_file_get_contents(filename, &contents, &length, NULL)) {
            gtk_text_buffer_insert_at_cursor(buffer, contents, length);
            g_free(contents);
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

static void save_file(GtkWidget* widget, gpointer text_view) {
    GtkWidget* dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;

    dialog = gtk_file_chooser_dialog_new(
        "Save File", GTK_WINDOW(gtk_widget_get_toplevel(widget)), action,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        char* filename;
        GtkTextIter start, end;
        GtkTextBuffer* buffer;

        GtkFileChooser* chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);

        buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        gtk_text_buffer_get_bounds(buffer, &start, &end);
        gchar* text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

        GError* error = NULL;
        if (!g_file_set_contents(filename, text, -1, &error)) {
            g_warning("%s", error->message);
            g_error_free(error);
        }

        g_free(text);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

static void quit_application(GtkWidget* widget, gpointer data) {
    gtk_main_quit();
}

static void show_about_dialog(GtkWidget* widget, gpointer data) {
    GdkPixbuf* icon_pixbuf = (GdkPixbuf*)data; // Retrieve the icon_pixbuf
    GtkWidget* about_dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog), "Zephyr");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), "3.3.24 (Nova)");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog), "Code at speed of light. \n Developed by Zephyr Industries");
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about_dialog), icon_pixbuf); // Set the icon_pixbuf for the about dialog
    gtk_dialog_run(GTK_DIALOG(about_dialog));
    gtk_widget_destroy(about_dialog);
}

static GdkPixbuf* create_pixbuf(const gchar* filename) {
    GdkPixbuf* pixbuf;
    GError* error = NULL;
    pixbuf = gdk_pixbuf_new_from_file(filename, &error);
    if (!pixbuf) {
        fprintf(stderr, "%s\n", error->message);
        g_error_free(error);
    }
    return pixbuf;
}
