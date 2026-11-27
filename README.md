# Ethos website

Open `index.html` to view the download page. Open `developers.html` for the SDK references and examples. To preview both pages over HTTP, run `py -3 -m http.server 8765 --bind 127.0.0.1` from this folder and open `http://127.0.0.1:8765`.

The site uses static files and needs no build step. Keep `assets/` and `sdk/` beside the HTML files when hosting it. The web installer downloads the current Windows x64 release from R2. The offline installer link points to a specific published release.

The developer bundle includes a Python sample resource with its matching file hash, the C++ ABI header and buildable example, OFS and GPC3 scripts, and the developer authorization contract. The SDK still imports as `visioncore` in Python.
