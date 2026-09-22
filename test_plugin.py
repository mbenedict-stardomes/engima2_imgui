import ctypes
import sys

print("Loading ImGui plugin using ctypes...")
try:
    plugin = ctypes.CDLL("./libimgui_plugin.so")
    print("Plugin loaded successfully.")
    
    print("Starting ImGui UI Loop (Press EXIT or Power on your remote to quit)...")
    plugin.StartImGuiPlugin()
    
    print("ImGui UI Loop cleanly exited! Handing control back to Python.")
except Exception as e:
    print(f"Failed to load or run plugin: {e}")
    sys.exit(1)
