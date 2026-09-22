with open('plugin_package/plugin.py', 'r') as f:
    data = f.read()

import re
data = re.sub(r'def check_exit\(self\):.*?def main', '''def check_exit(self):
        global g_imgui_running
        
        # Check for background playback requests
        self.imgui_lib.GetPendingPlayback.restype = ctypes.c_char_p
        pending = self.imgui_lib.GetPendingPlayback()
        if pending:
            ref_str = pending.decode('utf-8')
            print(f"[ImGui] Changing channel to: {ref_str}")
            self.session.nav.playService(eServiceReference(ref_str))
            
            # Close underlying menus (PluginBrowser) so video shows through our transparent background!
            try:
                for dialog in self.session.dialog_stack:
                    if dialog != self and hasattr(dialog, "close"):
                        dialog.close()
            except Exception as e:
                print(f"[ImGui] Failed to close background menus: {e}")
                
        if not g_imgui_running:
            self.timer.stop()
            self.close()

def main''', data, flags=re.DOTALL)

with open('plugin_package/plugin.py', 'w') as f:
    f.write(data)
