# Zong Engine [![License](https://img.shields.io/github/license/zong4/ZongEngine.svg)](https://github.com/zong4/ZongEngine/blob/master/LICENSE)

![Zong Engine](/Resources/Branding/icon_full.jpg?raw=true "Zong Engine")

Zong Engine will be built as a better 3D game engine from [Hazel Engine](https://github.com/TheCherno/Hazel).

***

## Getting started

### Windows

<ins>**1. Downloading the repository:**</ins>

Start by cloning the repository with `git clone --recursive https://github.com/zong4/ZongEngine`.

If the repository was cloned non-recursively previously, use `git submodule update --init` to clone the necessary submodules.

<ins>**2. Configuring the dependencies:**</ins>

1. Run the [Setup.bat](https://github.com/zong4/ZongEngine/blob/master/scripts/Setup.bat) file found in `scripts` folder. This will download the required prerequisites for the project if they are not present yet.
2. One prerequisite is the Vulkan SDK. If it is not installed, the script will execute the [`VulkanSDK.exe`](https://sdk.lunarg.com/sdk/download/1.3.224.1/windows/VulkanSDK-1.3.224.1-Installer.exe) file, and will prompt the user to install the SDK.
3. After installation, run the [Setup.bat](https://github.com/zong4/ZongEngine/blob/master/scripts/Setup.bat) file again. If the Vulkan SDK is installed properly, it will then download the Vulkan SDK Debug libraries. (This may take a longer amount of time)
4. After downloading and unzipping the files, the [Win-GenProjects.bat](https://github.com/zong4/ZongEngine/blob/master/scripts/Win-GenProjects.bat) script file will get executed automatically, which will then generate a Visual Studio solution file for user's usage.

If changes are made, or if you want to regenerate project files, rerun the [Win-GenProjects.bat](https://github.com/zong4/ZongEngine/blob/master/scripts/Win-GenProjects.bat) script file found in `scripts` folder.

### Linux

It's on the way.