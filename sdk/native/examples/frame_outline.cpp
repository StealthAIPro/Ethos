#include "visioncore_plugin.h"
#include <cstring>
#include <new>
#ifdef VC_TEST_PLUGIN
#include <string_view>
#include <windows.h>
#endif

namespace {
struct State {
    const VcHost *host;
#ifdef VC_TEST_PLUGIN
    bool crash;
#endif
};

void log(const VcHost &host, const char *message)
{
    host.log(host.context, message, uint32_t(std::strlen(message)));
}

int32_t initialize(const VcHost *host, const char *settings, void **instance)
{
    if (!host || host->abi_version != VC_ABI_VERSION || host->struct_size < VC_HOST_BASE_SIZE)
        return 1;
#ifdef VC_TEST_PLUGIN
    if (!VC_HOST_HAS(host, session_json) || !VC_HOST_HAS(host, frame_info) ||
        std::string_view(host->session_json(host->context)).find("\"sdkRevision\":2") == std::string_view::npos ||
        host->frame_info(host->context))
        return 1;
#endif
    auto *state = new (std::nothrow) State{host
#ifdef VC_TEST_PLUGIN
        , std::string_view(settings).find("\"crash\":true") != std::string_view::npos
#endif
    };
    if (!state)
        return 1;
    *instance = state;
    log(*host, "Frame Outline initialized");
    return 0;
}

int32_t frame(void *instance, const VcFrame *image)
{
    const auto &state = *static_cast<State *>(instance);
    const auto &host = *state.host;
#ifdef VC_TEST_PLUGIN
    if (state.crash)
        TerminateProcess(GetCurrentProcess(), 70);
    const auto *info = host.frame_info(host.context);
    if (!info || !info->sequence || info->skipped >= info->sequence)
        return 1;
    VcControllerOutput output{};
    output.axis_mask = 1u << VC_RX;
    output.axes[VC_RX] = 0.25f;
    host.set_controller(host.context, &output);
#endif
    const float width = float(image->width), height = float(image->height);
    const auto sampled = image->rgba[(image->height / 2) * image->stride + (image->width / 2) * 4];
    const bool a = image->controller.connected && (image->controller.buttons & (1u << VC_A));
    const VcOverlay outline{VC_RECT, width * 0.25f, height * 0.25f, width * 0.5f, height * 0.5f,
        a ? 0x7ee1aeu : sampled > 127 ? 0xda9fffu : 0xa979f3u, 3.0f, nullptr, 0};
    host.draw(host.context, &outline);
    return 0;
}

void shutdown(void *instance)
{
    auto *state = static_cast<State *>(instance);
    log(*state->host, "Frame Outline shut down");
    delete state;
}
}

const VcPlugin *vc_get_plugin(uint32_t hostAbi)
{
    static const VcPlugin plugin{
#ifdef VC_BAD_ABI
        VC_ABI_VERSION + 1,
#else
        VC_ABI_VERSION,
#endif
        sizeof(VcPlugin), 1, 0, 0, initialize, frame, shutdown};
    return hostAbi == VC_ABI_VERSION ? &plugin : nullptr;
}
