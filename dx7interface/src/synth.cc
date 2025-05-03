/* app */
#include "synth.h"

Synth::Synth(Glib::ustring name){
    caller = name;
    LOG( caller );
    LOG( LOG_IN());
    init_nls();
    #if defined(__RtMidi__)
        port_in = new RtMidiIn();
        port_out = new RtMidiOut();
    #endif
    connect_midi(name);
    LOG( caller );
    LOG( LOG_OUT());
};
Synth::~Synth(){
    LOG( caller );
    deconnect_midi();
    LOG( caller );
};

void Synth::unblock_midi(){
    block_midi_msg=false;
};

void Synth::block_midi(){
    block_midi_msg=true;
};

void Synth::connect_midi(Glib::ustring name){
    LOG( LOG_IN());
    #if defined(__ALSA__)
        snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0);
        //list_midi_ports();
        int num = get_last_interface_with_name(name);
        if( num != 0){
            name.append("_"+tostr<int>(num));
        };
        snd_seq_set_client_name(seq_handle, name.c_str());
        port_in =	snd_seq_create_simple_port(seq_handle, (name+"_in").c_str(),
                                                SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
                                                SND_SEQ_PORT_TYPE_APPLICATION);
        port_out =	snd_seq_create_simple_port(seq_handle, (name+"_out").c_str(),
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
    #if defined(__RtMidi__)
        try {
            int port_out_index = 0;
            int port_in_index = 0;

            unsigned int portCount;
            portCount = port_in->getPortCount();
            if (portCount == 0) {
                LOG( std::string(_("No MIDI Input Ports available.")));
            }else{
                LOG( std::string(_("Available Midi Input Ports: ")));
                for (unsigned int i = 0; i < portCount; i++) {
                    std::string portName = port_in->getPortName(i);
                    if( portName.find(name+"_in") == 0 ){
                        port_in_index = i;
                    }
                    LOG( std::string(std::to_string(i) + ": " + std::string(portName) ));
                }
                port_in->openPort(port_in_index);
                port_in_name = port_in->getPortName(port_in_index);
                LOG( std::string(_("Opened MIDI Input Port: ")) + std::string(port_in->getPortName(port_in_index)));
                port_in->setCallback(&Synth::midiInCallback, this);
            }
            portCount = port_out->getPortCount();
            if (portCount == 0) {
                LOG( std::string(_("No MIDI Output Ports available.")));
            }else{
                LOG( std::string(_("Available Midi Ouput Ports: ")));
                for (unsigned int i = 0; i < portCount; i++) {
                    std::string portName = port_out->getPortName(i);
                    if( portName.find(name+"_out") == 0 ){
                        port_out_index = i;
                    }
                    LOG( std::string(std::to_string(i) + std::string(": ") + std::string(portName)));
                }
                port_out->openPort(port_out_index);
                port_out_name = port_out->getPortName(port_out_index);
                LOG( std::string(_("Opened MIDI Output Port: ")) + std::string(port_out->getPortName(port_out_index)));
            }
            // Open virtual ports with specified names
            //port_in->openVirtualPort(port_in_name);
            //port_out->openVirtualPort(port_out_name);
        } catch (const RtMidiError& ex) {
            std::string err_msg = _("Error initializing MIDI: ") + std::string(__PRETTY_FUNCTION__)\
            + _("Reason: ") + ex.getMessage();
            LOG_ERR(err_msg);
            throw;
        }    
    #endif
    LOG( LOG_OUT());
};

void Synth::deconnect_midi(){
    LOG( LOG_IN());
    #if defined(__ALSA__)
        snd_seq_close(seq_handle);
    #endif
    #if defined(__RtMidi__)
        if (port_in && port_in->isPortOpen()) {
            port_in->closePort(); // Close the virtual input port
            LOG( std::string(_("Disconnected MIDI input port: ")) + std::string(port_in_name) );
        }

        if (port_out && port_out->isPortOpen()) {
            port_out->closePort(); // Close the virtual output port
            LOG( std::string(_("Disconnected MIDI output port: ")) + std::string(port_out_name) );
        };
    #endif
    LOG( LOG_OUT());
};

void Synth::send_midi(char ev_type, unsigned int size, unsigned char *msg){
    //LOG( LOG_IN());
    /* TODO; use seq queue */
    //#ifdef USE_ALSA_MIDI
    //LOG_ERR( (int)block_midi_msg ));
    if (!block_midi_msg){
        #if defined(__ALSA__)
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
        #if defined(__RtMidi__)
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
    //LOG( LOG_OUT());
};

/*** File ***/

void Synth::parse_sysex(Glib::RefPtr<Gio::File> file, Glib::RefPtr<Gio::DataInputStream>& data_stream, unsigned int& file_size){
    try{
        unsigned char data = data_stream->read_byte();
        if (data == 0xF0 ){
            for (uint8_t i=0; i < 5; i++){
                data_stream->read_byte();
            };
            file_size -= 8;
        }else{
            data_stream->close();
            data_stream = Gio::DataInputStream::create(file->read());
            file_size = (file->query_info(G_FILE_ATTRIBUTE_STANDARD_SIZE))->get_size();
        };
    }catch( const std::exception&  ex){
        std::string err_msg = error( __PRETTY_FUNCTION__, "Cannot Parse Sysex file", ex.what() );
        LOG_ERR( err_msg );
        LOG( LOG_OUT() );
    }
};

/** BANK **/
void Synth::set_bank(unsigned int data_stream_index, Glib::RefPtr<Gio::File> bank_file, std::function<void(unsigned int, Glib::RefPtr<Gio::File>)> funct ){
    // GENERIC
    LOG( LOG_IN() );
    block_ui();
    clean_bank();
    read_file_as_datastream(data_stream_index, bank_file, funct); //copy file content in _modif et _origin
    Glib::ustring filename = (bank_file->query_info(G_FILE_ATTRIBUTE_STANDARD_NAME))->get_name();
    Glib::ustring name = filename.substr(0,filename.find_last_of("."));
    set_bank_name(name);
    LOG( LOG_OUT() );
};

#if defined(__ALSA__)
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
    void Synth::list_midi_ports(){
        snd_seq_client_info_t *cinfo;
        snd_seq_client_info_alloca(&cinfo);
        snd_seq_client_info_set_client(cinfo, -1);

        printf("ALSA MIDI Clients and Ports:\n");

        // Loop through all clients
        while (snd_seq_query_next_client(seq_handle, cinfo) >= 0) {
            int client = snd_seq_client_info_get_client(cinfo);

            printf("Client %d: %s\n", client, snd_seq_client_info_get_name(cinfo));

            // Prepare to query ports for this client
            snd_seq_port_info_t *pinfo;
            snd_seq_port_info_alloca(&pinfo);
            snd_seq_port_info_set_client(pinfo, client);
            snd_seq_port_info_set_port(pinfo, -1);

            // Loop through all ports for this client
            while (snd_seq_query_next_port(seq_handle, pinfo) >= 0) {
                int port = snd_seq_port_info_get_port(pinfo);
                const char *port_name = snd_seq_port_info_get_name(pinfo);

                printf(" Port %d: %s\n", port, port_name);
            }
        }
    };
    int Synth::get_last_interface_with_name(Glib::ustring name){
        int count = 0;
        snd_seq_client_info_t *cinfo;
        snd_seq_client_info_alloca(&cinfo);
        snd_seq_client_info_set_client(cinfo, -1);
        while (snd_seq_query_next_client(seq_handle, cinfo) >= 0) {
            Glib::ustring client_name = snd_seq_client_info_get_name(cinfo);
            if( client_name.find(name) == 0 ){
                count += 1;
            };
        };
        return count;
    };
#endif
#if defined(__RtMidi__)
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
// threaded loop
bool Synth::Run() {
    #if defined(__ALSA__)
        listen_midi();
    #endif
    #if defined(__RtMidi__)
        while (nanosleep(&ts, NULL) == -1 && errno == EINTR) {
            // Retry if interrupted by a signal
        }
    #endif
    return true;
};

// Function to look up enum name
#if defined(__ALSA__)
    std::string Synth::get_event_name(int value) {
        auto it = enumMap.find(value);
        if (it != enumMap.end()) {
            return it->second;
        }
        return std::string(_("Unknown Event Type"));
    };
#endif
#if defined(__RtMidi__)
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
#if defined(__ALSA__)
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
        std::string msg;
        LOG("");
        msg = _("event: ") + get_event_name(int(ev->type)) + " "
        + "type: " + std::to_string((ev->type));
        LOG( msg );
        msg = "flags: " + std::to_string((ev->flags)) + " "
        + "tag: " + std::to_string((ev->tag)) + '\t'
        + "queue: " + std::to_string((ev->queue)) ;
        LOG( msg );
        msg = "ticks: " + std::to_string((ev->time.tick)) + " "
        + "time: " + std::to_string((ev->time.time.tv_sec));
        LOG( msg );
        msg = "source: " + std::to_string((ev->source.client)) + " " + '\t'
        + "dest: " + std::to_string((ev->dest.client));
        LOG( msg );
        msg = "channel: " + std::to_string((ev->data.control.channel)+1);
        LOG( msg );
        switch (ev->type) {
            case SND_SEQ_EVENT_NOTEON:
                msg = "Channel: "  + std::to_string(int(ev->data.control.channel) +1) + " " + '\t'
                + "value: " + std::to_string((ev->data.note.note));
                LOG( msg );
                break;
            case SND_SEQ_EVENT_NOTEOFF:
                msg = "Channel: "  + std::to_string(int(ev->data.control.channel) +1) + " " + '\t'
                + "value: " +  std::to_string(int(ev->data.note.note));
                LOG( msg );
                break;
            case SND_SEQ_EVENT_CONTROLLER:
                msg = "Channel: " + std::to_string(int(ev->data.control.channel) +1) + " " + '\t'
                + "param: "  + std::to_string(ev->data.control.param) + " "
                + "value: " + std::to_string((ev->data.control.value));
                LOG( msg );
                break;
            case SND_SEQ_EVENT_PITCHBEND:
                msg ="Channel: " + std::to_string(int(ev->data.control.channel) +1)+ " " + '\t'
                + "value: " + std::to_string((ev->data.control.value)) ;
                LOG( msg );
                break;
            case SND_SEQ_EVENT_PGMCHANGE:
                /*event data type = snd_seq_ev_ctrl_t */
                msg = "Channel : "  + std::to_string(int(ev->data.control.channel) +1) + '\t'
                + "param : "  + std::to_string(ev->data.control.param) + " "
                + "value : " + std::to_string((ev->data.control.value));
                LOG( msg );
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
        LOG("");
    };
#endif
#if defined(__RtMidi__)
    void Synth::print_event_info(){
        // Extract the event type from the status byte
        unsigned char status = message[0];
        unsigned char eventType = status & 0xF0; // High nibble indicates the event type
        unsigned char channel = (status & 0x0F) + 1; // Low nibble indicates the channel (1-based)

        // Print event type and channel
        std::string msg = _("Event: ") + get_event_name(eventType) + " "
                  + _("Type: ") + std::to_string(int(eventType));
        LOG( msg );
        msg = _("Channel: ") + std::to_string((channel));
        LOG( msg );
        switch (eventType) {
            case 0x90: { // Note On
                unsigned char note = message[1];
                unsigned char velocity = message[2];
                msg = _("Note: ") + std::to_string(int(note)) 
                    + _(", Velocity: ") + std::to_string(int(velocity));
                LOG( msg );
                break;
            }
            case 0x80: { // Note Off
                unsigned char note = message[1];
                unsigned char velocity = message[2];
                msg = _("Note: ") + std::to_string(int(note))
                    + _(", Velocity: ") + std::to_string(int(velocity));
                LOG( msg );
                break;
            }
            case 0xB0: { // Control Change
                unsigned char controller = message[1];
                unsigned char value = message[2];
                msg = _("Controller: ") + std::to_string(int(controller))
                    + _(", Value: ") + std::to_string(int(value));
                LOG( msg );
                break;
            }
            case 0xE0: { // Pitch Bend
                unsigned short pitchBend = (message[2] + 7) | message[1]; // Combine MSB and LSB
                msg = _("Value: ") + pitchBend;
                LOG( msg );
                break;
            }
            case 0xC0: { // Program Change
                unsigned char program = message[1];
                msg = _("Program: ") + std::to_string(int(program));
                LOG( msg );
                break;
            }
            default:
                msg = _("Unknown or unsupported MIDI event.");
                LOG( msg );
                break;
        };
    };
    
#endif


#if defined(__ALSA__)
    void Synth::listen_midi(){
        /* TODO : use all seq event */
        snd_seq_event_input(seq_handle, &ev);
        print_event_info(ev);
        snd_seq_free_event(ev);
    };
#endif 
#if defined(__RtMidi__)
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

