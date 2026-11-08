"""Restore HUB75 3.0.13 OE timing while using the 3.1.0 driver."""

from pathlib import Path


FUNCTION = "void MatrixPanel_I2S_DMA::setBrightnessOE("
WINDOW_START = "  // Maximum usable pixel window"
WINDOW_END = "  const int x_coord_max = x_coord_min + active_pixels;"
PLANE = "      --colouridx;"
WEIGHTED_WINDOW = """

      // shortcut: retain 3.0.13 OE weighting until upstream fixes reduced-depth gradients.
      const int bitplane = (2 * _depth - colouridx) % _depth;
      const int bitshift = (_depth - lsbMsbTransitionBit - 1) >> 1;
      const int rightshift = std::max(bitplane - bitshift - 2, 0);
      int active_pixels = ((_width - _blank) * brt) >> (7 + rightshift);
      active_pixels = (active_pixels >> 1) | (active_pixels & 1);
      const int x_coord_min = (_width - active_pixels) >> 1;
      const int x_coord_max = (_width + active_pixels + 1) >> 1;"""


def patch_source(source):
    start = source.index(FUNCTION)
    end = source.index("\n}\n", start) + 2
    body = source[start:end]
    if WEIGHTED_WINDOW in body:
        return source
    if body.count(PLANE) != 1 or body.count(WINDOW_START) != 1 or body.count(WINDOW_END) != 1:
        raise RuntimeError("HUB75 OE code changed; review the gradient timing patch before upgrading")
    window_start = body.index(WINDOW_START)
    window_end = body.index(WINDOW_END) + len(WINDOW_END)
    body = body[:window_start] + body[window_end:]
    body = body.replace(PLANE, PLANE + WEIGHTED_WINDOW, 1)
    return source[:start] + body + source[end:]


def patch_library(env):
    source_path = (Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV")
                   / "ESP32 HUB75 LED MATRIX PANEL DMA Display" / "src"
                   / "ESP32-HUB75-MatrixPanel-I2S-DMA.cpp")
    source = source_path.read_text(encoding="utf-8")
    patched = patch_source(source)
    if patched != source:
        source_path.write_text(patched, encoding="utf-8")
        print("Restored HUB75 gradient OE timing")


if "Import" in globals():
    Import("env")
    patch_library(env)
