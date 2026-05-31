from PIL import Image, ImageDraw

TILE_SIZE = 40
BG_COLOR = (214, 51, 132)      # #d63384
BORDER_COLOR = (255, 121, 198) # #ff79c6
ARROW_COLOR = (255, 255, 255)  # white

def draw_arrow(draw, direction, cx, cy, length=14):
    if direction == 'upright':
        draw.polygon([(cx, cy - length), (cx + 4, cy - length + 6), (cx - 2, cy - length + 6),
                      (cx - 2, cy + 2), (cx - length + 6, cy + 2), (cx - length + 6, cy + 4),
                      (cx - 4, cy + 4), (cx - 4, cy - length + 4)], fill=ARROW_COLOR)
    elif direction == 'upleft':
        draw.polygon([(cx, cy - length), (cx - 4, cy - length + 6), (cx + 2, cy - length + 6),
                      (cx + 2, cy + 2), (cx + length - 6, cy + 2), (cx + length - 6, cy + 4),
                      (cx + 4, cy + 4), (cx + 4, cy - length + 4)], fill=ARROW_COLOR)
    elif direction == 'downright':
        draw.polygon([(cx, cy + length), (cx + 4, cy + length - 6), (cx - 2, cy + length - 6),
                      (cx - 2, cy - 2), (cx - length + 6, cy - 2), (cx - length + 6, cy - 4),
                      (cx - 4, cy - 4), (cx - 4, cy + length - 4)], fill=ARROW_COLOR)
    elif direction == 'downleft':
        draw.polygon([(cx, cy + length), (cx - 4, cy + length - 6), (cx + 2, cy + length - 6),
                      (cx + 2, cy - 2), (cx + length - 6, cy - 2), (cx + length - 6, cy - 4),
                      (cx + 4, cy - 4), (cx + 4, cy + length - 4)], fill=ARROW_COLOR)
    elif direction == 'right':
        draw.polygon([(cx + length, cy), (cx + length - 6, cy - 4), (cx + length - 6, cy + 2),
                      (cx - 2, cy + 2), (cx - 2, cy - 2), (cx + length - 4, cy - 2),
                      (cx + length - 4, cy - 4), (cx + length - 4, cy + 4)], fill=ARROW_COLOR)
    elif direction == 'left':
        draw.polygon([(cx - length, cy), (cx - length + 6, cy - 4), (cx - length + 6, cy + 2),
                      (cx + 2, cy + 2), (cx + 2, cy - 2), (cx - length + 4, cy - 2),
                      (cx - length + 4, cy - 4), (cx - length + 4, cy + 4)], fill=ARROW_COLOR)

def create_image(name, direction):
    img = Image.new('RGBA', (TILE_SIZE, TILE_SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # 绘制圆角矩形背景
    margin = 2
    draw.rounded_rectangle(
        [margin, margin, TILE_SIZE - margin, TILE_SIZE - margin],
        radius=6,
        fill=BG_COLOR,
        outline=BORDER_COLOR,
        width=2
    )
    
    # 绘制箭头
    draw_arrow(draw, direction, TILE_SIZE // 2, TILE_SIZE // 2)
    
    img.save(f'resources/images/{name}.png')
    print(f'Generated: resources/images/{name}.png')

create_image('trampoline_upright', 'upright')
create_image('trampoline_upleft', 'upleft')
create_image('trampoline_downright', 'downright')
create_image('trampoline_downleft', 'downleft')
create_image('trampoline_right', 'right')
create_image('trampoline_left', 'left')
print('All trampoline placeholder images generated.')
