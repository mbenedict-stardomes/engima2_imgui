from Plugins.Plugin import PluginDescriptor
from Screens.Screen import Screen
from Components.ActionMap import ActionMap
from Components.NimManager import nimmanager
from enigma import eServiceReference, eServiceCenter, eTimer
import ctypes

def load_lamedb():
    tp_db = {}
    try:
        with open("/etc/enigma2/lamedb", "r") as f:
            lines = f.readlines()
            
        in_transponders = False
        curr_id = None
        for line in lines:
            line = line.strip()
            if line == "transponders":
                in_transponders = True
                continue
            if line == "services":
                break # Stop at services block
            if in_transponders:
                if line == "/":
                    curr_id = None
                    continue
                if ":" in line and curr_id is None:
                    curr_id = line.lower()
                elif curr_id is not None:
                    tp_db[curr_id] = line
    except:
        pass
    return tp_db


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
                        channel_name = channel_name.replace('&', '&amp;').replace('"', '&quot;').replace('<', '&lt;').replace('>', '&gt;')
                        ref_str = channel_ref.toString().replace('&', '&amp;').replace('"', '&quot;').replace('<', '&lt;').replace('>', '&gt;')
                        xml += f'        <channel number="{idx}" name="{channel_name}" ref="{ref_str}" />\n'
                        idx += 1
            xml += '    </bouquet>\n'
            
    # 2. Fetch ALL channels (Alphabetical) and separate Terrestrial
    all_ref = eServiceReference('1:7:1:0:0:0:0:0:0:0:(type == 1) || (type == 17) || (type == 195) || (type == 25) ORDER BY name')
    all_list = serviceHandler.list(all_ref)
    if all_list is not None:
        all_channels_xml = []
        terr_channels_xml = []
        
        idx = 1
        terr_idx = 1
        while True:
            channel_ref = all_list.getNext()
            if not channel_ref.valid():
                break
            if not (channel_ref.flags & eServiceReference.isMarker):
                info = serviceHandler.info(channel_ref)
                channel_name = info.getName(channel_ref) if info else channel_ref.getName()
                if not channel_name: channel_name = "Unknown Channel"
                channel_name = channel_name.replace('&', '&amp;').replace('"', '&quot;').replace('<', '&lt;').replace('>', '&gt;')
                
                ref_str = channel_ref.toString()
                safe_ref_str = ref_str.replace('&', '&amp;').replace('"', '&quot;').replace('<', '&lt;').replace('>', '&gt;')
                
                # Every channel goes to the All Channels list
                all_channels_xml.append(f'        <channel number="{idx}" name="{channel_name}" ref="{safe_ref_str}" />\n')
                idx += 1
                
                # Check namespace for Terrestrial (DVB-T / DVB-T2 / ISDB-T) -> Starts with EEEE
                parts = ref_str.split(':')
                if len(parts) > 6 and parts[6].upper().startswith('EEEE'):
                    terr_channels_xml.append(f'        <channel number="{terr_idx}" name="{channel_name}" ref="{safe_ref_str}" />\n')
                    terr_idx += 1
                    
        xml += '    <bouquet name="All Channels (A-Z)">\n'
        xml += "".join(all_channels_xml)
        xml += '    </bouquet>\n'
        
        if len(terr_channels_xml) > 0:
            xml += '    <bouquet name="All Terrestrial (DVB-T/T2/ISDB-T)">\n'
            xml += "".join(terr_channels_xml)
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
    # backgroundColor="transparent" in Enigma2 means opaque black.
    skin = """
        <screen name="ImGuiHostScreen" position="0,0" size="1920,1080" flags="wfNoBorder" backgroundColor="transparent">
        </screen>
    """
    
    def __init__(self, session):
        Screen.__init__(self, session)
        
        # High priority ActionMap to steal ALL remote control presses from Enigma2!
        self["actions"] = ActionMap(["DirectionActions", "OkCancelActions", "ColorActions", "NumberActions", "EPGSelectActions", "InfobarEPGActions", "InfobarChannelSelection", "InfobarMenuActions", "SetupActions", "InfoActions"], {
            "ok": self.key_ok,
            "cancel": self.key_cancel,
            "up": self.key_up,
            "down": self.key_down,
            "left": self.key_left,
            "right": self.key_right,
            "info": self.key_info,
            "epg": self.key_info,
            "showEventInfo": self.key_info,
            "zapUp": self.key_channels,
            "zapDown": self.key_channels,
            "channelUp": self.key_channels,
            "channelDown": self.key_channels,
            "mainMenu": self.key_menu,
            "menu": self.key_menu,
            "info": self.key_info,
            "1": self.dummy, "2": self.dummy, "3": self.dummy,
            "4": self.dummy, "5": self.dummy, "6": self.dummy,
            "7": self.dummy, "8": self.dummy, "9": self.dummy, "0": self.dummy
        }, 100)
        
        self.onLayoutFinish.append(self.start_imgui)
        self.timer = eTimer()
        self.timer.callback.append(self.check_exit)
        
        # Load lib for sending keys
        self.imgui_lib = ctypes.CDLL(os.path.dirname(os.path.realpath(__file__)) + "/libimgui_plugin.so")
        self.imgui_lib.SendImGuiAction.argtypes = [ctypes.c_char_p]
        
    def send_action(self, action_name):
        print(f"[ImGui] Python ActionMap caught: {action_name}")
        self.imgui_lib.SendImGuiAction(action_name.encode('utf-8'))
        
    def key_up(self): self.send_action("up")
    def key_down(self): self.send_action("down")
    def key_left(self): self.send_action("left")
    def key_right(self): self.send_action("right")
    def key_ok(self): self.send_action("ok")
    def key_cancel(self): self.send_action("cancel")
    def key_info(self): self.send_action("info")
    def key_channels(self): self.send_action("channels")
    def key_menu(self): self.send_action("menu")
    def key_info(self): self.send_action("info")
    
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
        
        self.timer.start(250, False)
        
        try:
            tuner_info = []
            for slot in nimmanager.nim_slots:
                if not slot.empty:
                    types = []
                    if slot.isCompatible("DVB-S2"): types.append("DVB-S2")
                    elif slot.isCompatible("DVB-S"): types.append("DVB-S")
                    
                    if slot.isCompatible("DVB-T2"): types.append("DVB-T2")
                    elif slot.isCompatible("DVB-T"): types.append("DVB-T")
                    
                    if slot.isCompatible("DVB-C"): types.append("DVB-C")
                    
                    type_str = ",".join(types)
                    tuner_info.append(f"{slot.slot}|{slot.description}|{type_str}")
            
            if tuner_info:
                payload = "^".join(tuner_info)
                self.send_action(f"hw_tuners|{payload}")
        except Exception as e:
            print(f"[ImGui] Failed to enumerate tuners: {e}")
        
    def check_exit(self):
        global g_imgui_running
        
        # Check for background playback requests
        self.imgui_lib.GetPendingPlayback.restype = ctypes.c_char_p

        # Poll Volume to update ImGui without breaking Enigma2's native volume handling
        try:
            from enigma import eDVBVolumecontrol
            vol_ctrl = eDVBVolumecontrol.getInstance()
            if vol_ctrl:
                vol = vol_ctrl.getVolume()
                if not hasattr(self, 'last_vol'): self.last_vol = vol
                if self.last_vol != vol:
                    self.send_action(f"vol_{vol}")
                    self.last_vol = vol
        except:
            pass


        # Poll Telemetry and EPG
        try:
            service = self.session.nav.getCurrentService()
            if service:
                # Telemetry
                feinfo = service.frontendInfo()
                if feinfo:
                    fe_data = feinfo.getFrontendData() if hasattr(feinfo, 'getFrontendData') else None
                    if fe_data:
                        fe_status = feinfo.getFrontendStatus() if hasattr(feinfo, 'getFrontendStatus') else {}
                        snr = fe_status.get('tuner_signal_quality', fe_status.get('snr', fe_data.get('snr', 0)))
                        agc = fe_status.get('tuner_signal_power', fe_status.get('agc', fe_data.get('agc', 0)))
                        ber = fe_status.get('tuner_bit_error_rate', fe_status.get('ber', fe_data.get('ber', 0)))
                        

                        # Convert out of 65536 if needed
                        if snr and snr > 100: snr = (snr * 100) // 65536
                        if agc and agc > 100: agc = (agc * 100) // 65536
                        
                        self.send_action(f"telemetry|{snr}|{agc}|{ber}")
                        
                        # EXTENDED TELEMETRY
                        fe_ext_str = ""
                        lamedb_ext = ""
                        hex_ids = ""
                        
                        # 1. Get Reference to extract IDs and query Lamedb
                        play_ref = self.session.nav.getCurrentlyPlayingServiceReference()
                        if play_ref:
                            ref_str = play_ref.toString()
                            parts = ref_str.split(':')
                            if len(parts) >= 7:
                                try:
                                    sid_hex = parts[3].zfill(4).upper()
                                    tsid_hex = parts[4].zfill(4).upper()
                                    onid_hex = parts[5].zfill(4).upper()
                                    namespace_hex = parts[6].zfill(8).lower()
                                    
                                    tp_key = f"{namespace_hex}:{tsid_hex.lower()}:{onid_hex.lower()}"
                                    
                                    if not hasattr(self, 'tp_db'):
                                        self.tp_db = load_lamedb()
                                        
                                    if tp_key in self.tp_db:
                                        tp_line = self.tp_db[tp_key]
                                        tp_parts = tp_line.split()
                                        if len(tp_parts) == 2:
                                            t_type = tp_parts[0]
                                            t_data = tp_parts[1].split(':')
                                            freq = int(t_data[0])
                                            if freq > 1000000: freq = freq // 1000
                                            
                                            if t_type == 's' and len(t_data) >= 2:
                                                sr = int(t_data[1])
                                                if sr > 1000000: sr = sr // 1000
                                                pol = int(t_data[2]) if len(t_data) > 2 else 0
                                                pol_str = "H" if pol == 0 else "V" if pol == 1 else "L" if pol == 2 else "R"
                                                lamedb_ext = f"TP: {freq} MHz {pol_str} {sr} KS/s"
                                            elif t_type == 't':
                                                lamedb_ext = f"TP: {freq} MHz DVB-T"
                                            elif t_type == 'c':
                                                sr = int(t_data[1]) if len(t_data) > 1 else 0
                                                if sr > 1000000: sr = sr // 1000
                                                lamedb_ext = f"TP: {freq} MHz {sr} KS/s DVB-C"
                                                
                                    hex_ids = f"SID: 0x{sid_hex}  TSID: 0x{tsid_hex}  ONID: 0x{onid_hex}"
                                except:
                                    pass
                        
                        # 2. Append Audio/Video PIDs from info
                        try:
                            from enigma import iServiceInformation
                            if info:
                                vid = info.getInfo(iServiceInformation.sVideoPID)
                                apid = info.getInfo(iServiceInformation.sAudioPID)
                                if vid > 0:
                                    hex_ids += f"  VPID: 0x{vid:04X}  APID: 0x{apid:04X}"
                        except:
                            pass
                            
                        # 3. Fallback to frontendInfo if lamedb failed
                        if not lamedb_ext and fe_data:
                            freq = fe_data.get('tuner_frequency', fe_data.get('frequency', 0))
                            if freq > 1000000: freq = freq // 1000
                            sr = fe_data.get('tuner_symbol_rate', fe_data.get('symbol_rate', 0))
                            if sr > 1000000: sr = sr // 1000
                            
                            sys_enum = fe_data.get('tuner_system', fe_data.get('system', 0))
                            sys_str = "DVB-S" if sys_enum == 0 else "DVB-S2" if sys_enum == 1 else "DVB-T" if sys_enum == 3 else "DVB-C" if sys_enum == 2 else "DVB-T2" if sys_enum == 4 else "DVB"
                            
                            if freq > 0:
                                if sr > 0:
                                    lamedb_ext = f"TP: {freq} MHz {sr} KS/s {sys_str}"
                                else:
                                    lamedb_ext = f"TP: {freq} MHz {sys_str}"
                                    
                        fe_ext_str = ""
                        if lamedb_ext: fe_ext_str += lamedb_ext
                        if hex_ids: fe_ext_str += (" | " if lamedb_ext else "") + hex_ids
                        
                        self.send_action(f"ext_telemetry|{fe_ext_str}")

                
                # EPG
                info = service.info()
                if info:
                    ev_now = info.getEvent(0)
                    if ev_now:
                        name = ev_now.getEventName()
                        desc = ev_now.getShortDescription() or ""
                        # Escape pipes
                        name = name.replace('|', '-') if name else ""
                        desc = desc.replace('|', '-') if desc else ""
                        self.send_action(f"epg_now|{name}|{desc}")
                    
                    ev_next = info.getEvent(1)
                    if ev_next:
                        name = ev_next.getEventName()
                        desc = ev_next.getShortDescription() or ""
                        name = name.replace('|', '-') if name else ""
                        desc = desc.replace('|', '-') if desc else ""
                        self.send_action(f"epg_next|{name}|{desc}")
        except Exception as e:
            pass

        pending = self.imgui_lib.GetPendingPlayback()
        if pending:
            ref_str = pending.decode('utf-8')
            if ref_str.startswith("start_scan|"):
                try:
                    from Screens.ScanSetup import ScanSetup
                    self.session.open(ScanSetup)
                    # We MUST kill the ImGui overlay so the user can actually see and interact with the native Enigma2 Scan Setup wizard!
                    global g_imgui_running
                    g_imgui_running = False
                    self.timer.stop()
                    self.close()
                except Exception as e:
                    print(f"[ImGui] Failed to launch ScanSetup: {e}")
            else:
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
