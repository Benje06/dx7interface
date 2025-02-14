/* app */
#include "synth.h"

Synth::Synth(Glib::ustring name){
    caller = name;
    std::cerr << caller;
    LOG_IN();
    connect_midi(name);
    std::cerr << caller;
    LOG_OUT();
};

Synth::~Synth(){
    std::cerr << caller;
    deconnect_midi();
    std::cerr << caller;
};

void Synth::unblock_midi(){
    block_midi_msg=false;
};

void Synth::block_midi(){
    block_midi_msg=true;
};

void Synth::connect_midi(Glib::ustring name){
    LOG_IN();
    /* 0 can be replace by SND_SEQ_NONBLOCK */
    snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0);
    snd_seq_set_client_name(seq_handle, name.c_str());
    port_in =	snd_seq_create_simple_port(seq_handle, name.append("_in").c_str(),
                                            SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
                                            SND_SEQ_PORT_TYPE_APPLICATION);
    port_out =	snd_seq_create_simple_port(seq_handle, name.append("_out").c_str(),
                                            SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
                                            SND_SEQ_PORT_TYPE_APPLICATION);

    client_id = snd_seq_client_id(seq_handle);
    //snd_seq_set_input_buffer_size(seq_handle,in_buff_size) ;
    //snd_seq_set_output_buffer_size(seq_handle,out_buff_size) ;
    //snd_seq_system_info(seq_handle,seq_info);
    /* polling */
    /* size of poll descriptors */
    spfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN|POLLOUT);
    /* array of poll descriptors */
    pfd = (struct pollfd *)alloca(spfd * sizeof(struct pollfd));
    /* Get poll descriptors. */
    snd_seq_poll_descriptors(seq_handle, pfd, spfd, POLLIN|POLLOUT);
    LOG_OUT();
};

void Synth::deconnect_midi(){
    LOG_IN();
    snd_seq_close(seq_handle);
    LOG_OUT();
};

void Synth::send_midi(char ev_type, uint size, u_char *msg){
    LOG_IN();
    /* TODO; use seq queue */
    //#ifdef USE_ALSA_MIDI
    //std::cerr << (int)block_midi_msg << std::endl;
    if (!block_midi_msg){
        /* seq event */
        snd_seq_event_t ev_out;
        snd_seq_ev_clear(&ev_out);
        snd_seq_ev_set_source(&ev_out, port_out);
        snd_seq_ev_set_subs(&ev_out);
        /* */
        snd_seq_ev_set_direct(&ev_out);

        ev_out.type = ev_type;

        snd_seq_ev_set_variable(&ev_out, size, msg);
        snd_seq_event_output(seq_handle, &ev_out);
        snd_seq_drain_output(seq_handle);
    };
    LOG_OUT();
};

void Synth::print_event_info(snd_seq_event_t* ev){
    std::cout << std::endl;
    std::cout << "event: " << get_event_name(int(ev->type)) << " "
    << "type: " << int(ev->type)<< std::endl;
    std::cout << "flags: " << int(ev->flags) << " "
    << "tag: " << int( ev->tag) << '\t'
    << "queue: " << int(ev->queue) << std::endl;
    std::cout << "ticks: " << int(ev->time.tick) << " "
    << "time: " << int(ev->time.time.tv_sec) << std::endl;
    std::cout << "source: " << int( ev->source.client) << " " << '\t'
    << "dest: " << int(ev->dest.client) << std::endl;
};

int Synth::get_port_in_number(){
    return port_in;
};

int Synth::get_port_out_number(){
    return port_out;
};
int Synth::get_client_id(){
    return client_id;
};
snd_seq_event_t* Synth::get_seq_event_handler(){
    return ev;
};
snd_seq_t* Synth::get_seq_handler(){
    return seq_handle;
};
snd_seq_system_info_t* Synth::get_seq_info(){
    return seq_info;
};

size_t* Synth::get_seq_buffer_size(){
    static size_t t_buff[2] = { snd_seq_get_input_buffer_size(seq_handle) , snd_seq_get_output_buffer_size(seq_handle)};
    return t_buff;
};

bool Synth::Run() {
    listen_midi();
    return true;
};

// Function to look up enum name
std::string Synth::get_event_name(int value) {
    auto it = enumMap.find(value);
    if (it != enumMap.end()) {
        return it->second;
    }
    return "Unknown Event";
};

void Synth::listen_midi(){
    /* TODO : use all seq event */
    snd_seq_event_input(seq_handle, &ev);
    /*
        * typedef struct snd_seq_event {
        *      snd_seq_event_*type_t type;
        *      unsigned char flags;
        *      unsigned char tag;
        *      unsigned char queue;
        *      snd_seq_timestamp_t time;
        *      snd_seq_addr_t source;
        *      snd_seq_addr_t dest;
        *      snd_seq_event_data_t data;
        * } snd_seq_event_t;
        */
    std::cout << std::endl;
    std::cout << "event: " << get_event_name(int(ev->type)) << " "
    << "type: " << int(ev->type)<< std::endl;
    std::cout << "flags: " << int(ev->flags) << " "
    << "tag: " << int( ev->tag) << '\t'
    << "queue: " << int(ev->queue) << std::endl;
    std::cout << "ticks: " << int(ev->time.tick) << " "
    << "time: " << int(ev->time.time.tv_sec) << std::endl;
    /*  snd_seq_timestamp_t time;
        *  typedef union *snd_seq_timestamp {
        *      snd_seq_tick_time_t tick;
        *      struct snd_seq_real_time time;
        *  } snd_seq_timestamp_t;*/
    /*  typedef struct snd_seq_real_time {
        *      unsigned int tv_sec;
        *      unsigned int tv_nsec;
        *  } snd_seq_real_time_t;
    */
    std::cout << "source: " << int( ev->source.client) << " " << '\t'
    << "dest: " << int(ev->dest.client) << std::endl;

    switch (ev->type) {
        case SND_SEQ_EVENT_NOTEON:
            std::cout << "Channel: "  << (int(ev->data.control.channel) +1) << " " << '\t'
            << "value: " << int(ev->data.note.note) << std::endl;;
            break;
        case SND_SEQ_EVENT_NOTEOFF:
            std::cout << "Channel: "  << (int(ev->data.control.channel) +1) << " " << '\t'
            << "value: " <<  int(ev->data.note.note) << std::endl;
            break;
        case SND_SEQ_EVENT_CONTROLLER:
            std::cout << "Channel: " << (int(ev->data.control.channel) +1) << " " << '\t'
            << "param: "  << ev->data.control.param << " "
            << "value: " << int(ev->data.control.value) << std::endl;
            break;
        case SND_SEQ_EVENT_PITCHBEND:
            std::cout << "Channel: " << (int(ev->data.control.channel) +1)<< " " << '\t'
            << "value: " << int(ev->data.control.value) << std::endl;
            break;
        case SND_SEQ_EVENT_PGMCHANGE:
            /*event data type = snd_seq_ev_ctrl_t */
            std::cout <<  "Channel : "  << (int(ev->data.control.channel) +1) << '\t'
            << "param : "  << ev->data.control.param << " "
            << "value : " << int(ev->data.control.value)
            << std::endl;
            break;
    };

            /* https://www.alsa-project.org/alsa-doc/alsa-lib/seq__event_8h_source.html */

            /*case SND_SEQ_EVENT_SYSTEM :  system status; event data type = snd_seq_result_t
                *  SND_SEQ_EVENT_RESULT 	returned result status; event data type = snd_seq_result_t
                *  SND_SEQ_EVENT_NOTE 	note on and off with duration; event data type = snd_seq_ev_note_t
                *  SND_SEQ_EVENT_NOTEON 	note on; event data type = snd_seq_ev_note_t
                *  SND_SEQ_EVENT_NOTEOFF 	note off; event data type = snd_seq_ev_note_t
                *  SND_SEQ_EVENT_KEYPRESS 	key pressure change (aftertouch); event data type = snd_seq_ev_note_t
                *  SND_SEQ_EVENT_CONTROLLER 	controller; event data type = snd_seq_ev_ctrl_t
                *  SND_SEQ_EVENT_PGMCHANGE 	program change; event data type = snd_seq_ev_ctrl_t
                *  SND_SEQ_EVENT_CHANPRESS 	channel pressure; event data type = snd_seq_ev_ctrl_t
                *  SND_SEQ_EVENT_PITCHBEND 	pitchwheel; event data type = snd_seq_ev_ctrl_t; data is from -8192 to 8191)
                *  SND_SEQ_EVENT_CONTROL14 	14 bit controller value; event data type = snd_seq_ev_ctrl_t
                *  SND_SEQ_EVENT_NONREGPARAM 	14 bit NRPN; event data type = snd_seq_ev_ctrl_t
                *  SND_SEQ_EVENT_REGPARAM 	14 bit RPN; event data type = snd_seq_ev_ctrl_t
                *  SND_SEQ_EVENT_SONGPOS 	SPP with LSB and MSB values; event data type = snd_seq_ev_ctrl_t
                *  SND_SEQ_EVENT_SONGSEL 	Song Select with song ID number; event data type = snd_seq_ev_ctrl_t
                *  SND_SEQ_EVENT_QFRAME 	midi time code quarter frame; event data type = snd_seq_ev_ctrl_t
                *  SND_SEQ_EVENT_TIMESIGN 	SMF Time Signature event; event data type = snd_seq_ev_ctrl_t
                *  SND_SEQ_EVENT_KEYSIGN 	SMF Key Signature event; event data type = snd_seq_ev_ctrl_t
                *  SND_SEQ_EVENT_START 	MIDI Real Time Start message; event data type = snd_seq_ev_queue_control_t
                *  SND_SEQ_EVENT_CONTINUE 	MIDI Real Time Continue message; event data type = snd_seq_ev_queue_control_t
                *  SND_SEQ_EVENT_STOP 	MIDI Real Time Stop message; event data type = snd_seq_ev_queue_control_t
                *  SND_SEQ_EVENT_SETPOS_TICK 	Set tick queue position; event data type = snd_seq_ev_queue_control_t
                *  SND_SEQ_EVENT_SETPOS_TIME 	Set real-time queue position; event data type = snd_seq_ev_queue_control_t
                *  SND_SEQ_EVENT_TEMPO 	(SMF) Tempo event; event data type = snd_seq_ev_queue_control_t
                *  SND_SEQ_EVENT_CLOCK 	MIDI Real Time Clock message; event data type = snd_seq_ev_queue_control_t
                *  SND_SEQ_EVENT_TICK 	MIDI Real Time Tick message; event data type = snd_seq_ev_queue_control_t
                *  SND_SEQ_EVENT_QUEUE_SKEW 	Queue timer skew; event data type = snd_seq_ev_queue_control_t
                *  SND_SEQ_EVENT_SYNC_POS 	Sync position changed; event data type = snd_seq_ev_queue_control_t
                *  SND_SEQ_EVENT_TUNE_REQUEST 	Tune request; event data type = none
                *  SND_SEQ_EVENT_RESET 	Reset to power-on state; event data type = none
                *  SND_SEQ_EVENT_SENSING 	Active sensing event; event data type = none
                *  SND_SEQ_EVENT_ECHO 	Echo-back event; event data type = any type
                *  SND_SEQ_EVENT_OSS 	OSS emulation raw event; event data type = any type
                *  SND_SEQ_EVENT_CLIENT_START 	New client has connected; event data type = snd_seq_addr_t
                *  SND_SEQ_EVENT_CLIENT_EXIT 	Client has left the system; event data type = snd_seq_addr_t
                *  SND_SEQ_EVENT_CLIENT_CHANGE 	Client status/info has changed; event data type = snd_seq_addr_t
                *  SND_SEQ_EVENT_PORT_START 	New port was created; event data type = snd_seq_addr_t
                *  SND_SEQ_EVENT_PORT_EXIT 	Port was deleted from system; event data type = snd_seq_addr_t
                *  SND_SEQ_EVENT_PORT_CHANGE 	Port status/info has changed; event data type = snd_seq_addr_t
                *  SND_SEQ_EVENT_PORT_SUBSCRIBED 	Ports connected; event data type = snd_seq_connect_t
                *  SND_SEQ_EVENT_PORT_UNSUBSCRIBED 	Ports disconnected; event data type = snd_seq_connect_t
                *  SND_SEQ_EVENT_USR0 	user-defined event; event data type = any (fixed size)
                *  SND_SEQ_EVENT_USR1 	user-defined emod.extpath.vent; event data type = any (fixed size)
                *  SND_SEQ_EVENT_USR2 	user-defined event; event data type = any (fixed size)
                *  SND_SEQ_EVENT_USR3 	user-defined event; event data type = any (fixed size)
                *  SND_SEQ_EVENT_USR4 	user-defined event; event data type = any (fixed size)
                *  SND_SEQ_EVENT_USR5 	user-defined event; event data type = any (fixed size)
                *  SND_SEQ_EVENT_USR6 	user-defined event; event data type = any (fixed size)
                *  SND_SEQ_EVENT_USR7 	user-defined event; event data type = any (fixed size)
                *  SND_SEQ_EVENT_USR8 	user-defined event; event data type = any (fixed size)
                *  SND_SEQ_EVENT_USR9 	user-defined event; event data type = any (fixed size)
                *  SND_SEQ_EVENT_SYSEX 	system exclusive data (variable length); event data type = snd_seq_ev_ext_t
                *  SND_SEQ_EVENT_BOUNCE 	error event; event data type = snd_seq_ev_ext_t
                *  SND_SEQ_EVENT_USR_VAR0 	reserved for user apps; event data type = snd_seq_ev_ext_t
                *  SND_SEQ_EVENT_USR_VAR1 	reserved for user apps; event data type = snd_seq_ev_ext_t
                *  SND_SEQ_EVENT_USR_VAR2 	reserved for user apps; event data type = snd_seq_ev_ext_t
                *  SND_SEQ_EVENT_USR_VAR3 	reserved for user apps; event data type = snd_seq_ev_ext_t
                *  SND_SEQ_EVENT_USR_VAR4 	reserved for user apps; event data type = snd_seq_ev_ext_t
                *  SND_SEQ_EVENT_NONE 	NOP; ignored in any case */
    std::cout << std::endl;
    snd_seq_free_event(ev);
};
