set -o errexit
cd $(dirname -- "$0"; )
cd ..

MODE=$1
if [[ $MODE == "debug" ]] then
  echo "[xv6] Mode is 'gdb'" 
  TARGET="qemu-gdb"
else
  echo "[xv6] Mode is 'run'" 
  TARGET="qemu"
fi 

echo "[xv6] Running XV6 OS..."
bear -- make $TARGET && make clean
echo "[xv6] Done!"
