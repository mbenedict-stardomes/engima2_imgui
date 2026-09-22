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


def main(session, **kwargs):
    print("[ImGui] Launching Enigma2 ImGui Prototype...")
    
    # Generate the XML of live bouquets/channels
    xml_data = generate_channel_xml()
    
    # Load our compiled C++ shared library
    plugin_path = os.path.dirname(os.path.realpath(__file__)) + "/libimgui_plugin.so"
    imgui_lib = ctypes.CDLL(plugin_path)
    
    # Configure C-type signatures
    imgui_lib.StartImGuiPlugin.restype = ctypes.c_char_p
    imgui_lib.SetChannelDataXML.argtypes = [ctypes.c_char_p]
    
    # Push XML into C++ memory
    imgui_lib.SetChannelDataXML(xml_data.encode('utf-8'))
    
    # Boot the ImGui UI (This function blocks the Enigma2 main loop until ImGui exits)
    selected_ref_bytes = imgui_lib.StartImGuiPlugin()
    
    # When ImGui exits, it hands back a service reference string if the user requested to play a channel
    if selected_ref_bytes:
        ref_str = selected_ref_bytes.decode('utf-8')
        if ref_str:
            print(f"[ImGui] Received playback request for: {ref_str}")
            session.nav.playService(eServiceReference(ref_str))
        else:
            print("[ImGui] Exited cleanly to Enigma2.")
    else:
        print("[ImGui] Exited cleanly to Enigma2.")


def Plugins(**kwargs):
    return [
        PluginDescriptor(
            name="Stardomes ImGui Prototype",
            description="Native EGL ImGui Interface",
            where=PluginDescriptor.WHERE_PLUGINMENU,
            fnc=main
        )
    ]
