from Plugins.Plugin import PluginDescriptor
from Screens.Screen import Screen
from Components.ActionMap import ActionMap
from enigma import eServiceReference, eServiceCenter, eTimer
import ctypes
import os
import threading

def generate_channel_xml():
    xml = "<bouquets>\n"
    serviceHandler = eServiceCenter.getInstance()
    tv_bouquets_ref = eServiceReference('1:7:1:0:0:0:0:0:0:0:(type == 1) || (type == 17) || (type == 195) || (type == 25) FROM BOUQUET "bouquets.tv" ORDER BY bouquet')
    
    # 1. Fetch standard bouquets (Favourites)
    bouquet_list = serviceHandler.list(tv_bouquets_ref)
    if bouquet_list is not None:
        while True:
            bouquet_ref = bouquet_list.getNext()
            if not bouquet_ref.valid():
                break
                
            info = serviceHandler.info(bouquet_ref)
            bouquet_name = info.getName(bouquet_ref) if info else bouquet_ref.getName()
            bouquet_name = bouquet_name.replace('"', '&quot;').replace('<', '&lt;').replace('>', '&gt;')
            xml += f'    <bouquet name="{bouquet_name}">\n'
            
            channel_list = serviceHandler.list(bouquet_ref)
            if channel_list is not None:
                idx = 1
                while True:
                    channel_ref = channel_list.getNext()
                    if not channel_ref.valid():
                        break
                    if not (channel_ref.flags & eServiceReference.isMarker):
                        info = serviceHandler.info(channel_ref)
                        channel_name = info.getName(channel_ref) if info else channel_ref.getName()
                        if not channel_name: channel_name = "Unknown Channel"
                        channel_name = channel_name.replace('"', '&quot;').replace('<', '&lt;').replace('>', '&gt;').replace('&', '&amp;')
                        ref_str = channel_ref.toString()
                        xml += f'        <channel number="{idx}" name="{channel_name}" ref="{ref_str}" />\n'
                        idx += 1
            xml += '    </bouquet>\n'
            
    # 2. Fetch Satellites (Tuners)
    sat_ref = eServiceReference('1:7:2:0:0:0:0:0:0:0:(type == 1) || (type == 17) || (type == 195) || (type == 25) FROM SATELLITES ORDER BY satellitePosition')
    sat_list = serviceHandler.list(sat_ref)
    if sat_list is not None:
        while True:
            folder_ref = sat_list.getNext()
            if not folder_ref.valid():
                break
            
            info = serviceHandler.info(folder_ref)
            folder_name = info.getName(folder_ref) if info else folder_ref.getName()
            folder_name = f"[Sat] {folder_name}".replace('"', '&quot;').replace('<', '&lt;').replace('>', '&gt;')
            xml += f'    <bouquet name="{folder_name}">\n'
            
            channel_list = serviceHandler.list(folder_ref)
            if channel_list is not None:
                idx = 1
                while True:
                    channel_ref = channel_list.getNext()
                    if not channel_ref.valid():
                        break
                    if not (channel_ref.flags & eServiceReference.isMarker):
                        info = serviceHandler.info(channel_ref)
                        channel_name = info.getName(channel_ref) if info else channel_ref.getName()
                        if not channel_name: channel_name = "Unknown Channel"
                        channel_name = channel_name.replace('"', '&quot;').replace('<', '&lt;').replace('>', '&gt;').replace('&', '&amp;')
                        ref_str = channel_ref.toString()
                        xml += f'        <channel number="{idx}" name="{channel_name}" ref="{ref_str}" />\n'
                        idx += 1
            xml += '    </bouquet>\n'
            
    # 3. Fetch ALL channels (Alphabetical)
    all_ref = eServiceReference('1:7:1:0:0:0:0:0:0:0:(type == 1) || (type == 17) || (type == 195) || (type == 25) ORDER BY name')
    all_list = serviceHandler.list(all_ref)
    if all_list is not None:
        xml += '    <bouquet name="All Channels (A-Z)">\n'
        idx = 1
        while True:
            channel_ref = all_list.getNext()
            if not channel_ref.valid():
                break
            if not (channel_ref.flags & eServiceReference.isMarker):
                info = serviceHandler.info(channel_ref)
                channel_name = info.getName(channel_ref) if info else channel_ref.getName()
                if not channel_name: channel_name = "Unknown Channel"
                channel_name = channel_name.replace('"', '&quot;').replace('<', '&lt;').replace('>', '&gt;').replace('&', '&amp;')
                ref_str = channel_ref.toString()
                xml += f'        <channel number="{idx}" name="{channel_name}" ref="{ref_str}" />\n'
                idx += 1
        xml += '    </bouquet>\n'

    xml += "</bouquets>\n"
    
    # Save a debug copy just to be sure!
    try:
        with open("/tmp/imgui_channels.xml", "w") as f:
            f.write(xml)
    except:
        pass
        
    return xml

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

class ImGuiHostScreen(Screen):
    # This screen covers Enigma2 with a solid black background, hiding the Plugin Browser!
    # backgroundColor="#000000" in Enigma2 means opaque black.
    skin = """
        <screen name="ImGuiHostScreen" position="0,0" size="1920,1080" flags="wfNoBorder" backgroundColor="#000000">
        </screen>
    """
    
    def __init__(self, session):
        Screen.__init__(self, session)
        
        # High priority ActionMap to steal ALL remote control presses from Enigma2!
        self["actions"] = ActionMap(["DirectionActions", "OkCancelActions", "ColorActions", "NumberActions"], {
            "ok": self.key_ok,
            "cancel": self.key_cancel,
            "up": self.key_up,
            "down": self.key_down,
            "left": self.key_left,
            "right": self.key_right,
            "1": self.dummy, "2": self.dummy, "3": self.dummy,
            "4": self.dummy, "5": self.dummy, "6": self.dummy,
            "7": self.dummy, "8": self.dummy, "9": self.dummy, "0": self.dummy
        }, -1)
        
        self.onLayoutFinish.append(self.start_imgui)
        self.timer = eTimer()
        self.timer.callback.append(self.check_exit)
        
        # Load lib for sending keys
        self.imgui_lib = ctypes.CDLL(os.path.dirname(os.path.realpath(__file__)) + "/libimgui_plugin.so")
        self.imgui_lib.SendImGuiAction.argtypes = [ctypes.c_char_p]
        
    def send_action(self, action_name):
        self.imgui_lib.SendImGuiAction(action_name.encode('utf-8'))
        
    def key_up(self): self.send_action("up")
    def key_down(self): self.send_action("down")
    def key_left(self): self.send_action("left")
    def key_right(self): self.send_action("right")
    def key_ok(self): self.send_action("ok")
    def key_cancel(self): self.send_action("cancel")
    
    def dummy(self):
        pass
        
    def start_imgui(self):
        global g_imgui_running, g_selected_ref
        if g_imgui_running: return
            
        xml_data = generate_channel_xml()
        g_selected_ref = None
        g_imgui_running = True
        
        plugin_path = os.path.dirname(os.path.realpath(__file__)) + "/libimgui_plugin.so"
        threading.Thread(target=run_imgui_thread, args=(xml_data, plugin_path)).start()
        
        self.timer.start(500, False)
        
    def check_exit(self):
        global g_imgui_running, g_selected_ref
        if not g_imgui_running:
            self.timer.stop()
            self.close() # Close our black screen
            
            if g_selected_ref:
                print(f"[ImGui] Changing channel to: {g_selected_ref}")
                self.session.nav.playService(eServiceReference(g_selected_ref))
                
                # Force Enigma2 to drop back to Live TV by closing underlying menus (like PluginBrowser)
                try:
                    for dialog in self.session.dialog_stack:
                        dialog.close()
                except Exception as e:
                    print(f"[ImGui] Failed to close background menus: {e}")

def main(session, **kwargs):
    session.open(ImGuiHostScreen)

def Plugins(**kwargs):
    return [
        PluginDescriptor(
            name="Stardomes ImGui Prototype",
            description="Native EGL ImGui Interface",
            where=PluginDescriptor.WHERE_PLUGINMENU,
            fnc=main
        )
    ]
