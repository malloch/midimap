
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include "RtMidi.h"
#include "mapper/mapper.h"

#define INSTANCES 10

#define NOTE_OFF 0x80
#define NOTE_ON 0x90
#define AFTERTOUCH 0xA0
#define CONTROL_CHANGE 0xB0
#define PROGRAM_CHANGE 0xC0
#define CHANNEL_PRESSURE 0xD0
#define PITCH_WHEEL 0xE0

int done = 0;
mapper_timetag_t tt;

typedef struct _midimap_channel {
    unsigned char           number;
    struct _midimap_device  *device;
    mapper_signal           pitch;
    mapper_signal           velocity;
    mapper_signal           aftertouch;
    mapper_signal           pitch_wheel;
    mapper_signal           poly_pressure;
    mapper_signal           channel_pressure;
    mapper_signal           control_change;
    mapper_signal           program_change;
} *midimap_channel;

typedef struct _midimap_device {
    char            *name;
    mapper_device   mapper_dev;
    RtMidiIn        *midiin;
    RtMidiOut       *midiout;
    int             is_linked;
    midimap_channel channel[16];
    struct _midimap_device *next;
} *midimap_device;

struct _midimap_device *inputs = 0;
struct _midimap_device *outputs = 0;

std::vector<unsigned char> outmess (3, 0);

void cleanup_device(midimap_device dev);

void pitch_handler(mapper_signal sig, mapper_id instance, const void *value,
                   int count, mapper_timetag_t *timetag)
{
    midimap_channel chan = (midimap_channel)mapper_signal_user_data(sig);
    if (!chan || !chan->device || !chan->device->midiout)
        return;

    if (value) {
        // make sure pitch instance is matched to velocity and aftertouch instances
        mapper_signal_instance_activate(chan->velocity, instance);
        mapper_signal_instance_activate(chan->aftertouch, instance);
    }
}

void velocity_handler(mapper_signal sig, mapper_id instance, const void *value,
                      int count, mapper_timetag_t *timetag)
{
    midimap_channel chan = (midimap_channel)mapper_signal_user_data(sig);
    if (!chan || !chan->device || !chan->device->midiout)
        return;

    if (value) {
        // make sure velocity instance is matched to pitch and aftertouch instances
        mapper_signal_instance_activate(chan->pitch, instance);
        mapper_signal_instance_activate(chan->aftertouch, instance);
    }

    int *v = (int *)value;

    // output MIDI NOTEON message

    /* Get pitch value for this instance. This might seem a bit weird since
     * the pitch instance may have been remotely released already implying
     * that it has no value, however libmapper does not (currently)
     * overwrite the last value of a remotely released instance. */
    int *note = (int *)mapper_signal_instance_value(chan->pitch, instance, 0);

    outmess[0] = chan->number + NOTE_ON;
    outmess[1] = note ? (unsigned char)(*note) : 60;
    outmess[2] = value ? v[0] : 0;
    chan->device->midiout->sendMessage(&outmess);

    if (!value) {
        mapper_signal_instance_release(sig, instance, MAPPER_NOW);
        mapper_signal_instance_release(chan->pitch, instance, MAPPER_NOW);
    }
}

void aftertouch_handler(mapper_signal sig, mapper_id instance, const void *value,
                        int count, mapper_timetag_t *timetag)
{
    midimap_channel chan = (midimap_channel)mapper_signal_user_data(sig);
    if (!value || !chan || !chan->device || !chan->device->midiout)
        return;

    // make sure aftertouch instance is matched to pitch and velocity instances
    mapper_signal_instance_activate(chan->pitch, instance);
    mapper_signal_instance_activate(chan->velocity, instance);

    // check if note exists
    if (!mapper_signal_instance_value(chan->velocity, instance, 0))
        return;

    // output MIDI AFTERTOUCH message
    int *note = (int *)mapper_signal_instance_value(chan->pitch, instance, 0);

    int *v = (int *)value;
    outmess[0] = chan->number + AFTERTOUCH;
    outmess[1] = note ? (unsigned char)(*note) : 60;
    outmess[2] = v[0];
    chan->device->midiout->sendMessage(&outmess);
}

void pitch_wheel_handler(mapper_signal sig, mapper_id instance, const void *value,
                         int count, mapper_timetag_t *timetag)
{
    // pitch wheel messages passed straight through with no instances
    midimap_channel chan = (midimap_channel)mapper_signal_user_data(sig);
    if (!value || !chan || !chan->device || !chan->device->midiout)
        return;

    int *v = (int *)value;

    outmess[0] = chan->number + PITCH_WHEEL;
    outmess[1] = v[0];
    outmess[2] = v[0] >> 8;
    chan->device->midiout->sendMessage(&outmess);
}

void control_change_handler(mapper_signal sig, mapper_id instance, const void *value,
                            int count, mapper_timetag_t *timetag)
{
    // control change messages passed straight through with no instances
    midimap_channel chan = (midimap_channel)mapper_signal_user_data(sig);
    if (!value || !chan || !chan->device || !chan->device->midiout)
        return;

    int *v = (int *)value;

    outmess[0] = chan->number + CONTROL_CHANGE;
    outmess[1] = v[0];
    outmess[2] = v[1];
    chan->device->midiout->sendMessage(&outmess);
}

void program_change_handler(mapper_signal sig, mapper_id instance, const void *value,
                            int count, mapper_timetag_t *timetag)
{
    // program change messages passed straight through with no instances
    midimap_channel chan = (midimap_channel)mapper_signal_user_data(sig);
    if (!value || !chan || !chan->device || !chan->device->midiout)
        return;

    int *v = (int *)value;

    outmess[0] = chan->number + PROGRAM_CHANGE;
    outmess[1] = v[0];
    chan->device->midiout->sendMessage(&outmess);
}

void channel_pressure_handler(mapper_signal sig, mapper_id instance, const void *value,
                              int count, mapper_timetag_t *timetag)
{
    // channel pressure messages passed straight through with no instances
    midimap_channel chan = (midimap_channel)mapper_signal_user_data(sig);
    if (!value || !chan || !chan->device || !chan->device->midiout)
        return;

    int *v = (int *)value;

    outmess[0] = chan->number + CHANNEL_PRESSURE;
    outmess[1] = v[0];
    chan->device->midiout->sendMessage(&outmess);
}

void event_handler(mapper_signal sig, mapper_id instance,
                   mapper_instance_event event, mapper_timetag_t *timetag)
{
    if (!(event & MAPPER_INSTANCE_OVERFLOW))
        return;

    midimap_channel chan = (midimap_channel)mapper_signal_user_data(sig);

    // steal oldest instance
    mapper_id id = mapper_signal_oldest_active_instance(sig);

    // maybe release peer signals
    if (   sig == chan->pitch
        || sig == chan->velocity
        || sig == chan->aftertouch) {
        mapper_signal_instance_release(chan->pitch, id, MAPPER_NOW);
        mapper_signal_instance_release(chan->velocity, id, MAPPER_NOW);
        mapper_signal_instance_release(chan->aftertouch, id, MAPPER_NOW);
    }
    else {
        mapper_signal_instance_release(sig, id, MAPPER_NOW);
    }
}

void add_input_signals(midimap_device dev)
{
    mapper_signal sig;
    char signame[64];
    int i, min = 0, max7bit = 127, max14bit = 16383;
    for (i = 0; i < 16; i++) {
        midimap_channel chan = (midimap_channel) malloc(sizeof(struct _midimap_channel));
        chan->number = i;
        chan->device = dev;
        dev->channel[i] = chan;

        snprintf(signame, 64, "/channel.%i/note/pitch", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_INCOMING,
                                       INSTANCES, signame, 1, 'i', "midinote",
                                       &min, &max7bit, pitch_handler, chan);
        mapper_signal_set_instance_stealing_mode(sig, MAPPER_STEAL_OLDEST);
        dev->channel[i]->pitch = sig;

        snprintf(signame, 64, "/channel.%i/note/velocity", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_INCOMING,
                                       INSTANCES, signame, 1, 'i', 0, &min,
                                       &max7bit, velocity_handler, chan);
        mapper_signal_set_instance_stealing_mode(sig, MAPPER_STEAL_OLDEST);
        dev->channel[i]->velocity = sig;

        snprintf(signame, 64, "/channel.%i/note/aftertouch", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_INCOMING,
                                       INSTANCES, signame, 1, 'i', 0, &min,
                                       &max7bit, aftertouch_handler, chan);
        mapper_signal_set_instance_stealing_mode(sig, MAPPER_STEAL_OLDEST);
        dev->channel[i]->aftertouch = sig;

//        snprintf(signame, 64, "/channel.%i/note/pressure", i+1);
//        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_INCOMING,
//                                       INSTANCES, signame, 1, 'i', 0, &min,
//                                       &max7bit, poly_pressure_handler, chan);
//        mapper_signal_set_instance_stealing_mode(sig, MAPPER_STEAL_OLDEST);
//        dev->channel[i]->poly_pressure = sig;

        snprintf(signame, 64, "/channel.%i/pitch_wheel", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_INCOMING,
                                       INSTANCES, signame, 1, 'i', 0, &min,
                                       &max14bit, pitch_wheel_handler, chan);
        mapper_signal_set_instance_stealing_mode(sig, MAPPER_STEAL_OLDEST);
        dev->channel[i]->pitch_wheel = sig;

        // TODO: declare meaningful control change signals
        snprintf(signame, 64, "/channel.%i/control_change", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_INCOMING,
                                       INSTANCES, signame, 2, 'i', "midi", &min,
                                       &max7bit, control_change_handler, chan);
        mapper_signal_set_instance_stealing_mode(sig, MAPPER_STEAL_OLDEST);
        dev->channel[i]->control_change = sig;

        snprintf(signame, 64, "/channel.%i/program_change", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_INCOMING,
                                       INSTANCES, signame, 1, 'i', 0, &min,
                                       &max7bit, program_change_handler, chan);
        mapper_signal_set_instance_stealing_mode(sig, MAPPER_STEAL_OLDEST);
        dev->channel[i]->program_change = sig;

        snprintf(signame, 64, "/channel.%i/channel_pressure", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_INCOMING,
                                       INSTANCES, signame, 1, 'i', 0, &min,
                                       &max7bit, channel_pressure_handler, chan);
        mapper_signal_set_instance_stealing_mode(sig, MAPPER_STEAL_OLDEST);
        dev->channel[i]->channel_pressure = sig;
    }
}

// Declare output signals
void add_output_signals(midimap_device dev)
{
    mapper_signal sig;
    char signame[64];
    int i, min = 0, max7bit = 127, max14bit = 16383;
    for (i = 0; i < 16; i++) {
        midimap_channel chan = (midimap_channel) malloc(sizeof(struct _midimap_channel));
        chan->number = i;
        chan->device = dev;
        dev->channel[i] = chan;

        snprintf(signame, 64, "/channel.%i/note/pitch", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_OUTGOING,
                                       INSTANCES, signame, 1, 'i', "midinote",
                                       &min, &max7bit, 0, chan);
        mapper_signal_set_instance_event_callback(sig, event_handler,
                                                  MAPPER_INSTANCE_OVERFLOW);
        dev->channel[i]->pitch = sig;

        snprintf(signame, 64, "/channel.%i/note/velocity", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_OUTGOING,
                                       INSTANCES, signame, 1, 'i', 0, &min,
                                       &max7bit, 0, chan);
        mapper_signal_set_instance_event_callback(sig, event_handler,
                                                  MAPPER_INSTANCE_OVERFLOW);
        dev->channel[i]->velocity = sig;

        snprintf(signame, 64, "/channel.%i/note/aftertouch", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_OUTGOING,
                                       INSTANCES, signame, 1, 'i', 0, &min,
                                       &max7bit, 0, chan);
        mapper_signal_set_instance_event_callback(sig, event_handler,
                                                  MAPPER_INSTANCE_OVERFLOW);
        dev->channel[i]->aftertouch = sig;

//        snprintf(signame, 64, "/channel.%i/note/pressure", i+1);
//        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_OUTGOING,
//                                       INSTANCES, signame, 1, 'i', 0, &min,
//                                       &max7bit, 0, chan);
//        dev->channel[i]->poly_pressure = sig;

        snprintf(signame, 64, "/channel.%i/pitch_wheel", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_OUTGOING,
                                       INSTANCES, signame, 1, 'i', 0, &min,
                                       &max14bit, 0, chan);
        mapper_signal_set_instance_event_callback(sig, event_handler,
                                                  MAPPER_INSTANCE_OVERFLOW);
        dev->channel[i]->pitch_wheel = sig;

        // TODO: declare meaningful control change signals
        snprintf(signame, 64, "/channel.%i/control_change", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_OUTGOING,
                                       INSTANCES, signame, 2, 'i', "midi",
                                       &min, &max7bit, 0, chan);
        mapper_signal_set_instance_event_callback(sig, event_handler,
                                                  MAPPER_INSTANCE_OVERFLOW);
        dev->channel[i]->control_change = sig;

        snprintf(signame, 64, "/channel.%i/program_change", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_OUTGOING,
                                       INSTANCES, signame, 1, 'i', 0, &min,
                                       &max7bit, 0, chan);
        mapper_signal_set_instance_event_callback(sig, event_handler,
                                                  MAPPER_INSTANCE_OVERFLOW);
        dev->channel[i]->program_change = sig;

        snprintf(signame, 64, "/channel.%i/channel_pressure", i+1);
        sig = mapper_device_add_signal(dev->mapper_dev, MAPPER_DIR_OUTGOING,
                                       INSTANCES, signame, 1, 'i', 0, &min,
                                       &max7bit, 0, chan);
        mapper_signal_set_instance_event_callback(sig, event_handler,
                                                  MAPPER_INSTANCE_OVERFLOW);
        dev->channel[i]->channel_pressure = sig;
    }
}

void parse_midi(double deltatime, std::vector<unsigned char> *message, void *user_data)
{
    if ((unsigned int)message->size() != 3)
        return;

    midimap_device dev = (midimap_device)user_data;
    if (!mapper_device_ready(dev->mapper_dev))
        return;

    int msg_type = ((int)message->at(0) - 0x80) / 0x0F;
    int chan = ((int)message->at(0) - 0x80) % 0x0F - 1;
    int data[2] = {(int)message->at(1), (int)message->at(2)};

    mapper_timetag_now(&tt);
    mapper_device_start_queue(dev->mapper_dev, tt);
    switch (msg_type) {
        case 0: // note-off message
            mapper_signal_instance_release(dev->channel[chan]->pitch,
                                           data[0], tt);
            mapper_signal_instance_release(dev->channel[chan]->velocity,
                                           data[0], tt);
            mapper_signal_instance_release(dev->channel[chan]->aftertouch,
                                           data[0], tt);
            break;
        case 1: // note-on message
            if (data[1]) {
                mapper_signal_instance_update(dev->channel[chan]->pitch,
                                              data[0], &data[0], 1, tt);
                mapper_signal_instance_update(dev->channel[chan]->velocity,
                                              data[0], &data[1], 1, tt);
            }
            else {
                mapper_signal_instance_release(dev->channel[chan]->pitch,
                                               data[0], tt);
                mapper_signal_instance_release(dev->channel[chan]->velocity,
                                               data[0], tt);
                mapper_signal_instance_release(dev->channel[chan]->aftertouch,
                                               data[0], tt);
            }
            break;
        case 2: // aftertouch message
            mapper_signal_instance_update(dev->channel[chan]->aftertouch,
                                          data[0], &data[1], 1, tt);
            break;
        case 3: // control change message
            mapper_signal_update(dev->channel[chan]->control_change, (void *)data, 1, tt);
            break;
        case 4: // program change message
            mapper_signal_update(dev->channel[chan]->program_change, (void *)data, 1, tt);
            break;
        case 5: // channel pressure message
            mapper_signal_update(dev->channel[chan]->channel_pressure, (void *)data, 1, tt);
            break;
        case 6: // pitch wheel message
        {
            int value = data[0] + (data[1] << 8);
            mapper_signal_update(dev->channel[chan]->pitch_wheel, &value, 1, tt);
            break;
        }
        default:
            break;
    }
    mapper_device_send_queue(dev->mapper_dev, tt);
}

// Check if any MIDI ports are available on the system
void scan_midi_devices()
{
    printf("Searching for MIDI devices...\n");
    char devname[128];

    RtMidiIn *midiin = 0;
    RtMidiOut *midiout = 0;

    try {
        midiin = new RtMidiIn();
        unsigned int nPorts = midiin->getPortCount();
        std::cout << "There are " << nPorts << " MIDI input sources available.\n";

        for (unsigned int i=0; i<nPorts; i++) {
            std::string portName = midiin->getPortName(i);
            std::cout << "  Input Port #" << i+1 << ": " << portName << '\n';
            // check if record already exists
            midimap_device temp = outputs;
            while (temp) {
                if (portName.compare(temp->name) == 0) {
                    break;
                }
                temp = temp->next;
            }
            if (temp)
                continue;
            // new device discovered
            midimap_device dev = (midimap_device) calloc(1, sizeof(struct _midimap_device));
            dev->name = strdup(portName.c_str());
            // remove illegal characters in device name
            unsigned int len = strlen(dev->name), k = 0;
            for (unsigned int j=0; j<len; j++) {
                if (isalnum(dev->name[j])) {
                    devname[k++] = dev->name[j];
                    devname[k] = 0;
                }
            }
            dev->mapper_dev = mapper_device_new(devname, 0, 0);
            dev->midiin = new RtMidiIn();
            dev->midiin->openPort(i);
            dev->midiin->setCallback(&parse_midi, dev);
            dev->midiin->ignoreTypes(true, true, true);
            dev->next = outputs;
            outputs = dev;
            add_output_signals(dev);
        }
    }
    catch (RtError &error) {
        error.printMessage();
    }

    delete midiin;

    try {
        midiout = new RtMidiOut();
        unsigned int nPorts = midiout->getPortCount();
        std::cout << "There are " << nPorts << " MIDI output ports available.\n";

        for (unsigned int i=0; i<nPorts; i++) {
            std::string portName = midiout->getPortName(i);
            std::cout << "  Output Port #" << i+1 << ": " << portName << '\n';
            // check if record already exists
            midimap_device temp = inputs;
            while (temp) {
                if (portName.compare(temp->name) == 0) {
                    break;
                }
                temp = temp->next;
            }
            if (temp)
                continue;
            // new device discovered
            midimap_device dev = (midimap_device) calloc(1, sizeof(struct _midimap_device));
            dev->name = strdup(portName.c_str());
            // remove illegal characters in device name
            unsigned int len = strlen(dev->name), k = 0;
            for (unsigned int j=0; j<len; j++) {
                if (isalnum(dev->name[j])) {
                    devname[k++] = dev->name[j];
                    devname[k] = 0;
                }
            }
            dev->mapper_dev = mapper_device_new(devname, 0, 0);
            dev->midiin = 0;
            dev->midiout = new RtMidiOut();
            dev->midiout->openPort(i);
            dev->next = inputs;
            inputs = dev;
            add_input_signals(dev);
        }
    }
    catch (RtError &error) {
        error.printMessage();
    }

    delete midiout;

    return;

    /*
    // check for dropped devices
    midimap_device *temp = &outputs;
    midimap_device found = 0;
    while (*temp) {
        found  = 0;
        for (i = 0; i < Pm_CountDevices(); i++) {
            const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
            if (strcmp((*temp)->name, info->name) == 0) {
                found = *temp;
                break;
            }
        }
        if (!found) {
            // MIDI device has disappeared
            printf("    Removed %s\n", found->name);
            *temp = found->next;
            cleanup_device(found);
        }
        temp = &(*temp)->next;
    }*/
}

void cleanup_device(midimap_device dev)
{
    int i;
    if (dev->name) {
        free(dev->name);
    }
    if (dev->mapper_dev) {
        mapper_device_free(dev->mapper_dev);
    }
    if (dev->midiin) {
        delete dev->midiin;
    }
    if (dev->midiout) {
        delete dev->midiout;
    }
    for (i=0; i<16; i++) {
        free(dev->channel[i]);
    }
}

void cleanup_all_devices()
{
    printf("\nCleaning up!\n");
    midimap_device dev;
    while (inputs) {
        dev = inputs;
        inputs = dev->next;
        cleanup_device(dev);
    }
    while (outputs) {
        dev = outputs;
        outputs = dev->next;
        cleanup_device(dev);
    }
}

void loop()
{
    //int counter = 0;
    midimap_device temp;

    scan_midi_devices();

    while (!done) {
        // poll libmapper outputs
        temp = outputs;
        while (temp) {
            mapper_device_poll(temp->mapper_dev, 0);
            temp = temp->next;
        }
        // poll libmapper inputs
        temp = inputs;
        while (temp) {
            mapper_device_poll(temp->mapper_dev, 0);
            temp = temp->next;
        }
        usleep(10 * 1000);
        // TODO: debug & enable MIDI device rescan
        //if (counter++ > 500) {
        //    scan_midi_devices();
        //    counter = 0;
        //}
    }
}

void ctrlc(int sig)
{
    done = 1;
}

int main ()
{
    signal(SIGINT, ctrlc);

    loop();

    cleanup_all_devices();
    return 0;
}
