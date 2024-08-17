#include <stdio.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

static void wl_output_handle_bind(struct wl_client *client, void *data,
                                  uint32_t version, uint32_t id) {
  struct my_state *state = data;
  // TODO
}

struct my_state {};

int main(int argc, char *argv[]) {
  struct wl_display *display = wl_display_create();
  if (!display) {
    fprintf(stderr, "Unable to create Wayland display!");
    return 1;
  }

  struct my_state state = {};
  wl_global_create(display, &wl_output_interface, 1, &state,
                   wl_output_handle_bind);

  const char *socket = wl_display_add_socket_auto(display);
  if (!socket) {
    fprintf(stderr, "Unable to add socket to Wayland display.\n");
    return 0;
  }

  fprintf(stderr, "Running Wayland display on %s\n", socket);
  wl_display_run(display); // Should run on infinite loop?

  wl_display_destroy(display);
  return 0;
}
