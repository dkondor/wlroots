#include <assert.h>
#include <stdlib.h>
#include <wlr/interfaces/wlr_output.h>
#include <wlr/util/log.h>
#include "backend/noop.h"
#include "util/signal.h"

struct wlr_noop_backend *noop_backend_from_backend(
		struct wlr_backend *wlr_backend) {
	assert(wlr_backend_is_noop(wlr_backend));
	return (struct wlr_noop_backend *)wlr_backend;
}

static bool backend_start(struct wlr_backend *wlr_backend) {
	struct wlr_noop_backend *backend = noop_backend_from_backend(wlr_backend);
	wlr_log(WLR_INFO, "Starting noop backend");

	struct wlr_noop_output *output;
	wl_list_for_each(output, &backend->outputs, link) {
		wlr_output_update_enabled(&output->wlr_output, true);
		wlr_signal_emit_safe(&backend->backend.events.new_output,
			&output->wlr_output);
	}

	backend->started = true;
	return true;
}

static void backend_destroy(struct wlr_backend *wlr_backend) {
	struct wlr_noop_backend *backend = noop_backend_from_backend(wlr_backend);
	if (!wlr_backend) {
		return;
	}

	struct wlr_noop_output *output, *output_tmp;
	wl_list_for_each_safe(output, output_tmp, &backend->outputs, link) {
		wlr_output_destroy(&output->wlr_output);
	}

	wlr_backend_finish(wlr_backend);

	wl_list_remove(&backend->display_destroy.link);

	free(backend);
}

static const struct wlr_backend_impl backend_impl = {
	.start = backend_start,
	.destroy = backend_destroy,
};

static void handle_display_destroy(struct wl_listener *listener, void *data) {
	struct wlr_noop_backend *noop =
		wl_container_of(listener, noop, display_destroy);
	backend_destroy(&noop->backend);
}

struct wlr_backend *wlr_noop_backend_create(struct wl_display *display) {
	wlr_log(WLR_INFO, "Creating noop backend");

	struct wlr_noop_backend *backend =
		calloc(1, sizeof(struct wlr_noop_backend));
	if (!backend) {
		wlr_log(WLR_ERROR, "Failed to allocate wlr_noop_backend");
		return NULL;
	}
	wlr_backend_init(&backend->backend, &backend_impl);
	backend->display = display;
	wl_list_init(&backend->outputs);

	backend->display_destroy.notify = handle_display_destroy;
	wl_display_add_destroy_listener(display, &backend->display_destroy);

	return &backend->backend;
}

bool wlr_backend_is_noop(struct wlr_backend *backend) {
	return backend->impl == &backend_impl;
}
