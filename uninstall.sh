#!/bin/bash
# Uninstall gtop system monitor

echo "Uninstalling gtop system monitor..."

# Remove the binary from /usr/local/bin
if [ -f "/usr/local/bin/gtop" ]; then
    sudo rm /usr/local/bin/gtop
    echo "Removed /usr/local/bin/gtop"
else
    echo "gtop not found in /usr/local/bin"
fi

# Check if compiled binary exists in current directory
if [ -f "gtop" ]; then
    read -p "Remove local compiled binary 'gtop' from this directory? (y/n): " choice
    if [[ "$choice" =~ ^[Yy]$ ]]; then
        rm gtop
        echo "Removed local gtop binary"
    fi
fi

echo "Uninstallation complete!"