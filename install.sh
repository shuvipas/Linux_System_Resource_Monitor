#!/bin/bash
g++ -o gtop system_monitor.cpp
sudo cp gtop /usr/local/bin/
echo "system_monitor installed! You can now run it with 'gtop' from anywhere"