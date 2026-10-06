#ifndef VG_BRIDGE_H
#define VG_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vg_session vg_session;

/*
 * Pass {"layout":"dachen"} for spelling-only input, or
 * {"layout":"dachen","lexicon":"/absolute/path/to/factory.txtMap"} to enable
 * the upstream session, candidates, and commits. Optional
 * mixedAlphanumericalEnabled and furiousTypingEnabled4Zhuyin booleans override
 * their upstream preferences. A loaded TextMap is shared by all live sessions
 * in this process. Invalid configuration returns NULL.
 */
vg_session *vg_session_new(const char *config_json);
void vg_session_free(vg_session *session);

/*
 * Session creation, destruction, and session calls must run on the host's main
 * thread; calls for a session must be serialized. The upstream session API is
 * MainActor-bound.
 * The returned JSON string is owned by the caller and must use vg_string_free.
 */
char *vg_feed_key(vg_session *session, uint32_t fcitx_keycode,
                  uint32_t fcitx_modifiers, int is_key_down);
char *vg_select_candidate(vg_session *session, int index);
void vg_reset(vg_session *session);
void vg_string_free(char *value);

#ifdef __cplusplus
}
#endif

#endif
