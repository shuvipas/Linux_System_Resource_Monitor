# Linux System Resource Monitor

A real-time system monitoring tool that tracks CPU, RAM, and GPU usage across processes with intelligent grouping and terminal display.

![System Monitor](https://img.shields.io/badge/platform-Linux-blue) ![C++](https://img.shields.io/badge/language-C%2B%2B17-orange) ![NVIDIA](https://img.shields.io/badge/GPU-NVIDIA-green)

## Features

- **Real-time System Monitoring**
  - CPU usage (system-wide and per-process)
  - RAM utilization with used percentage
  - Process grouping by command name
  - Groups similar processes for cleaner display
  - Filters out low-resource processes
  
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




### TODO
- todo make the terminal not "blink"
- todo make option to install and use systm wide as a bash cmd