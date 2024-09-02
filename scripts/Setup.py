import os
import subprocess
import CheckPython

# Make sure everything we need is installed
CheckPython.ValidatePackages()

import Vulkan
import Utils

import colorama
from colorama import Fore
from colorama import Back
from colorama import Style


# Change from Scripts directory to root
os.chdir('../')

colorama.init()

if (not Vulkan.CheckVulkanSDK()):
    print("Vulkan SDK not installed.")
    exit()
    
if (Vulkan.CheckVulkanSDKDebugLibs()):
    print(f"{Style.BRIGHT}{Back.GREEN}Vulkan SDK debug libs located.{Style.RESET_ALL}")

subprocess.call(["git", "lfs", "pull"])
subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

if (Utils.IsRunningAsAdmin()):
    Utils.CreateSymlink('Hazel/vendor/mono/lib/windows/Dist', 'Release')
else:
    print(f"{Style.BRIGHT}{Back.YELLOW}Re-run as admin to create symlinks required for Dist builds.{Style.RESET_ALL}")

print(f"{Style.BRIGHT}{Back.GREEN}Generating Visual Studio 2022 solution.{Style.RESET_ALL}")
subprocess.call(["vendor/bin/premake5.exe", "vs2022"])
os.chdir('Hazelnut/SandboxProject')
subprocess.call(["../../vendor/bin/premake5.exe", "vs2022"])