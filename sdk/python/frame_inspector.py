from visioncore import VisionCore

app = VisionCore()


@app.on_start
def start():
    app.log(f"Frame callback is ready on profile {app.session.get('profile', 'unknown')}.")


@app.on_frame
def inspect_frame(frame):
    inset = min(frame.width, frame.height) // 8
    app.rect(inset, inset, frame.width - 2 * inset, frame.height - 2 * inset)
    red, green, blue, _ = frame.pixel(frame.width // 2, frame.height // 2)
    app.circle(frame.width / 2, frame.height / 2, inset / 2, color=f"#{red:02x}{green:02x}{blue:02x}")
    app.text(inset, inset - 8, f"{frame.width} × {frame.height}  ·  Frame {frame.sequence}  ·  Skipped {frame.skipped}")


@app.on_shutdown
def stop():
    app.log("Frame callback stopped.")


app.run()
