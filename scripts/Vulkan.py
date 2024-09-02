import os
import subprocess
import sys
from pathlib import Path

import Utils

from io import BytesIO
from urllib.request import urlopen
from zipfile import ZipFile

import colorama
from colorama import Fore
from colorama import Back
from colorama import Style

VULKAN_SDK = os.environ.get('VULKAN_SDK')
HAZEL_REQUIRED_VULKAN_VERSION = '1.3.' # Any 1.3 version is fine
HAZEL_INSTALL_VULKAN_VERSION = '1.3.216.0' # Specifically install this one if no 1.3 version is present
VULKAN_SDK_INSTALLER_URL = f'https://sdk.lunarg.com/sdk/download/{HAZEL_INSTALL_VULKAN_VERSION}/windows/VulkanSDK-{HAZEL_INSTALL_VULKAN_VERSION}-Installer.exe'
VULKAN_SDK_LOCAL_PATH = 'Hazel/vendor/VulkanSDK'
VULKAN_SDK_EXE_PATH = f'{VULKAN_SDK_LOCAL_PATH}/VulkanSDK.exe'

colorama.init()

def InstallVulkanSDK():
    Path(VULKAN_SDK_LOCAL_PATH).mkdir(parents=True, exist_ok=True)
    print('Downloading {} to {}'.format(VULKAN_SDK_INSTALLER_URL, VULKAN_SDK_EXE_PATH))
    Utils.DownloadFile(VULKAN_SDK_INSTALLER_URL, VULKAN_SDK_EXE_PATH)
    print("Running Vulkan SDK installer...")
    print(f"{Style.BRIGHT}{Back.YELLOW}Make sure to install shader debug libs if you want to build in Debug!")
    print(f"Follow instructions on https://docs.hazelengine.com/GettingStarted#vulkan{Style.RESET_ALL}")
    os.startfile(os.path.abspath(VULKAN_SDK_EXE_PATH))
    print(f"{Style.BRIGHT}{Back.RED}Re-run this script after installation{Style.RESET_ALL}")

def InstallVulkanPrompt():
    print("Would you like to install the Vulkan SDK?")
    install = Utils.YesOrNo()
    if (install):
        InstallVulkanSDK()
        quit()

def CheckVulkanSDK():
    if (VULKAN_SDK is None):
        print(f"{Style.BRIGHT}{Back.RED}You don't have the Vulkan SDK installed!{Style.RESET_ALL}")
        InstallVulkanPrompt()
        return False
    elif (HAZEL_REQUIRED_VULKAN_VERSION not in VULKAN_SDK):
        print(f"Located Vulkan SDK at {VULKAN_SDK}")
        print(f"{Style.BRIGHT}{Back.RED}You don't have the correct Vulkan SDK version! (Hazel requires {HAZEL_REQUIRED_VULKAN_VERSION}){Style.RESET_ALL}")
        InstallVulkanPrompt()
        return False
    
    print(f"{Style.BRIGHT}{Back.GREEN}Correct Vulkan SDK located at {VULKAN_SDK}{Style.RESET_ALL}")
    return True

def CheckVulkanSDKDebugLibs():
    shadercdLib = Path(f"{VULKAN_SDK}/Lib/shaderc_sharedd.lib")
    if (not shadercdLib.exists()):
        print(f"{Style.BRIGHT}{Back.YELLOW}Warning: No Vulkan SDK debug libs found. (Checked {shadercdLib})")
        print(f"{Back.RED}Debug builds are not possible.{Style.RESET_ALL}")
        print("Please install Vulkan SDK debug libs as shown in https://docs.hazelengine.com/GettingStarted#vulkan")
        return False

    return True