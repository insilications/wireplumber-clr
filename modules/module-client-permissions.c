/* WirePlumber
 *
 * Copyright © 2019 Collabora Ltd.
 *    @author George Kiagiadakis <george.kiagiadakis@collabora.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <wp/wp.h>
#include <pipewire/pipewire.h>

static void
client_added (WpCore * core, WpProxyClient *client, gpointer data)
{
  g_autoptr (WpProperties) properties = NULL;
  const char *access;
  guint32 id = wp_proxy_get_global_id (WP_PROXY (client));

  g_debug ("Client added: %d", id);

  properties = wp_proxy_client_get_properties (client);
  access = wp_properties_get (properties, PW_KEY_ACCESS);

  if (!g_strcmp0 (access, "flatpak") || !g_strcmp0 (access, "restricted")) {
    g_debug ("Granting full access to client %d", id);
    wp_proxy_client_update_permissions (client, 1, -1, PW_PERM_RWX);
  }
}

void
wireplumber__module_init (WpModule * module, WpCore * core, GVariant * args)
{
  wp_core_set_default_proxy_features (core, WP_TYPE_PROXY_CLIENT,
      WP_PROXY_FEATURE_PW_PROXY | WP_PROXY_FEATURE_INFO);

  g_signal_connect(core, "remote-global-added::client",
      (GCallback) client_added, NULL);
}
