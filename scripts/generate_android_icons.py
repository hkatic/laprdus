#!/usr/bin/env python3
"""Generate Android launcher icons from the Laprdus logo.

Generates:
- Adaptive icon foreground layers (with safe zone padding) for all densities
- Circular-masked round launcher icons for pre-API 26 devices
"""

from PIL import Image, ImageDraw
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
SOURCE_IMAGE = os.path.join(PROJECT_ROOT, "images", "laprdus_icon_1024x1024.png")
RES_DIR = os.path.join(PROJECT_ROOT, "android", "app", "src", "main", "res")

# Logo occupies 80% of foreground canvas - keeps metallic circle (~79% of logo)
# within the 66.67% safe zone: 79% * 80% = 63.2% < 66.67%
FOREGROUND_SCALE = 0.80

# Adaptive icon foreground sizes (108dp at each density)
ADAPTIVE_FOREGROUND_SIZES = {
    "drawable-mdpi": 108,
    "drawable-hdpi": 162,
    "drawable-xhdpi": 216,
    "drawable-xxhdpi": 324,
    "drawable-xxxhdpi": 432,
}

# Mipmap launcher icon sizes
MIPMAP_SIZES = {
    "mipmap-mdpi": 48,
    "mipmap-hdpi": 72,
    "mipmap-xhdpi": 96,
    "mipmap-xxhdpi": 144,
    "mipmap-xxxhdpi": 192,
}


def extract_background_color(source):
    """Extract the background color from the top-left corner of the source image."""
    # Sample a few corner pixels and average them for accuracy
    pixels = []
    for x, y in [(0, 0), (1, 0), (0, 1), (1, 1), (2, 2)]:
        pixels.append(source.getpixel((x, y)))

    r = sum(p[0] for p in pixels) // len(pixels)
    g = sum(p[1] for p in pixels) // len(pixels)
    b = sum(p[2] for p in pixels) // len(pixels)
    a = 255

    color = (r, g, b, a)
    print(f"Extracted background color: RGB({r}, {g}, {b}) = #{r:02X}{g:02X}{b:02X}")
    return color


def generate_foregrounds(source, bg_color):
    """Generate adaptive icon foreground PNGs with proper safe-zone padding."""
    print("\nGenerating adaptive icon foregrounds...")
    for dir_name, canvas_size in ADAPTIVE_FOREGROUND_SIZES.items():
        logo_size = round(canvas_size * FOREGROUND_SCALE)
        padding = (canvas_size - logo_size) // 2

        canvas = Image.new("RGBA", (canvas_size, canvas_size), bg_color)
        logo = source.resize((logo_size, logo_size), Image.LANCZOS)
        canvas.paste(logo, (padding, padding), logo)

        out_dir = os.path.join(RES_DIR, dir_name)
        os.makedirs(out_dir, exist_ok=True)
        out_path = os.path.join(out_dir, "ic_launcher_foreground.png")
        canvas.save(out_path)
        print(f"  {dir_name}/ic_launcher_foreground.png ({canvas_size}x{canvas_size}, logo={logo_size}x{logo_size})")


def generate_round_icons(source):
    """Generate circular-masked round launcher icons for pre-API 26."""
    print("\nGenerating round launcher icons...")
    for dir_name, size in MIPMAP_SIZES.items():
        icon = source.resize((size, size), Image.LANCZOS).convert("RGBA")

        # Create circular mask
        mask = Image.new("L", (size, size), 0)
        draw = ImageDraw.Draw(mask)
        draw.ellipse([0, 0, size - 1, size - 1], fill=255)

        # Apply mask - transparent outside circle
        output = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        output.paste(icon, (0, 0), mask)

        out_dir = os.path.join(RES_DIR, dir_name)
        os.makedirs(out_dir, exist_ok=True)
        out_path = os.path.join(out_dir, "ic_launcher_round.png")
        output.save(out_path)
        print(f"  {dir_name}/ic_launcher_round.png ({size}x{size}, circular)")


def generate_playstore_icon(source, bg_color):
    """Generate 512x512 Play Store icon matching adaptive icon rendering.

    Google Play renders icons with a rounded square mask. This generates the
    Play Store icon from the same foreground/background layers used by the
    adaptive icon, ensuring the installed and store icons match exactly.
    """
    print("\nGenerating Play Store icon...")
    size = 512
    # Google Play uses ~17.86% corner radius (91.6/512)
    corner_radius = round(size * 0.1786)

    # Build the same foreground composition as adaptive icon
    logo_size = round(size * FOREGROUND_SCALE)
    padding = (size - logo_size) // 2
    canvas = Image.new("RGBA", (size, size), bg_color)
    logo = source.resize((logo_size, logo_size), Image.LANCZOS)
    canvas.paste(logo, (padding, padding), logo)

    # Apply Google Play rounded square mask
    mask = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(mask)
    draw.rounded_rectangle([0, 0, size - 1, size - 1], radius=corner_radius, fill=255)

    output = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    output.paste(canvas, (0, 0), mask)

    out_path = os.path.join(PROJECT_ROOT, "android", "app", "src", "main",
                            "ic_launcher-playstore.png")
    output.save(out_path)
    print(f"  ic_launcher-playstore.png ({size}x{size}, rounded square, r={corner_radius})")


def main():
    if not os.path.exists(SOURCE_IMAGE):
        print(f"ERROR: Source image not found: {SOURCE_IMAGE}", file=sys.stderr)
        sys.exit(1)

    source = Image.open(SOURCE_IMAGE).convert("RGBA")
    print(f"Source: {SOURCE_IMAGE} ({source.size[0]}x{source.size[1]})")

    bg_color = extract_background_color(source)

    generate_foregrounds(source, bg_color)
    generate_round_icons(source)
    generate_playstore_icon(source, bg_color)

    # Print the hex color for use in XML files
    print(f"\nBackground color for XML: #{bg_color[0]:02X}{bg_color[1]:02X}{bg_color[2]:02X}")
    print("Done! All icons generated.")


if __name__ == "__main__":
    main()
