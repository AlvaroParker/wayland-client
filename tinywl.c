#ifndef WLR_USE_UNSTABLE
#define WLR_USE_UNSTABLE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlroots-0.18/wlr/backend.h>
#include <wlroots-0.18/wlr/render/allocator.h>
#include <wlroots-0.18/wlr/render/wlr_renderer.h>
#include <wlroots-0.18/wlr/types/wlr_compositor.h>
#include <wlroots-0.18/wlr/types/wlr_cursor.h>
#include <wlroots-0.18/wlr/types/wlr_data_device.h>
#include <wlroots-0.18/wlr/types/wlr_input_device.h>
#include <wlroots-0.18/wlr/types/wlr_keyboard.h>
#include <wlroots-0.18/wlr/types/wlr_output.h>
#include <wlroots-0.18/wlr/types/wlr_output_layout.h>
#include <wlroots-0.18/wlr/types/wlr_pointer.h>
#include <wlroots-0.18/wlr/types/wlr_scene.h>
#include <wlroots-0.18/wlr/types/wlr_seat.h>
#include <wlroots-0.18/wlr/types/wlr_subcompositor.h>
#include <wlroots-0.18/wlr/types/wlr_xcursor_manager.h>
#include <wlroots-0.18/wlr/types/wlr_xdg_shell.h>
#include <wlroots-0.18/wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

struct tinywl_server {
	struct wl_display *wl_display;
	struct wlr_backend *backend;
	struct wlr_renderer *renderer;
	struct wlr_allocator *allocator;
};

int main(int argc, char *argv[]) {
	wlr_log_init(WLR_DEBUG, NULL);
	char *startup_cmd = "alacritty";

	struct tinywl_server server = {0};
	server.wl_display = wl_display_create();

	// A backend provides a set of input and output devices. It allows use to control mices, displays, etc.
	server.backend = wlr_backend_autocreate(wl_display_get_event_loop(server.wl_display), NULL);
	if (server.backend == NULL) {
		wlr_log(WLR_ERROR, "Error while creating WLR backend");
		return 1;
	}

	// Renderer for basics 2D operations
	server.renderer = wlr_renderer_autocreate(server.backend);
	if (server.renderer == NULL) {
		wlr_log(WLR_ERROR, "Failed to create wlr_renderer");
		return 1;
	}

	wlr_renderer_init_wl_display(server.renderer, server.wl_display);

	// An wlr allocator is responsible for allocating memory for pixel buffers
	server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
	if (server.allocator == NULL) {
		wlr_log(WLR_ERROR, "Failed to create wlr_allocator");
		return 1;
	}

	return 0;
}
