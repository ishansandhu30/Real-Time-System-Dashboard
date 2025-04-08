# Real-Time System Dashboard

An interactive dashboard for visualizing live system metrics and process data streams using OpenGL rendering. Built with C++17, Qt, and modern OpenGL 3.3+, it features multi-threaded data collection, smooth 60fps chart rendering, and configurable monitoring modules.

## Architecture

The project follows a layered architecture separating data collection, rendering, and UI concerns:

```
                     +-----------------------+
                     |     DashboardWindow   |  (QMainWindow - app layer)
                     |   Menu | Status | Grid|
                     +-----+-----+-----+----+
                           |     |     |
              +------------+     |     +-------------+
              |                  |                    |
     +--------v------+  +-------v--------+  +--------v--------+
     |  CpuMonitor   |  | MemoryMonitor  |  |  ProcessTable   |  (widget layer)
     |  (QGroupBox)  |  |  (QGroupBox)   |  |  (QTableView)   |
     +--------+------+  +-------+--------+  +--------+--------+
              |                  |                    |
     +--------v------+  +-------v--------+           |
     | GLChartWidget |  | GLChartWidget  |           |
     | (QOpenGLWidget)|  | (QOpenGLWidget)|           |
     +--------+------+  +-------+--------+           |
              |                  |                    |
     +--------v------------------v--------------------v--------+
     |                   GLRenderer                             |  (rendering layer)
     |  LineGraph  |  BarChart  |  Shaders  |  VAO/VBO mgmt    |
     +-------------------------------------------------------------+
              ^                  ^                    ^
              |                  |                    |
     +--------+---------+-------+---------+----------+---------+
     |            SystemDataCollector (QThread)                 |  (data layer)
     |  DataBuffer<T>  |  CpuMetrics  |  MemoryMetrics         |
     |  ProcessInfo    |  SystemSnapshot                        |
     +-------------------------------------------------------------+
              |
     +--------v---------+
     |  OS APIs          |
     |  macOS: sysctl,   |
     |    mach, libproc   |
     |  Linux: /proc     |
     +-------------------+
```

### Layer Descriptions

- **Data Layer** (`src/data/`): Thread-safe circular buffers, metric structs, and a QThread-based system data collector polling platform-specific APIs.
- **Rendering Layer** (`src/rendering/`): Core OpenGL utilities (VAO/VBO management, shader loading), line graph renderer with auto-scaling and anti-aliasing, and bar chart renderer for comparative views.
- **Widget Layer** (`src/widgets/`): Qt widgets composing OpenGL charts with info labels. GLChartWidget provides the base QOpenGLWidget with projection setup. CPU and memory monitors display real-time data. ProcessTable shows a sortable, searchable process list.
- **App Layer** (`src/app/`): Main window with grid layout, menu bar, settings dialog, and refresh timer.

## Dependencies

- **C++17** compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **Qt 6** (or Qt 5.12+) with modules: Core, Gui, Widgets, OpenGL, OpenGLWidgets
- **OpenGL 3.3+** capable GPU and drivers
- **CMake 3.16+**
- **Python 3.7+** with `pandas`, `matplotlib`, `numpy` (for analysis scripts)

### Platform-Specific

- **macOS**: Uses `mach/mach.h`, `sys/sysctl.h`, `libproc.h` for system metrics.
- **Linux**: Reads `/proc/stat`, `/proc/meminfo`, `/proc/[pid]/stat` for system metrics.

## Build Instructions

```bash
# Clone and navigate to the project
cd Real-Time-System-Dashboard

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
cmake --build . --parallel

# Run
./RealTimeSystemDashboard
```

### macOS with Homebrew Qt

```bash
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
cmake --build . --parallel
```

### Specifying Qt Version

```bash
# Force Qt6
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt6

# Force Qt5
cmake .. -DQt6_DIR="" -DCMAKE_PREFIX_PATH=/path/to/qt5
```

## Usage

### Dashboard Controls

- **File > Export Data**: Export collected metrics to CSV for offline analysis.
- **File > Quit** (`Ctrl+Q`): Exit the application.
- **View > CPU Monitor**: Toggle CPU monitoring panel visibility.
- **View > Memory Monitor**: Toggle memory monitoring panel visibility.
- **View > Process Table**: Toggle process list visibility.
- **Settings > Configure** (`Ctrl+,`): Open the settings dialog.

### Settings Dialog

Configure:
- **Polling interval**: How frequently system metrics are sampled (100ms - 10s).
- **Process polling**: How frequently the process list updates (500ms - 30s).
- **Buffer size**: Number of historical samples retained (50 - 10,000).
- **Refresh rate**: UI render target (10 - 144 fps).
- **Module visibility**: Show/hide individual monitoring panels.
- **Color theme**: Customize chart colors.

### Python Tools

Generate synthetic test data:
```bash
python python/data_generator.py --duration 300 --cores 8 --output test_data.csv
```

Analyze exported metrics:
```bash
python python/analyze_metrics.py test_data.csv --output-dir ./analysis
```

## Configuration

Default settings are stored in `config/dashboard.json`. The application searches for this file in:
1. `./config/dashboard.json` (relative to working directory)
2. `<app_dir>/config/dashboard.json` (relative to executable)
3. `~/.config/system-dashboard/dashboard.json` (user config)

## Project Structure

```
Real-Time-System-Dashboard/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── config/
│   └── dashboard.json          # Default configuration
├── python/
│   ├── data_generator.py       # Synthetic metrics generator
│   └── analyze_metrics.py      # Offline metrics analysis
├── resources/
│   └── shaders/
│       ├── line.vert / .frag   # Line graph shaders
│       └── bar.vert / .frag    # Bar chart shaders
└── src/
    ├── main.cpp                # Application entry point
    ├── app/
    │   ├── dashboard.h/cpp     # Main window
    │   └── settings_dialog.h/cpp
    ├── data/
    │   ├── data_buffer.h       # Thread-safe circular buffer
    │   ├── metrics.h           # Metric data structures
    │   └── system_data_collector.h/cpp
    ├── rendering/
    │   ├── gl_renderer.h/cpp   # Core OpenGL utilities
    │   ├── line_graph.h/cpp    # Line graph renderer
    │   └── bar_chart.h/cpp     # Bar chart renderer
    └── widgets/
        ├── gl_chart_widget.h/cpp
        ├── cpu_monitor.h/cpp
        ├── memory_monitor.h/cpp
        └── process_table.h/cpp
```

## License

MIT
