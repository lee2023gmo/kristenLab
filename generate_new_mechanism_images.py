from PIL import Image, ImageDraw, ImageFont
import os

OUT_DIR = "resources/images"
os.makedirs(OUT_DIR, exist_ok=True)

TILE = 40

def save(img, name):
    path = os.path.join(OUT_DIR, name)
    img.save(path, "PNG")
    print(f"Generated {name}")

try:
    font = ImageFont.truetype("arial.ttf", 14)
    font_small = ImageFont.truetype("arial.ttf", 12)
except Exception:
    font = ImageFont.load_default()
    font_small = ImageFont.load_default()

# Key (K) - golden key
img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)
cx, cy = TILE // 2, TILE // 2
# key head
draw.ellipse([cx - 9, cy - 10, cx + 9, cy + 2], fill=(250, 204, 21, 255), outline=(255, 255, 255, 255), width=2)
# key hole
draw.ellipse([cx - 3, cy - 6, cx + 3, cy - 1], fill=(16, 19, 31, 255))
# key shaft
draw.rectangle([cx - 3, cy, cx + 3, cy + 14], fill=(250, 204, 21, 255))
draw.rectangle([cx + 1, cy + 8, cx + 5, cy + 11], fill=(250, 204, 21, 255))
draw.text((TILE // 2, TILE - 6), "K", fill=(255, 255, 255, 255), font=font_small, anchor="mm")
save(img, "key.png")

# Door (A) - closed wooden door
img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)
margin = 4
draw.rounded_rectangle([margin, margin, TILE - margin, TILE - margin], radius=4, fill=(146, 64, 14, 255), outline=(245, 158, 11, 255), width=2)
# door panels
draw.line([(TILE // 2, margin + 3), (TILE // 2, TILE - margin - 3)], fill=(120, 53, 15, 255), width=2)
draw.line([(margin + 3, TILE // 2), (TILE - margin - 3, TILE // 2)], fill=(120, 53, 15, 255), width=2)
# knob
draw.ellipse([TILE - margin - 8, TILE // 2 - 3, TILE - margin - 4, TILE // 2 + 3], fill=(245, 158, 11, 255))
draw.text((TILE // 2, margin + 8), "A", fill=(255, 255, 255, 255), font=font_small, anchor="mm")
save(img, "door.png")

# Portal (B) - purple warp ring
img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)
cx, cy = TILE // 2, TILE // 2
# outer ring
draw.ellipse([cx - 15, cy - 15, cx + 15, cy + 15], fill=(76, 29, 149, 200), outline=(167, 139, 250, 255), width=3)
# inner core
draw.ellipse([cx - 8, cy - 8, cx + 8, cy + 8], fill=(124, 58, 237, 255), outline=(221, 214, 254, 255), width=1)
# swirl accents
draw.arc([cx - 12, cy - 12, cx + 12, cy + 12], start=30, end=180, fill=(221, 214, 254, 200), width=2)
draw.text((TILE // 2, cy), "B", fill=(255, 255, 255, 255), font=font_small, anchor="mm")
save(img, "portal.png")

print("New mechanism placeholder assets generated.")
