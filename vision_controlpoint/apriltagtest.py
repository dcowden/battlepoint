# OpenMV H7 — Team color via red/blue blobs + AprilTag ID + distance/yaw
# NOTE: OpenMV supports AprilTags, NOT ArUco. Use e.g. TAG36H11 markers.
# Tune thresholds and TAG_SIZE_M for your prints. Put the tag on the armband or jersey.

import sensor, image, time, math

# ======= CAMERA SETUP =======
sensor.reset()
# Use RGB565 for color blob detection; QQVGA/QVGA keeps it fast.
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)   # 320x240
sensor.set_auto_whitebal(False)     # lock AWB for stable color thresholds
sensor.set_auto_gain(False)
sensor.set_auto_exposure(True)
sensor.skip_frames(time = 500)

clock = time.clock()

# ======= COLOR THRESHOLDS (LAB) =======
# Tune with Tools -> Machine Vision -> Threshold Editor (OpenMV IDE).
# Two reds to cover hue wrap; conservative blues.
RED_THRESHOLDS  = [(30,  70,  30,  80,  10,  80),   # bright warm reds
                   (20,  70,  15,  80,  -10, 70)]   # darker reds
BLUE_THRESHOLDS = [(15,  60, -10, 15,  -80, -25),   # saturated blues
                   (10,  55, -5,  20,  -70, -10)]

# Limit search to lower third (typical armband height in a centered shot).
# Adjust or set to None for full frame.
USE_ROI = True
def armband_roi(img_w, img_h):
    h = img_h // 3
    y = img_h - h
    return (0, y, img_w, h)  # x, y, w, h

# ======= APRILTAG + POSE PARAMS =======
# Use TAG36H11 by default (widely available, good robustness)
APRILTAG_FAMILY = image.TAG16H5
# Physical tag size in meters (print dimension of the black square border)
TAG_SIZE_M = 0.0686  # 45 mm; CHANGE to your printed size

# Camera intrinsics (focal length in pixels).
# Approximate fx, fy from lens FOV or calibrate. For OV7725 on QVGA, these are decent starts.
# If you calibrated, paste real fx, fy, cx, cy here.
IMG_W, IMG_H = 320, 240
CX, CY = IMG_W/2, IMG_H/2
# Rough fx/fy estimates for a typical OpenMV wide lens at QVGA:
FX = 260  # tune if distance seems biased
FY = 260

# ======= HELPERS =======
def team_color_from_blobs(img, roi):
    # Return 'RED', 'BLUE', or 'UNKNOWN', with confidence 0..1
    stats = []
    for label, thresh in (('RED', RED_THRESHOLDS), ('BLUE', BLUE_THRESHOLDS)):
        total_area = 0
        max_d = 0
        for blob in img.find_blobs(thresh,
                                   roi=roi,
                                   pixels_threshold=60,
                                   area_threshold=200,
                                   merge=True,
                                   margin=5):
            total_area += blob.pixels()
            # favor horizontal bands: armbands are wider than tall (heuristic)
            d = blob.w() - blob.h()
            if d > max_d: max_d = d
        stats.append((label, total_area, max_d))
    # Decide by area + shape bias
    stats.sort(key=lambda x: (x[1], x[2]))  # area, then width-height
    best = stats[-1]
    second = stats[-2] if len(stats) > 1 else ('', 0, 0)
    if best[1] < 400:  # not enough pixels
        return 'UNKNOWN', 0.0
    # Confidence by area contrast
    conf = 1.0 if second[1] == 0 else min(1.0, (best[1] - second[1]) / float(best[1] + 1))
    return best[0], conf

def draw_team_roi(img, roi, color):
    x,y,w,h = roi
    img.draw_rectangle(roi, color=color, thickness=2)
    img.draw_cross(x+w//2, y+h//2, color=color)

# ======= MAIN LOOP =======
while True:
    clock.tick()
    img = sensor.snapshot()

    # Define ROI for armbands
    roi = armband_roi(IMG_W, IMG_H) if USE_ROI else (0,0,IMG_W,IMG_H)

    # --- Team color ---
    team, conf = team_color_from_blobs(img, roi)
    draw_team_roi(img, roi, color=(255,0,0) if team=='RED' else (0,0,255) if team=='BLUE' else (200,200,200))
    img.draw_string(4, 4, "Team: %s (%.2f)" % (team, conf), mono_space=False, color=(255,255,255), scale=1)

    # --- AprilTags ---
    # Supply intrinsics and tag size to get pose (xyz translations; z ~ distance)
    tags = img.find_apriltags(families=APRILTAG_FAMILY,
                              fx=FX, fy=FY, cx=CX, cy=CY)

    nearest = None
    for t in tags:
        img.draw_rectangle(t.rect(), color=(0,255,0))
        img.draw_cross(t.cx(), t.cy(), color=(0,255,0))
        # Pose data (meters if TAG_SIZE_M is correct)
        # OpenMV calculates in "tag-size units" unless you give tag_size; we scale below for clarity.
        # Distance approximation:
        # If you pass fx/fy/cx/cy and tag_size (implicitly by post-scale), z_translation is in tag-size units.
        # We scale to meters using TAG_SIZE_M.
        # Many OpenMV builds return t.z_translation() already scaled if intrinsics are given;
        # to be explicit, compute from observed size too:
        # distance ≈ fx * TagSize / observed_width_in_pixels
        obs_w = max(1, t.w())  # tag width in pixels
        d_m = (FX * TAG_SIZE_M) / float(obs_w)

        yaw_deg = math.degrees(t.rotation()) if hasattr(t, 'rotation') else 0.0

        label = "ID:%d d=%.2fm yaw=%.0f" % (t.id(), d_m, yaw_deg)
        img.draw_string(t.x(), max(0, t.y()-12), label, mono_space=False, color=(0,255,0), scale=1)

        if (nearest is None) or (d_m < nearest[1]):
            nearest = (t, d_m)

    # HUD
    fps = clock.fps()
    img.draw_string(4, 20, "Tags:%d  FPS:%.1f" % (len(tags), fps), mono_space=False, color=(255,255,0), scale=1)

    # Optional: print the strongest result each frame
    if nearest:
        t, d = nearest
        print("TEAM=%s,CONF=%.2f, TAG_ID=%d, DIST=%.3f m, CX=%d,CY=%d"
              % (team, conf, t.id(), d, t.cx(), t.cy()))
    else:
        print("TEAM=%s,CONF=%.2f, TAG_ID=None" % (team, conf))
