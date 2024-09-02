#!/bin/sh

# TODO(Emily): This file does not respect `HAZEL_DIR` atm.

set -e

NAME=hazel:latest

# TODO(Emily): These should be environment overridable
VULKAN_VER=1.3.283.0
BUILD_CONFIG=Debug
ARCH=x86_64

docker build --build-arg BUILD_CONFIG=$BUILD_CONFIG --build-arg VULKAN_VER=$VULKAN_VER -t $NAME "$@" .

# Copy out the artifacts we want to keep
mkdir -p bin/x86_64/bin
mkdir -p Hazelnut/Resources/Scripts
mkdir -p Hazelnut/SandboxProject/Assets/Scripts/Binaries
IMG=$(docker create $NAME)
WKS=$IMG:/workspace
	# TODO(Emily): Copy out other components
	docker cp $WKS/bin/$BUILD_CONFIG-linux-x86_64/Hazelnut/Hazelnut bin/Hazelnut
	
	docker cp $WKS/$VULKAN_VER/x86_64/lib/libdxcompiler.so bin/
	docker cp $WKS/$VULKAN_VER/x86_64/bin/dxc bin/x86_64/bin/
	docker cp $WKS/$VULKAN_VER/x86_64/bin/dxc-3.7 bin/x86_64/bin/
	docker cp $WKS/Hazel/vendor/NvidiaAftermath/lib/x64/linux/libGFSDK_Aftermath_Lib.so bin/
	docker cp $WKS/Hazel/vendor/NvidiaAftermath/lib/x64/linux/libGFSDK_Aftermath_Lib.x64.so bin/

	docker cp $WKS/Hazelnut/DotNet Hazelnut/DotNet
	docker cp $WKS/Hazelnut/Resources/Scripts/Hazel-ScriptCore.dll Hazelnut/Resources/Scripts/Hazel-ScriptCore.dll

	docker cp $WKS/Hazelnut/SandboxProject/Assets/Scripts/Binaries/Sandbox.dll Hazelnut/SandboxProject/Assets/Scripts/Binaries/Sandbox.dll
docker rm $IMG
