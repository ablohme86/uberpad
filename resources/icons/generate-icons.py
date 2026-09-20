#!/usr/bin/env python3
"""Regenerates the UberPad chameleon icon: uberpad.svg plus the PNG size set.

Usage:  python3 resources/icons/generate-icons.py [--dark]
Requires rsvg-convert (librsvg) for the PNG rasterisation step.
"""
import argparse, math, os, shutil, subprocess, sys

SIZES = (16, 24, 32, 48, 64, 128, 256, 512)
SKIN = (("0", "#2DD4BF"), (".52", "#3B82F6"), ("1", "#8B5CF6"))
INK = (("0", "#1F2033"), ("1", "#101122"))


def tail(cx, cy, r0, r1, th0, sweep, w0, w1, n=240):
    """Logarithmic spiral swept with a tapering width, as a closed path."""
    pts = []
    for i in range(n + 1):
        t = i / n
        th = math.radians(th0 + sweep * t)
        r = r0 * (r1 / r0) ** t
        pts.append((cx + r * math.cos(th), cy + r * math.sin(th), t))

    left, right = [], []
    for i, (x, y, t) in enumerate(pts):
        j0, j1 = max(0, i - 1), min(n, i + 1)
        dx, dy = pts[j1][0] - pts[j0][0], pts[j1][1] - pts[j0][1]
        length = math.hypot(dx, dy) or 1.0
        nx, ny = -dy / length, dx / length
        k = (1 - t) ** 1.5
        w = (w0 * k + w1 * (1 - k)) / 2.0
        left.append((x + nx * w, y + ny * w))
        right.append((x - nx * w, y - ny * w))

    d = ["M %.1f %.1f" % left[0]] + ["L %.1f %.1f" % p for p in left[1:]]
    d.append("A %.1f %.1f 0 0 1 %.1f %.1f" % (w1 / 2, w1 / 2, right[-1][0], right[-1][1]))
    d += ["L %.1f %.1f" % p for p in reversed(right[:-1])] + ["Z"]
    return " ".join(d)


TAIL = tail(cx=178, cy=348, r0=58, r1=9, th0=-90, sweep=-480, w0=54, w1=7)

# snout -> brow -> casque -> nape -> dorsal ridge -> rump -> belly -> throat -> jaw
BODY = ("M 420 234 C 414 213 403 197 387 187 C 366 172 341 160 322 143 "
        "C 313 168 305 188 290 201 C 267 197 242 194 220 206 "
        "C 195 220 179 252 174 290 C 172 303 178 313 188 319 "
        "C 213 339 263 343 298 324 C 320 312 334 294 342 276 "
        "C 358 255 389 241 420 234 Z")

LEGS = ('<path d="M 249 334 C 240 356 236 372 244 386"/>'
        '<path d="M 244 386 L 268 392" stroke-width="20"/>'
        '<path d="M 336 310 C 348 332 352 352 346 370"/>'
        '<path d="M 346 370 L 370 376" stroke-width="20"/>')


def svg(bg, fg):
    defs = ""
    for name, stops in (("bg", bg), ("fg", fg)):
        if stops:
            s = "".join('<stop offset="%s" stop-color="%s"/>' % x for x in stops)
            defs += ('<linearGradient id="%s" gradientUnits="userSpaceOnUse" '
                     'x1="72" y1="48" x2="440" y2="464">%s</linearGradient>' % (name, s))
    paint = "url(#fg)" if fg else "#ffffff"
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512" width="512" height="512">
  <title>UberPad</title>
  <defs>{defs}</defs>
  <rect x="32" y="32" width="448" height="448" rx="104" fill="url(#bg)"/>
  <g fill="{paint}" stroke="{paint}" stroke-width="34" stroke-linecap="round" stroke-linejoin="round">
    <path d="{TAIL}" stroke="none"/>
    <path d="{BODY}" stroke="none"/>
    <g fill="none">{LEGS}</g>
  </g>
  <circle cx="352" cy="204" r="16" fill="url(#bg)"/>
</svg>
'''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dark", action="store_true",
                    help="dark slate background with a gradient chameleon")
    args = ap.parse_args()

    here = os.path.dirname(os.path.abspath(__file__))
    target = os.path.join(here, "uberpad.svg")
    with open(target, "w") as fh:
        fh.write(svg(INK, SKIN) if args.dark else svg(SKIN, None))
    print("wrote %s" % target)

    if not shutil.which("rsvg-convert"):
        sys.exit("rsvg-convert not found - SVG written, PNGs skipped")
    for size in SIZES:
        png = os.path.join(here, "uberpad-%d.png" % size)
        subprocess.run(["rsvg-convert", "-w", str(size), "-h", str(size),
                        target, "-o", png], check=True)
        print("wrote %s" % png)


if __name__ == "__main__":
    main()
