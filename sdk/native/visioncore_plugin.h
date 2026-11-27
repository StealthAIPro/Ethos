#ifndef VISIONCORE_PLUGIN_H
#define VISIONCORE_PLUGIN_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VC_ABI_VERSION 1u
#define VC_SDK_REVISION 2u
#define VC_PLUGIN_CONTROLLER_ONLY 0x1u
#if defined(VC_BUILD_PLUGIN) && defined(_WIN32)
#define VC_EXPORT __declspec(dllexport)
#else
#define VC_EXPORT
#endif

enum VcButton {
    VC_A, VC_B, VC_X, VC_Y, VC_LB, VC_RB, VC_BACK, VC_START, VC_LS, VC_RS,
    VC_GUIDE, VC_TOUCHPAD, VC_SHARE, VC_PADDLE_R, VC_PADDLE_L, VC_MISC1,
    VC_PADDLE_R2, VC_PADDLE_L2, VC_UP, VC_DOWN, VC_LEFT, VC_RIGHT, VC_BUTTON_COUNT
};
enum VcAxis { VC_LX, VC_LY, VC_RX, VC_RY, VC_AXIS_COUNT };
enum VcTrigger { VC_LT, VC_RT, VC_TRIGGER_COUNT };
enum VcShape { VC_RECT, VC_CIRCLE, VC_LINE, VC_TEXT };

typedef struct VcController {
    uint32_t connected;
    uint32_t buttons;
    float axes[VC_AXIS_COUNT];
    float triggers[VC_TRIGGER_COUNT];
} VcController;

typedef struct VcControllerOutput {
    uint32_t button_mask;
    uint32_t buttons;
    uint32_t axis_mask;
    uint32_t trigger_mask;
    float axes[VC_AXIS_COUNT];
    float triggers[VC_TRIGGER_COUNT];
} VcControllerOutput;

typedef struct VcFrame {
    uint32_t width, height, stride;
    const uint8_t *rgba;
    VcController controller;
} VcFrame;

typedef struct VcFrameInfo {
    uint64_t sequence;      /* 1-based at the runtime input. */
    uint64_t received_ns;   /* Monotonic runtime clock, relative to this launch. */
    uint64_t skipped;       /* Runtime inputs skipped since the last delivered frame. */
} VcFrameInfo;

typedef struct VcOverlay {
    uint32_t kind;
    float x, y, a, b;            /* rect: width/height, circle: radius in a, line: end x/y */
    uint32_t rgb;                /* 0xRRGGBB */
    float weight;               /* stroke width or text size */
    const char *text;           /* UTF-8, required for VC_TEXT only */
    uint32_t text_bytes;
} VcOverlay;

typedef struct VcHost {
    uint32_t abi_version, struct_size;
    void *context;
    void (*log)(void *context, const char *utf8, uint32_t bytes);
    void (*draw)(void *context, const VcOverlay *overlay);
    void (*set_controller)(void *context, const VcControllerOutput *output);
    const VcFrameInfo *(*frame_info)(void *context); /* Optional. Valid during on_frame. */
    const char *(*session_json)(void *context);     /* Optional. UTF-8 JSON for the current launch. */
} VcHost;

#define VC_HOST_BASE_SIZE offsetof(VcHost, frame_info)
#define VC_HOST_HAS(host, field) ((host) != NULL && \
    (host)->struct_size >= offsetof(VcHost, field) + sizeof((host)->field) && (host)->field != NULL)

typedef struct VcPlugin {
    uint32_t abi_version, struct_size;
    uint32_t version_major, version_minor, version_patch;
    int32_t (*initialize)(const VcHost *host, const char *settings_json, void **instance);
    int32_t (*on_frame)(void *instance, const VcFrame *frame);
    void (*shutdown)(void *instance);
    uint32_t flags; /* Optional ABI 1 extension. Controller-only frames have rgba == NULL. */
} VcPlugin;

typedef const VcPlugin *(*VcQueryPlugin)(uint32_t host_abi);
VC_EXPORT const VcPlugin *vc_get_plugin(uint32_t host_abi);

#ifdef __cplusplus
}
#endif
#endif
