#!/usr/bin/env bash
set -euo pipefail

# Ensure the script is run with root privileges
if [ "$EUID" -ne 0 ]; then
  echo "==> Re-running script with sudo..."
  exec sudo bash "$0" "$@"
fi

echo "=========================================="
echo " Current ZRAM & Swap Status"
echo "=========================================="
swapon --show || true
free -h
echo ""

echo "==> Configuring /etc/systemd/zram-generator.conf for 50 GB..."
cat << 'EOF' > /etc/systemd/zram-generator.conf
[zram0]
compression-algorithm = zstd
zram-size = 51200
swap-priority = 100
fs-type = swap
EOF

echo "==> Reloading systemd configuration..."
swapoff /dev/zram0 || true
systemctl daemon-reload
systemctl restart systemd-zram-setup@zram0.service
swapon /dev/zram0 || swapon -a

echo ""
echo "=========================================="
echo " Updated ZRAM & Swap Status"
echo "=========================================="
swapon --show
free -h
echo ""
zramctl /dev/zram0
echo ""
echo "==> ZRAM successfully resized to 50 GB!"
