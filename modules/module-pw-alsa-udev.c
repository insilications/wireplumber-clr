/* WirePlumber
 *
 * Copyright © 2019 Collabora Ltd.
 *    @author George Kiagiadakis <george.kiagiadakis@collabora.com>
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * module-pw-alsa-udev provides alsa device detection through pipewire
 * and automatically creates endpoints for all alsa device nodes that appear
 */

#include <wp/wp.h>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

struct impl
{
  WpModule *module;
  WpRemotePipewire *remote_pipewire;
};

static void
on_endpoint_created(GObject *initable, GAsyncResult *res, gpointer d)
{
  g_autoptr (WpEndpoint) endpoint = NULL;
  guint global_id = 0;

  /* Get the endpoint */
  endpoint = wp_endpoint_new_finish(initable, res, NULL);
  if (!endpoint)
    return;

  /* Get the endpoint global id */
  g_object_get (endpoint, "global-id", &global_id, NULL);
  g_debug ("Created alsa endpoint for global id %d", global_id);

  /* Register the endpoint */
  wp_endpoint_register (endpoint);
}

static void
on_node_added(WpRemotePipewire *rp, guint id, guint parent_id, gconstpointer p,
    gpointer d)
{
  struct impl *impl = d;
  const struct spa_dict *props = p;
  g_autoptr (WpCore) core = wp_module_get_core (impl->module);
  const gchar *name, *media_class;
  GVariantBuilder b;
  g_autoptr (GVariant) endpoint_props = NULL;

  /* Make sure the node has properties */
  g_return_if_fail(props);

  /* Get the name and media_class */
  name = spa_dict_lookup(props, "node.name");
  media_class = spa_dict_lookup(props, "media.class");

  /* Make sure the media class is non-dsp audio */
  if (!g_str_has_prefix (media_class, "Audio/"))
    return;
  if (g_str_has_prefix (media_class, "Audio/DSP"))
    return;

  /* Set the properties */
  g_variant_builder_init (&b, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&b, "{sv}",
      "name", g_variant_new_take_string (g_strdup_printf (
          "Endpoint %u: %s", id, name)));
  g_variant_builder_add (&b, "{sv}",
      "media-class", g_variant_new_string (media_class));
  g_variant_builder_add (&b, "{sv}",
      "global-id", g_variant_new_uint32 (id));
  endpoint_props = g_variant_builder_end (&b);

  /* Create the endpoint async */
  wp_factory_make_async (core, "pw-audio-softdsp-endpoint", WP_TYPE_ENDPOINT,
      endpoint_props, on_endpoint_created, impl);
}

static void
module_destroy (gpointer data)
{
  struct impl *impl = data;

  /* Set to NULL module and remote pipewire as we don't own the reference */
  impl->module = NULL;
  impl->remote_pipewire = NULL;

  /* Clean up */
  g_slice_free (struct impl, impl);
}

void
wireplumber__module_init (WpModule * module, WpCore * core, GVariant * args)
{
  struct impl *impl;
  WpRemotePipewire *rp;

  /* Make sure the remote pipewire is valid */
  rp = wp_core_get_global (core, WP_GLOBAL_REMOTE_PIPEWIRE);
  if (!rp) {
    g_critical ("module-pw-alsa-udev cannot be loaded without a registered "
        "WpRemotePipewire object");
    return;
  }

  /* Create the module data */
  impl = g_slice_new0(struct impl);
  impl->module = module;
  impl->remote_pipewire = rp;

  /* Set destroy callback for impl */
  wp_module_set_destroy_callback (module, module_destroy, impl);

  /* Register the global addded callbacks */
  g_signal_connect(rp, "global-added::node", (GCallback)on_node_added, impl);
}
