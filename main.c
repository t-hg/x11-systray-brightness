#include <stdbool.h>
#include <gtk/gtk.h>

GtkStatusIcon *status_icon;

void flogf(FILE* stream, const char *fmt1, const char* fmt2, ...) {
  char *fmt = malloc(strlen(fmt1) + strlen(fmt2) + 1);
  strcpy(fmt, fmt1);
  strcat(fmt, fmt2);
  va_list args;
  va_start(args, fmt2);
  vfprintf(stderr, fmt, args); 
  va_end(args);
  free(fmt);
}

#define LOG_INFO(fmt, ...) flogf(stdout, "[*] %s:%d ", fmt, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(fmt, ...) flogf(stderr, "[!] %s:%d ", fmt, __FILE__, __LINE__, __VA_ARGS__)

const char* brightnessctl_run(const char* args) {
  const char *bin = "brightnessctl ";
  char *cmd = malloc(strlen(bin) + strlen(args) + 1);
  strcpy(cmd, bin);
  strcat(cmd, args);
  FILE *f = popen(cmd, "r");
  if (f == NULL) {
    LOG_ERROR("popen failed\n", NULL);
    goto defer;
  }
  char buf[128] = "";
  while(fgets(buf, sizeof(buf), f) != NULL) {
  }
  if (pclose(f) != 0) {
    LOG_ERROR("%s failed\n", cmd);
    goto defer;
  }
defer:
  free(cmd);
  const char *result = buf;
  return result;
}

int get_max_brightness() {
  const char* result = brightnessctl_run("max");
  return atoi(result);
}

int get_brightness() {
  const char* result = brightnessctl_run("get");
  return atoi(result);
}

void set_brightness(int value) {
  char args[128];
  sprintf(args, "set %d", value);
  brightnessctl_run(args);
}

gchar* scale_format_value(GtkWidget *scale, gdouble value, gpointer user_data) {
  // hide value
  return g_strdup_printf("");
}

void scale_on_brightness_changed(GtkWidget *scale, gpointer user_data) {
  int value = (int) gtk_range_get_value(GTK_RANGE(scale));
  set_brightness(value);
}

void destroy_menu(GtkWidget *window, GdkEvent *event, gpointer user_data) {
  gdouble x = event->button.x;
  gdouble y = event->button.y;
  gint w, h;
  gtk_window_get_size(GTK_WINDOW(window), &w, &h);
  if (x < 0 || y < 0 || x > w || y > h) {
    gdk_pointer_ungrab(event->button.time);
    gtk_widget_destroy(window);
  }
}

void make_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data){
  GtkWidget *label_brightness = gtk_label_new("Brightness:");
  gtk_widget_set_halign(label_brightness, GTK_ALIGN_START);
  GtkWidget *scale_brightness = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, get_max_brightness(), 1);
  gtk_scale_set_value_pos(GTK_SCALE(scale_brightness), GTK_POS_RIGHT);
  gtk_range_set_value(GTK_RANGE(scale_brightness), get_brightness());
  g_signal_connect(G_OBJECT(scale_brightness), "format-value", G_CALLBACK(scale_format_value), NULL);
  g_signal_connect(G_OBJECT(scale_brightness), "value-changed", G_CALLBACK(scale_on_brightness_changed), NULL);
  GtkWidget *vbox = gtk_vbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(vbox), label_brightness, true, true, 0);
  gtk_box_pack_start(GTK_BOX(vbox), scale_brightness, true, true, 0);
  GtkWidget *window = gtk_window_new(GTK_WINDOW_POPUP);
  gtk_window_set_decorated(GTK_WINDOW(window), false);
  gtk_window_set_resizable(GTK_WINDOW(window), false);
  gtk_window_set_default_size(GTK_WINDOW(window), 200, -1);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_container_set_border_width(GTK_CONTAINER(window), 10);
  gtk_widget_show_all(window);
  gdk_pointer_grab(gtk_widget_get_window(window), true, GDK_BUTTON_PRESS_MASK, NULL, NULL, activate_time);
  g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(destroy_menu), NULL);
}

void make_status_icon(void){
  status_icon = gtk_status_icon_new();
  g_signal_connect(G_OBJECT(status_icon), "activate", G_CALLBACK(make_menu), NULL);
  g_signal_connect(G_OBJECT(status_icon), "popup-menu", G_CALLBACK(make_menu), NULL);
  gtk_status_icon_set_from_icon_name(status_icon, "display-brightness");
  gtk_status_icon_set_visible(status_icon, true);
}

int main(int argc, char **argv) {
  gtk_init(&argc, &argv);
  make_status_icon();
  gtk_main();
  return 0;
}
