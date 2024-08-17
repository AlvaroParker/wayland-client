#include "pool.h"
#include "xdg-shell-client-protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

struct client_state {
	struct wl_display *wl_display;
	struct wl_registry *wl_registry;
	struct wl_surface *wl_surface;
	struct wl_shm *wl_shm;
	struct wl_compositor *wl_compositor;
	struct xdg_wm_base *xdg_wm_base;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	/* State */
	float offset;
	uint32_t last_frame;
};

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
	/* Sent by the compositor when it's no longer using this buffer */
	wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
	.release = wl_buffer_release,
};

void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) { xdg_wm_base_pong(xdg_wm_base, serial); }
static const struct xdg_wm_base_listener xdg_wm_base_listener = {.ping = xdg_wm_base_ping};

void registry_global(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version) {
	struct client_state *state = data;
	// We want to bind to the wl_shm, wl_compositor and xdg_wm_base interfaces
	if (strcmp(interface, wl_shm_interface.name) == 0) {
		state->wl_shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		state->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, 4);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		state->xdg_wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, state);
	}
}

uint32_t generate_random_32_bit() {
	uint32_t random_num = ((uint32_t)rand() << 16 | (uint32_t)rand());
	return random_num;
}

// Create a shm object, attach data to it and return wl_buffer with memory mapped to that shared memory obejct
static struct wl_buffer *draw_frame(struct client_state *state) {
	const int width = 1920, height = 1080;
	int stride = width * 4;
	int size = stride * height;

	int fd = allocate_shm_file(size);
	if (fd == -1) {
		return NULL;
	}
	uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		close(fd);
		return NULL;
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(pool);

	close(fd);

	/* Draw checkerboxed background */
	int offset = (int)state->offset % 8;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (((x + offset) + (y + offset) / 8 * 8) % 16 < 8)
				// data[y * width + x] = generate_random_32_bit();
				data[y * width + x] = 0xFF666666;
			else
				// data[y * width + x] = generate_random_32_bit();
				data[y * width + x] = 0xFFEEEEEE;
		}
	}

	munmap(data, size);
	wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
	return buffer;
}

static const struct wl_registry_listener wl_registry_listener = {.global = registry_global};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
	struct client_state *state = data;
	xdg_surface_ack_configure(xdg_surface, serial);

	struct wl_buffer *buffer = draw_frame(state);
	wl_surface_attach(state->wl_surface, buffer, 0, 0);
	wl_surface_commit(state->wl_surface);
}
static const struct xdg_surface_listener xdg_surface_listener = {.configure = xdg_surface_configure};

static const struct wl_callback_listener wl_surface_frame_listener;

static void wl_surface_frame_done(void *data, struct wl_callback *cb, uint32_t time) {
	wl_callback_destroy(cb);

	// Request another frame
	struct client_state *state = data;
	cb = wl_surface_frame(state->wl_surface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, state);

	if (state->last_frame != 0) {
		int elapsed = time - state->last_frame;
		state->offset += elapsed / 1000.0 * 24;
	}

	struct wl_buffer *buffer = draw_frame(state);
	wl_surface_attach(state->wl_surface, buffer, 0, 0);
	wl_surface_damage(state->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(state->wl_surface);

	state->last_frame = time;
}

static const struct wl_callback_listener wl_surface_frame_listener = {.done = wl_surface_frame_done};

int main(int argc, char *argv[]) {
	struct client_state state = {0};
	state.wl_display = wl_display_connect(NULL);								// Connect to default wayland display
	state.wl_registry = wl_display_get_registry(state.wl_display);				// Get the registry
	wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state); // This will get called for each global in the registry

	// Wait for the initial set of globals to appear
	wl_display_roundtrip(state.wl_display);

	// Create a wl surface
	state.wl_surface = wl_compositor_create_surface(state.wl_compositor);
	// Get an xdg surface
	// This will make our wl_surface have a valid state in a desktop env
	state.xdg_surface = xdg_wm_base_get_xdg_surface(state.xdg_wm_base, state.wl_surface);

	// Add a listener for the configure event which get called when a re-render is considered necesary by the compositor
	xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);

	state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
	wl_surface_commit(state.wl_surface);
	// Register a frame callback
	struct wl_callback *cb = wl_surface_frame(state.wl_surface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, &state);
	while (wl_display_dispatch(state.wl_display)) {
		// This line was left in blank
	}
	return 0;
}
