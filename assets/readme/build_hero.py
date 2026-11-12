"""Generates hero.svg from the firmware's real bitmaps (src/Bitmaps).

Face = eyeDefault (32x18 @ 15,0) + noseNew (8x5 @ 56,7) + mouthDefault (50x14 @ 14,18) on one 64x32 panel,
mirrored onto the second panel exactly like Hub75DMA::drawBitmap. Run from the repo root.
"""
import re, pathlib

SRC = pathlib.Path("src/Bitmaps")
# Firmware default (ColorEffects mode 0): vertical gradient by row, scaled by pixel brightness.
TOP, BOT = (255, 163, 147), (255, 43, 91)
ACCENT, DIM = "#ff6b84", "#1c1216"
MONO = "ui-monospace,SFMono-Regular,Menlo,Consolas,monospace"
SANS = "-apple-system,BlinkMacSystemFont,'Segoe UI',Helvetica,Arial,sans-serif"
MIN = 16  # skip near-black pixels

def load(name):
    for f in SRC.rglob("*.h"):
        m = re.search(r"PROGMEM %s\[\d*\]\s*=\s*\{(.*?)\};" % name, f.read_text(), re.S)
        if m:
            return [int(x, 16) for x in re.findall(r"0x[0-9a-fA-F]+", m.group(1))]
    raise KeyError(name)

def best_frame(prefix):  # fullest viseme frame = fully open mouth
    return max((load(f"{prefix}{i}") for i in range(1, 61)), key=lambda b: sum(v > MIN for v in b))

def circles(pixels, ox, oy, pitch, r):
    out = []
    for x, y, v in pixels:
        f = min(y, 31) / 31
        rgb = [round((t * (1 - f) + b * f) * v / 255) for t, b in zip(TOP, BOT)]
        out.append(f'<circle cx="{ox+(x+.5)*pitch:.1f}" cy="{oy+(y+.5)*pitch:.1f}" r="{r}" fill="#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}"/>')
    return "".join(out)

def blit(img, w, h, x0, y0, out):
    for i in range(h):
        for j in range(w):
            v = img[i * w + j]
            if v > MIN:
                out[(x0 + j, y0 + i)] = v

# --- the face on the 128x32 screen
left = {}
blit(load("eyeDefault"), 32, 18, 15, 0, left)
blit(load("mouthDefault"), 50, 14, 14, 18, left)
blit(load("noseNew"), 8, 5, 56, 7, left)  # drawNose(noseNew)
screen = {}
for (x, y), v in left.items():
    screen[(x, y)] = v
    screen[(127 - x, y)] = v  # right panel is the mirror image
PITCH, PX, PY = 4.2, 600 + 12, 92 + 12
face = circles([(x, y, v) for (x, y), v in screen.items()], PX, PY, PITCH, 1.6)

svg = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1200 340" width="100%" role="img" aria-labelledby="t d">
<title id="t">KMMX Controller</title>
<desc id="d">The default face from the firmware on its 128 by 32 LED matrix beside the title KMMX Controller.</desc>
<defs><pattern id="g" width="{PITCH}" height="{PITCH}" patternUnits="userSpaceOnUse" x="{PX}" y="{PY}"><circle cx="{PITCH/2}" cy="{PITCH/2}" r="0.8" fill="{DIM}"/></pattern></defs>
<rect width="1200" height="340" rx="22" fill="#0a0709"/>
<g transform="translate(60 0)">
<text x="0" y="86" font-family="{MONO}" font-size="20" fill="{ACCENT}" letter-spacing="3">ESP32-S3 · HUB75 · BLE</text>
<text x="-4" y="178" font-family="{SANS}" font-size="104" font-weight="800" fill="#eaf4f7" letter-spacing="-2">KMMX</text>
<text x="0" y="232" font-family="{SANS}" font-size="46" font-weight="300" fill="#eaf4f7">Controller</text>
<text x="0" y="276" font-family="{SANS}" font-size="22" fill="#8fa3ad">Firmware that gives a fursuit face its expressions.</text>
<text x="0" y="306" font-family="{MONO}" font-size="18" fill="#5b6b75">128×32 px · visemes · boop · motion · V4</text>
</g>
<rect x="600" y="92" width="{128*PITCH+24}" height="{32*PITCH+24}" rx="12" fill="#120b0e" stroke="#2b1a20" stroke-width="2"/>
<rect x="{PX}" y="{PY}" width="{128*PITCH}" height="{32*PITCH}" fill="url(#g)"/>
<line x1="{PX+64*PITCH}" y1="{PY}" x2="{PX+64*PITCH}" y2="{PY+32*PITCH}" stroke="#2b1a20" stroke-width="1"/>
{face}
</svg>
'''
pathlib.Path("assets/readme/hero.svg").write_text(svg, encoding="utf-8")
print(len(svg))
