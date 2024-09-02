#!/bin/sh

set -e

if [ -n "${HAZEL_DIR+set}" ]
	then
		true
	else
		export HAZEL_DIR=$(realpath .)
fi

if [ -n "${BUILD_CONFIG+set}" ]
	then
		true
	else
		export BUILD_CONFIG=Debug
fi

if [ -n "${VULKAN_SDK+set}" ]
	then
		true
	else
		export VULKAN_SDK=$(realpath Hazel/vendor/VulkanSDK/x86_64)
fi

# Build Coral.Managed and Script Core
	premake5 vs2022 --file=Hazel-ScriptCore/premake5-dotnet.lua
	dotnet build -c $BUILD_CONFIG --property WarningLevel=0 Hazel-ScriptCore/Hazel-ScriptCore.sln

# Copy Coral Files
	CORAL_DIR=$HAZEL_DIR/Hazel/vendor/Coral
	HAZELNUT_DIR=$HAZEL_DIR/Hazelnut
	DOTNET_DIR=$HAZELNUT_DIR/DotNet

	mkdir -p $DOTNET_DIR

	cp $CORAL_DIR/Coral.Managed/Coral.Managed.runtimeconfig.json $DOTNET_DIR/Coral.Managed.runtimeconfig.json
	cp $CORAL_DIR/Build/$BUILD_CONFIG/Coral.Managed.dll $DOTNET_DIR/Coral.Managed.dll
	cp $CORAL_DIR/Build/$BUILD_CONFIG/Coral.Managed.pdb $DOTNET_DIR/Coral.Managed.pdb
	cp $CORAL_DIR/Build/$BUILD_CONFIG/Coral.Managed.deps.json $DOTNET_DIR/Coral.Managed.deps.json

# Build Sandbox
	premake5 vs2022 --file=Hazelnut/SandboxProject/premake5.lua
	dotnet build -c $BUILD_CONFIG --property WarningLevel=0 Hazelnut/SandboxProject/Sandbox.sln

# Build Hazel
	premake5 gmake2 --cc=clang --verbose
	# TODO: `config=$BUILD_CONFIG` need to lowercase
	make "$@"
