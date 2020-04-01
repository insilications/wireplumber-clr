/* WirePlumber
 *
 * Copyright © 2019 Collabora Ltd.
 *    @author George Kiagiadakis <george.kiagiadakis@collabora.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __WIREPLUMBER_SESSION_H__
#define __WIREPLUMBER_SESSION_H__

#include "proxy.h"

G_BEGIN_DECLS

/**
 * WpDefaultEndpointType:
 * @WP_DEFAULT_ENDPOINT_TYPE_AUDIO_SOURCE: the default audio source (capture)
 *    endpoint
 * @WP_DEFAULT_ENDPOINT_TYPE_AUDIO_SINK: the default audio sink (playback)
 *    endpoint
 * @WP_DEFAULT_ENDPOINT_TYPE_VIDEO_SOURCE: the default video source endpoint
 */
typedef enum {
  WP_DEFAULT_ENDPOINT_TYPE_AUDIO_SOURCE = 0x1000000 /* SPA_PROP_START_CUSTOM */,
  WP_DEFAULT_ENDPOINT_TYPE_AUDIO_SINK,
  WP_DEFAULT_ENDPOINT_TYPE_VIDEO_SOURCE,
} WpDefaultEndpointType;

/**
 * WpSessionFeatures:
 * @WP_SESSION_FEATURE_DEFAULT_ENDPOINT: enables the use of
 *   wp_session_get_default_endpoint() and wp_session_set_default_endpoint()
 *   to store default endpoint preferences on the session
 *
 * An extension of #WpProxyFeatures
 */
typedef enum { /*< flags >*/
  WP_SESSION_FEATURE_DEFAULT_ENDPOINT = WP_PROXY_FEATURE_LAST,
} WpSessionFeatures;

/**
 * WP_TYPE_SESSION:
 *
 * The #WpSession #GType
 */
#define WP_TYPE_SESSION (wp_session_get_type ())
WP_API
G_DECLARE_DERIVABLE_TYPE (WpSession, wp_session, WP, SESSION, WpProxy)

struct _WpSessionClass
{
  WpProxyClass parent_class;

  guint32 (*get_default_endpoint) (WpSession * self,
      WpDefaultEndpointType type);
  void (*set_default_endpoint) (WpSession * self,
      WpDefaultEndpointType type, guint32 id);
};

WP_API
guint32 wp_session_get_default_endpoint (WpSession * self,
    WpDefaultEndpointType type);

WP_API
void wp_session_set_default_endpoint (WpSession * self,
    WpDefaultEndpointType type, guint32 id);

/**
 * WP_TYPE_IMPL_SESSION:
 *
 * The #WpImplSession #GType
 */
#define WP_TYPE_IMPL_SESSION (wp_impl_session_get_type ())
WP_API
G_DECLARE_FINAL_TYPE (WpImplSession, wp_impl_session, WP, IMPL_SESSION, WpSession)

WP_API
WpImplSession * wp_impl_session_new (WpCore * core);

WP_API
void wp_impl_session_set_property (WpImplSession * self,
    const gchar * key, const gchar * value);

WP_API
void wp_impl_session_update_properties (WpImplSession * self,
    WpProperties * updates);

G_END_DECLS

#endif
