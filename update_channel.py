with open('channel_list.cpp', 'r') as f:
    data = f.read()
data = data.replace('g_selected_channel_ref = bq.channels[current_channel_idx].ref;', 'extern void SetCurrentChannelName(const char*); extern "C" void TriggerPlayback(const char*); extern uint64_t g_infobar_timer; extern uint64_t get_time_ms(); extern int g_currentState; TriggerPlayback(bq.channels[current_channel_idx].ref.c_str()); SetCurrentChannelName(bq.channels[current_channel_idx].name.c_str()); g_infobar_timer = get_time_ms(); g_currentState = 2; // MENU_INFOBAR_SMALL')
data = data.replace('g_trigger_exit = true;', '')
with open('channel_list.cpp', 'w') as f:
    f.write(data)
