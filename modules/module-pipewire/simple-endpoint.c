/* WirePlumber
 *
 * Copyright © 2019 Collabora Ltd.
 *    @author George Kiagiadakis <george.kiagiadakis@collabora.com>
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * The simple endpoint is a WpEndpoint implementation that represents
 * all ports of a single direction of a single pipewire node.
 * It can be used to create an Endpoint for a client node or for any
 * other arbitrary node that does not need any kind of internal management.
 */

#include <wp/wp.h>
#include <pipewire/pipewire.h>
#include <spa/pod/parser.h>
#include <spa/param/props.h>

#include "port.h"

struct _WpPipewireSimpleEndpoint
{
  WpEndpoint parent;

  /* Node */
  struct pw_node_proxy *node;
  struct spa_hook proxy_listener;
  struct spa_hook node_proxy_listener;

  /* Info */
  struct pw_node_info *info;

  /* controls cache */
  gfloat volume;
  gboolean mute;
};

enum {
  PROP_0,
  PROP_NODE_PROXY,
};

enum {
  CONTROL_VOLUME = 0,
  CONTROL_MUTE,
};

G_DECLARE_FINAL_TYPE (WpPipewireSimpleEndpoint,
    simple_endpoint, WP_PIPEWIRE, SIMPLE_ENDPOINT, WpEndpoint)

G_DEFINE_TYPE (WpPipewireSimpleEndpoint, simple_endpoint, WP_TYPE_ENDPOINT)

static void
node_proxy_param (void *object, int seq, uint32_t id,
    uint32_t index, uint32_t next, const struct spa_pod *param)
{
  WpPipewireSimpleEndpoint *self = WP_PIPEWIRE_SIMPLE_ENDPOINT (object);

  switch (id) {
    case SPA_PARAM_Props:
    {
      struct spa_pod_prop *prop;
      struct spa_pod_object *obj = (struct spa_pod_object *) param;
      float volume = self->volume;
      bool mute = self->mute;

      SPA_POD_OBJECT_FOREACH(obj, prop) {
        switch (prop->key) {
        case SPA_PROP_volume:
          spa_pod_get_float(&prop->value, &volume);
          break;
        case SPA_PROP_mute:
          spa_pod_get_bool(&prop->value, &mute);
          break;
        default:
          break;
        }
      }

      g_debug ("WpEndpoint:%p param event, vol:(%lf -> %f) mute:(%d -> %d)",
          self, self->volume, volume, self->mute, mute);

      if (self->volume != volume) {
        self->volume = volume;
        wp_endpoint_notify_control_value (WP_ENDPOINT (self), CONTROL_VOLUME);
      }
      if (self->mute != mute) {
        self->mute = mute;
        wp_endpoint_notify_control_value (WP_ENDPOINT (self), CONTROL_MUTE);
      }

      break;
    }
    default:
      break;
  }
}

static void node_proxy_info(void *object, const struct pw_node_info *info)
{
  WpPipewireSimpleEndpoint *self = WP_PIPEWIRE_SIMPLE_ENDPOINT (object);
  self->info = pw_node_info_update(self->info, info);
}

static const struct pw_node_proxy_events node_node_proxy_events = {
  PW_VERSION_NODE_PROXY_EVENTS,
  .info = node_proxy_info,
  .param = node_proxy_param,
};

static void
simple_endpoint_init (WpPipewireSimpleEndpoint * self)
{
}

static void
node_proxy_destroy (void *data)
{
  WpPipewireSimpleEndpoint *self = WP_PIPEWIRE_SIMPLE_ENDPOINT (data);
  self->node = NULL;

  wp_endpoint_unregister (WP_ENDPOINT (self));
}

static const struct pw_proxy_events node_proxy_events = {
  PW_VERSION_PROXY_EVENTS,
  .destroy = node_proxy_destroy,
};


static void
simple_endpoint_constructed (GObject * object)
{
  WpPipewireSimpleEndpoint *self = WP_PIPEWIRE_SIMPLE_ENDPOINT (object);
  GVariantDict d;
  uint32_t ids[1] = { SPA_PARAM_Props };
  uint32_t n_ids = 1;

  pw_proxy_add_listener ((struct pw_proxy *) self->node, &self->proxy_listener,
      &node_proxy_events, self);

  pw_node_proxy_add_listener (self->node, &self->node_proxy_listener,
      &node_node_proxy_events, self);
  pw_node_proxy_subscribe_params (self->node, ids, n_ids);

  g_variant_dict_init (&d, NULL);
  g_variant_dict_insert (&d, "id", "u", 0);
  g_variant_dict_insert (&d, "name", "s", "default");
  wp_endpoint_register_stream (WP_ENDPOINT (self), g_variant_dict_end (&d));

  /* Audio streams have volume & mute controls */
  if (g_strrstr (wp_endpoint_get_media_class (WP_ENDPOINT (self)), "Audio")) {
    g_variant_dict_init (&d, NULL);
    g_variant_dict_insert (&d, "id", "u", CONTROL_VOLUME);
    g_variant_dict_insert (&d, "stream-id", "u", 0);
    g_variant_dict_insert (&d, "name", "s", "volume");
    g_variant_dict_insert (&d, "type", "s", "d");
    g_variant_dict_insert (&d, "range", "(dd)", 0.0, 1.0);
    g_variant_dict_insert (&d, "default-value", "d", 1.0);
    wp_endpoint_register_control (WP_ENDPOINT (self), g_variant_dict_end (&d));

    g_variant_dict_init (&d, NULL);
    g_variant_dict_insert (&d, "id", "u", CONTROL_MUTE);
    g_variant_dict_insert (&d, "stream-id", "u", 0);
    g_variant_dict_insert (&d, "name", "s", "mute");
    g_variant_dict_insert (&d, "type", "s", "b");
    g_variant_dict_insert (&d, "default-value", "b", FALSE);
    wp_endpoint_register_control (WP_ENDPOINT (self), g_variant_dict_end (&d));
  }

  G_OBJECT_CLASS (simple_endpoint_parent_class)->constructed (object);
}

static void
simple_endpoint_finalize (GObject * object)
{
  WpPipewireSimpleEndpoint *self = WP_PIPEWIRE_SIMPLE_ENDPOINT (object);

  if (self->node) {
    spa_hook_remove (&self->proxy_listener);
    pw_proxy_destroy ((struct pw_proxy *) self->node);
  }

  G_OBJECT_CLASS (simple_endpoint_parent_class)->finalize (object);
}

static void
simple_endpoint_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  WpPipewireSimpleEndpoint *self = WP_PIPEWIRE_SIMPLE_ENDPOINT (object);

  switch (property_id) {
  case PROP_NODE_PROXY:
    self->node = g_value_get_pointer (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
simple_endpoint_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  WpPipewireSimpleEndpoint *self = WP_PIPEWIRE_SIMPLE_ENDPOINT (object);

  switch (property_id) {
  case PROP_NODE_PROXY:
    g_value_set_pointer (value, self->node);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static gboolean
simple_endpoint_prepare_link (WpEndpoint * ep, guint32 stream_id,
    WpEndpointLink * link, GVariant ** properties, GError ** error)
{
  WpPipewireSimpleEndpoint *self = WP_PIPEWIRE_SIMPLE_ENDPOINT (ep);
  GVariantBuilder b;

  /* TODO: Since the linking with a 1 port client works when passing -1 as
   * a port parameter, there is no need to find the port and set it in the
   * properties. However, we need to add logic here and select the correct
   * port in case the client has more than 1 port */

  /* Set the port format here */

  /* Set the properties */
  g_variant_builder_init (&b, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&b, "{sv}", "node-id",
      g_variant_new_uint32 (self->info->id));
  g_variant_builder_add (&b, "{sv}", "node-port-id",
      g_variant_new_uint32 (-1));
  *properties = g_variant_builder_end (&b);

  return TRUE;
}

static GVariant *
simple_endpoint_get_control_value (WpEndpoint * ep, guint32 control_id)
{
  WpPipewireSimpleEndpoint *self = WP_PIPEWIRE_SIMPLE_ENDPOINT (ep);

  switch (control_id) {
    case CONTROL_VOLUME:
      return g_variant_new_double (self->volume);
    case CONTROL_MUTE:
      return g_variant_new_boolean (self->mute);
    default:
      g_warning ("Unknown control id %u", control_id);
      return NULL;
  }
}

static gboolean
simple_endpoint_set_control_value (WpEndpoint * ep, guint32 control_id,
    GVariant * value)
{
  WpPipewireSimpleEndpoint *self = WP_PIPEWIRE_SIMPLE_ENDPOINT (ep);
  char buf[1024];
  struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
  float volume;
  bool mute;

  switch (control_id) {
    case CONTROL_VOLUME:
      volume = g_variant_get_double (value);

      g_debug("WpEndpoint:%p set volume control (%u) value, vol:%f", self,
          control_id, volume);

      pw_node_proxy_set_param (self->node,
          SPA_PARAM_Props, 0,
          spa_pod_builder_add_object (&b,
              SPA_TYPE_OBJECT_Props, SPA_PARAM_Props,
              SPA_PROP_volume, SPA_POD_Float(volume),
              NULL));
      break;

    case CONTROL_MUTE:
      mute = g_variant_get_boolean (value);

      g_debug("WpEndpoint:%p set mute control (%u) value, mute:%d", self,
          control_id, mute);

      pw_node_proxy_set_param (self->node,
          SPA_PARAM_Props, 0,
          spa_pod_builder_add_object (&b,
              SPA_TYPE_OBJECT_Props, SPA_PARAM_Props,
              SPA_PROP_mute, SPA_POD_Bool(mute),
              NULL));
      break;

    default:
      g_warning ("Unknown control id %u", control_id);
      return FALSE;
  }

  return TRUE;
}

static void
simple_endpoint_class_init (WpPipewireSimpleEndpointClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  WpEndpointClass *endpoint_class = (WpEndpointClass *) klass;

  object_class->constructed = simple_endpoint_constructed;
  object_class->finalize = simple_endpoint_finalize;
  object_class->set_property = simple_endpoint_set_property;
  object_class->get_property = simple_endpoint_get_property;

  endpoint_class->prepare_link = simple_endpoint_prepare_link;
  endpoint_class->get_control_value = simple_endpoint_get_control_value;
  endpoint_class->set_control_value = simple_endpoint_set_control_value;

  g_object_class_install_property (object_class, PROP_NODE_PROXY,
      g_param_spec_pointer ("node-proxy", "node-proxy",
          "Pointer to the pw_node_proxy* to wrap",
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}

gpointer
simple_endpoint_factory (WpFactory * factory, GType type,
    GVariant * properties)
{
  guint64 proxy;
  const gchar *name;
  const gchar *media_class;

  g_return_val_if_fail (type == WP_TYPE_ENDPOINT, NULL);
  g_return_val_if_fail (properties != NULL, NULL);
  g_return_val_if_fail (g_variant_is_of_type (properties,
          G_VARIANT_TYPE_VARDICT), NULL);

  if (!g_variant_lookup (properties, "name", "&s", &name))
      return NULL;
  if (!g_variant_lookup (properties, "media-class", "&s", &media_class))
      return NULL;
  if (!g_variant_lookup (properties, "node-proxy", "t", &proxy))
      return NULL;

  return g_object_new (simple_endpoint_get_type (),
      "name", name,
      "media-class", media_class,
      "node-proxy", (gpointer) proxy,
      NULL);
}
