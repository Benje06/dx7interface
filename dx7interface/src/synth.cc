/* app */
#include "synth.h"

Synth::Synth(Glib::ustring name){
    caller = name;
    std::cerr << caller;
    LOG_IN();
    init_nls();
    #if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
        port_in = new RtMidiIn();
        port_out = new RtMidiOut();
    #endif
    connect_midi(name);
    std::cerr << caller;
    LOG_OUT();
};

Synth::~Synth(){
    std::cerr << caller;
    deconnect_midi();
    std::cerr << caller;
};

void Synth::init_nls(){
    #ifdef ENABLE_NLS
        //setlocale (LC_ALL, "");
        std::locale::global(std::locale(""));
        textdomain (GETTEXT_PACKAGE);
        bindtextdomain (GETTEXT_PACKAGE, PROGRAMNAME_LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    #endif
};
void Synth::unblock_midi(){
    block_midi_msg=false;
};

void Synth::block_midi(){
    block_midi_msg=true;
};

void Synth::connect_midi(Glib::ustring name){
    LOG_IN();
    #ifdef __linux__
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
    #endif
    #if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
        try {
            // Create RtMidiIn and RtMidiOut instances
            //port_in = std::make_unique<RtMidiIn>();
            //port_out = std::make_unique<RtMidiOut>();

            int port_out_index = 0;
            int port_in_index = 0;

            unsigned int portCount;
            portCount = port_in->getPortCount();
            if (portCount == 0) {
                std::cout << "No MIDI Input Ports available.\n";
            }else{
                std::cout << "Available Midi Input Ports"  << std::endl;
                for (unsigned int i = 0; i < portCount; i++) {
                    std::string portName = port_in->getPortName(i);
                    if( portName.find(name+"_in") == 0 ){
                        port_in_index = i;
                    }
                    std::cout << i << ": " << portName << std::endl;
                }
                port_in->openPort(port_in_index);
                port_in_name = port_in->getPortName(port_in_index);
                std::cout << "Opened MIDI Input Port: " << port_in->getPortName(port_in_index) << "\n";
                port_in->setCallback(&Synth::midiInCallback, this);
            }
            portCount = port_out->getPortCount();
            if (portCount == 0) {
                std::cout << "No MIDI Output Ports available.\n";
            }else{
                std::cout << "Available Midi Ouput Ports"  << std::endl;
                for (unsigned int i = 0; i < portCount; i++) {
                    std::string portName = port_out->getPortName(i);
                    if( portName.find(name+"_out") == 0 ){
                        port_out_index = i;
                    }
                    std::cout << i << ": " << portName << std::endl;
                }
                port_out->openPort(port_out_index);
                port_out_name = port_out->getPortName(port_out_index);
                std::cout << "Opened MIDI Output Port: " << port_out->getPortName(port_out_index) << "\n";
            }
            // Open virtual ports with specified names
            //port_in->openVirtualPort(port_in_name);
            //port_out->openVirtualPort(port_out_name);
        } catch (const RtMidiError& error) {
            std::cerr << "Error initializing MIDI: " << error.getMessage() << std::endl;
            throw;
        }    
    #endif
    LOG_OUT();
};

void Synth::deconnect_midi(){
    LOG_IN();
    #ifdef __linux__
        snd_seq_close(seq_handle);
    #endif
    #if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
        if (port_in && port_in->isPortOpen()) {
            port_in->closePort(); // Close the virtual input port
            std::cout << "Disconnected MIDI input port: " << port_in_name << std::endl;
        }

        if (port_out && port_out->isPortOpen()) {
            port_out->closePort(); // Close the virtual output port
            std::cout << "Disconnected MIDI output port: " << port_out_name << std::endl;
        };
    #endif
    LOG_OUT();
};

void Synth::send_midi(char ev_type, unsigned int size, unsigned char *msg){
    LOG_IN();
    /* TODO; use seq queue */
    //#ifdef USE_ALSA_MIDI
    //std::cerr << (int)block_midi_msg << std::endl;
    if (!block_midi_msg){
        #ifdef __linux__
            /* seq event */
            snd_seq_event_t ev_out;
            snd_seq_ev_clear(&ev_out);
            snd_seq_ev_set_source(&ev_out, port_out);
            snd_seq_ev_set_subs(&ev_out);
            /* */
            snd_seq_ev_set_direct(&ev_out);
            ev_out.type = ev_type;
            if (ev_type == SND_SEQ_EVENT_CONTROLLER ){
                ev_out.data.control.channel = msg[0] & 0x0F;
                ev_out.data.control.param = msg[1];
                ev_out.data.control.value = msg[2];
                snd_seq_ev_set_fixed(&ev_out);
            }else{
                snd_seq_ev_set_variable(&ev_out, size, msg);
            }
            snd_seq_event_output(seq_handle, &ev_out);
            snd_seq_drain_output(seq_handle);
        #endif
        #if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
            if ( ev_type == SND_SEQ_EVENT_CONTROLLER ) {
                message.push_back(0xB0 | (msg[0] & 0x0F)); // CC + channel
                message.push_back(msg[1]);  // Controller number
                message.push_back(msg[2]);  // Value
            }else {
                // Pass raw MIDI bytes directly
                message.assign(msg, msg + size);
            }
            port_out->sendMessage(&message);
            // TODO clear message;
        #endif
    };
    LOG_OUT();
};


#ifdef __linux__
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
    int Synth::get_port_in_number(){
        return port_in;
    };
    int Synth::get_port_out_number(){
        return port_out;
    };
#endif
#if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
    /*std::pair<RtMidiIn*, RtMidiOut*> Synth::get_midi_io() {
        return std::make_pair(midiIn_.get(), midiOut_.get());
    }*/
    /*std::unique_ptr<RtMidiIn> Synth::get_port_in_number(){
        return port_in;
    };
    std::unique_ptr<RtMidiOut> Synth::get_port_out_number(){
        return port_out;
    };*/
#endif

bool Synth::Run() {
    #ifdef __linux__
        listen_midi();
    #endif
    #if defined(__WIN32) || defined(__MINGW32__)
        while (nanosleep(&ts, NULL) == -1 && errno == EINTR) {
            // Retry if interrupted by a signal
        }
    #endif
    return true;
};

// Function to look up enum name
#ifdef __linux__
    std::string Synth::get_event_name(int value) {
        auto it = enumMap.find(value);
        if (it != enumMap.end()) {
            return it->second;
        }
        return "Unknown Event";
    };
#endif
#if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
    std::string Synth::get_event_name(unsigned char statusByte) {
        switch (statusByte & 0xF0) { // Mask high nibble to get event type
            case 0x80: return "Note Off";
            case 0x90: return "Note On";
            case 0xA0: return "Aftertouch";
            case 0xB0: return "Control Change";
            case 0xC0: return "Program Change";
            case 0xD0: return "Channel Pressure";
            case 0xE0: return "Pitch Bend Change";
            case 0xF0: return "System Message"; // Includes SysEx, Clock, etc.
            default:   return "Unknown Event";
        }
    }
#endif
#ifdef __linux__
    void Synth::print_event_info(snd_seq_event_t* ev){
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
        << "type: " << int(ev->type) << std::endl;
        std::cout << "flags: " << int(ev->flags) << " "
        << "tag: " << int( ev->tag) << '\t'
        << "queue: " << int(ev->queue) << std::endl;
        std::cout << "ticks: " << int(ev->time.tick) << " "
        << "time: " << int(ev->time.time.tv_sec) << std::endl;
        std::cout << "source: " << int( ev->source.client) << " " << '\t'
        << "dest: " << int(ev->dest.client) << std::endl;
        std::cout << "channel: " << int(ev->data.control.channel)+1 << std::endl;
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
    };
#endif
#if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
    void Synth::print_event_info(){
        // Extract the event type from the status byte
        unsigned char status = message[0];
        unsigned char eventType = status & 0xF0; // High nibble indicates the event type
        unsigned char channel = (status & 0x0F) + 1; // Low nibble indicates the channel (1-based)

        // Print event type and channel
        std::cout << "Event: " << get_event_name(eventType) << " "
                  << "Type: " << int(eventType) << std::endl;
        std::cout << "Channel: " << int(channel) << std::endl;
        switch (eventType) {
            case 0x90: { // Note On
                unsigned char note = message[1];
                unsigned char velocity = message[2];
                std::cout << "Note: " << int(note)
                          << ", Velocity: " << int(velocity) << std::endl;
                break;
            }
            case 0x80: { // Note Off
                unsigned char note = message[1];
                unsigned char velocity = message[2];
                std::cout << "Note: " << int(note)
                          << ", Velocity: " << int(velocity) << std::endl;
                break;
            }
            case 0xB0: { // Control Change
                unsigned char controller = message[1];
                unsigned char value = message[2];
                std::cout << "Controller: " << int(controller)
                          << ", Value: " << int(value) << std::endl;
                break;
            }
            case 0xE0: { // Pitch Bend
                unsigned short pitchBend = (message[2] << 7) | message[1]; // Combine MSB and LSB
                std::cout << "Value: " << pitchBend << std::endl;
                break;
            }
            case 0xC0: { // Program Change
                unsigned char program = message[1];
                std::cout << "Program: " << int(program) << std::endl;
                break;
            }
            default:
                std::cout << "Unknown or unsupported MIDI event." << std::endl;
                break;
        };
    };
    
#endif


#ifdef __linux__
    void Synth::listen_midi(){
        /* TODO : use all seq event */
        snd_seq_event_input(seq_handle, &ev);
        print_event_info(ev);
        snd_seq_free_event(ev);
    };
#endif 
#if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
    void Synth::midiInCallback(double timestamp, std::vector<unsigned char>* message, void* userData) {
        if (!message || message->empty()) return;
        auto* synth = static_cast<Synth*>(userData); // Cast userData back to Synth instance
        synth->listen_midi(timestamp, message, userData);
    }
    void Synth::listen_midi(double timestamp, std::vector<unsigned char>* _message, void* userData){
        message.assign(_message->begin(),_message->end());
        print_event_info();        
    };
#endif

