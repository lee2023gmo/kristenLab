from PIL import Image, ImageDraw, ImageFont
import os

OUT_DIR = "resources/images"
os.makedirs(OUT_DIR, exist_ok=True)

TILE = 40
BALL = 32

def save(img, name):
    path = os.path.join(OUT_DIR, name)
    if os.path.exists(path):
        print(f"Skipped {name} (already exists)")
        return
    img.save(path, "PNG")
    print(f"Generated {name}")

def make_rgba(size, color):
    img = Image.new("RGBA", size, (0,0,0,0))
    return img

# 1. Ball - glowing purple sphere
img = make_rgba((BALL, BALL), None)
draw = ImageDraw.Draw(img)
cx, cy = BALL//2, BALL//2
r = BALL//2 - 2
for i in range(r, 0, -1):
    ratio = i / r
    col = (
        int(138 + (128-138)*ratio),
        int(92 + (247-92)*ratio),
        int(255 + (255-255)*ratio),
        255
    )
    draw.ellipse([cx-i, cy-i, cx+i, cy+i], fill=col)
draw.ellipse([cx-r, cy-r, cx+r, cy+r], outline=(180, 220, 255, 255), width=2)
save(img, "ball.png")

# 2. Wall - dark brick pattern
img = make_rgba((TILE, TILE), None)
draw = ImageDraw.Draw(img)
draw.rectangle([0,0,TILE-1,TILE-1], fill=(59,66,82,255), outline=(89,98,117,255), width=2)
draw.line([(TILE//2,0),(TILE//2,TILE//2)], fill=(40,45,55,255), width=2)
draw.line([(0,TILE//2),(TILE,TILE//2)], fill=(40,45,55,255), width=2)
draw.line([(TILE//2,TILE//2),(TILE//2,TILE)], fill=(40,45,55,255), width=2)
save(img, "wall.png")

# 3. End - green portal
img = make_rgba((TILE, TILE), None)
draw = ImageDraw.Draw(img)
margin = 4
draw.rounded_rectangle([margin, margin, TILE-margin, TILE-margin], radius=6, fill=(46,204,113,255), outline=(39,174,96,255), width=3)
try:
    font = ImageFont.truetype("arial.ttf", 14)
except:
    font = ImageFont.load_default()
draw.text((TILE//2, TILE//2), "END", fill=(255,255,255,255), font=font, anchor="mm")
save(img, "end.png")

# 4. Death - red hazard
img = make_rgba((TILE, TILE), None)
draw = ImageDraw.Draw(img)
margin = 6
draw.rectangle([margin, margin, TILE-margin, TILE-margin], fill=(184,50,50,255), outline=(255,77,77,255), width=2)
draw.line([(margin+2, margin+2), (TILE-margin-2, TILE-margin-2)], fill=(255,200,200,255), width=2)
draw.line([(TILE-margin-2, margin+2), (margin+2, TILE-margin-2)], fill=(255,200,200,255), width=2)
save(img, "death.png")

# 5. Bounce - orange upward arrow
img = make_rgba((TILE, TILE), None)
draw = ImageDraw.Draw(img)
margin = 6
draw.rounded_rectangle([margin, margin, TILE-margin, TILE-margin], radius=4, fill=(243,156,18,255), outline=(241,196,15,255), width=2)
draw.polygon([(TILE//2, margin+4), (TILE-margin-4, TILE//2+4), (margin+4, TILE//2+4)], fill=(255,255,255,255))
draw.rectangle([TILE//2-4, TILE//2+4, TILE//2+4, TILE-margin-2], fill=(255,255,255,255))
save(img, "bounce.png")

# 6. Slow - blue snowflake-like S
img = make_rgba((TILE, TILE), None)
draw = ImageDraw.Draw(img)
margin = 6
draw.rounded_rectangle([margin, margin, TILE-margin, TILE-margin], radius=4, fill=(9,132,227,255), outline=(116,185,255,255), width=2)
try:
    font = ImageFont.truetype("arial.ttf", 18)
except:
    font = ImageFont.load_default()
draw.text((TILE//2, TILE//2), "S", fill=(255,255,255,255), font=font, anchor="mm")
save(img, "slow.png")

# 7. Conveyor - green right arrows
img = make_rgba((TILE, TILE), None)
draw = ImageDraw.Draw(img)
margin = 6
draw.rounded_rectangle([margin, margin, TILE-margin, TILE-margin], radius=4, fill=(0,184,148,255), outline=(85,239,196,255), width=2)
draw.polygon([(TILE-margin-6, TILE//2), (margin+10, TILE//2-6), (margin+10, TILE//2+6)], fill=(255,255,255,255))
save(img, "conveyor.png")

# 8. Data Fragment - cyan diamond
img = make_rgba((TILE, TILE), None)
draw = ImageDraw.Draw(img)
cx, cy = TILE//2, TILE//2
size = 10
draw.polygon([(cx, cy-size), (cx+size, cy), (cx, cy+size), (cx-size, cy)], fill=(0,245,212,255), outline=(255,255,255,255), width=2)
save(img, "data_fragment.png")

# 9. Empty / Floor - subtle grid cell
img = make_rgba((TILE, TILE), None)
draw = ImageDraw.Draw(img)
draw.rectangle([0,0,TILE-1,TILE-1], fill=(16,19,31,255), outline=(39,48,74,255), width=1)
save(img, "empty.png")

# 10. Candidate Edit Point overlay - dashed frame
img = make_rgba((TILE, TILE), None)
draw = ImageDraw.Draw(img)
margin = 4
draw.rectangle([margin, margin, TILE-margin, TILE-margin], outline=(128,247,255,180), width=2)
save(img, "candidate_edit.png")

# 11. Laser - active (red beam)
img = make_rgba((TILE, TILE), None)
draw = ImageDraw.Draw(img)
# 红色发光光束
beam_margin_h = 4
beam_margin_v = TILE // 2 - 4
draw.rectangle([beam_margin_h, beam_margin_v, TILE - beam_margin_h, TILE - beam_margin_v],
               fill=(255, 23, 68, 255), outline=(255, 77, 109, 255), width=2)
# 中心高亮
draw.rectangle([beam_margin_h + 2, beam_margin_v + 2, TILE - beam_margin_h - 2, TILE - beam_margin_v - 2],
               fill=(255, 100, 130, 200))
try:
    font = ImageFont.truetype("arial.ttf", 14)
except:
    font = ImageFont.load_default()
draw.text((TILE // 2, TILE // 2), "L", fill=(255, 255, 255, 255), font=font, anchor="mm")
save(img, "laser.png")

# 12. Laser - inactive (dimmed)
img = make_rgba((TILE, TILE), None)
draw = ImageDraw.Draw(img)
draw.rectangle([beam_margin_h, beam_margin_v, TILE - beam_margin_h, TILE - beam_margin_v],
               fill=(51, 65, 92, 255), outline=(98, 112, 138, 255), width=2)
draw.rectangle([beam_margin_h + 2, beam_margin_v + 2, TILE - beam_margin_h - 2, TILE - beam_margin_v - 2],
               fill=(70, 82, 105, 200))
draw.text((TILE // 2, TILE // 2), "L", fill=(138, 150, 173, 255), font=font, anchor="mm")
save(img, "laser_inactive.png")

# 13. Map background - dark tech pattern (larger size, will be scaled by Qt)
BG_W, BG_H = 400, 400
img = make_rgba((BG_W, BG_H), None)
draw = ImageDraw.Draw(img)
# 深色底
draw.rectangle([0, 0, BG_W, BG_H], fill=(16, 19, 31, 255))
#  subtle grid pattern
for i in range(0, BG_W, 40):
    draw.line([(i, 0), (i, BG_H)], fill=(39, 48, 74, 120), width=1)
for j in range(0, BG_H, 40):
    draw.line([(0, j), (BG_W, j)], fill=(39, 48, 74, 120), width=1)
# 随机 subtle 噪点/装饰
import random
random.seed(42)
for _ in range(60):
    rx = random.randint(0, BG_W - 4)
    ry = random.randint(0, BG_H - 4)
    rw = random.randint(2, 6)
    rh = random.randint(2, 6)
    draw.rectangle([rx, ry, rx + rw, ry + rh], fill=(30, 38, 60, 100))
# 边缘暗化 vignette
for step in range(20):
    alpha = int(20 - step)
    draw.rectangle([step, step, BG_W - step, BG_H - step], outline=(0, 0, 0, alpha), width=1)
save(img, "map_background.png")

print("All placeholder assets generated.")
