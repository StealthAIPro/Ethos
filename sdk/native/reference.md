# Ethos native SDK

In Vision Core, select **C++**, import a native plugin, and start a physical capture card. Select **Start plugin** and approve the binary. The native worker reports the plugin version after initialization. Use the output console for logs.

## DLL plugins

Build a Windows x64 DLL using `visioncore_plugin.h`. Define `VC_BUILD_PLUGIN` in the plugin build to export the C entry point `vc_get_plugin`. Return the static `VcPlugin` table for ABI 1, with `struct_size` set to `sizeof(VcPlugin)`. Supply initialization, frame, and shutdown callbacks. The bundled `VisionCoreFrameOutline.dll` and its source are under `examples`.

- `initialize` receives the host API, the profile settings as a UTF-8 JSON object, and a place to store the instance pointer. Return zero to accept. If initialization fails, free any partially initialized instance before returning.
- When a resource declares a Discord policy, `discordAccess` in the settings contains `verified`, `userId`, `guildId`, and matched `roles`. The launcher completes verification before loading or starting the plugin. The same object appears in the `initialize.settings` message for standalone packages. See the [resource policy fields](../python/reference.md#discord-resource-policy).
- A resource with `developerProduct` matching its registered manifest ID receives `developerAccess` in `settings_json` or `initialize.settings`. The fields are `productId`, `discordId`, `deviceId`, `expiresAt` in Unix milliseconds, and the signed `assertion`. Send the assertion to your product backend for verification. See the [developer authorization contract](../developer-authorization.md).
- `on_frame` receives read-only RGBA8 pixels and the physical controller snapshot. Use `stride` to locate each row and keep frame pointers within the callback. Return zero to publish that frame’s output.
- Call `draw` during a frame callback to publish rectangles, circles, lines, and text in frame coordinates. Call `set_controller` during a frame callback to override only the fields selected by its masks. Buttons are bitsets, axes are −1 to 1 with positive Y up, and triggers are 0 to 1. The active bridge determines which controls it supports. Send an override on each frame where it applies.
- `shutdown` releases the instance after an orderly stop. The host terminates a stalled worker after its deadline.

Plugins that only use the controller can set `VcPlugin.flags` to `VC_PLUGIN_CONTROLLER_ONLY`. In the ordinary capture-driven runner, the host skips the pixel copy and passes a null `rgba` pointer. Vision Core also runs bundled Neptune directly from controller updates. In this mode, callbacks receive a null pixel pointer, zero dimensions, and the current controller snapshot. ABI 1 plugins built before the flags field continue to receive pixels.

Invoke host callbacks synchronously on the plugin callback thread. The `VcHost` table is valid from initialization through shutdown. Log text and overlay text are copied during each call. Keep exceptions and memory ownership on the side of the ABI that created them.

### SDK revision 2

Native plugins still use ABI 1. Check `host->struct_size >= VC_HOST_BASE_SIZE` for the original callbacks. Use `VC_HOST_HAS(host, frame_info)` and `VC_HOST_HAS(host, session_json)` before calling either optional callback.

- `frame_info(context)` gives a read-only `VcFrameInfo` during `on_frame`. It may return null. Copy the values you need before returning. `sequence` starts at 1, `received_ns` is the monotonic time since launch when the script runtime received the frame, and `skipped` counts runtime inputs skipped since the previous delivered frame. These values exclude frames dropped upstream by capture.
- `session_json(context)` returns UTF-8 JSON valid until shutdown with `sdkRevision`, `appVersion`, `runtime`, `profile`, `controllerBackend`, `captureSource`, and `captureFormat`. It describes the workspace at launch. Current controller input comes from `VcFrame.controller`.

Build the sample directly from this SDK folder with the Visual Studio C++ tools.

```powershell
cmake -S examples -B example-build -G "Visual Studio 17 2022" -A x64
cmake --build example-build --config Release
```

## Standalone packages

The optional `initialize.session` object matches `session_json`. The optional `frame.frameInfo` object carries `sequence`, `receivedNs`, and `skipped` using the same units as `VcFrameInfo`.

Place a Windows x64 executable and a manifest ending in `.vcplugin.json` in the same folder. Import the manifest. The bundled `FrameOutline.vcplugin.json` declares the packaged sample executable.

```json
{"name":"Frame Outline","kind":"process","abi":1,"entry":"VisionCoreFrameOutlineProcess.exe"}
```

The executable is launched with `--visioncore-native-v1 <mapping-name> <capacity-bytes>`. Read UTF-8 JSON lines from stdin. Reserve stdout for JSON message lines and use stderr or `log` messages for diagnostics. Flush each protocol message.

| Direction | Type | Fields |
| --- | --- | --- |
| Host → plugin | `initialize` | `abi: 1` and `settings` object. Wait for this message before initializing the plugin. |
| Plugin → host | `ready` | `abi: 1` and `version` as a numeric version string such as `1.0.0`. |
| Host → plugin | `frame` | `width`, `height`, and `stride` for the read-only RGBA8 Windows shared-memory view, plus the physical `controller` snapshot. |
| Plugin → host | `done` | `overlays` array and optional `controller` object with partial button, axis, and trigger overrides. Send one reply per frame after reading from the shared memory. |
| Either lifecycle stage, plugin → host | `log` or `error` | UTF-8 `message`. Exit with a nonzero code after a fatal error. |
| Host → plugin | `stop` | Finish the current frame, release resources, and exit. Treat stdin closure as shutdown too. |

Use `OpenFileMappingW(FILE_MAP_READ, ...)` and validate row size and buffer capacity before accessing pixels. Only one frame is pending at a time. The native host source shows how to consume the protocol and how overlays are encoded. The controller message uses the same `buttons`, `axes`, and `triggers` keys and control names as the [Python SDK](../python/reference.md).

Importing reads metadata without executing it. Running requires explicit approval of the current entry binary. DLL dependencies can load from the DLL’s folder, the Vision Core installation folder, or Windows system libraries. Workers run under the current Windows user with job-based cleanup. Approve only packages and dependencies you trust.

