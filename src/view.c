#include <gtk/gtk.h>
#include "../include/project.h"

void controller_init(void);

// Global GTK widget'ları
static GtkWidget *window;
static GtkWidget *terminal_text_view;
static GtkWidget *message_text_view;
static GtkCssProvider *css_provider;

static void append_text_to_view(GtkWidget *text_view, const char *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(buffer, &end_iter);
    gtk_text_buffer_insert(buffer, &end_iter, text, -1);
    gtk_text_buffer_insert(buffer, &end_iter, "\n", -1);
}

static void on_entry_activate(GtkEntry *entry, gpointer user_data) {
    (void)user_data;
    const char *input = gtk_entry_get_text(entry);
    controller_parse_input(input);
    gtk_entry_set_text(entry, "");
}

static void toggle_theme(GtkToggleButton *button, gpointer user_data) {
    (void)user_data;
    GError *error = NULL;
    
    if (gtk_toggle_button_get_active(button)) {
        gtk_css_provider_load_from_path(css_provider, "dark_theme.css", &error);
        gtk_button_set_label(GTK_BUTTON(button), "Koyu Tema");
    } else {
        gtk_css_provider_load_from_path(css_provider, "light_theme.css", &error);
        gtk_button_set_label(GTK_BUTTON(button), "Açık Tema");
    }

    if (error != NULL) {
        g_printerr("Tema değiştirilirken hata: %s\n", error->message);
        g_clear_error(&error);
    } else {
        gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                                  GTK_STYLE_PROVIDER(css_provider),
                                                  GTK_STYLE_PROVIDER_PRIORITY_USER);
    }
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Multi-User Communicating Shells");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    css_provider = gtk_css_provider_new();
    GError *error = NULL;
    gtk_css_provider_load_from_path(css_provider, "dark_theme.css", &error);
    if (error != NULL) {
        g_printerr("CSS yüklenirken hata: %s\n", error->message);
        g_clear_error(&error);
    } else {
        gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                                  GTK_STYLE_PROVIDER(css_provider),
                                                  GTK_STYLE_PROVIDER_PRIORITY_USER);
    }

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Toggle buton ekleme ve boyut ayarı
    GtkWidget *toggle_button = gtk_toggle_button_new_with_label("Koyu Tema");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), TRUE);
    gtk_widget_set_size_request(toggle_button, 100, 30); // Genişlik: 100px, Yükseklik: 30px
    gtk_box_pack_start(GTK_BOX(vbox), toggle_button, FALSE, FALSE, 5); // 5px boşluk
    g_signal_connect(toggle_button, "toggled", G_CALLBACK(toggle_theme), NULL);

    GtkWidget *terminal_label = gtk_label_new("Terminal Output:");
    gtk_box_pack_start(GTK_BOX(vbox), terminal_label, FALSE, FALSE, 0);

    terminal_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(terminal_text_view), FALSE);
    GtkWidget *terminal_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(terminal_scrolled), terminal_text_view);
    gtk_box_pack_start(GTK_BOX(vbox), terminal_scrolled, TRUE, TRUE, 0);

    GtkWidget *message_label = gtk_label_new("Messages:");
    gtk_box_pack_start(GTK_BOX(vbox), message_label, FALSE, FALSE, 0);

    message_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(message_text_view), FALSE);
    GtkWidget *message_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(message_scrolled), message_text_view);
    gtk_box_pack_start(GTK_BOX(vbox), message_scrolled, TRUE, TRUE, 0);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Komut veya @msg ile mesaj girin...");
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
    g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate), NULL);

    gtk_widget_show_all(window);
    controller_init();
}

void view_update_terminal(const char *output) {
    if (terminal_text_view && output) {
        append_text_to_view(terminal_text_view, output);
    }
}

void view_display_message(const char *msg) {
    if (message_text_view && msg) {
        append_text_to_view(message_text_view, msg);
    }
}

int view_main(int argc, char **argv) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    }

    GtkApplication *app;
    int status;

    const char *app_id = "org.example.MultiUserShell";
    app = gtk_application_new(app_id, G_APPLICATION_DEFAULT_FLAGS);
    if (app == NULL) {
        fprintf(stderr, "Failed to create GTK application\n");
        return -1;
    }

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
