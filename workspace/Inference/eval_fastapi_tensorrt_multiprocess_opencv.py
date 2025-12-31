"""
使用 TensorRT + Multiprocess + OpenCV 的 FastAPI 服務
這是優化版本的推理服務，結合了：
- TensorRT：加速推理
- Multiprocess：並行統計計算
- OpenCV：快速圖像疊加
"""

from fastapi import FastAPI
import uvicorn
import numpy as np
import cv2
import os
import time
from inference_tensorrt import TensorRTInference
import albumentations as A
from albumentations.pytorch import ToTensorV2
import multiprocessing as mp
from multiprocessing import Pool

# 導入 NVTX 標記（用於性能分析）
try:
    from nvtx_helper import nvtx_range, nvtx_mark
except ImportError:
    # 如果無法導入，創建空的佔位符
    class nvtx_range:
        def __init__(self, *args, **kwargs):
            pass
        def __enter__(self):
            return self
        def __exit__(self, *args, **kwargs):
            return False
    def nvtx_mark(*args, **kwargs):
        pass

# 設置 matplotlib 使用非交互式後端（避免無顯示環境錯誤）
import matplotlib
matplotlib.use('Agg')  # 必須在導入 pyplot 之前設置

# 導入 OpenCV overlay 函數
from overlay_utils import overlay_mask_opencv_optimized

pixel_size_um = 5.92
fitting_order = 3
Refractive_index_epidermis = 1.424

# TensorRT 引擎路徑
trt_img_path = '/home/aboil/jetson_gui/workspace/Inference/Output/tensorrt/model_img.trt'
trt_mask_path = '/home/aboil/jetson_gui/workspace/Inference/Output/tensorrt/model_mask.trt'
test_base_dir = '/home/aboil/jetson_gui/quant_raw/'

app = FastAPI()

# 檢查 TensorRT 引擎檔案是否存在
if not os.path.exists(trt_img_path):
    raise FileNotFoundError(f"TensorRT 引擎檔案不存在: {trt_img_path}\n請先執行 convert_to_tensorrt.py 進行轉換")
if not os.path.exists(trt_mask_path):
    raise FileNotFoundError(f"TensorRT 引擎檔案不存在: {trt_mask_path}\n請先執行 convert_to_tensorrt.py 進行轉換")

print("=" * 70)
print("正在載入 TensorRT 引擎...")
print("=" * 70)
t0 = time.perf_counter()
trt_img = TensorRTInference(trt_img_path, input_shape=(1, 1, 370, 944))
trt_mask = TensorRTInference(trt_mask_path, input_shape=(1, 1, 370, 944))
t1 = time.perf_counter()
print(f"✓ TensorRT 引擎載入完成，耗時: {t1 - t0:.3f} 秒")
print("=" * 70)
print("✓ 使用 TensorRT 進行推理（不是 PyTorch）")
print("✓ 使用 Multiprocess 進行並行統計計算")
print("✓ 使用 OpenCV 進行快速圖像疊加")
print("=" * 70)

def load_bin_image(image_path, width=1024, height=1000, dtype=np.float32):
    with open(image_path, 'rb') as f:
        data = np.fromfile(f, dtype=dtype)
    assert data.size == width * height, f"Unexpected data size: {data.size}"
    image = data.reshape((height, width))
    image = np.transpose(image, (1, 0))
    return image

def process_image(image_path, trt_inference, output_dir, is_mask=False):
    """
    使用 TensorRT 引擎處理圖像
    
    Args:
        image_path: 圖像檔案路徑
        trt_inference: TensorRT 推理物件
        output_dir: 輸出目錄
        is_mask: 是否為遮罩模型
    """
    model_type = "mask" if is_mask else "image"
    with nvtx_range(f"process_image ({model_type}): {os.path.basename(image_path)}"):
        with nvtx_range("Load and Preprocess Image"):
            image = load_bin_image(image_path)
            image = image[:370, 40:984]  
            
            # 轉換為 numpy array 並正規化
            image = image.astype(np.float32)
            max_val = np.max(image)
            min_val = np.min(image)
            image = image / 65535.0
            
            # 新增 batch 和 channel 維度: (370, 944) -> (1, 1, 370, 944)
            image_input = image.reshape(1, 1, 370, 944)
        
        # 使用 TensorRT 推理（內部已有 NVTX 標記）
        with nvtx_range(f"TensorRT Inference ({model_type})"):
            prediction = trt_inference.infer(image_input)
        
        # 如果是遮罩模型，需要套用 sigmoid
        if is_mask:
            with nvtx_range("Post-process Mask (Sigmoid)"):
                output_mean = prediction.mean()
                target_mean = -11.0
                offset = output_mean - target_mean
                prediction_adjusted = prediction - offset
                prediction = 1.0 / (1.0 + np.exp(-prediction_adjusted))
                prediction = np.clip(prediction, 0.0, 1.0)
        
        # 移除 batch 和 channel 維度: (1, 1, 370, 944) -> (370, 944)
        with nvtx_range("Post-process Output"):
            prediction = prediction.squeeze()
            prediction = np.abs(prediction)
    
    return prediction, max_val, min_val

def polyfit_and_evaluate(Red_line, Blue_line, fitting_order):
    with nvtx_range("polyfit_and_evaluate (CPU)"):
        Red_line = Red_line[50:]
        Blue_line = Blue_line[50:]
        x = np.arange(0, 894)
        with nvtx_range("polyfit Red_line"):
            p1 = np.polyfit(x, Red_line[:894], fitting_order)
            y1 = np.polyval(p1, x)
        with nvtx_range("polyfit Blue_line"):
            p2 = np.polyfit(x, Blue_line[:894], fitting_order)
            y2 = np.polyval(p2, x)
        upperfit = np.abs(y1 - Red_line[:894])
        junctionfit = np.abs(y2 - Blue_line[:894])
        upperfit = np.sort(upperfit)[99:-93]
        junctionfit = np.sort(junctionfit)[99:-93]
        return upperfit, junctionfit

def save_final_analysis_to_txt(mean_thickness, std_thickness, avg_ep_u, std_ep_u, avg_dm_u, std_dm_u, mean_surface, std_surface, mean_EDJ, std_EDJ, eval_dir, filename, image_name, indie=False):
    if indie:
        full_path = os.path.join(eval_dir, f"{image_name[:-4]}_analysis.txt")
    else:   
        full_path = os.path.join(eval_dir, f"{filename}.txt")
    
    with open(full_path, 'w') as file:
        file.write(f"Thickness: mean = {mean_thickness:.3f} (um),\t std = {std_thickness:.3f}\n")
        file.write(f"Epidermis: mean = {avg_ep_u:.3f} (1/mm),\t std = {std_ep_u:.3f}\n")
        file.write(f"Dermis: mean = {avg_dm_u:.3f} (1/mm),\t std = {std_dm_u:.3f}\n")
        file.write(f"Surface: mean = {mean_surface:.3f} (um),\t std = {std_surface:.3f}\n")
        file.write(f"EDJ: mean = {mean_EDJ:.3f} (um), \t std = {std_EDJ:.3f}\n")


# 用於並行處理的 worker 函數（需要在全域定義，以便 pickle）
def process_statistics_worker(args):
    """並行處理單張圖像的統計計算"""
    prediction_mask, prediction_img, pixel_size_um_val, fitting_order_val, Refractive_index_epidermis_val = args
    
    # 需要重新匯入函數（多進程需要）
    import numpy as np
    
    # 計算 thickness
    thicknesses = []
    Red_line = np.zeros(prediction_mask.shape[1], dtype=int)
    Blue_line = np.zeros(prediction_mask.shape[1], dtype=int)
    for x in range(prediction_mask.shape[1]): 
        column = prediction_mask[:, x] > 127
        first_pos = np.argmax(column) if np.any(column) else 0
        last_pos = len(column) - np.argmax(column[::-1]) - 1 if np.any(column) else 0
        Red_line[x] = first_pos
        Blue_line[x] = last_pos
        thickness = (last_pos - first_pos) * pixel_size_um_val / Refractive_index_epidermis_val
        thicknesses.append(thickness)
    sorted_thicknesses = np.sort(thicknesses)
    filtered_thicknesses = sorted_thicknesses[70:-73]
    
    # 計算 intensity
    ep_u = np.zeros(prediction_img.shape[1])
    dm_u = np.zeros(prediction_img.shape[1])
    for x in range(prediction_img.shape[1]):
        if Red_line[x] < Blue_line[x]:
            ep_u[x] = np.mean(prediction_img[Red_line[x]:Blue_line[x], x])
        else:
            ep_u[x] = 0
        dm_u[x] = np.mean(prediction_img[Blue_line[x]:Blue_line[x] + 21, x])
    
    # 計算 polyfit
    Red_line_trimmed = Red_line[50:]
    Blue_line_trimmed = Blue_line[50:]
    x = np.arange(0, 894)
    p1 = np.polyfit(x, Red_line_trimmed[:894], fitting_order_val)
    y1 = np.polyval(p1, x)
    p2 = np.polyfit(x, Blue_line_trimmed[:894], fitting_order_val)
    y2 = np.polyval(p2, x)
    upperfit = np.abs(y1 - Red_line_trimmed[:894])
    junctionfit = np.abs(y2 - Blue_line_trimmed[:894])
    surface = np.sort(upperfit)[99:-93]
    EDJ = np.sort(junctionfit)[99:-93]
    surface = surface * pixel_size_um_val / Refractive_index_epidermis_val
    EDJ = EDJ * pixel_size_um_val / Refractive_index_epidermis_val
    
    return filtered_thicknesses, ep_u, dm_u, surface, EDJ, Red_line, Blue_line


# ===== API =====
@app.get("/")
def root():
    return {
        "msg": "TensorRT + Multiprocess + OpenCV 推理服務正在執行",
        "engine_type": "TensorRT",
        "optimizations": ["TensorRT", "Multiprocess", "OpenCV"],
        "engines_loaded": {
            "model_img": os.path.exists(trt_img_path),
            "model_mask": os.path.exists(trt_mask_path)
        }
    }

@app.get("/run/{sub_dir}")
def run_inference(sub_dir: str):
    with nvtx_range(f"FastAPI run_inference: {sub_dir}"):
        t3 = time.perf_counter()

        test_image_dir = os.path.join(test_base_dir, sub_dir)

        base_output_dir = '/home/aboil/jetson_gui/modelresult'
        eval_dir_img   = os.path.join(base_output_dir, 'eval_img')
        eval_dir_mask  = os.path.join(base_output_dir, 'eval_mask')
        eval_dir       = os.path.join(base_output_dir, 'eval_overlay')
        eval_dir_OAC   = os.path.join(base_output_dir, 'eval_OAC')
        eval_dir_anal  = os.path.join(base_output_dir, 'Analysis')
        histogram_dir  = os.path.join(base_output_dir, 'histogram')

        for directory in [eval_dir_img, eval_dir_mask, eval_dir, eval_dir_OAC, eval_dir_anal, histogram_dir]:
            os.makedirs(directory, exist_ok=True)

        if not os.path.exists(test_image_dir):
            return {"error": f"Folder {test_image_dir} does not exist"}

        image_names = sorted([name for name in os.listdir(test_image_dir) if name.endswith('.bin')])

        # 第一階段：完成所有推理（串行，因為 TensorRT 引擎不支援多進程）
        print("第一階段：執行推理...")
        inference_results = []  # 儲存推理結果，用於後續並行處理

        for image_name in image_names:
            with nvtx_range(f"Process Image: {image_name}"):
                image_path = os.path.join(test_image_dir, image_name)

                # 使用 TensorRT 推理（不是 PyTorch）
                prediction_img, max_val, min_val = process_image(image_path, trt_img, eval_dir_img)
                prediction_img = prediction_img * 65535
                output_img = prediction_img.astype(np.uint16)
                output_path_img = os.path.join(eval_dir_img, f"pred_{image_name[:-4]}.pgm")
                cv2.imwrite(output_path_img, output_img)
                print(f"Saved prediction_img to {output_path_img}")
            
                prediction_mask, _, _ = process_image(image_path, trt_mask, eval_dir_mask, is_mask=True)
                prediction_mask = np.where(prediction_mask > 0.5, 255, 0)
                output_mask = prediction_mask.astype(np.uint8) 
                output_path_mask = os.path.join(eval_dir_mask, f"pred_{image_name[:-4]}.png")
                cv2.imwrite(output_path_mask, output_mask)
                print(f"Saved prediction_mask to {output_path_mask}")
                
                # 預處理 prediction_image（用於 overlay）
                prediction_image = prediction_img * 1.76
                prediction_image[prediction_image < 50] = 0
                prediction_image = np.abs(10 * np.log10(prediction_image + 1e-6))
                prediction_image = np.round((prediction_image - 16.8) * 65536 / 30)
                prediction_image = np.nan_to_num(prediction_image, nan=0, posinf=0, neginf=0)
                prediction_image = prediction_image.astype(np.uint16)
                
                output_path_OAC = os.path.join(eval_dir_OAC, f"pred_{image_name[:-4]}.pgm")
                cv2.imwrite(output_path_OAC, prediction_image)
                
                # 儲存推理結果，用於後續並行處理
                inference_results.append({
                    'image_name': image_name,
                    'prediction_img': prediction_img,
                    'prediction_mask': prediction_mask,
                    'prediction_image': prediction_image
                })

        # 第二階段：並行處理統計計算
        print(f"\n第二階段：處理 {len(inference_results)} 張圖像的統計計算...")
        calc_start_time = time.perf_counter()

        # 準備參數
        worker_args = [
            (result['prediction_mask'], result['prediction_img'], pixel_size_um, fitting_order, Refractive_index_epidermis)
            for result in inference_results
        ]

        # 當圖像數量較少時，使用串行處理避免多進程開銷
        # 閾值設為 3：當圖像數 >= 3 時才使用多進程
        if len(worker_args) >= 3:
            # 使用多進程並行處理
            num_workers = min(mp.cpu_count(), len(worker_args), 4)  # 限制進程數，避免過多
            print(f"使用 {num_workers} 個進程並行處理...")
            with Pool(processes=num_workers) as pool:
                results = pool.map(process_statistics_worker, worker_args)
        else:
            # 串行處理（圖像數量少時，多進程開銷大於收益）
            print("圖像數量較少，使用串行處理...")
            results = [process_statistics_worker(args) for args in worker_args]

        calc_end_time = time.perf_counter()
        calc_time = calc_end_time - calc_start_time
        print(f"統計計算完成，耗時: {calc_time:.3f} 秒")

        # 收集結果並產生 overlay 圖像
        all_thicknesses = []
        all_ep_us = []
        all_dm_us = []
        all_surface = []
        all_EDJ = []

        for i, (filtered_thicknesses, ep_u, dm_u, surface, EDJ, Red_line, Blue_line) in enumerate(results):
            result = inference_results[i]
            image_name = result['image_name']
            prediction_image = result['prediction_image']
            
            # 使用 OpenCV 進行快速圖像疊加
            with nvtx_range(f"OpenCV Overlay: {image_name}"):
                output_image = overlay_mask_opencv_optimized(prediction_image, Red_line, Blue_line)
                output_path_overlay = os.path.join(eval_dir, f"overlay_{image_name[:-4]}.png")
                cv2.imwrite(output_path_overlay, output_image)
            
            all_thicknesses.append(filtered_thicknesses)
            all_ep_us.extend(ep_u)
            all_dm_us.extend(dm_u)
            all_surface.extend(surface)
            all_EDJ.extend(EDJ)
            
            # 儲存單張圖像的分析結果
            mean_thickness_d = np.mean(filtered_thicknesses)
            std_thickness_d = np.std(filtered_thicknesses)
            avg_ep_u_d = np.mean(ep_u[50:]) * 1e-3
            std_ep_u_d = np.std(ep_u[50:]) * 1e-3
            avg_dm_u_d = np.mean(dm_u[50:]) * 1e-3
            std_dm_u_d = np.std(dm_u[50:]) * 1e-3
            mean_surface_d = np.mean(surface)
            std_surface_d = np.std(surface)
            mean_EDJ_d = np.mean(EDJ)
            std_EDJ_d = np.std(EDJ)
            save_final_analysis_to_txt(mean_thickness_d, std_thickness_d, avg_ep_u_d, std_ep_u_d, avg_dm_u_d, std_dm_u_d, mean_surface_d, std_surface_d, mean_EDJ_d, std_EDJ_d, eval_dir_anal, sub_dir, image_name, indie=True)
        
        # 計算總體統計
        mean_thickness = np.mean(all_thicknesses)
        std_thickness = np.std(all_thicknesses)
        all_ep_us = np.array(all_ep_us)
        all_dm_us = np.array(all_dm_us)
        avg_ep_u = np.mean(all_ep_us[50:]) * 1e-3
        std_ep_u = np.std(all_ep_us[50:]) * 1e-3
        avg_dm_u = np.mean(all_dm_us[50:]) * 1e-3
        std_dm_u = np.std(all_dm_us[50:]) * 1e-3
        mean_surface = np.mean(all_surface)
        std_surface = np.std(all_surface)
        mean_EDJ = np.mean(all_EDJ)
        std_EDJ = np.std(all_EDJ)
        
        save_final_analysis_to_txt(mean_thickness, std_thickness, avg_ep_u, std_ep_u, avg_dm_u, std_dm_u, mean_surface, std_surface, mean_EDJ, std_EDJ, eval_dir_anal, sub_dir, image_names[-1] if image_names else "", indie=False)
        

        print("\n")
        print(f"Thickness: mean = {mean_thickness:.3f} (um), std = {std_thickness:.3f}")
        print(f"Epidermis: mean = {avg_ep_u:.3f} (1/mm), std = {std_ep_u:.3f}")
        print(f"Dermis: mean = {avg_dm_u:.3f} (1/mm), std = {std_dm_u:.3f}")
        print(f"Surface: mean = {mean_surface:.3f} (um), std = {std_surface:.3f}")
        print(f"EDJ: mean = {mean_EDJ:.3f} (um), std = {std_EDJ:.3f}")
        t4 = time.perf_counter()
        print(f"total time : {t4 - t3:.3f} sec")
        print("=" * 70)
        print("✓ 使用 TensorRT + Multiprocess + OpenCV 完成")
        print("=" * 70)

if __name__ == "__main__":
    print("\n" + "=" * 70)
    print("TensorRT + Multiprocess + OpenCV FastAPI 服務啟動")
    print("=" * 70)
    print("此服務使用：")
    print("  - TensorRT 引擎進行推理")
    print("  - Multiprocess 進行並行統計計算")
    print("  - OpenCV 進行快速圖像疊加")
    print("=" * 70 + "\n")
    uvicorn.run(app, host="0.0.0.0", port=8000)

