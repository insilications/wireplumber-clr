/* WirePlumber
 *
 * Copyright © 2020 Collabora Ltd.
 *    @author George Kiagiadakis <george.kiagiadakis@collabora.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __WIREPLUMBER_SESSION_ITEM_H__
#define __WIREPLUMBER_SESSION_ITEM_H__

#include "transition.h"

G_BEGIN_DECLS

typedef struct _WpSession WpSession;

/**
 * WP_TYPE_SESSION_ITEM:
 *
 * The #WpSessionItem #GType
 */
#define WP_TYPE_SESSION_ITEM (wp_session_item_get_type ())
WP_API
G_DECLARE_DERIVABLE_TYPE (WpSessionItem, wp_session_item,
                          WP, SESSION_ITEM, GObject)

/**
 * WpSiFlags:
 * @WP_SI_FLAG_ACTIVATING: set when an activation transition is in progress
 * @WP_SI_FLAG_ACTIVE: set when an activation transition completes successfully
 * @WP_SI_FLAG_EXPORTED: set when the item has exported all necessary objects
 *   to PipeWire
 * @WP_SI_FLAG_IN_ERROR: set when there was an error in the activation process;
 *   to recover, the handler must call wp_session_item_reset() before anything
 *   else
 * @WP_SI_FLAG_CONFIGURED: must be set by subclasses when all the required
 *   (%WP_SI_CONFIG_OPTION_REQUIRED) configuration options have been set
 */
typedef enum {
  /* immutable flags, set internally */
  WP_SI_FLAG_ACTIVATING = (1<<0),
  WP_SI_FLAG_ACTIVE = (1<<1),
  WP_SI_FLAG_EXPORTED = (1<<2),
  WP_SI_FLAG_IN_ERROR = (1<<3),

  /* flags that can be changed by subclasses */
  WP_SI_FLAG_CONFIGURED = (1<<8),

  /* implementation-specific flags */
  WP_SI_FLAG_CUSTOM_START = (1<<16),
} WpSiFlags;

/**
 * WpSiConfigOptionFlags:
 * @WP_SI_CONFIG_OPTION_WRITEABLE: the option can be set externally
 * @WP_SI_CONFIG_OPTION_REQUIRED: the option is required to complete activation
 * @WP_SI_CONFIG_OPTION_PROVIDED: the value of this option can be provided
 *   by the implementation if it is not set externally; this can be used to
 *   have a "default fallback" value or to report immutable configuration
 *   that is discovered from an underlying layer (ex. hardware properties)
 */
typedef enum {
  WP_SI_CONFIG_OPTION_WRITEABLE = (1<<0),
  WP_SI_CONFIG_OPTION_REQUIRED = (1<<1),
  WP_SI_CONFIG_OPTION_PROVIDED = (1<<2),
} WpSiConfigOptionFlags;

/**
 * WpSessionItemClass:
 * @get_config_spec: See wp_session_item_get_config_spec()
 * @configure: See wp_session_item_configure()
 * @get_configuration: See wp_session_item_get_configuration()
 * @get_next_step: Implements #WpTransitionClass.get_next_step() for the
 *   transition of wp_session_item_activate()
 * @execute_step: Implements #WpTransitionClass.execute_step() for the
 *   transition of wp_session_item_activate()
 * @reset: See wp_session_item_reset()
 */
struct _WpSessionItemClass
{
  GObjectClass parent_class;

  GVariant * (*get_config_spec) (WpSessionItem * self);
  gboolean (*configure) (WpSessionItem * self, GVariant * args);
  GVariant * (*get_configuration) (WpSessionItem * self);

  guint (*get_next_step) (WpSessionItem * self, WpTransition * transition,
      guint step);
  void (*execute_step) (WpSessionItem * self, WpTransition * transition,
      guint step);

  void (*reset) (WpSessionItem * self);
};

/* properties */

WP_API
WpSession * wp_session_item_get_session (WpSessionItem * self);

WP_API
WpSiFlags wp_session_item_get_flags (WpSessionItem * self);

WP_API
void wp_session_item_set_flag (WpSessionItem * self, WpSiFlags flag);

WP_API
void wp_session_item_clear_flag (WpSessionItem * self, WpSiFlags flag);

/* configuration */

WP_API
GVariant * wp_session_item_get_config_spec (WpSessionItem * self);

WP_API
gboolean wp_session_item_configure (WpSessionItem * self, GVariant * args);

WP_API
GVariant * wp_session_item_get_configuration (WpSessionItem * self);

/* state management */

WP_API
void wp_session_item_activate (WpSessionItem * self,
    GAsyncReadyCallback callback, gpointer callback_data);

WP_API
void wp_session_item_reset (WpSessionItem * self);

G_END_DECLS

#endif