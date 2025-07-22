#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <gtk/gtk.h>
#include <adwaita.h>

typedef struct {
    GtkWidget *widget;
    double start_value;
    double end_value;
    guint duration_ms;
} AnimationParams;

// Basic animation function
void animate_property(AnimationParams *params, const char *property);

// Rotation functions
void start_rotation_cycle(GtkWidget *widget, guint duration_ms);
void rotate_with_pushback(GtkWidget *widget, gdouble rotation, guint duration_ms);
void reset_rotation(GtkWidget *widget);

// Combined movement and rotation
void move_and_rotate(GtkWidget *widget, GtkWidget *slides, 
                    gint y_position, gdouble rotation, 
                    guint move_duration, guint rotate_duration);

#endif
