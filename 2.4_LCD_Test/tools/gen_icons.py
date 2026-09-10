# -*- coding: utf-8 -*-
"""SU-4100 버키 LCD UI 에셋(PNG) 생성기

버키 LCD UI 사양 문서(아티팩트)의 화면 구성을 기준으로, TouchGFX에서 쓸
아이콘/패널 PNG를 일관된 스타일로 생성한다.

- 8배 슈퍼샘플링으로 그린 뒤 LANCZOS 축소 → 부드러운 안티앨리어싱
- 스트로크 기반, 라운드 캡, 투명 배경(ARGB8888)
- 테마 색상은 아래 THEME 딕셔너리만 바꿔서 재생성하면 됨 (디자이너 확정 시)

사용법:
    python gen_icons.py            → OUT 경로(2.4_LCD 프로젝트 assets/images)에 출력
    python gen_icons.py <출력폴더>  → 지정 폴더에 출력

필요 패키지: pillow  (python -m pip install pillow)
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

S = 8  # 슈퍼샘플 배율

# ---- 테마 (라이트) -----------------------------------------------------------
THEME = {
    "icon":          (22, 104, 196, 255),   # #1668C4 주 아이콘 파랑
    "red":           (229, 72, 77, 255),    # #E5484D Emergency
    "cell_fill":     (255, 255, 255, 255),  # 진단 셀 기본 채움
    "cell_border":   (199, 210, 222, 255),  # #C7D2DE 옅은 테두리
    "pressed_fill":  (227, 240, 255, 255),  # #E3F0FF 눌림 채움
    "pressed_border": (22, 104, 196, 255),
    "panel_fill":    (255, 255, 255, 255),  # 토스트 패널
    "panel_border":  (199, 210, 222, 255),
    "shadow":        (20, 35, 60, 60),      # 토스트 그림자 (알파 포함)
}

DEFAULT_OUT = r"C:\TouchGFXProjects\2.4_LCD\TouchGFX\assets\images"


def canvas(w_px, h_px=None):
    h_px = h_px if h_px is not None else w_px
    img = Image.new("RGBA", (w_px * S, h_px * S), (0, 0, 0, 0))
    return img, ImageDraw.Draw(img)


def P(x, y):
    return (x * S, y * S)


def rline(d, p1, p2, w, color):
    """라운드 캡 직선"""
    d.line([P(*p1), P(*p2)], fill=color, width=int(w * S))
    r = w * S / 2
    for (x, y) in (p1, p2):
        d.ellipse([x * S - r, y * S - r, x * S + r, y * S + r], fill=color)


def polyline(d, pts, w, color):
    for a, b in zip(pts, pts[1:]):
        rline(d, a, b, w, color)


def chevron(d, tip, direction_deg, arm, w, color):
    """tip에서 진행방향 반대로 벌어지는 화살촉 두 팔"""
    t = math.radians(direction_deg)
    for spread in (140, -140):
        a = t + math.radians(spread)
        end = (tip[0] + arm * math.cos(a), tip[1] + arm * math.sin(a))
        rline(d, tip, end, w, color)


def arc_stroke(d, c, rc, a1, a2, w, color):
    """중심선 반지름 rc 기준의 호 + 양끝 라운드 캡.
    주의: Pillow의 arc(width=)는 바깥 원에서 안쪽으로 두께를 채우므로
    바운딩 박스는 rc + w/2 로 잡아야 캡과 호가 정확히 맞는다."""
    r_out = rc + w / 2.0
    bb = [c[0] * S - r_out * S, c[1] * S - r_out * S,
          c[0] * S + r_out * S, c[1] * S + r_out * S]
    d.arc(bb, a1, a2, fill=color, width=int(w * S))
    for ang in (a1, a2):
        t = math.radians(ang)
        x = c[0] + rc * math.cos(t)
        y = c[1] + rc * math.sin(t)
        rr = w * S / 2
        d.ellipse([x * S - rr, y * S - rr, x * S + rr, y * S + rr], fill=color)


def save(img, out_dir, name, w_px, h_px=None):
    h_px = h_px if h_px is not None else w_px
    img = img.resize((w_px, h_px), Image.LANCZOS)
    img.save(os.path.join(out_dir, name))
    print("wrote", name)
    return img


def generate(out_dir):
    os.makedirs(out_dir, exist_ok=True)
    W = 2.6  # 32px 아이콘 기본 스트로크
    C = THEME["icon"]
    res = {}

    # ---- 화살표 상/하 (ARM UP / DOWN) ----
    for name, flip in (("icon_arrow_up.png", False), ("icon_arrow_down.png", True)):
        img, d = canvas(32)
        rline(d, (16, 6.5), (16, 26.5), W, C)
        chevron(d, (16, 6.5), -90, 10.5, W, C)
        if flip:
            img = img.transpose(Image.FLIP_TOP_BOTTOM)
        res[name] = save(img, out_dir, name, 32)

    # ---- 슬라이드 좌/우 (TUBE/DET) : 레일 + 화살표 ----
    for name, flip in (("icon_slide_left.png", False), ("icon_slide_right.png", True)):
        img, d = canvas(32)
        rline(d, (5.5, 8), (5.5, 24), W, C)
        rline(d, (10.5, 16), (27, 16), W, C)
        chevron(d, (10.5, 16), 180, 10.5, W, C)
        if flip:
            img = img.transpose(Image.FLIP_LEFT_RIGHT)
        res[name] = save(img, out_dir, name, 32)

    # ---- 회전 시계/반시계 (ARM ROT, DET ROT) : 270도 호 + 채운 삼각형 ----
    for name, flip in (("icon_rotate_cw.png", False), ("icon_rotate_ccw.png", True)):
        img, d = canvas(32)
        cx, cy, rc = 16.0, 17.0, 9.0
        arc_stroke(d, (cx, cy), rc, 0, 270, W, C)   # 갭 = 우상단 사분면
        p = (cx, cy - rc)                            # 호 꼭대기 끝
        tri = [P(p[0] + 7.0, p[1]), P(p[0] - 0.5, p[1] - 4.6), P(p[0] - 0.5, p[1] + 4.6)]
        d.polygon(tri, fill=C)
        if flip:
            img = img.transpose(Image.FLIP_LEFT_RIGHT)
        res[name] = save(img, out_dir, name, 32)

    # ---- MOV (4방향 이동) ----
    img, d = canvas(32)
    for ang in (0, 90, 180, 270):
        t = math.radians(ang)
        base = (16 + 3.5 * math.cos(t), 16 + 3.5 * math.sin(t))
        tip = (16 + 11.5 * math.cos(t), 16 + 11.5 * math.sin(t))
        rline(d, base, tip, W, C)
        chevron(d, tip, ang, 5.2, W, C)
    res["icon_move.png"] = save(img, out_dir, "icon_move.png", 32)

    # ---- COLLI (콜리메이터: 프레임 + 크로스헤어) ----
    img, d = canvas(32)
    d.rounded_rectangle([P(6, 6), P(26, 26)], radius=3 * S, outline=C, width=int(W * S))
    rline(d, (16, 12), (16, 20), W, C)
    rline(d, (12, 16), (20, 16), W, C)
    res["icon_colli.png"] = save(img, out_dir, "icon_colli.png", 32)

    # ---- Emergency 경고 삼각형 (80x80) ----
    img, d = canvas(80)
    R = THEME["red"]
    polyline(d, [(40, 9), (73, 67), (7, 67), (40, 9)], 5.0, R)
    rline(d, (40, 30), (40, 47), 5.6, R)
    d.ellipse([P(40 - 3.4, 57 - 3.4), P(40 + 3.4, 57 + 3.4)], fill=R)
    res["icon_warning.png"] = save(img, out_dir, "icon_warning.png", 80)

    # ---- 스피너 (270도 호, TextureMapper 회전용) ----
    img, d = canvas(32)
    arc_stroke(d, (16, 16), 11, -90, 180, 3.2, C)
    res["icon_spinner.png"] = save(img, out_dir, "icon_spinner.png", 32)

    # ---- 통신 연결 중 토스트 패널 (196x88, 라운드 + 그림자) ----
    img, _ = canvas(196, 88)
    # 그림자: 라운드 사각을 블러 처리해 아래로 3px 오프셋
    sh = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ds = ImageDraw.Draw(sh)
    ds.rounded_rectangle([P(8, 8), P(188, 78)], radius=8 * S, fill=THEME["shadow"])
    sh = sh.filter(ImageFilter.GaussianBlur(5 * S))
    img.alpha_composite(sh, (0, 3 * S))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([P(8, 8), P(188, 78)], radius=8 * S,
                        fill=THEME["panel_fill"], outline=THEME["panel_border"], width=int(1.5 * S))
    res["panel_toast.png"] = save(img, out_dir, "panel_toast.png", 196, 88)

    # ---- 진단 화면 키 셀 (104x32, 기본/눌림) ----
    for name, fill, border, bw in (
        ("cell_key_normal.png", THEME["cell_fill"], THEME["cell_border"], 1.5),
        ("cell_key_pressed.png", THEME["pressed_fill"], THEME["pressed_border"], 2.0),
    ):
        img, d = canvas(104, 32)
        d.rounded_rectangle([P(1, 1), P(103, 31)], radius=6 * S,
                            fill=fill, outline=border, width=int(bw * S))
        res[name] = save(img, out_dir, name, 104, 32)

    return res


def contact_sheet(res, path):
    """라이트 배경 검수 시트"""
    names = list(res.keys())
    cell, pad, cols = 220, 20, 5
    rows = (len(names) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * cell + pad, rows * (cell - 40) + pad), (245, 247, 250, 255))
    df = ImageDraw.Draw(sheet)
    for i, n in enumerate(names):
        ic = res[n]
        scale = 4 if max(ic.size) <= 40 else (2 if max(ic.size) <= 110 else 1)
        up = ic.resize((ic.size[0] * scale, ic.size[1] * scale), Image.NEAREST)
        x = pad + (i % cols) * cell
        y = pad + (i // cols) * (cell - 40)
        sheet.alpha_composite(up, (x + (196 - up.size[0]) // 2, y + (130 - up.size[1]) // 2))
        df.text((x + 4, y + 136), n.replace(".png", ""), fill=(90, 100, 112, 255))
    sheet.convert("RGB").save(path)
    print("sheet:", path)


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUT
    r = generate(out)
    contact_sheet(r, os.path.join(out, "_contact_sheet_preview.png"))
