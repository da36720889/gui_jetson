# Jetson GUI - SDOCT Image Acquisition and Processing System

A Qt-based graphical user interface application for controlling USB devices to acquire, process, and display SDOCT (Spectral Domain Optical Coherence Tomography) images. This system provides real-time image acquisition, visualization, and AI-powered analysis capabilities.

## Features

- **USB Device Control**: Communicate with USB devices through CYUSB library with automatic device initialization
- **Multiple Scan Modes**: Support for 2D, 3D, Cross, and Wang scan modes
- **Real-time Image Display**: Four display windows for real-time visualization:
  - Display 1: Real-time B-scan images with 90° rotation
  - Display 2: B-scan sequence browsing with slider control
  - Display 3: Enface view with interactive navigation
  - Display 4: AI inference results with overlay visualization
- **Data Capture and Save**: Capture image sequences (120 frames) and save raw float32 data as binary files
- **AI Inference Integration**: Integrated TensorRT inference service (auto-started) for image analysis and quantitative evaluation
- **Multi-threaded Architecture**: Separate threads for scanning, capture, and GUI updates for optimal performance

## System Requirements

### Hardware Requirements
- **NVIDIA Jetson Xavier NX 16GB** (tested and recommended)
- JetPack 5.x
- Supported USB device (CYUSB compatible)

### Software Dependencies
- **JetPack 5.x** (includes CUDA, cuDNN, TensorRT)
- **Qt 5** (Note: This project was originally designed for Qt 4, but JetPack 5.x only provides Qt 5. Users need to adapt Qt 5 code to meet Qt 4 requirements)
- CMake or qmake
- C++17 compiler (GCC 9.4+)
- libusb-1.0
- CYUSB library

### Python Environment (for inference service)
- Python 3.8+ (included in JetPack 5.x)
- TensorRT (included in JetPack 5.x)
- FastAPI
- OpenCV (may need to install separately or use JetPack's version)
- NumPy
- Albumentations

## Project Structure

```
jetson_gui/
├── main.cpp                 # Main program entry point
├── mainwindow.h             # Main window header file
├── mainwindow.cpp           # Main window implementation
├── mainwindow.ui            # Qt UI design file
├── jetson_gui.pro           # Qt project file
├── include/                 # Header files directory
│   └── cyusb.h             # CYUSB library header
├── lib/                     # Library files directory
│   ├── libcyusb.cpp        # CYUSB library implementation
│   └── libcyusb.so.1       # CYUSB dynamic library
├── configs/                 # Configuration files
│   ├── 88-cyusb.rules      # USB device rules file
│   ├── cy_renumerate.sh    # USB device re-enumeration script
│   └── cyusb.conf          # CYUSB configuration file
└── workspace/               # Workspace directory
    └── Inference/          # Inference service
        ├── eval_fastapi_tensorrt_multiprocess_opencv.py
        └── overlay_utils.py
```

## Building and Installation

### 1. Install Dependencies

#### Jetson Xavier NX (JetPack 5.x)
```bash
sudo apt-get update
sudo apt-get install qtbase5-dev qttools5-dev libusb-1.0-0-dev build-essential
```

**Important Notes**:
- JetPack 5.x already includes:
  - CUDA toolkit
  - TensorRT
  - cuDNN
  - Python 3.8+ (check with `python3 --version`)
- **Qt Version Compatibility**: This project was originally designed for Qt 4, but JetPack 5.x only provides Qt 5. You may need to modify Qt 5 code to meet Qt 4 requirements. The project file (`jetson_gui.pro`) includes compatibility checks, but some manual adjustments may be necessary.

#### Configure USB Device Permissions
```bash
sudo cp configs/88-cyusb.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### 2. Build the Project

Using qmake:
```bash
qmake jetson_gui.pro
make
```

Or using Qt Creator:
1. Open `jetson_gui.pro`
2. Configure the build kit
3. Build the project

### 3. Run

```bash
./jetson_gui
```

**Note**: The FastAPI inference service will be automatically started in the background when the application launches.

## Qt Version Compatibility

### Qt 4 to Qt 5 Migration

This project was originally designed for **Qt 4**, but **JetPack 5.x only provides Qt 5**. The project file includes basic compatibility checks, but you may need to make manual adjustments:

1. **Install Qt 5 packages**:
   ```bash
   sudo apt-get install qtbase5-dev qttools5-dev
   ```

2. **Common compatibility issues to check**:
   - Signal/slot syntax: Qt 5 uses the same syntax, but some deprecated macros may need updating
   - Module dependencies: Qt 5 uses a modular approach (ensure `QT += widgets` is present)
   - Header includes: Some Qt 4 headers may need updating
   - Deprecated APIs: Review compiler warnings for deprecated Qt 4 APIs

3. **Project file**: The `jetson_gui.pro` file includes `greaterThan(QT_MAJOR_VERSION, 4): QT += widgets` which automatically adds widgets module for Qt 5+

4. **If you encounter issues**: Check the compilation errors and update deprecated Qt 4 code to Qt 5 equivalents as needed.

## Usage

### Basic Workflow

1. **Initialize Device**
   - Click the "Initialize" button to initialize the USB device

2. **Select Scan Mode**
   - 2D: Two-dimensional scan mode
   - 3D: Three-dimensional scan mode (scan mode only, no 3D visualization)
   - Cross: Cross scan mode
   - Wang: Specific scan mode

3. **Start Scanning**
   - Click "Start" to begin real-time preview
   - Click "Stop" to stop scanning
   - Click "Capture" to capture image sequences

4. **View Data**
   - Display 1: Real-time B-scan images
   - Display 2: B-scan sequence browsing (using X slider)
   - Display 3: Enface view (using Y slider)
   - Display 4: AI inference results

5. **Save Data**
   - Enter filename in the save path input box
   - Click "Save" to save raw data as binary file
   - Data is saved to `/home/aboil/jetson_gui/raw_data/[filename].bin`
   - Format: Raw float32 little-endian binary data

6. **Quantitative Analysis**
   - Click "Quantize" button to trigger AI inference (FastAPI service must be running)
   - The service processes data and generates:
     - Overlay images saved to `modelresult/eval_overlay/`
     - Analysis results saved to `modelresult/Analysis/`
   - Results are automatically displayed in Display 4 and a dialog window

### USB Device Configuration

If you encounter USB device connection issues, please check:

1. Whether USB device rules are correctly installed
2. Whether the user is in the `dialout` or related user group
3. Whether the device is occupied by kernel driver (needs detach)

```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Re-login for changes to take effect
```

## Python Inference Service

### Automatic Startup

The FastAPI inference service is automatically started when the GUI application launches. The service runs in the background and communicates with the GUI through HTTP requests.

### Manual Startup (if needed)

If you need to start the service manually:

```bash
cd workspace/Inference
python3 eval_fastapi_tensorrt_multiprocess_opencv.py
```

The service runs on `http://0.0.0.0:8000` by default.

### API Endpoints

- `POST /run/test`: Execute inference and analysis
  - Processes raw data from `quant_raw/` directory
  - Generates overlay images and analysis results
  - Returns results to GUI for display

### Output Directories

- `modelresult/eval_overlay/`: Generated overlay images (PNG format)
- `modelresult/Analysis/`: Quantitative analysis results (TXT format)

## Development Notes

### Code Structure

- **Main Thread**: GUI event handling and display updates
- **StartThread**: Real-time scanning data acquisition thread (continuous loop)
- **CaptureThread**: Image sequence capture thread (120 frames)
- **Python FastAPI Process**: Background inference service (auto-started)
- **Data Flow**: USB → Raw Data → Float32 Conversion → Image Display

### Key Parameters

- `GUI_display_height`: Display height (default: 1000, max: 4096)
- `GUI_display_width`: Display width (default: 2048, max: 1000)
- Image dimensions: 1024 × 1000 pixels
- Data packet size: 4,096,000 bytes (1024 × 1000 × 4 bytes float32)
- Capture frames: 120 frames per capture sequence
- USB endpoints:
  - Output endpoint: 0x01
  - Input endpoint: 0x81

### Data Format

- **Input**: Raw USB bulk transfer data (4,096,000 bytes per frame)
- **Processing**: Converted from byte array to float32 little-endian
- **Display**: Scaled to 8-bit grayscale images (0-255)
- **Storage**: Binary float32 format (`.bin` files)

## Troubleshooting

### USB Device Not Found
- Check USB connection
- Verify device rules are installed
- Check user permissions

### Image Display Issues
- Check if data packet size is correct
- Verify display parameter settings
- Check debug output

### Inference Service Connection Failed
- Check if FastAPI service started automatically (check application logs)
- Verify service is running: `curl http://0.0.0.0:8000/run/test`
- Check if port 8000 is occupied: `netstat -tuln | grep 8000`
- View service logs in the application console output
- Ensure Python dependencies are installed correctly
- Check if TensorRT model files exist in the expected paths

### Application Crashes on Startup
- Verify USB device is connected
- Check USB device permissions
- Ensure CYUSB library is properly linked
- Check Qt installation and version compatibility
- **Qt 4 to Qt 5 Migration**: If you encounter compilation errors, you may need to:
  - Update deprecated Qt 4 APIs to Qt 5 equivalents
  - Check signal/slot syntax compatibility
  - Verify module dependencies (Qt 5 uses modular approach)
  - Review the project file (`jetson_gui.pro`) for Qt version-specific settings

## Directory Structure

The application expects the following directory structure:

```
/home/aboil/jetson_gui/
├── raw_data/              # Raw data storage (created automatically)
├── quant_raw/             # Input data for inference service
├── modelresult/           # Inference results
│   ├── eval_overlay/     # Overlay images (PNG)
│   └── Analysis/         # Analysis results (TXT)
└── workspace/Inference/  # Inference service scripts
```

## Notes

- The application automatically initializes the USB device on startup
- USB device handle is managed globally and shared across threads
- Image rotation (90°) is applied for proper display orientation
- The capture mode selects random frames for model inference
- All data processing uses little-endian byte order
- **Qt Version**: Originally designed for Qt 4, but adapted for Qt 5 on JetPack 5.x. Some manual code adjustments may be required for full compatibility.

## Author

Y.T. Tsai

## Version History

- v0.01 - Initial version
