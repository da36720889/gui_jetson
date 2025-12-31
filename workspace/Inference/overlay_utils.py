"""
圖像疊加工具函數
提供 matplotlib 和 OpenCV 兩種實現方式
"""

import numpy as np
import cv2

# 設置 matplotlib 使用非交互式後端（避免無顯示環境錯誤）
import matplotlib
matplotlib.use('Agg')  # 必須在導入 pyplot 之前設置
import matplotlib.pyplot as plt


def overlay_mask_matplotlib(image, red_line, blue_line):
    """
    使用 matplotlib 繪製圖像並疊加紅藍線（原始版本，較慢）
    
    Args:
        image: 輸入圖像 (numpy array)
        red_line: 紅線座標 (1D array)
        blue_line: 藍線座標 (1D array)
    
    Returns:
        疊加後的圖像 (numpy array, BGR format)
    """
    if len(image.shape) == 2 or image.shape[2] == 1:
        image = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
    norm_image = cv2.normalize(image, None, alpha=0, beta=1, norm_type=cv2.NORM_MINMAX, dtype=cv2.CV_32F)
    fig, ax = plt.subplots(figsize=(image.shape[1] / 100, image.shape[0] / 100), dpi=100)
    ax.imshow(norm_image, cmap='gray', aspect='auto')
    for x in range(len(red_line) - 1):
        ax.plot([x, x + 1], [red_line[x], red_line[x + 1]], color='blue', linewidth=2, antialiased=True)
    for x in range(len(blue_line) - 1):
        ax.plot([x, x + 1], [blue_line[x], blue_line[x + 1]], color='red', linewidth=2, antialiased=True)
    ax.axis('off')
    fig.subplots_adjust(left=0, right=1, top=1, bottom=0)
    fig.canvas.draw()
    data = np.frombuffer(fig.canvas.buffer_rgba(), dtype=np.uint8)
    data = data.reshape(fig.canvas.get_width_height()[::-1] + (4,))
    data = data[:, :, :3]
    plt.close(fig)
    return data


def overlay_mask_opencv(image, red_line, blue_line):
    """
    使用 OpenCV 直接繪製圖像並疊加紅藍線（優化版本，較快）
    
    Args:
        image: 輸入圖像 (numpy array)
        red_line: 紅線座標 (1D array)
        blue_line: 藍線座標 (1D array)
    
    Returns:
        疊加後的圖像 (numpy array, BGR format)
    """
    # 確保圖像是 BGR 格式
    if len(image.shape) == 2:
        # 灰度圖轉 BGR
        image_bgr = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
    elif image.shape[2] == 1:
        image_bgr = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
    else:
        image_bgr = image.copy()
    
    # 確保圖像是 uint8 類型
    if image_bgr.dtype != np.uint8:
        # 歸一化到 0-255
        if image_bgr.max() > 1.0:
            image_bgr = cv2.normalize(image_bgr, None, alpha=0, beta=255, norm_type=cv2.NORM_MINMAX, dtype=cv2.CV_8U)
        else:
            image_bgr = (image_bgr * 255).astype(np.uint8)
    
    # 繪製藍線（Red_line，在原始代碼中顯示為藍色）
    for x in range(len(red_line) - 1):
        pt1 = (x, int(red_line[x]))
        pt2 = (x + 1, int(red_line[x + 1]))
        cv2.line(image_bgr, pt1, pt2, (255, 0, 0), 2, cv2.LINE_AA)  # BGR: 藍色
    
    # 繪製紅線（Blue_line，在原始代碼中顯示為紅色）
    for x in range(len(blue_line) - 1):
        pt1 = (x, int(blue_line[x]))
        pt2 = (x + 1, int(blue_line[x + 1]))
        cv2.line(image_bgr, pt1, pt2, (0, 0, 255), 2, cv2.LINE_AA)  # BGR: 紅色
    
    return image_bgr


def overlay_mask_opencv_optimized(image, red_line, blue_line):
    """
    使用 OpenCV 繪製（優化版本，使用 polylines 批量繪製）
    
    Args:
        image: 輸入圖像 (numpy array)
        red_line: 紅線座標 (1D array)
        blue_line: 藍線座標 (1D array)
    
    Returns:
        疊加後的圖像 (numpy array, BGR format)
    """
    # 確保圖像是 BGR 格式
    if len(image.shape) == 2:
        image_bgr = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
    elif image.shape[2] == 1:
        image_bgr = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
    else:
        image_bgr = image.copy()
    
    # 確保圖像是 uint8 類型
    if image_bgr.dtype != np.uint8:
        if image_bgr.max() > 1.0:
            image_bgr = cv2.normalize(image_bgr, None, alpha=0, beta=255, norm_type=cv2.NORM_MINMAX, dtype=cv2.CV_8U)
        else:
            image_bgr = (image_bgr * 255).astype(np.uint8)
    
    # 準備點座標（批量繪製）
    # 藍線（Red_line）
    blue_points = np.array([[x, int(red_line[x])] for x in range(len(red_line))], dtype=np.int32)
    blue_points = blue_points.reshape((-1, 1, 2))
    cv2.polylines(image_bgr, [blue_points], False, (255, 0, 0), 2, cv2.LINE_AA)
    
    # 紅線（Blue_line）
    red_points = np.array([[x, int(blue_line[x])] for x in range(len(blue_line))], dtype=np.int32)
    red_points = red_points.reshape((-1, 1, 2))
    cv2.polylines(image_bgr, [red_points], False, (0, 0, 255), 2, cv2.LINE_AA)
    
    return image_bgr

