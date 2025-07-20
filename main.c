#include <gtk/gtk.h>
#include <adwaita.h>

// Structure to hold application data
typedef struct {
    GtkWidget *window;
    GtkWidget *slide_view;
    GtkWidget *sidebar;
    GtkWidget *properties_panel;
    GtkWidget *statusbar;
} AppData;

static void setup_main_window(GtkApplication *app, AppData *app_data) {
    // Create the main window
    app_data->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(app_data->window), "Present - PowerPoint Alternative");
    gtk_window_set_default_size(GTK_WINDOW(app_data->window), 1200, 800);
    
    // Create main box
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(app_data->window), main_box);
    
    // Create sidebar
    app_data->sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(app_data->sidebar, 250, -1);
    gtk_widget_add_css_class(app_data->sidebar, "sidebar");
    gtk_box_append(GTK_BOX(main_box), app_data->sidebar);
    
    // Create slide thumbnails
    GtkWidget *thumbnails = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_top(thumbnails, 10);
    gtk_widget_set_margin_bottom(thumbnails, 10);
    gtk_widget_set_margin_start(thumbnails, 5);
    gtk_widget_set_margin_end(thumbnails, 5);
    
    // Add some example thumbnails
    for (int i = 1; i <= 5; i++) {
        GtkWidget *thumbnail = gtk_button_new();
        gtk_widget_set_size_request(thumbnail, -1, 60);
        gtk_widget_add_css_class(thumbnail, "slide-thumbnail");
        
        GtkWidget *label = gtk_label_new(g_strdup_printf("Slide %d", i));
        gtk_button_set_child(GTK_BUTTON(thumbnail), label);
        gtk_box_append(GTK_BOX(thumbnails), thumbnail);
    }
    
    gtk_box_append(GTK_BOX(app_data->sidebar), thumbnails);
    
    // Add new slide button
    GtkWidget *add_slide_btn = gtk_button_new_with_label("+ New Slide");
    gtk_widget_set_margin_start(add_slide_btn, 5);
    gtk_widget_set_margin_end(add_slide_btn, 5);
    gtk_box_append(GTK_BOX(app_data->sidebar), add_slide_btn);
    
    // Create main content area
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(main_box), content_box);
    
    // Create toolbar
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_add_css_class(toolbar, "toolbar");
    gtk_widget_set_margin_start(toolbar, 5);
    gtk_widget_set_margin_end(toolbar, 5);
    gtk_widget_set_margin_top(toolbar, 5);
    gtk_box_append(GTK_BOX(content_box), toolbar);
    
    // Add toolbar buttons
    const char *toolbar_icons[] = {"document-new", "document-open", "document-save", 
                                  "edit-cut", "edit-copy", "edit-paste", "format-text-bold", 
                                  "format-text-italic", "insert-image", "insert-object"};
    
    for (size_t i = 0; i < sizeof(toolbar_icons)/sizeof(toolbar_icons[0]); i++) {
        GtkWidget *btn = gtk_button_new_from_icon_name(toolbar_icons[i]);
        gtk_box_append(GTK_BOX(toolbar), btn);
    }
    
    // Create slide view
    GtkWidget *slide_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(slide_container, TRUE);
    gtk_widget_set_vexpand(slide_container, TRUE);
    gtk_box_append(GTK_BOX(content_box), slide_container);
    
    // Actual slide view
    app_data->slide_view = gtk_drawing_area_new();
    gtk_widget_set_size_request(app_data->slide_view, 800, 600);
    gtk_widget_add_css_class(app_data->slide_view, "slide-view");
    gtk_box_append(GTK_BOX(slide_container), app_data->slide_view);
    
    // Create properties panel
    app_data->properties_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(app_data->properties_panel, 250, -1);
    gtk_widget_add_css_class(app_data->properties_panel, "properties-panel");
    gtk_box_append(GTK_BOX(main_box), app_data->properties_panel);
    
    // Add some property controls
    GtkWidget *props_label = gtk_label_new("Slide Properties");
    gtk_widget_set_margin_top(props_label, 10);
    gtk_widget_set_margin_start(props_label, 10);
    gtk_box_append(GTK_BOX(app_data->properties_panel), props_label);
    
    GtkWidget *bg_color_btn = gtk_color_button_new();
    GtkWidget *bg_label = gtk_label_new("Background Color:");
    gtk_widget_set_margin_start(bg_label, 10);
    gtk_widget_set_margin_top(bg_label, 5);
    gtk_box_append(GTK_BOX(app_data->properties_panel), bg_label);
    gtk_widget_set_margin_start(bg_color_btn, 10);
    gtk_widget_set_margin_top(bg_color_btn, 5);
    gtk_box_append(GTK_BOX(app_data->properties_panel), bg_color_btn);
    
    // Create status bar
    app_data->statusbar = gtk_statusbar_new();
    gtk_statusbar_push(GTK_STATUSBAR(app_data->statusbar), 0, "Ready");
    gtk_box_append(GTK_BOX(content_box), app_data->statusbar);
}

static void activate(GtkApplication *app, gpointer user_data) {
    AppData *app_data = (AppData *)user_data;
    setup_main_window(app, app_data);
    gtk_application_add_window(app, GTK_WINDOW(app_data->window));
    gtk_widget_show(app_data->window);
}

int main(int argc, char **argv) {
    AppData app_data = {0};
    
    AdwApplication *app = adw_application_new("com.example.Present", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &app_data);
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}
