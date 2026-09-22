import ctypes
import sys

print("Loading ImGui plugin using ctypes...")
try:
    plugin = ctypes.CDLL("./libimgui_plugin.so")
    print("Plugin loaded successfully.")
    
    # The C++ function returns a const char*
    plugin.StartImGuiPlugin.restype = ctypes.c_char_p
    
    print("Starting ImGui UI Loop (Press EXIT or Power on your remote to quit)...")
    selected_channel_ref = plugin.StartImGuiPlugin()
    
    if selected_channel_ref:
        # decode the bytes to string
        ref_str = selected_channel_ref.decode('utf-8')
        print("=======================================")
        print(f"ImGui exited with channel playback request!")
        print(f"Enigma2 Service Reference: {ref_str}")
        print(f"Enigma2 Python would now execute: self.session.nav.playService(eServiceReference('{ref_str}'))")
        print("=======================================")
    else:
        print("ImGui UI Loop cleanly exited! Handing control back to Python.")
except Exception as e:
    print(f"Failed to load or run plugin: {e}")
    sys.exit(1)
