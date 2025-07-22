#include "animations.h"
#include <gtk/gtk.h>
#include <adwaita.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *rotate_btn;
    GtkWidget *pushback_btn;
    GtkWidget *pulse_btn;
    GtkWidget *test_widget;
    GtkWidget *camera_widget;
    GtkWidget *slides_widget;
    gboolean rotation_active;
    gboolean pushback_active;
    gboolean pulse_active;
    gboolean camera_active;
} TestApp;

static void on_rotate_clicked(GtkButton *button, gpointer user_data) {
    TestApp *app = (TestApp *)user_data;
    
    if (app->rotation_active) {
        g_print("Continuous rotation already active\n");
        return;
    }
    
    g_print("Rotate button clicked\n");
    app->rotation_active = TRUE;
    app->pushback_active = FALSE; // Stop pushback if running
    
    gtk_button_set_label(button, "Rotating...");
    gtk_widget_set_sensitive(app->pushback_btn, FALSE);
    
    start_rotation_cycle(app->test_widget, 5);
    
    // Re-enable after a delay (for demo purposes)
    g_timeout_add(2000, (GSourceFunc)gtk_widget_set_sensitive, app->pushback_btn);
}

static void on_pushback_clicked(GtkButton *button, gpointer user_data) {
    TestApp *app = (TestApp *)user_data;
    
    if (app->pushback_active) {
        g_print("Pushback rotation already active\n");
        return;
    }
    
    g_print("Pushback button clicked\n");
    app->pushback_active = TRUE;
    app->rotation_active = FALSE; // Stop continuous if running
    
    gtk_button_set_label(button, "Rotating...");
    gtk_widget_set_sensitive(app->rotate_btn, FALSE);
    
    rotate_with_pushback(app->test_widget, 360, 6);
    
    // Reset button after animation
    g_timeout_add(6000, (GSourceFunc)gtk_button_set_label, button);
    g_timeout_add(6000, (GSourceFunc)gtk_widget_set_sensitive, app->rotate_btn);
    g_timeout_add(6100, (GSourceFunc)g_object_set_data, button); // Reset active flag
}

static gboolean reset_pushback_state(gpointer user_data) {
    TestApp *app = (TestApp *)user_data;
    app->pushback_active = FALSE;
    gtk_button_set_label(GTK_BUTTON(app->pushback_btn), "Pushback Rotation");
    return G_SOURCE_REMOVE;
}

static void on_pulse_clicked(GtkButton *button, gpointer user_data) {
    TestApp *app = (TestApp *)user_data;
    
    if (app->pulse_active) {
        g_print("Pulse animation already active, stopping...\n");
        // Stop pulse by removing animation context
        g_object_set_data(G_OBJECT(app->test_widget), "pulse_ctx", NULL);
        app->pulse_active = FALSE;
        gtk_button_set_label(button, "Start Pulse");
        return;
    }
    
    g_print("Pulse button clicked - starting looping pulse\n");
    app->pulse_active = TRUE;
    gtk_button_set_label(button, "Stop Pulse");
    
    // Use opacity animation with looping
    AnimationParams alpha_params = {
        .widget = app->test_widget,
        .start_value = 1.0,
        .end_value = 0.3,
        .duration_ms = 500
    };
    animate_property(&alpha_params, "opacity");
}

static void on_camera_clicked(GtkButton *button, gpointer user_data) {
    TestApp *app = (TestApp *)user_data;
    
    if (app->camera_active) {
        g_print("Camera animation already active\n");
        return;
    }
    
    g_print("Camera button clicked\n");
    app->camera_active = TRUE;
    gtk_button_set_label(button, "Moving...");
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
    
    move_and_rotate(app->camera_widget, app->slides_widget, -1300, 3.14, 2000, 4000);
    
    // Reset button after animation
    g_timeout_add(5000, (GSourceFunc)gtk_button_set_label, button);
    g_timeout_add(5000, (GSourceFunc)gtk_widget_set_sensitive, button);
    g_timeout_add(5100, (GSourceFunc)g_object_set_data, app); // Reset active flag
}

static gboolean reset_camera_state(gpointer user_data) {
    TestApp *app = (TestApp *)user_data;
    app->camera_active = FALSE;
    gtk_button_set_label(GTK_BUTTON(gtk_widget_get_last_child(gtk_widget_get_first_child(GTK_WIDGET(app->window)))), "Camera Move");
    return G_SOURCE_REMOVE;
}

static void setup_test_window(GtkApplication *app, gpointer user_data) {
    TestApp *test_app = (TestApp *)user_data;
    
    // Initialize state
    test_app->rotation_active = FALSE;
    test_app->pushback_active = FALSE;
    test_app->pulse_active = FALSE;
    test_app->camera_active = FALSE;
    
    test_app->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(test_app->window), "Animation Tests - Fixed");
    gtk_window_set_default_size(GTK_WINDOW(test_app->window), 800, 600);
    
    // Create main container
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(main_box, 10);
    gtk_widget_set_margin_bottom(main_box, 10);
    gtk_widget_set_margin_start(main_box, 10);
    gtk_widget_set_margin_end(main_box, 10);
    gtk_window_set_child(GTK_WINDOW(test_app->window), main_box);
    
    // Button container
    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(btn_box), TRUE);
    gtk_box_append(GTK_BOX(main_box), btn_box);
    
    // Create buttons
    test_app->rotate_btn = gtk_button_new_with_label("Start Rotation");
    g_signal_connect(test_app->rotate_btn, "clicked", G_CALLBACK(on_rotate_clicked), test_app);
    gtk_box_append(GTK_BOX(btn_box), test_app->rotate_btn);
    
    test_app->pushback_btn = gtk_button_new_with_label("Pushback Rotation");
    g_signal_connect(test_app->pushback_btn, "clicked", G_CALLBACK(on_pushback_clicked), test_app);
    gtk_box_append(GTK_BOX(btn_box), test_app->pushback_btn);
    
    test_app->pulse_btn = gtk_button_new_with_label("Start Pulse");
    g_signal_connect(test_app->pulse_btn, "clicked", G_CALLBACK(on_pulse_clicked), test_app);
    gtk_box_append(GTK_BOX(btn_box), test_app->pulse_btn);
    
    GtkWidget *camera_btn = gtk_button_new_with_label("Camera Move");
    g_signal_connect(camera_btn, "clicked", G_CALLBACK(on_camera_clicked), test_app);
    gtk_box_append(GTK_BOX(btn_box), camera_btn);
    
    // Test area
    GtkWidget *test_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_hexpand(test_area, TRUE);
    gtk_widget_set_vexpand(test_area, TRUE);
    gtk_widget_set_halign(test_area, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(test_area, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(main_box), test_area);
    
    // Test widget - make it more visible
    test_app->test_widget = gtk_image_new_from_icon_name("starred");
    gtk_image_set_pixel_size(GTK_IMAGE(test_app->test_widget), 64);
    gtk_widget_add_css_class(test_app->test_widget, "test-widget");
    gtk_box_append(GTK_BOX(test_area), test_app->test_widget);
    
    // Other widgets
    test_app->camera_widget = gtk_image_new_from_icon_name("camera-photo");
    gtk_image_set_pixel_size(GTK_IMAGE(test_app->camera_widget), 48);
    gtk_box_append(GTK_BOX(test_area), test_app->camera_widget);
    
    test_app->slides_widget = gtk_image_new_from_icon_name("view-paged");
    gtk_image_set_pixel_size(GTK_IMAGE(test_app->slides_widget), 48);
    gtk_box_append(GTK_BOX(test_area), test_app->slides_widget);
    
    // Apply CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
        ".test-widget { "
        "  background-color: #3584e4; "
        "  color: white; "
        "  border-radius: 12px; "
        "  padding: 16px; "
        "  margin: 8px; "
        "} "
        ".test-widget:hover { "
        "  background-color: #1c71d8; "
        "}";
    
    gtk_css_provider_load_from_string(provider, css);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    g_object_unref(provider);
    
    // Show the window
    gtk_window_present(GTK_WINDOW(test_app->window));
}

int main(int argc, char **argv) {
    TestApp app = {0};
    
    // Initialize Adwaita
    adw_init();
    
    GtkApplication *gtk_app = gtk_application_new("com.example.animation_test_fixed", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(gtk_app, "activate", G_CALLBACK(setup_test_window), &app);
    
    int status = g_application_run(G_APPLICATION(gtk_app), argc, argv);
    g_object_unref(gtk_app);
    
    return status;
}
