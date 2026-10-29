# Divine Engine

Divine Engine is a small C++20 browser-engine project. SDL3 provides the window, renderer, and event loop. The current pipeline parses local HTML, resolves a limited CSS subset, builds a layout tree, measures and paints text, and scrolls the page vertically.

## Requirements

- CMake 3.20 or newer
- A C++20 compiler
- Ninja (for the commands below)
- SDL3 development package
- FreeType development package
- Fontconfig is optional; when present it locates the system sans-serif font

## Build and test

From the project directory:

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Tests use SDL's dummy video driver and do not require a visible desktop window. Font-specific pixel checks run when a usable font can be found; a missing font is handled without crashing.

## Open a local page

Open a page in the interactive window with either form:

```sh
./build/divine-engine --open ./example.html
./build/divine-engine ./example.html
```

For a one-frame headless smoke test:

```sh
./build/divine-engine --open ./example.html --headless --frames 1
```

An HTML file may include a `<style>` element. A `<link rel="stylesheet" href="styles.css">` loads a local stylesheet relative to the HTML file. Missing local files are reported. Remote stylesheet URLs are not fetched.

Mouse-wheel scrolling and the Up/Down, PageUp/PageDown, Home, and End keys move the page. The toolbar remains fixed while page content scrolls. Resizing relayouts the page and reclamps the scroll position.

## Font configuration

FreeType supplies text metrics and glyph rasterization. To choose a font, set `DIVINE_FONT_PATH` or pass `--font`:

```sh
DIVINE_FONT_PATH="$HOME/.local/share/fonts/DejaVuSans.ttf" ./build/divine-engine --open ./example.html
./build/divine-engine --font /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf --open ./example.html
```

The requested file is tried first. If it is unavailable, the engine checks common fallback locations and, when built with Fontconfig, asks Fontconfig for a system sans-serif font. If no font can be loaded, page rendering continues without text and prints a warning. Font paths are machine-specific; choose a font installed on the target system rather than relying on the examples above.

## Existing command-line tools

The non-interactive parser and inspection modes remain available:

```sh
./build/divine-engine --parse-html '<p>Hello</p>'
./build/divine-engine --parse-file ./example.html
./build/divine-engine --parse-css 'p { color: red; }'
./build/divine-engine --resolve-style '<p>Hello</p>' 'p { color: blue; }'
./build/divine-engine --layout '<p>Hello</p>' '' 800
./build/divine-engine --help
```

## Current limitations

This is an early browser engine, not a standards-complete browser. CSS selectors and properties are limited; layout and inline flow are basic, and wrapping is based on DOM text nodes rather than complete CSS line boxes. External stylesheets are local only. There is no networking, JavaScript, image decoding, horizontal scrolling, or functional address bar/navigation controls.
# Browser
# Browser
