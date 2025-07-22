#include "animations.h"
#include <math.h>

// Animation context for custom animations
typedef struct {
    GtkWidget *widget;
    double current_rotation;
    gint64 start_time;
    guint duration_ms;
    guint timeout_id;
    gboolean is_continuous;
    GtkCssProvider *provider;
    gint cycle_count; // Track rotation cycles
} RotationContext;

typedef struct {
    GtkWidget *widget;
    double start_opacity;
    double target_opacity;
    double start_scale;
    double target_scale;
    gint64 start_time;
    guint duration_ms;
    guint timeout_id;
    GtkCssProvider *provider;
    gint phase; // 0=fade in, 1=scale up, 2=fade out, 3=scale down, 4=interval
    gint64 phase_start_time;
} PulseContext;

typedef struct {
    GtkWidget *camera_widget;
    GtkWidget *slides_widget;
    GtkWidget *main_container;
    gint start_y;
    gint target_y;
    double start_rotation;
    double target_rotation;
    gint64 move_start_time;
    gint64 rotate_start_time;
    guint move_duration_ms;
    guint rotate_duration_ms;
    guint timeout_id;
    GtkCssProvider *move_provider;
    GtkCssProvider *rotate_provider;
} CameraContext;

// Static helper functions
static gboolean update_rotation_timeout(gpointer user_data);
static gboolean update_pulse_timeout(gpointer user_data);
static gboolean update_camera_timeout(gpointer user_data);
static void cleanup_rotation_context(RotationContext *ctx);
static void cleanup_pulse_context(PulseContext *ctx);
static void cleanup_camera_context(CameraContext *ctx);

void animate_property(AnimationParams *params, const char *property) {
    if (g_strcmp0(property, "opacity") == 0) {
        // Stop any existing pulse animation
        PulseContext *existing = g_object_get_data(G_OBJECT(params->widget), "pulse_ctx");
        if (existing) {
            g_object_set_data(G_OBJECT(params->widget), "pulse_ctx", NULL);
        }
        
        PulseContext *ctx = g_new0(PulseContext, 1);
        ctx->widget = params->widget;
        ctx->start_opacity = 1.0; // Start visible
        ctx->target_opacity = 0.3; // Fade to 30%
        ctx->start_scale = 1.0;
        ctx->target_scale = 1.5;
        ctx->duration_ms = 500; // Each phase duration
        ctx->phase = 0; // Start with fade in
        ctx->start_time = g_get_monotonic_time() / 1000;
        ctx->phase_start_time = ctx->start_time;
        ctx->provider = gtk_css_provider_new();
        
        g_object_set_data_full(G_OBJECT(params->widget), "pulse_ctx", ctx,
                              (GDestroyNotify)cleanup_pulse_context);
        
        ctx->timeout_id = g_timeout_add(16, update_pulse_timeout, ctx);
        g_print("Starting looping pulse animation\n");
    } else {
        g_warning("Property '%s' is not supported for animation", property);
    }
}

void start_rotation_cycle(GtkWidget *widget, guint duration_ms) {
    g_print("Starting continuous rotation cycle\n");
    
    // Clean up any existing animation on this widget
    RotationContext *existing = g_object_get_data(G_OBJECT(widget), "rotation_ctx");
    if (existing) {
        g_object_set_data(G_OBJECT(widget), "rotation_ctx", NULL);
    }
    
    // Reset widget rotation first
    reset_rotation(widget);
    
    RotationContext *ctx = g_new0(RotationContext, 1);
    ctx->widget = widget;
    ctx->current_rotation = 0.0;
    ctx->start_time = g_get_monotonic_time() / 1000;
    ctx->duration_ms = duration_ms * 1000; // Convert to milliseconds for full cycle
    ctx->is_continuous = TRUE;
    ctx->cycle_count = 0;
    ctx->provider = gtk_css_provider_new();
    
    g_object_set_data_full(G_OBJECT(widget), "rotation_ctx", ctx,
                          (GDestroyNotify)cleanup_rotation_context);
    
    ctx->timeout_id = g_timeout_add(16, update_rotation_timeout, ctx);
}

void rotate_with_pushback(GtkWidget *widget, gdouble rotation, guint duration_ms) {
    g_print("Starting pushback rotation\n");
    
    // Clean up any existing animation
    RotationContext *existing = g_object_get_data(G_OBJECT(widget), "rotation_ctx");
    if (existing) {
        g_object_set_data(G_OBJECT(widget), "rotation_ctx", NULL);
    }
    
    RotationContext *ctx = g_new0(RotationContext, 1);
    ctx->widget = widget;
    ctx->current_rotation = 0.0; // Always start from 0
    ctx->start_time = g_get_monotonic_time() / 1000;
    ctx->duration_ms = duration_ms * 1000; // Convert to milliseconds
    ctx->is_continuous = FALSE;
    ctx->provider = gtk_css_provider_new();
    
    g_object_set_data_full(G_OBJECT(widget), "rotation_ctx", ctx,
                          (GDestroyNotify)cleanup_rotation_context);
    
    ctx->timeout_id = g_timeout_add(16, update_rotation_timeout, ctx);
}

void move_and_rotate(GtkWidget *widget, GtkWidget *slides, 
                    gint y_position, gdouble rotation, 
                    guint move_duration, guint rotate_duration) {
    g_print("Starting camera movement with viewport rotation\n");
    
    // Find the main container (should be the child of the window)
    GtkWidget *main_container = widget;
    while (main_container && gtk_widget_get_parent(main_container)) {
        GtkWidget *parent = gtk_widget_get_parent(main_container);
        if (GTK_IS_WINDOW(parent)) {
            break; // main_container is now the direct child of the window
        }
        main_container = parent;
    }
    
    // Clean up any existing camera animation
    CameraContext *existing = g_object_get_data(G_OBJECT(widget), "camera_ctx");
    if (existing) {
        g_object_set_data(G_OBJECT(widget), "camera_ctx", NULL);
    }
    
    CameraContext *ctx = g_new0(CameraContext, 1);
    ctx->camera_widget = widget;
    ctx->slides_widget = slides;
    ctx->main_container = main_container;
    ctx->start_y = 0;
    ctx->target_y = y_position;
    ctx->start_rotation = 0.0;
    ctx->target_rotation = rotation * 180.0 / M_PI; // Convert radians to degrees
    ctx->move_start_time = g_get_monotonic_time() / 1000;
    ctx->rotate_start_time = ctx->move_start_time;
    ctx->move_duration_ms = move_duration;
    ctx->rotate_duration_ms = rotate_duration;
    ctx->move_provider = gtk_css_provider_new();
    ctx->rotate_provider = gtk_css_provider_new();
    
    g_object_set_data_full(G_OBJECT(widget), "camera_ctx", ctx,
                          (GDestroyNotify)cleanup_camera_context);
    
    ctx->timeout_id = g_timeout_add(16, update_camera_timeout, ctx);
}

void reset_rotation(GtkWidget *widget) {
    g_print("Resetting rotation\n");
    
    // Clean up any existing animation
    g_object_set_data(G_OBJECT(widget), "rotation_ctx", NULL);
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, "* { transform: rotate(0deg); transition: none; }");
    
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(widget),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
    
    g_object_unref(provider);
}

static gboolean update_rotation_timeout(gpointer user_data) {
    RotationContext *ctx = (RotationContext *)user_data;
    gint64 current_time = g_get_monotonic_time() / 1000;
    gint64 elapsed = current_time - ctx->start_time;
    double progress = (double)elapsed / (double)ctx->duration_ms;
    
    if (ctx->is_continuous) {
        // Continuous rotation - smooth sine wave motion
        if (progress >= 1.0) {
            // Reset for next cycle
            ctx->start_time = current_time;
            ctx->cycle_count++;
            progress = 0.0;
            
            // Reset rotation to 0 every cycle to prevent accumulation
            if (ctx->cycle_count % 2 == 0) {
                reset_rotation(ctx->widget);
                return G_SOURCE_CONTINUE;
            }
        }
        
        // Sine wave easing for smooth continuous rotation
        double sine_progress = sin(progress * M_PI * 2) * 0.5 + 0.5;
        ctx->current_rotation = sine_progress * 360.0;
        
    } else {
        // Single rotation with pushback
        if (progress >= 1.0) {
            progress = 1.0;
            ctx->current_rotation = 360.0;
            
            char *css = g_strdup_printf("* { transform: rotate(%.2fdeg); transition: none; }", 
                                       ctx->current_rotation);
            gtk_css_provider_load_from_string(ctx->provider, css);
            g_free(css);
            
            gtk_style_context_add_provider(
                gtk_widget_get_style_context(ctx->widget),
                GTK_STYLE_PROVIDER(ctx->provider),
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
            
            g_print("Pushback rotation complete\n");
            return G_SOURCE_REMOVE;
        }
        
        // Back easing (pushback effect)
        const double c1 = 1.70158;
        const double c3 = c1 + 1;
        double eased_progress = 1 + c3 * pow(progress - 1, 3) + c1 * pow(progress - 1, 2);
        ctx->current_rotation = eased_progress * 360.0;
    }
    
    char *css = g_strdup_printf("* { transform: rotate(%.2fdeg); transition: none; }", 
                               ctx->current_rotation);
    gtk_css_provider_load_from_string(ctx->provider, css);
    g_free(css);
    
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(ctx->widget),
        GTK_STYLE_PROVIDER(ctx->provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
    
    return G_SOURCE_CONTINUE;
}

static gboolean update_pulse_timeout(gpointer user_data) {
    PulseContext *ctx = (PulseContext *)user_data;
    gint64 current_time = g_get_monotonic_time() / 1000;
    gint64 elapsed = current_time - ctx->phase_start_time;
    double progress = (double)elapsed / (double)ctx->duration_ms;
    
    if (progress >= 1.0) {
        // Move to next phase
        ctx->phase = (ctx->phase + 1) % 5; // 5 phases: fade in, scale up, fade out, scale down, interval
        ctx->phase_start_time = current_time;
        progress = 0.0;
        
        if (ctx->phase == 4) {
            // Interval phase - wait 1 second
            ctx->duration_ms = 1000;
        } else {
            // Animation phase
            ctx->duration_ms = 500;
        }
    }
    
    double current_opacity, current_scale;
    
    switch (ctx->phase) {
        case 0: // Fade in (opacity 1.0 -> 0.3)
            current_opacity = 1.0 - (0.7 * progress);
            current_scale = 1.0;
            break;
        case 1: // Scale up (scale 1.0 -> 1.5)
            current_opacity = 0.3;
            current_scale = 1.0 + (0.5 * progress);
            break;
        case 2: // Fade out (opacity 0.3 -> 1.0)
            current_opacity = 0.3 + (0.7 * progress);
            current_scale = 1.5;
            break;
        case 3: // Scale down (scale 1.5 -> 1.0)
            current_opacity = 1.0;
            current_scale = 1.5 - (0.5 * progress);
            break;
        case 4: // Interval
            current_opacity = 1.0;
            current_scale = 1.0;
            break;
        default:
            current_opacity = 1.0;
            current_scale = 1.0;
            break;
    }
    
    char *css = g_strdup_printf("* { opacity: %.2f; transform: scale(%.2f); transition: none; }", 
                               current_opacity, current_scale);
    gtk_css_provider_load_from_string(ctx->provider, css);
    g_free(css);
    
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(ctx->widget),
        GTK_STYLE_PROVIDER(ctx->provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
    
    return G_SOURCE_CONTINUE; // Keep looping
}

static gboolean update_camera_timeout(gpointer user_data) {
    CameraContext *ctx = (CameraContext *)user_data;
    gint64 current_time = g_get_monotonic_time() / 1000;
    
    gint64 move_elapsed = current_time - ctx->move_start_time;
    gint64 rotate_elapsed = current_time - ctx->rotate_start_time;
    
    double move_progress = (double)move_elapsed / (double)ctx->move_duration_ms;
    double rotate_progress = (double)rotate_elapsed / (double)ctx->rotate_duration_ms;
    
    gboolean move_complete = move_progress >= 1.0;
    gboolean rotate_complete = rotate_progress >= 1.0;
    
    // Update camera position (viewport movement)
    if (!move_complete) {
        // Ease in-out for smooth movement
        double eased_move = move_progress < 0.5 ? 
            2 * move_progress * move_progress : 
            1 - 2 * (1 - move_progress) * (1 - move_progress);
            
        gint current_y = ctx->start_y + (gint)((ctx->target_y - ctx->start_y) * eased_move);
        
        // Apply viewport transformation with rotation
        char *move_css = g_strdup_printf(
            "* { transform: translateY(%dpx) rotate(%.2fdeg); transition: none; }", 
            current_y, ctx->target_rotation);
        gtk_css_provider_load_from_string(ctx->move_provider, move_css);
        g_free(move_css);
        
        if (ctx->main_container) {
            gtk_style_context_add_provider(
                gtk_widget_get_style_context(ctx->main_container),
                GTK_STYLE_PROVIDER(ctx->move_provider),
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 2);
        }
    }
    
    // Update slides rotation
    if (!rotate_complete) {
        // Sine ease in-out
        double eased_rotate = sin(rotate_progress * M_PI / 2);
        double current_rotation = ctx->start_rotation + (ctx->target_rotation - ctx->start_rotation) * eased_rotate;
        
        char *rotate_css = g_strdup_printf("* { transform: rotate(%.2fdeg); transition: none; }", 
                                          current_rotation);
        gtk_css_provider_load_from_string(ctx->rotate_provider, rotate_css);
        g_free(rotate_css);
        
        gtk_style_context_add_provider(
            gtk_widget_get_style_context(ctx->slides_widget),
            GTK_STYLE_PROVIDER(ctx->rotate_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
    }
    
    if (move_complete && rotate_complete) {
        g_print("Camera movement complete\n");
        return G_SOURCE_REMOVE;
    }
    
    return G_SOURCE_CONTINUE;
}

static void cleanup_rotation_context(RotationContext *ctx) {
    if (ctx) {
        if (ctx->timeout_id > 0) {
            g_source_remove(ctx->timeout_id);
        }
        if (ctx->provider) {
            g_object_unref(ctx->provider);
        }
        g_free(ctx);
    }
}

static void cleanup_pulse_context(PulseContext *ctx) {
    if (ctx) {
        if (ctx->timeout_id > 0) {
            g_source_remove(ctx->timeout_id);
        }
        if (ctx->provider) {
            g_object_unref(ctx->provider);
        }
        g_free(ctx);
    }
}

static void cleanup_camera_context(CameraContext *ctx) {
    if (ctx) {
        if (ctx->timeout_id > 0) {
            g_source_remove(ctx->timeout_id);
        }
        if (ctx->move_provider) {
            g_object_unref(ctx->move_provider);
        }
        if (ctx->rotate_provider) {
            g_object_unref(ctx->rotate_provider);
        }
        g_free(ctx);
    }
}
