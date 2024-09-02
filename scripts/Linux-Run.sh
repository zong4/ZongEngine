#!/bin/sh

# Build Hazel
export HAZEL_DIR=`realpath .`
export VULKAN_SDK=`realpath Hazel/vendor/VulkanSDK/x86_64`
export LD_LIBRARY_PATH="$VULKAN_SDK/lib:$HAZEL_DIR/Hazel/vendor/NvidiaAftermath/lib/x64/linux"

cd Hazelnut
$_EXEC $HAZEL_DIR/bin/Debug-linux-x86_64/Hazelnut/Hazelnut $@
