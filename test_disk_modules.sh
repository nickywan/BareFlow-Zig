#!/bin/bash
# Test disk module loading - non-interactive with serial output

timeout --foreground 15 qemu-system-i386 \
  -drive format=raw,file=fluid.img \
  -drive format=raw,file=build/fat16_test.img,index=1,media=disk \
  -m 64M \
  -serial stdio \
  -display none \
  2>&1
