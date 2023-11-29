set -o errexit
cd $(dirname -- "$0"; )
cd ..

echo "[xv6] Running XV6 OS..."
bear -- make qemu && make clean
echo "[xv6] Done!"
