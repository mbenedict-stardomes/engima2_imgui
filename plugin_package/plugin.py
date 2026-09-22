from Plugins.Plugin import PluginDescriptor
from enigma import eServiceReference, eServiceCenter
import ctypes
import os

def generate_channel_xml():
    xml = "<bouquets>\n"
    
    serviceHandler = eServiceCenter.getInstance()
    # Enigma2 magic reference for all TV bouquets
    tv_bouquets_ref = eServiceReference('1:7:1:0:0:0:0:0:0:0:(type == 1) || (type == 17) || (type == 195) || (type == 25) FROM BOUQUET "bouquets.tv" ORDER BY bouquet')
    
    bouquet_list = serviceHandler.list(tv_bouquets_ref)
    if bouquet_list is not None:
        while True:
            bouquet_ref = bouquet_list.getNext()
            if not bouquet_ref.valid():
                break
            
            bouquet_name = bouquet_ref.getName()
            bouquet_name = bouquet_name.replace('"', '&quot;').replace('<', '&lt;').replace('>', '&gt;')
            
            xml += f'    <bouquet name="{bouquet_name}">\n'
            
            channel_list = serviceHandler.list(bouquet_ref)
            if channel_list is not None:
                idx = 1
                while True:
                    channel_ref = channel_list.getNext()
                    if not channel_ref.valid():
                        break
                    
                    # Ignore markers (like --- News ---)
                    if not (channel_ref.flags & eServiceReference.isMarker):
                        channel_name = channel_ref.getName()
                        channel_name = channel_name.replace('"', '&quot;').replace('<', '&lt;').replace('>', '&gt;')
                        ref_str = channel_ref.toString()
                        
                        xml += f'        <channel number="{idx}" name="{channel_name}" ref="{ref_str}" />\n'
                        idx += 1
                        
            xml += '    </bouquet>\n'
            
    xml += "</bouquets>\n"
    return xml


import threading
from enigma import eTimer

g_imgui_running = False
g_selected_ref = None

def run_imgui_thread(xml_data, plugin_path):
    global g_imgui_running, g_selected_ref
    
    imgui_lib = ctypes.CDLL(plugin_path)
    imgui_lib.StartImGuiPlugin.restype = ctypes.c_char_p
    imgui_lib.SetChannelDataXML.argtypes = [ctypes.c_char_p]
    
    imgui_lib.SetChannelDataXML(xml_data.encode('utf-8'))
    selected_ref_bytes = imgui_lib.StartImGuiPlugin()
    
    if selected_ref_bytes:
        g_selected_ref = selected_ref_bytes.decode('utf-8')
    else:
        g_selected_ref = None
        
    g_imgui_running = False

class ImGuiMonitor:
    def __init__(self, session):
        self.session = session
        self.timer = eTimer()
        self.timer.callback.append(self.check_exit)
        self.timer.start(500, False)
        
    def check_exit(self):
        global g_imgui_running, g_selected_ref
        if not g_imgui_running:
            self.timer.stop()
            if g_selected_ref:
                print(f"[ImGui] Received playback request for: {g_selected_ref}")
                self.session.nav.playService(eServiceReference(g_selected_ref))
            else:
                print("[ImGui] Exited cleanly to Enigma2.")

def main(session, **kwargs):
    global g_imgui_running, g_selected_ref
    if g_imgui_running:
        return
        
    print("[ImGui] Launching Enigma2 ImGui Prototype in Background Thread...")
    xml_data = generate_channel_xml()
    
    g_selected_ref = None
    g_imgui_running = True
    
    # Start monitor on main thread
    ImGuiMonitor(session)
    
    # Run ImGui on background thread
    plugin_path = os.path.dirname(os.path.realpath(__file__)) + "/libimgui_plugin.so"
    threading.Thread(target=run_imgui_thread, args=(xml_data, plugin_path)).start()


def Plugins(**kwargs):
    return [
        PluginDescriptor(
            name="Stardomes ImGui Prototype",
            description="Native EGL ImGui Interface",
            where=PluginDescriptor.WHERE_PLUGINMENU,
            fnc=main
        )
    ]
