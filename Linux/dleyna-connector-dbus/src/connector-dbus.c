/*
 * dLeyna
 *
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Regis Merlino <regis.merlino@intel.com>
 *
 */

#include <gio/gio.h>
#include <string.h>

#include <libdleyna/core/connector.h>
#include <libdleyna/core/error.h>
#include <libdleyna/core/log.h>

typedef struct dleyna_dbus_object_t_ dleyna_dbus_object_t;
struct dleyna_dbus_object_t_ {
	guint id;
	gchar *root_path;
	const dleyna_connector_dispatch_cb_t *dispatch_table;
	guint dispatch_table_size;
	dleyna_connector_interface_filter_cb_t filter_cb;
};

typedef struct dleyna_dbus_call_info_t_ dleyna_dbus_call_info_t;
struct dleyna_dbus_call_info_t_ {
	dleyna_dbus_object_t *object;
	guint interface_index;
};

typedef struct dleyna_dbus_context_t_ dleyna_dbus_context_t;
struct dleyna_dbus_context_t_ {
	GHashTable *objects;
	GHashTable *clients;
	GDBusNodeInfo *root_node_info;
	GDBusNodeInfo *server_node_info;
	guint owner_id;
	GDBusConnection *connection;
	dleyna_connector_connected_cb_t connected_cb;
	dleyna_connector_disconnected_cb_t disconnected_cb;
	dleyna_connector_client_lost_cb_t client_lost_cb;
};

static dleyna_dbus_context_t g_context;

#define DLEYNA_SERVICE "com.intel.dleyna"

static const GDBusErrorEntry g_error_entries[] = {
	{ DLEYNA_ERROR_BAD_PATH, DLEYNA_SERVICE".BadPath" },
	{ DLEYNA_ERROR_OBJECT_NOT_FOUND, DLEYNA_SERVICE".ObjectNotFound" },
	{ DLEYNA_ERROR_BAD_QUERY, DLEYNA_SERVICE".BadQuery" },
	{ DLEYNA_ERROR_OPERATION_FAILED, DLEYNA_SERVICE".OperationFailed" },
	{ DLEYNA_ERROR_BAD_RESULT, DLEYNA_SERVICE".BadResult" },
	{ DLEYNA_ERROR_UNKNOWN_INTERFACE, DLEYNA_SERVICE".UnknownInterface" },
	{ DLEYNA_ERROR_UNKNOWN_PROPERTY, DLEYNA_SERVICE".UnknownProperty" },
	{ DLEYNA_ERROR_DEVICE_NOT_FOUND, DLEYNA_SERVICE".DeviceNotFound" },
	{ DLEYNA_ERROR_DIED, DLEYNA_SERVICE".Died" },
	{ DLEYNA_ERROR_CANCELLED, DLEYNA_SERVICE".Cancelled" },
	{ DLEYNA_ERROR_NOT_SUPPORTED, DLEYNA_SERVICE".NotSupported" },
	{ DLEYNA_ERROR_LOST_OBJECT, DLEYNA_SERVICE".LostObject" },
	{ DLEYNA_ERROR_BAD_MIME, DLEYNA_SERVICE".BadMime" },
	{ DLEYNA_ERROR_HOST_FAILED, DLEYNA_SERVICE".HostFailed" },
	{ DLEYNA_ERROR_IO, DLEYNA_SERVICE".IO" }
};

const dleyna_connector_t *dleyna_connector_get_interface(void);

static void prv_object_method_call(GDBusConnection *conn,
				   const gchar *sender,
				   const gchar *object,
				   const gchar *interface,
				   const gchar *method,
				   GVariant *parameters,
				   GDBusMethodInvocation *invocation,
				   gpointer user_data);

static const GDBusInterfaceVTable g_object_vtable = {
	prv_object_method_call,
	NULL,
	NULL
};

static gchar **prv_subtree_enumerate(GDBusConnection *connection,
				     const gchar *sender,
				     const gchar *object_path,
				     gpointer user_data);

static GDBusInterfaceInfo **prv_subtree_introspect(
	GDBusConnection *connection,
	const gchar *sender,
	const gchar *object_path,
	const gchar *node,
	gpointer user_data);

static const GDBusInterfaceVTable *prv_subtree_dispatch(
	GDBusConnection *connection,
	const gchar *sender,
	const gchar *object_path,
	const gchar *interface_name,
	const gchar *node,
	gpointer *out_user_data,
	gpointer user_data);


static const GDBusSubtreeVTable g_subtree_vtable = {
	prv_subtree_enumerate,
	prv_subtree_introspect,
	prv_subtree_dispatch
};

static void prv_subtree_method_call(GDBusConnection *conn,
				    const gchar *sender,
				    const gchar *object_path,
				    const gchar *interface,
				    const gchar *method,
				    GVariant *parameters,
				    GDBusMethodInvocation *invocation,
				    gpointer user_data);

static const GDBusInterfaceVTable g_subtree_interface_vtable = {
	prv_subtree_method_call,
	NULL,
	NULL
};

static void prv_connector_init_error_domain(GQuark error_quark)
{
	guint index = sizeof(g_error_entries) / sizeof(const GDBusErrorEntry);

	while (index) {
		index--;
		g_dbus_error_register_error(
				error_quark,
				g_error_entries[index].error_code,
				g_error_entries[index].dbus_error_name);
	}
}

static void prv_free_dbus_object(gpointer data)
{
	dleyna_dbus_object_t *object = data;

	g_free(object->root_path);

	g_free(object);
}

static gboolean prv_connector_initialize(const gchar *server_info,
					 const gchar *root_info,
					 GQuark error_quark,
					 gpointer user_data)
{
	gboolean success = TRUE;

	DLEYNA_LOG_DEBUG("Enter");

	memset(&g_context, 0, sizeof(g_context));

	g_context.objects = g_hash_table_new_full(g_direct_hash, g_direct_equal,
						  g_free, prv_free_dbus_object);
	g_context.clients = g_hash_table_new_full(g_str_hash, g_str_equal,
						  g_free, NULL);

	g_context.root_node_info = g_dbus_node_info_new_for_xml(root_info,
								NULL);
	if (!g_context.root_node_info) {
		success = FALSE;
		goto out;
	}

	g_context.server_node_info = g_dbus_node_info_new_for_xml(server_info,
								  NULL);
	if (!g_context.server_node_info) {
		success = FALSE;
		goto out;
	}

	prv_connector_init_error_domain(error_quark);

out:
	DLEYNA_LOG_DEBUG("Exit");

	return success;
}

static void prv_connector_disconnect(void)
{
	if (g_context.owner_id) {
		g_bus_unown_name(g_context.owner_id);
		g_context.owner_id = 0;
	}

}

static void prv_connector_shutdown(void)
{
	DLEYNA_LOG_DEBUG("Enter");

	if (g_context.objects)
		g_hash_table_unref(g_context.objects);

	if (g_context.clients)
		g_hash_table_unref(g_context.clients);

	prv_connector_disconnect();

	if (g_context.connection)
		g_object_unref(g_context.connection);

	if (g_context.server_node_info)
		g_dbus_node_info_unref(g_context.server_node_info);

	if (g_context.root_node_info)
		g_dbus_node_info_unref(g_context.root_node_info);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_bus_acquired(GDBusConnection *connection, const gchar *name,
			     gpointer user_data)
{
	g_context.connection = connection;
	g_context.connected_cb((dleyna_connector_id_t)connection);
}

static void prv_name_lost(GDBusConnection *connection, const gchar *name,
			  gpointer user_data)
{
	g_context.disconnected_cb((dleyna_connector_id_t)connection);
}

static void prv_connector_connect(
			const gchar *server_name,
			dleyna_connector_connected_cb_t connected_cb,
			dleyna_connector_disconnected_cb_t disconnected_cb)
{
	DLEYNA_LOG_DEBUG("Enter");

	g_context.connected_cb = connected_cb;
	g_context.disconnected_cb = disconnected_cb;

	g_context.owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
					    server_name,
					    G_BUS_NAME_OWNER_FLAGS_NONE,
					    prv_bus_acquired, NULL,
					    prv_name_lost, NULL, NULL);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_connector_unwatch_client(const gchar *client_name)
{
	guint client_id;

	DLEYNA_LOG_DEBUG("Enter");

	client_id = GPOINTER_TO_UINT(g_hash_table_lookup(g_context.clients,
							 client_name));
	(void) g_hash_table_remove(g_context.clients, client_name);

	g_bus_unwatch_name(client_id);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_lost_client(GDBusConnection *connection, const gchar *name,
			    gpointer user_data)
{
	g_context.client_lost_cb(name);
	prv_connector_unwatch_client(name);
}

static gboolean prv_connector_watch_client(const gchar *client_name)
{
	guint watch_id;
	gboolean added = TRUE;

	DLEYNA_LOG_DEBUG("Enter");

	if (g_hash_table_lookup(g_context.clients, client_name)) {
		added = FALSE;
		goto out;
	}

	watch_id = g_bus_watch_name(G_BUS_TYPE_SESSION, client_name,
				      G_BUS_NAME_WATCHER_FLAGS_NONE,
				      NULL, prv_lost_client, NULL,
				      NULL);
	g_hash_table_insert(g_context.clients, g_strdup(client_name),
			    GUINT_TO_POINTER(watch_id));

out:
	DLEYNA_LOG_DEBUG("Exit");

	return added;
}

static void prv_connector_set_client_lost_cb(
				dleyna_connector_client_lost_cb_t lost_cb)
{
	g_context.client_lost_cb = lost_cb;
}

static GDBusInterfaceInfo *prv_find_interface_info(gboolean root,
						   const gchar *interface_name)
{
	GDBusNodeInfo *node;
	GDBusInterfaceInfo **interface;

	node = (root) ? g_context.root_node_info : g_context.server_node_info;

	interface = node->interfaces;
	while (*interface) {
		if (!strcmp(interface_name, (*interface)->name))
			break;
		interface++;
	}

	return  *interface;
}

static void prv_object_method_call(GDBusConnection *conn,
				   const gchar *sender,
				   const gchar *object_path,
				   const gchar *interface,
				   const gchar *method,
				   GVariant *parameters,
				   GDBusMethodInvocation *invocation,
				   gpointer user_data)
{
	dleyna_dbus_object_t *object = user_data;

	object->dispatch_table[0]((dleyna_connector_id_t)conn,
				   sender,
				   object_path,
				   interface,
				   method,
				   parameters,
				   (dleyna_connector_msg_id_t)invocation);
}

static void prv_subtree_method_call(GDBusConnection *conn,
				   const gchar *sender,
				   const gchar *object_path,
				   const gchar *interface,
				   const gchar *method,
				   GVariant *parameters,
				   GDBusMethodInvocation *invocation,
				   gpointer user_data)
{
	dleyna_dbus_call_info_t *call_info = user_data;
	dleyna_connector_dispatch_cb_t callback =
		call_info->object->dispatch_table[call_info->interface_index];

	callback((dleyna_connector_id_t)conn, sender, object_path,
		 interface, method, parameters,
		 (dleyna_connector_msg_id_t)invocation);

	g_free(call_info);
}

static guint prv_connector_publish_object(
			dleyna_connector_id_t connection,
			const gchar *object_path,
			gboolean root,
			const gchar* interface_name,
			const dleyna_connector_dispatch_cb_t *cb_table_1)
{
	guint object_id;
	GDBusInterfaceInfo *info;
	dleyna_dbus_object_t *object;
	guint *object_key;

	DLEYNA_LOG_DEBUG("Enter, path = <%s>", object_path);

	object = g_new0(dleyna_dbus_object_t, 1);

	info = prv_find_interface_info(root, interface_name);
	object_id = g_dbus_connection_register_object(
						(GDBusConnection *)connection,
						object_path,
						info,
						&g_object_vtable,
						object, NULL, NULL);
	if (object_id) {
		object->id = object_id;
		object->dispatch_table = cb_table_1;
		object->dispatch_table_size = 1;

		object_key = g_new(guint, 1);
		*object_key = object_id;
		g_hash_table_insert(g_context.objects, object_key, object);
	} else {
		g_free(object);
	}

	DLEYNA_LOG_DEBUG("Exit, object_id = %u", object_id);

	return object_id;
}

static gchar **prv_subtree_enumerate(GDBusConnection *connection,
				     const gchar *sender,
				     const gchar *object_path,
				     gpointer user_data)
{
	return g_malloc0(sizeof(gchar *));
}

static GDBusInterfaceInfo **prv_subtree_introspect(
	GDBusConnection *connection,
	const gchar *sender,
	const gchar *object_path,
	const gchar *node,
	gpointer user_data)
{
	GDBusInterfaceInfo **retval;
	GDBusInterfaceInfo *info;
	unsigned int i;
	unsigned count = 0;
	const gchar *iface_name;
	dleyna_dbus_object_t *object = user_data;

	retval = g_new0(GDBusInterfaceInfo *, object->dispatch_table_size + 1);

	for (i = 0; i < object->dispatch_table_size; i++) {
		iface_name = g_context.server_node_info->interfaces[i]->name;
		if (object->filter_cb(object_path, node, iface_name)) {
			info = g_context.server_node_info->interfaces[i];
			retval[count++] =  g_dbus_interface_info_ref(info);
		}
	}

	return retval;
}

static const GDBusInterfaceVTable *prv_subtree_dispatch(
	GDBusConnection *connection,
	const gchar *sender,
	const gchar *object_path,
	const gchar *interface_name,
	const gchar *node,
	gpointer *out_user_data,
	gpointer user_data)
{
	const GDBusInterfaceVTable *retval = NULL;
	dleyna_dbus_object_t *object = user_data;
	unsigned int i;
	GDBusInterfaceInfo *info;
	dleyna_dbus_call_info_t *out_call_info;

	for (i = 0; i < object->dispatch_table_size; i++) {
		info = g_context.server_node_info->interfaces[i];
		if (!strcmp(interface_name, info->name))
			break;
	}

	out_call_info = g_new(dleyna_dbus_call_info_t, 1);
	out_call_info->object = object;
	out_call_info->interface_index = i;
	*out_user_data = out_call_info;

	retval = &g_subtree_interface_vtable;

	return retval;
}

static guint prv_connector_publish_subtree(
				dleyna_connector_id_t connection,
				const gchar *object_path,
				const dleyna_connector_dispatch_cb_t *cb_table,
				guint cb_table_size,
				dleyna_connector_interface_filter_cb_t cb)
{
	guint flags;
	guint object_id;
	dleyna_dbus_object_t *object;
	guint *object_key;

	DLEYNA_LOG_DEBUG("Enter, path = <%s>", object_path);

	object = g_new0(dleyna_dbus_object_t, 1);

	flags = G_DBUS_SUBTREE_FLAGS_DISPATCH_TO_UNENUMERATED_NODES;
	object_id = g_dbus_connection_register_subtree(
						(GDBusConnection *)connection,
						object_path,
						&g_subtree_vtable,
						flags,
						object,
						NULL, NULL);

	if (object_id) {
		object->id = object_id;
		object->root_path = g_strdup(object_path);
		object->dispatch_table = cb_table;
		object->dispatch_table_size = cb_table_size;
		object->filter_cb = cb;

		object_key = g_new(guint, 1);
		*object_key = object_id;
		g_hash_table_insert(g_context.objects, object_key, object);
	} else {
		g_free(object);
	}

	DLEYNA_LOG_DEBUG("Exit, object_id = %u", object_id);

	return object_id;
}

static void prv_connector_unpublish_object(dleyna_connector_id_t connection,
					   guint object_id)
{
	DLEYNA_LOG_DEBUG("Enter, object_id = %u", object_id);

	g_dbus_connection_unregister_object((GDBusConnection *)connection,
					    object_id);

	(void) g_hash_table_remove(g_context.objects, &object_id);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_connector_unpublish_subtree(dleyna_connector_id_t connection,
					    guint object_id)
{
	DLEYNA_LOG_DEBUG("Enter, object_id = %u", object_id);

	g_dbus_connection_unregister_subtree((GDBusConnection *)connection,
					     object_id);

	(void) g_hash_table_remove(g_context.objects, &object_id);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_connector_return_response(dleyna_connector_msg_id_t message_id,
					  GVariant *parameters)
{
	g_dbus_method_invocation_return_value(
					(GDBusMethodInvocation *)message_id,
					parameters);
}

static void prv_connector_return_error(dleyna_connector_msg_id_t message_id,
				       const GError *error)
{
	g_dbus_method_invocation_return_gerror(
					(GDBusMethodInvocation *)message_id,
					error);
}

static gboolean prv_connector_notify(dleyna_connector_id_t connection,
				     const gchar *object_path,
				     const gchar *interface_name,
				     const gchar *notification_name,
				     GVariant *parameters,
				     GError **error)
{
	return g_dbus_connection_emit_signal((GDBusConnection *)connection,
					     NULL,
					     object_path,
					     interface_name,
					     notification_name,
					     parameters,
					     NULL);
}

static const dleyna_connector_t g_dbus_connector = {
	prv_connector_initialize,
	prv_connector_shutdown,
	prv_connector_connect,
	prv_connector_disconnect,
	prv_connector_watch_client,
	prv_connector_unwatch_client,
	prv_connector_set_client_lost_cb,
	prv_connector_publish_object,
	prv_connector_publish_subtree,
	prv_connector_unpublish_object,
	prv_connector_unpublish_subtree,
	prv_connector_return_response,
	prv_connector_return_error,
	prv_connector_notify,
};

const dleyna_connector_t *dleyna_connector_get_interface(void)
{
	return &g_dbus_connector;
}
