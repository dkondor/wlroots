
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/util/log.h>
#include "wlr-foreign-toplevel-management-unstable-v1-protocol.h"

#define FOREIGN_TOPLEVEL_MANAGEMENT_V1_VERSION 4

static const struct zwlr_foreign_toplevel_handle_v1_interface toplevel_handle_impl;

static struct wlr_foreign_toplevel_handle_v1 *toplevel_handle_from_resource(
		struct wl_resource *resource) {
	assert(wl_resource_instance_of(resource,
			&zwlr_foreign_toplevel_handle_v1_interface,
			&toplevel_handle_impl));
	return wl_resource_get_user_data(resource);
}

static void toplevel_handle_send_maximized_event(struct wl_resource *resource,
		bool state) {
	struct wlr_foreign_toplevel_handle_v1 *toplevel =
		toplevel_handle_from_resource(resource);
	if (!toplevel) {
		return;
	}
	struct wlr_foreign_toplevel_handle_v1_maximized_event event = {
		.toplevel = toplevel,
		.maximized = state,
	};
	wl_signal_emit_mutable(&toplevel->events.request_maximize, &event);
}

static void foreign_toplevel_handle_set_maximized(struct wl_client *client,
		struct wl_resource *resource) {
	toplevel_handle_send_maximized_event(resource, true);
}

static void foreign_toplevel_handle_unset_maximized(struct wl_client *client,
		struct wl_resource *resource) {
	toplevel_handle_send_maximized_event(resource, false);
}

static void toplevel_send_minimized_event(struct wl_resource *resource,
		bool state) {
	struct wlr_foreign_toplevel_handle_v1 *toplevel =
		toplevel_handle_from_resource(resource);
	if (!toplevel) {
		return;
	}

	struct wlr_foreign_toplevel_handle_v1_minimized_event event = {
		.toplevel = toplevel,
		.minimized = state,
	};
	wl_signal_emit_mutable(&toplevel->events.request_minimize, &event);
}

static void foreign_toplevel_handle_set_minimized(struct wl_client *client,
		struct wl_resource *resource) {
	toplevel_send_minimized_event(resource, true);
}

static void foreign_toplevel_handle_unset_minimized(struct wl_client *client,
		struct wl_resource *resource) {
	toplevel_send_minimized_event(resource, false);
}

static void toplevel_send_fullscreen_event(struct wl_resource *resource,
		bool state, struct wl_resource *output_resource) {
	struct wlr_foreign_toplevel_handle_v1 *toplevel =
		toplevel_handle_from_resource(resource);
	if (!toplevel) {
		return;
	}

	struct wlr_output *output = NULL;
	if (output_resource) {
		output = wlr_output_from_resource(output_resource);
	}
	struct wlr_foreign_toplevel_handle_v1_fullscreen_event event = {
		.toplevel = toplevel,
		.fullscreen = state,
		.output = output,
	};
	wl_signal_emit_mutable(&toplevel->events.request_fullscreen, &event);
}

static void foreign_toplevel_handle_set_fullscreen(struct wl_client *client,
		struct wl_resource *resource, struct wl_resource *output) {
	toplevel_send_fullscreen_event(resource, true, output);
}

static void foreign_toplevel_handle_unset_fullscreen(struct wl_client *client,
		struct wl_resource *resource) {
	toplevel_send_fullscreen_event(resource, false, NULL);
}

static void foreign_toplevel_handle_activate(struct wl_client *client,
		struct wl_resource *resource, struct wl_resource *seat) {
	struct wlr_foreign_toplevel_handle_v1 *toplevel =
		toplevel_handle_from_resource(resource);
	if (!toplevel) {
		return;
	}
	struct wlr_seat_client *seat_client = wlr_seat_client_from_resource(seat);
	if (!seat_client) {
		return;
	}

	struct wlr_foreign_toplevel_handle_v1_activated_event event = {
		.toplevel = toplevel,
		.seat = seat_client->seat,
	};
	wl_signal_emit_mutable(&toplevel->events.request_activate, &event);
}

static void foreign_toplevel_handle_close(struct wl_client *client,
		struct wl_resource *resource) {
	struct wlr_foreign_toplevel_handle_v1 *toplevel =
		toplevel_handle_from_resource(resource);
	if (!toplevel) {
		return;
	}
	wl_signal_emit_mutable(&toplevel->events.request_close, toplevel);
}

static void foreign_toplevel_handle_set_rectangle(struct wl_client *client,
		struct wl_resource *resource, struct wl_resource *surface,
		int32_t x, int32_t y, int32_t width, int32_t height) {
	struct wlr_foreign_toplevel_handle_v1 *toplevel =
		toplevel_handle_from_resource(resource);
	if (!toplevel) {
		return;
	}

	if (width < 0 || height < 0) {
		wl_resource_post_error(resource,
			ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_ERROR_INVALID_RECTANGLE,
			"invalid rectangle passed to set_rectangle: width/height < 0");
		return;
	}

	struct wlr_foreign_toplevel_handle_v1_set_rectangle_event event = {
		.toplevel = toplevel,
		.surface = wlr_surface_from_resource(surface),
		.x = x,
		.y = y,
		.width = width,
		.height = height,
	};
	wl_signal_emit_mutable(&toplevel->events.set_rectangle, &event);
}

static void foreign_toplevel_handle_destroy(struct wl_client *client,
		struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static const struct zwlr_foreign_toplevel_handle_v1_interface toplevel_handle_impl = {
	.set_maximized = foreign_toplevel_handle_set_maximized,
	.unset_maximized = foreign_toplevel_handle_unset_maximized,
	.set_minimized = foreign_toplevel_handle_set_minimized,
	.unset_minimized = foreign_toplevel_handle_unset_minimized,
	.activate = foreign_toplevel_handle_activate,
	.close = foreign_toplevel_handle_close,
	.set_rectangle = foreign_toplevel_handle_set_rectangle,
	.destroy = foreign_toplevel_handle_destroy,
	.set_fullscreen = foreign_toplevel_handle_set_fullscreen,
	.unset_fullscreen = foreign_toplevel_handle_unset_fullscreen,
};

static void toplevel_idle_send_done(void *data) {
	struct wlr_foreign_toplevel_handle_v1 *toplevel = data;
	struct wl_resource *resource;
	wl_resource_for_each(resource, &toplevel->resources) {
		zwlr_foreign_toplevel_handle_v1_send_done(resource);
	}

	toplevel->idle_source = NULL;
}

static void toplevel_update_idle_source(
	struct wlr_foreign_toplevel_handle_v1 *toplevel) {
	if (toplevel->idle_source) {
		return;
	}

	toplevel->idle_source = wl_event_loop_add_idle(toplevel->manager->event_loop,
		toplevel_idle_send_done, toplevel);
}

void wlr_foreign_toplevel_handle_v1_set_title(
		struct wlr_foreign_toplevel_handle_v1 *toplevel, const char *title) {
	free(toplevel->title);
	toplevel->title = strdup(title);
	if (toplevel->title == NULL) {
		wlr_log(WLR_ERROR, "failed to allocate memory for toplevel title");
		return;
	}

	struct wl_resource *resource;
	wl_resource_for_each(resource, &toplevel->resources) {
		zwlr_foreign_toplevel_handle_v1_send_title(resource, title);
	}

	toplevel_update_idle_source(toplevel);
}

void wlr_foreign_toplevel_handle_v1_set_app_id(
		struct wlr_foreign_toplevel_handle_v1 *toplevel, const char *app_id) {
	free(toplevel->app_id);
	toplevel->app_id = strdup(app_id);
	if (toplevel->app_id == NULL) {
		wlr_log(WLR_ERROR, "failed to allocate memory for toplevel app_id");
		return;
	}

	struct wl_resource *resource;
	wl_resource_for_each(resource, &toplevel->resources) {
		zwlr_foreign_toplevel_handle_v1_send_app_id(resource, app_id);
	}

	toplevel_update_idle_source(toplevel);
}

static bool toplevel_has_gtk_shell1_dbus_properties(
		struct wlr_foreign_toplevel_handle_v1 *toplevel) {
	return (toplevel->gtk_shell1_application_id != NULL ||
		toplevel->gtk_shell1_app_menu_path != NULL ||
		toplevel->gtk_shell1_application_object_path != NULL ||
		toplevel->gtk_shell1_menubar_path != NULL ||
		toplevel->gtk_shell1_unique_bus_name != NULL ||
		toplevel->gtk_shell1_window_object_path != NULL);
}

void wlr_foreign_toplevel_handle_v1_set_gtk_shell1_dbus_properties(
		struct wlr_foreign_toplevel_handle_v1 *toplevel,
		const char *application_id,
		const char *app_menu_path,
		const char *menubar_path,
		const char *window_object_path,
		const char *application_object_path,
		const char *unique_bus_name) {

	bool have_previous_props = toplevel_has_gtk_shell1_dbus_properties(toplevel);
	bool have_new_props = (application_id != NULL ||
		app_menu_path != NULL || menubar_path != NULL || window_object_path != NULL ||
		application_object_path != NULL || unique_bus_name != NULL);

	if (! (have_previous_props || have_new_props) ) return;

	if (have_previous_props) {
		free(toplevel->gtk_shell1_application_id);
		free(toplevel->gtk_shell1_app_menu_path);
		free(toplevel->gtk_shell1_menubar_path);
		free(toplevel->gtk_shell1_window_object_path);
		free(toplevel->gtk_shell1_application_object_path);
		free(toplevel->gtk_shell1_unique_bus_name);
	}

	if (application_id != NULL) {
		toplevel->gtk_shell1_application_id = strdup(application_id);
		if (toplevel->gtk_shell1_application_id == NULL) {
			wlr_log(WLR_ERROR, "failed to allocate memory for toplevel gtk_shell1_application_id");
			return;
		}
	}
	else toplevel->gtk_shell1_application_id = NULL;

	if (app_menu_path != NULL) {
		toplevel->gtk_shell1_app_menu_path = strdup(app_menu_path);
		if (toplevel->gtk_shell1_app_menu_path == NULL) {
			wlr_log(WLR_ERROR, "failed to allocate memory for toplevel gtk_shell1_app_menu_path");
			return;
		}
	}
	else toplevel->gtk_shell1_app_menu_path = NULL;

	if (menubar_path != NULL) {
		toplevel->gtk_shell1_menubar_path = strdup(menubar_path);
		if (toplevel->gtk_shell1_menubar_path == NULL) {
			wlr_log(WLR_ERROR, "failed to allocate memory for toplevel gtk_shell1_menubar_path");
			return;
		}
	}
	else toplevel->gtk_shell1_menubar_path = NULL;

	if (window_object_path != NULL) {
		toplevel->gtk_shell1_window_object_path = strdup(window_object_path);
		if (toplevel->gtk_shell1_window_object_path == NULL) {
			wlr_log(WLR_ERROR, "failed to allocate memory for toplevel gtk_shell1_window_object_path");
			return;
		}
	}
	else toplevel->gtk_shell1_window_object_path = NULL;

	if (application_object_path != NULL) {
		toplevel->gtk_shell1_application_object_path = strdup(application_object_path);
		if (toplevel->gtk_shell1_application_object_path == NULL) {
			wlr_log(WLR_ERROR, "failed to allocate memory for toplevel gtk_shell1_application_object_path");
			return;
		}
	}
	else toplevel->gtk_shell1_application_object_path = NULL;

	if (unique_bus_name != NULL) {
		toplevel->gtk_shell1_unique_bus_name = strdup(unique_bus_name);
		if (toplevel->gtk_shell1_unique_bus_name == NULL) {
			wlr_log(WLR_ERROR, "failed to allocate memory for toplevel gtk_shell1_unique_bus_name");
			return;
		}
	}
	else toplevel->gtk_shell1_unique_bus_name = NULL;

	struct wl_resource *resource;
	wl_resource_for_each(resource, &toplevel->resources) {
		if (wl_resource_get_version(resource) >=
				ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_GTK_SHELL1_SURFACE_DBUS_PROPERTIES_SINCE_VERSION)
			zwlr_foreign_toplevel_handle_v1_send_gtk_shell1_surface_dbus_properties(
				resource,
				toplevel->gtk_shell1_application_id,
				toplevel->gtk_shell1_app_menu_path,
				toplevel->gtk_shell1_menubar_path,
				toplevel->gtk_shell1_window_object_path,
				toplevel->gtk_shell1_application_object_path,
				toplevel->gtk_shell1_unique_bus_name);
	}

	toplevel_update_idle_source(toplevel);
}

void wlr_foreign_toplevel_handle_v1_set_kde_appmenu_path(
		struct wlr_foreign_toplevel_handle_v1 *toplevel,
		const char *service_name,
		const char *object_path) {
	bool have_previous_path = (
		toplevel->kde_appmenu_service_name != NULL ||
		toplevel->kde_appmenu_object_path != NULL);
	bool have_new_path = (service_name != NULL ||
		object_path != NULL);
	if (! (have_previous_path || have_new_path) ) return;

	if (have_previous_path) {
		free(toplevel->kde_appmenu_service_name);
		free(toplevel->kde_appmenu_object_path);
	}

	if (service_name != NULL) {
		toplevel->kde_appmenu_service_name = strdup(service_name);
		if (toplevel->kde_appmenu_service_name == NULL) {
			wlr_log(WLR_ERROR, "failed to allocate memory for toplevel kde_appmenu_service_name");
			return;
		}
	}
	else toplevel->kde_appmenu_service_name = NULL;

	if (object_path != NULL) {
		toplevel->kde_appmenu_object_path = strdup(object_path);
		if (toplevel->kde_appmenu_object_path == NULL) {
			wlr_log(WLR_ERROR, "failed to allocate memory for toplevel kde_appmenu_object_path");
			return;
		}
	}
	else toplevel->kde_appmenu_object_path = NULL;

	struct wl_resource *resource;
	wl_resource_for_each(resource, &toplevel->resources) {
		if (wl_resource_get_version(resource) >=
				ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_KDE_APPLICATION_MENU_SINCE_VERSION)
			zwlr_foreign_toplevel_handle_v1_send_kde_application_menu(
				resource,
				toplevel->kde_appmenu_service_name,
				toplevel->kde_appmenu_object_path);
	}

	toplevel_update_idle_source(toplevel);
}

static void send_output_to_resource(struct wl_resource *resource,
		struct wlr_output *output, bool enter) {
	struct wl_client *client = wl_resource_get_client(resource);
	struct wl_resource *output_resource;

	wl_resource_for_each(output_resource, &output->resources) {
		if (wl_resource_get_client(output_resource) == client) {
			if (enter) {
				zwlr_foreign_toplevel_handle_v1_send_output_enter(resource,
					output_resource);
			} else {
				zwlr_foreign_toplevel_handle_v1_send_output_leave(resource,
					output_resource);
			}
		}
	}
}

static void toplevel_send_output(struct wlr_foreign_toplevel_handle_v1 *toplevel,
		struct wlr_output *output, bool enter) {
	struct wl_resource *resource;
	wl_resource_for_each(resource, &toplevel->resources) {
		send_output_to_resource(resource, output, enter);
	}

	toplevel_update_idle_source(toplevel);
}

static void toplevel_handle_output_bind(struct wl_listener *listener,
		void *data) {
	struct wlr_foreign_toplevel_handle_v1_output *toplevel_output =
		wl_container_of(listener, toplevel_output, output_bind);
	struct wlr_output_event_bind *event = data;
	struct wl_client *client = wl_resource_get_client(event->resource);

	struct wl_resource *resource;
	wl_resource_for_each(resource, &toplevel_output->toplevel->resources) {
		if (wl_resource_get_client(resource) == client) {
			send_output_to_resource(resource, toplevel_output->output, true);
		}
	}

	toplevel_update_idle_source(toplevel_output->toplevel);
}

static void toplevel_handle_output_destroy(struct wl_listener *listener,
		void *data) {
	struct wlr_foreign_toplevel_handle_v1_output *toplevel_output =
		wl_container_of(listener, toplevel_output, output_destroy);
	wlr_foreign_toplevel_handle_v1_output_leave(toplevel_output->toplevel,
		toplevel_output->output);
}

void wlr_foreign_toplevel_handle_v1_output_enter(
		struct wlr_foreign_toplevel_handle_v1 *toplevel,
		struct wlr_output *output) {
	struct wlr_foreign_toplevel_handle_v1_output *toplevel_output;
	wl_list_for_each(toplevel_output, &toplevel->outputs, link) {
		if (toplevel_output->output == output) {
			return; // we have already sent output_enter event
		}
	}

	toplevel_output = calloc(1, sizeof(*toplevel_output));
	if (!toplevel_output) {
		wlr_log(WLR_ERROR, "failed to allocate memory for toplevel output");
		return;
	}

	toplevel_output->output = output;
	toplevel_output->toplevel = toplevel;
	wl_list_insert(&toplevel->outputs, &toplevel_output->link);

	toplevel_output->output_bind.notify = toplevel_handle_output_bind;
	wl_signal_add(&output->events.bind, &toplevel_output->output_bind);

	toplevel_output->output_destroy.notify = toplevel_handle_output_destroy;
	wl_signal_add(&output->events.destroy, &toplevel_output->output_destroy);

	toplevel_send_output(toplevel, output, true);
}

static void toplevel_output_destroy(
		struct wlr_foreign_toplevel_handle_v1_output *toplevel_output) {
	wl_list_remove(&toplevel_output->link);
	wl_list_remove(&toplevel_output->output_bind.link);
	wl_list_remove(&toplevel_output->output_destroy.link);
	free(toplevel_output);
}

void wlr_foreign_toplevel_handle_v1_output_leave(
		struct wlr_foreign_toplevel_handle_v1 *toplevel,
		struct wlr_output *output) {
	struct wlr_foreign_toplevel_handle_v1_output *toplevel_output_iterator;
	struct wlr_foreign_toplevel_handle_v1_output *toplevel_output = NULL;

	wl_list_for_each(toplevel_output_iterator, &toplevel->outputs, link) {
		if (toplevel_output_iterator->output == output) {
			toplevel_output = toplevel_output_iterator;
			break;
		}
	}

	if (toplevel_output) {
		toplevel_send_output(toplevel, output, false);
		toplevel_output_destroy(toplevel_output);
	} else {
		// XXX: log an error? crash?
	}
}

static bool fill_array_from_toplevel_state(struct wl_array *array,
		uint32_t state) {
	if (state & WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED) {
		uint32_t *index = wl_array_add(array, sizeof(uint32_t));
		if (index == NULL) {
			return false;
		}
		*index = ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED;
	}
	if (state & WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED) {
		uint32_t *index = wl_array_add(array, sizeof(uint32_t));
		if (index == NULL) {
			return false;
		}
		*index = ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED;
	}
	if (state & WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED) {
		uint32_t *index = wl_array_add(array, sizeof(uint32_t));
		if (index == NULL) {
			return false;
		}
		*index = ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED;
	}
	if (state & WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN) {
		uint32_t *index = wl_array_add(array, sizeof(uint32_t));
		if (index == NULL) {
			return false;
		}
		*index = ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN;
	}

	return true;
}

static void toplevel_send_state(struct wlr_foreign_toplevel_handle_v1 *toplevel) {
	struct wl_array states;
	wl_array_init(&states);
	bool r = fill_array_from_toplevel_state(&states, toplevel->state);
	if (!r) {
		struct wl_resource *resource;
		wl_resource_for_each(resource, &toplevel->resources) {
			wl_resource_post_no_memory(resource);
		}

		wl_array_release(&states);
		return;
	}

	struct wl_resource *resource;
	wl_resource_for_each(resource, &toplevel->resources) {
		zwlr_foreign_toplevel_handle_v1_send_state(resource, &states);
	}

	wl_array_release(&states);
	toplevel_update_idle_source(toplevel);
}

void wlr_foreign_toplevel_handle_v1_set_maximized(
		struct wlr_foreign_toplevel_handle_v1 *toplevel, bool maximized) {
	if (maximized == !!(toplevel->state &
			WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED)) {
		return;
	}
	if (maximized) {
		toplevel->state |= WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED;
	} else {
		toplevel->state &= ~WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED;
	}
	toplevel_send_state(toplevel);
}

void wlr_foreign_toplevel_handle_v1_set_minimized(
		struct wlr_foreign_toplevel_handle_v1 *toplevel, bool minimized) {
	if (minimized == !!(toplevel->state &
			WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED)) {
		return;
	}
	if (minimized) {
		toplevel->state |= WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED;
	} else {
		toplevel->state &= ~WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED;
	}
	toplevel_send_state(toplevel);
}

void wlr_foreign_toplevel_handle_v1_set_activated(
		struct wlr_foreign_toplevel_handle_v1 *toplevel, bool activated) {
	if (activated == !!(toplevel->state &
			WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED)) {
		return;
	}
	if (activated) {
		toplevel->state |= WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED;
	} else {
		toplevel->state &= ~WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED;
	}
	toplevel_send_state(toplevel);
}

void wlr_foreign_toplevel_handle_v1_set_fullscreen(
		struct wlr_foreign_toplevel_handle_v1 * toplevel, bool fullscreen) {
	if (fullscreen == !!(toplevel->state &
			WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN)) {
		return;
	}
	if (fullscreen) {
		toplevel->state |= WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN;
	} else {
		toplevel->state &= ~WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN;
	}
	toplevel_send_state(toplevel);
}

static void toplevel_resource_send_parent(
		struct wl_resource *toplevel_resource,
		struct wlr_foreign_toplevel_handle_v1 *parent) {
	if (wl_resource_get_version(toplevel_resource) <
			ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_PARENT_SINCE_VERSION) {
		return;
	}
	struct wl_client *client = wl_resource_get_client(toplevel_resource);
	struct wl_resource *parent_resource = NULL;
	if (parent) {
		parent_resource = wl_resource_find_for_client(&parent->resources, client);
		if (!parent_resource) {
			/* don't send an event if this client destroyed the parent handle */
			return;
		}
	}
	zwlr_foreign_toplevel_handle_v1_send_parent(toplevel_resource,
		parent_resource);
}

void wlr_foreign_toplevel_handle_v1_set_parent(
		struct wlr_foreign_toplevel_handle_v1 *toplevel,
		struct wlr_foreign_toplevel_handle_v1 *parent) {
	if (parent == toplevel->parent) {
		/* only send parent event to the clients if there was a change */
		return;
	}
	struct wl_resource *toplevel_resource, *tmp;
	wl_resource_for_each_safe(toplevel_resource, tmp, &toplevel->resources) {
		toplevel_resource_send_parent(toplevel_resource, parent);
	}
	toplevel->parent = parent;
	toplevel_update_idle_source(toplevel);
}

void wlr_foreign_toplevel_handle_v1_destroy(
		struct wlr_foreign_toplevel_handle_v1 *toplevel) {
	if (!toplevel) {
		return;
	}

	wl_signal_emit_mutable(&toplevel->events.destroy, toplevel);

	struct wl_resource *resource, *tmp;
	wl_resource_for_each_safe(resource, tmp, &toplevel->resources) {
		zwlr_foreign_toplevel_handle_v1_send_closed(resource);
		wl_resource_set_user_data(resource, NULL);
		wl_list_remove(wl_resource_get_link(resource));
		wl_list_init(wl_resource_get_link(resource));
	}

	struct wlr_foreign_toplevel_handle_v1_output *toplevel_output, *tmp2;
	wl_list_for_each_safe(toplevel_output, tmp2, &toplevel->outputs, link) {
		toplevel_output_destroy(toplevel_output);
	}

	if (toplevel->idle_source) {
		wl_event_source_remove(toplevel->idle_source);
	}

	wl_list_remove(&toplevel->link);

	/* need to ensure no other toplevels hold a pointer to this one as
	 * a parent, so that a later call to foreign_toplevel_manager_bind()
	 * will not result in a segfault */
	struct wlr_foreign_toplevel_handle_v1 *tl, *tmp3;
	wl_list_for_each_safe(tl, tmp3, &toplevel->manager->toplevels, link) {
		if (tl->parent == toplevel) {
			/* Note: we send a parent signal to all clients in this case;
			 * the caller should first destroy the child handles if it
			 * wishes to avoid this behavior. */
			wlr_foreign_toplevel_handle_v1_set_parent(tl, NULL);
		}
	}

	free(toplevel->gtk_shell1_application_id);
	free(toplevel->gtk_shell1_app_menu_path);
	free(toplevel->gtk_shell1_menubar_path);
	free(toplevel->gtk_shell1_window_object_path);
	free(toplevel->gtk_shell1_application_object_path);
	free(toplevel->gtk_shell1_unique_bus_name);
	free(toplevel->kde_appmenu_service_name);
	free(toplevel->kde_appmenu_object_path);
	free(toplevel->title);
	free(toplevel->app_id);
	free(toplevel);
}

static void foreign_toplevel_resource_destroy(struct wl_resource *resource) {
	wl_list_remove(wl_resource_get_link(resource));
}

static struct wl_resource *create_toplevel_resource_for_resource(
		struct wlr_foreign_toplevel_handle_v1 *toplevel,
		struct wl_resource *manager_resource) {
	struct wl_client *client = wl_resource_get_client(manager_resource);
	struct wl_resource *resource = wl_resource_create(client,
			&zwlr_foreign_toplevel_handle_v1_interface,
			wl_resource_get_version(manager_resource), 0);
	if (!resource) {
		wl_client_post_no_memory(client);
		return NULL;
	}

	wl_resource_set_implementation(resource, &toplevel_handle_impl, toplevel,
		foreign_toplevel_resource_destroy);

	wl_list_insert(&toplevel->resources, wl_resource_get_link(resource));
	zwlr_foreign_toplevel_manager_v1_send_toplevel(manager_resource, resource);
	return resource;
}

struct wlr_foreign_toplevel_handle_v1 *
wlr_foreign_toplevel_handle_v1_create(
		struct wlr_foreign_toplevel_manager_v1 *manager) {
	struct wlr_foreign_toplevel_handle_v1 *toplevel = calloc(1, sizeof(*toplevel));
	if (!toplevel) {
		return NULL;
	}

	wl_list_insert(&manager->toplevels, &toplevel->link);
	toplevel->manager = manager;

	wl_list_init(&toplevel->resources);
	wl_list_init(&toplevel->outputs);

	wl_signal_init(&toplevel->events.request_maximize);
	wl_signal_init(&toplevel->events.request_minimize);
	wl_signal_init(&toplevel->events.request_activate);
	wl_signal_init(&toplevel->events.request_fullscreen);
	wl_signal_init(&toplevel->events.request_close);
	wl_signal_init(&toplevel->events.set_rectangle);
	wl_signal_init(&toplevel->events.destroy);

	struct wl_resource *manager_resource, *tmp;
	wl_resource_for_each_safe(manager_resource, tmp, &manager->resources) {
		create_toplevel_resource_for_resource(toplevel, manager_resource);
	}

	return toplevel;
}

static const struct zwlr_foreign_toplevel_manager_v1_interface
	foreign_toplevel_manager_impl;

static void foreign_toplevel_manager_handle_stop(struct wl_client *client,
		struct wl_resource *resource) {
	assert(wl_resource_instance_of(resource,
		&zwlr_foreign_toplevel_manager_v1_interface,
		&foreign_toplevel_manager_impl));

	zwlr_foreign_toplevel_manager_v1_send_finished(resource);
	wl_resource_destroy(resource);
}

static const struct zwlr_foreign_toplevel_manager_v1_interface
		foreign_toplevel_manager_impl = {
	.stop = foreign_toplevel_manager_handle_stop
};

static void foreign_toplevel_manager_resource_destroy(
		struct wl_resource *resource) {
	wl_list_remove(wl_resource_get_link(resource));
}

static void toplevel_send_details_to_toplevel_resource(
		struct wlr_foreign_toplevel_handle_v1 *toplevel,
		struct wl_resource *resource) {
	if (toplevel->title) {
		zwlr_foreign_toplevel_handle_v1_send_title(resource, toplevel->title);
	}
	if (toplevel->app_id) {
		zwlr_foreign_toplevel_handle_v1_send_app_id(resource, toplevel->app_id);
	}
	if (toplevel_has_gtk_shell1_dbus_properties(toplevel) &&
			wl_resource_get_version(resource) >=
			ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_GTK_SHELL1_SURFACE_DBUS_PROPERTIES_SINCE_VERSION) {
		zwlr_foreign_toplevel_handle_v1_send_gtk_shell1_surface_dbus_properties(
			resource,
			toplevel->gtk_shell1_application_id,
			toplevel->gtk_shell1_app_menu_path,
			toplevel->gtk_shell1_menubar_path,
			toplevel->gtk_shell1_window_object_path,
			toplevel->gtk_shell1_application_object_path,
			toplevel->gtk_shell1_unique_bus_name);
	}
	if ( (toplevel->kde_appmenu_service_name != NULL || toplevel->kde_appmenu_object_path != NULL) &&
			wl_resource_get_version(resource) >=
			ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_KDE_APPLICATION_MENU_SINCE_VERSION) {
		zwlr_foreign_toplevel_handle_v1_send_kde_application_menu(
			resource,
			toplevel->kde_appmenu_service_name,
			toplevel->kde_appmenu_object_path);
	}

	struct wlr_foreign_toplevel_handle_v1_output *output;
	wl_list_for_each(output, &toplevel->outputs, link) {
		send_output_to_resource(resource, output->output, true);
	}

	struct wl_array states;
	wl_array_init(&states);
	bool r = fill_array_from_toplevel_state(&states, toplevel->state);
	if (!r) {
		wl_resource_post_no_memory(resource);
		wl_array_release(&states);
		return;
	}

	zwlr_foreign_toplevel_handle_v1_send_state(resource, &states);
	wl_array_release(&states);

	toplevel_resource_send_parent(resource, toplevel->parent);

	zwlr_foreign_toplevel_handle_v1_send_done(resource);
}

static void foreign_toplevel_manager_bind(struct wl_client *client, void *data,
		uint32_t version, uint32_t id) {
	struct wlr_foreign_toplevel_manager_v1 *manager = data;
	struct wl_resource *resource = wl_resource_create(client,
			&zwlr_foreign_toplevel_manager_v1_interface, version, id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(resource, &foreign_toplevel_manager_impl,
			manager, foreign_toplevel_manager_resource_destroy);

	wl_list_insert(&manager->resources, wl_resource_get_link(resource));

	struct wlr_foreign_toplevel_handle_v1 *toplevel, *tmp;
	/* First loop: create a handle for all toplevels for all clients.
	 * Separation into two loops avoid the case where a child handle
	 * is created before a parent handle, so the parent relationship
	 * could not be sent to a client. */
	wl_list_for_each_safe(toplevel, tmp, &manager->toplevels, link) {
		create_toplevel_resource_for_resource(toplevel, resource);
	}
	/* Second loop: send details about each toplevel. */
	wl_list_for_each_safe(toplevel, tmp, &manager->toplevels, link) {
		struct wl_resource *toplevel_resource =
			wl_resource_find_for_client(&toplevel->resources, client);
		toplevel_send_details_to_toplevel_resource(toplevel,
			toplevel_resource);
	}
}

static void handle_display_destroy(struct wl_listener *listener, void *data) {
	struct wlr_foreign_toplevel_manager_v1 *manager =
		wl_container_of(listener, manager, display_destroy);
	wl_signal_emit_mutable(&manager->events.destroy, manager);
	wl_list_remove(&manager->display_destroy.link);
	wl_global_destroy(manager->global);
	free(manager);
}

struct wlr_foreign_toplevel_manager_v1 *wlr_foreign_toplevel_manager_v1_create(
		struct wl_display *display) {
	struct wlr_foreign_toplevel_manager_v1 *manager = calloc(1, sizeof(*manager));
	if (!manager) {
		return NULL;
	}

	manager->event_loop = wl_display_get_event_loop(display);
	manager->global = wl_global_create(display,
			&zwlr_foreign_toplevel_manager_v1_interface,
			FOREIGN_TOPLEVEL_MANAGEMENT_V1_VERSION, manager,
			foreign_toplevel_manager_bind);
	if (!manager->global) {
		free(manager);
		return NULL;
	}

	wl_signal_init(&manager->events.destroy);
	wl_list_init(&manager->resources);
	wl_list_init(&manager->toplevels);

	manager->display_destroy.notify = handle_display_destroy;
	wl_display_add_destroy_listener(display, &manager->display_destroy);

	return manager;
}
