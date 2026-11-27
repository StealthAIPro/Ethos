# Ethos Python SDK

Import the example at examples/frame_inspector.py into Vision Core. The application supplies the SDK automatically to managed scripts.

    from visioncore import VisionCore

    app = VisionCore()

    @app.on_start
    def setup():
        app.log("Ready")

    @app.on_frame
    def process(frame):
        app.rect(16, 16, frame.width - 32, frame.height - 32)
        app.text(20, 38, f"{frame.width} x {frame.height}")

    @app.on_shutdown
    def cleanup():
        app.log("Stopped")

    app.run()

Register one frame callback and call app.run() once. Start the physical capture card before starting a script. Losing the capture feed stops the script. A callback exception ends the Python process and appears in the output console while capture continues.

## API

- app.on_start, app.on_frame, and app.on_shutdown register lifecycle callbacks. The frame callback receives a Frame. Shutdown runs when a responsive script is stopped.
- frame.width, frame.height, frame.stride, and frame.pixels describe read-only RGBA8888 pixels. The stride is in bytes. The view is valid during the callback. Copy pixels you need to retain after the callback.
- frame.as_numpy() returns a read-only H × W × 4 uint8 view when NumPy is installed in the environment. To use OpenCV algorithms that expect BGR, convert the RGBA frame inside the callback.
- app.settings returns the profile’s JSON settings for the current run.
- `frame.as_numpy(region=(x, y, width, height))` returns a read-only view of a rectangle in source pixels. `frame.pixel(x, y)` returns an RGBA tuple and `frame.copy_region(x, y, width, height)` returns packed RGBA bytes that can be kept after the callback. Both work without NumPy.
- `frame.sequence` is the 1-based sequence at the script runtime. `frame.received_ns` is the monotonic time since launch at which the runtime received it. `frame.skipped` counts inputs skipped by the runtime since the previous delivered frame. Frames dropped upstream by capture are outside this count.
- `app.session` returns launch context with `sdkRevision`, `appVersion`, `runtime`, `profile`, `controllerBackend`, `captureSource`, and `captureFormat`. Use it inside `on_start` or a frame callback. Read `app.controller` for current controller input.
- `app.discord` returns the verified Discord access for the current launch as a dict with `verified`, `userId`, `guildId`, and matching `roles`. It returns `None` for resources without a Discord policy. Read it in `on_start` or during processing.
- `app.identity` returns the launch authorization for a registered developer product. It contains `productId`, `discordId`, `deviceId`, `expiresAt` in Unix milliseconds, and a short-lived signed `assertion`. It returns `None` for other launches.
- app.log(message) writes to the workspace console. Standard script output and Python tracebacks are also captured.
- app.rect(x, y, width, height, color, weight), app.circle(x, y, radius, color, weight), app.line(x1, y1, x2, y2, color, weight), and app.text(x, y, text, color, size) draw overlays in source-pixel coordinates. Call them within the callback. The limit is 128 drawings per frame.
- `app.controller` returns the current frame’s physical controller snapshot. It contains `connected`, `buttons`, `axes`, and `triggers`. Buttons are boolean. Axes `lx`, `ly`, `rx`, and `ry` range from −1 to 1, with positive Y up. Triggers `lt` and `rt` range from 0 to 1.
- Call `app.set_controller_output(buttons={"a": False}, axes={"rx": 0.25})` in a frame callback to override individual controls on the active bridge. Unspecified controls follow the physical input. Stop issuing an override to release it. All overrides expire shortly after the last frame and are released on stop or disconnect. Button names include `a`, `b`, `x`, `y`, `lb`, `rb`, `back`, `start`, `ls`, `rs`, `guide`, and `up`/`down`/`left`/`right`. Each bridge supports the controls exposed by its hardware and receiver.

Frames are sent through a shared-memory buffer with one callback in flight. When a script is busy, capture and display continue and the host sends the latest available frame once the script is ready. The current transport supports up to 64 MiB per RGBA frame.

## Discord resource policy

Add `"discord": {"guildId": "222", "roles": ["333"]}` to the `.vcresource.json` manifest and replace the sample IDs with your own. Vision Core reuses the account from **Settings → Account** and checks the specified server and roles before launch. Roles use any-match by default. Set `requireAll` to `true` to require all roles, omit `roles` for membership alone, or add a `userId` to require a particular account. Existing manifests with an explicit OAuth client can still use their registered loopback settings when the workspace is signed out.

### Developer product authorization

For a registered product, set `developerProduct` to the same identifier as the manifest `id`, for example `studio.example`. The host signs in with the Vision Core account, activates the product with its own device key, and supplies `app.identity` before `on_start`. Send the `assertion` over HTTPS to your product backend. Validate its ES256 signature with the trusted service public key and check its issuer, product audience, expiry, and one-time use before opening protected functionality. The signed `sub` and `deviceId` are the account and product-scoped device identity to persist. See the [developer authorization contract](../developer-authorization.md).


