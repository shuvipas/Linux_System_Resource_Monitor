# Linux System Resource Monitor

A real-time system monitoring tool that tracks CPU, RAM, and GPU usage across processes with intelligent grouping and terminal display.

![System Monitor](https://img.shields.io/badge/platform-Linux-blue) ![C++](https://img.shields.io/badge/language-C%2B%2B17-orange) ![NVIDIA](https://img.shields.io/badge/GPU-NVIDIA-green)

## Features

- **Real-time System Monitoring**
  - CPU usage (system-wide and per-process)
  - RAM utilization with used percentage
  - Process grouping by command name
  - Filters out low-resource processes
  - Groups similar processes for cleaner display
    - System Processes: Combined into "system_processes" group
    - User Processes: Grouped by command name 


- **NVIDIA GPU Support**
  - GPU utilization monitoring
  - VRAM usage tracking
  - GPU process detection
  - Driver and GPU model detection
  

## Requirements

- **Linux** (uses `/proc` filesystem)
- **C++17** compatible compiler
- **NVIDIA GPU** (optional, for GPU monitoring)
- **nvidia-smi** (for GPU statistics)

## Installation
To install the program as a bash commend "gtop" system-wide.
Clone the repository and run the installation script:

```bash
git clone https://github.com/shuvipas/Linux_System_Resource_Monitor
cd linux-system-monitor
chmod +x install.sh
sudo ./install.sh
```
To uninstall:
```bash
chmod +x uninstall.sh
sudo ./uninstall.sh
```




### TODO
- todo make the terminal not "blink"
- todo make option to install and use systm wide as a bash cmd
- add system network usege
- optimize
- better grouping (if the info isnt in \proc use a module and ioctl)
