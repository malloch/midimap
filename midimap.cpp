
#include <iostream>
#include <cstdlib>
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
int port = 9000;
mapper_timetag_t tt;

typedef struct _midimap_device {
    char            *name;
    mapper_device   mapper_dev;
    RtMidiIn        *midiin;
    RtMidiOut       *midiout;
    int             is_linked;
    mapper_signal   signals[8][16];
    struct _midimap_device *next;
} *midimap_device;

struct _midimap_device *inputs = 0;
struct _midimap_device *outputs = 0;

std::vector<unsigned char> outmess (3, 0);

void cleanup_device(midimap_device dev);

int get_channel_from_signame(const char *name)
{
    char channel_str[4] = {0, 0, 0, 0};
    int channel = 0;
    if (name[8] != '.')
        return -1;
    strncpy(channel_str, &name[9], 3);
    channel_str[strchr(channel_str, '/') - channel_str] = 0;
    channel = atoi(channel_str) - 1;
    if (channel < 1 || channel > 16)
        return -1;
    return channel;
}

void pitch_handler(mapper_signal sig,
                   mapper_db_signal props,
                   int instance_id,
                   void *value,
                   int count,
                   mapper_timetag_t *timetag)
{
    // noteoff messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev)
        return;
    if (!dev->midiout)
        return;

    int channel = get_channel_from_signame(props->name);

    int *v = (int *)value;

    // if velocity already received, output pitch-bend message
    unsigned char *velocity = (unsigned char *)msig_instance_value(dev->signals[1][channel],
                                                                   instance_id, 0);
    int *cached = (int *)msig_get_instance_data(sig, instance_id);
    if (velocity) {
        unsigned int pitchbend = 0;
        if (cached)
            pitchbend = (v[0] - (float)(*cached)) / 12.0f * 8152.0f + 8152;
        else
            pitchbend = (v[0] - 60.0f) / 12.0f * 8152.0f + 8152;
        outmess[0] = channel + PITCH_WHEEL;
        outmess[1] = (unsigned char)pitchbend;
        outmess[2] = (unsigned char)pitchbend >> 8;
        dev->midiout->sendMessage(&outmess);
    }
    else {
        // cache pitch value in user_data
        if (cached)
            *cached = *v;
        else {
            cached = (int *)malloc(sizeof(int));
            *cached = *v;
            msig_set_instance_data(sig, instance_id, cached);
        }
    }
}

void velocity_handler(mapper_signal sig,
                      mapper_db_signal props,
                      int instance_id,
                      void *value,
                      int count,
                      mapper_timetag_t *timetag)
{
    // noteon messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev)
        return;
    if (!dev->midiout)
        return;

    int channel = get_channel_from_signame(props->name);
    int *v = (int *)value;

    // if pitch already received, output MIDI message
    unsigned char pitch = (long int)msig_instance_value(dev->signals[0][channel],
                                                        instance_id, 0);
    outmess[0] = channel + NOTE_ON;
    outmess[1] = pitch ? pitch : 60;
    outmess[2] = v[0];
    dev->midiout->sendMessage(&outmess);
}

void aftertouch_handler(mapper_signal sig,
                        mapper_db_signal props,
                        int instance_id,
                        void *value,
                        int count,
                        mapper_timetag_t *timetag)
{
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev || !value)
        return;

    int channel = get_channel_from_signame(props->name);

    // check if note exists
    if (!msig_instance_value(dev->signals[1][channel], instance_id, 0))
        return;

    // if note info exists, output MIDI message
    unsigned char note = (long int)msig_get_instance_data(dev->signals[0][channel],
                                                          instance_id);
    if (note) {
        int *v = (int *)value;
        outmess[0] = channel + AFTERTOUCH;
        outmess[1] = note;
        outmess[2] = v[0];
        dev->midiout->sendMessage(&outmess);
    }
}

void control_change_handler(mapper_signal sig,
                            mapper_db_signal props,
                            int instance_id,
                            void *value,
                            int count,
                            mapper_timetag_t *timetag)
{
    // control change messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev || !value)
        return;

    int channel = get_channel_from_signame(props->name);
    int *v = (int *)value;

    outmess[0] = channel + CONTROL_CHANGE;
    outmess[1] = v[0];
    outmess[2] = v[1];
    dev->midiout->sendMessage(&outmess);
}

void program_change_handler(mapper_signal sig,
                            mapper_db_signal props,
                            int instance_id,
                            void *value,
                            int count,
                            mapper_timetag_t *timetag)
{
    // program change messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev || !value)
        return;

    int channel = get_channel_from_signame(props->name);
    int *v = (int *)value;

    outmess[0] = channel + PROGRAM_CHANGE;
    outmess[1] = v[0];
    dev->midiout->sendMessage(&outmess);
}

void channel_pressure_handler(mapper_signal sig,
                              mapper_db_signal props,
                              int instance_id,
                              void *value,
                              int count,
                              mapper_timetag_t *timetag)
{
    // channel pressure messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev || !value)
        return;

    int channel = get_channel_from_signame(props->name);
    int *v = (int *)value;

    outmess[0] = channel + CHANNEL_PRESSURE;
    outmess[1] = v[0];
    dev->midiout->sendMessage(&outmess);
}

void add_input_signals(midimap_device dev)
{
    char signame[64];
    int i, min = 0, max = 127;
    for (i = 1; i < 17; i++) {
        snprintf(signame, 64, "/channel.%i/note/pitch", i);
        dev->signals[0][i] = mdev_add_input(dev->mapper_dev, signame, 1, 'i', "midinote",
                                            &min, &max, pitch_handler, dev);
        msig_reserve_instances(dev->signals[0][i], INSTANCES - 1);

        snprintf(signame, 64, "/channel.%i/note/velocity", i);
        dev->signals[1][i] = mdev_add_input(dev->mapper_dev, signame, 1, 'i', 0,
                                            &min, &max, velocity_handler, dev);
        msig_reserve_instances(dev->signals[1][i], INSTANCES - 1);

        snprintf(signame, 64, "/channel.%i/note/aftertouch", i);
        dev->signals[2][i] = mdev_add_input(dev->mapper_dev, signame, 1, 'i', 0,
                                            &min, &max, aftertouch_handler, dev);
        msig_reserve_instances(dev->signals[2][i], INSTANCES - 1);

        // TODO: declare meaningful control change signals
        snprintf(signame, 64, "/channel.%i/control_change", i);
        dev->signals[3][i] = mdev_add_input(dev->mapper_dev, signame, 2, 'i', "midi",
                                            &min, &max, control_change_handler, dev);
        msig_reserve_instances(dev->signals[3][i], INSTANCES - 1);

        snprintf(signame, 64, "/channel.%i/program_change", i);
        dev->signals[4][i] = mdev_add_input(dev->mapper_dev, signame, 1, 'i', 0,
                                            &min, &max, program_change_handler, dev);
        msig_reserve_instances(dev->signals[4][i], INSTANCES - 1);

        snprintf(signame, 64, "/channel.%i/channel_pressure", i);
        dev->signals[5][i] = mdev_add_input(dev->mapper_dev, signame, 1, 'i', 0,
                                            &min, &max, channel_pressure_handler, dev);
        msig_reserve_instances(dev->signals[5][i], INSTANCES - 1);
    }
}

// Declare output signals
void add_output_signals(midimap_device dev)
{
    char signame[64];
    int i, min = 0, max = 127;
    // TODO: Need to declare these signals for each MIDI channel
    for (i = 1; i < 17; i++) {
        snprintf(signame, 64, "/channel.%i/note/pitch", i);
        dev->signals[0][i] = mdev_add_output(dev->mapper_dev, signame, 1,
                                             'i', "midinote", &min, &max);
        msig_reserve_instances(dev->signals[0][i], INSTANCES - 1);

        snprintf(signame, 64, "/channel.%i/note/velocity", i);
        dev->signals[1][i] = mdev_add_output(dev->mapper_dev, signame, 1,
                                             'i', 0, &min, &max);
        msig_reserve_instances(dev->signals[1][i], INSTANCES - 1);

        snprintf(signame, 64, "/channel.%i/note/aftertouch", i);
        dev->signals[2][i] = mdev_add_output(dev->mapper_dev, signame, 1,
                                             'i', 0, &min, &max);
        msig_reserve_instances(dev->signals[2][i], INSTANCES - 1);

        // TODO: declare meaningful control change signals
        snprintf(signame, 64, "/channel.%i/control_change", i);
        dev->signals[3][i] = mdev_add_output(dev->mapper_dev, signame, 2,
                                             'i', "midi", &min, &max);
        msig_reserve_instances(dev->signals[3][i], INSTANCES - 1);

        snprintf(signame, 64, "/channel.%i/program_change", i);
        dev->signals[4][i] = mdev_add_output(dev->mapper_dev, signame, 1,
                                             'i', 0, &min, &max);
        msig_reserve_instances(dev->signals[4][i], INSTANCES - 1);

        snprintf(signame, 64, "/channel.%i/channel_pressure", i);
        dev->signals[5][i] = mdev_add_output(dev->mapper_dev, signame, 1,
                                             'i', 0, &min, &max);
        msig_reserve_instances(dev->signals[5][i], INSTANCES - 1);
    }
}

void parse_midi(double deltatime, std::vector<unsigned char> *message, void *user_data)
{
    if ((unsigned int)message->size() != 3)
        return;

    midimap_device dev = (midimap_device)user_data;
    if (!mdev_ready(dev->mapper_dev))
        return;

    int msg_type = ((int)message->at(0) - 0x80) / 0x0F;
    int channel = ((int)message->at(0) - 0x80) % 0x0F;
    int data[2] = {(int)message->at(1), (int)message->at(2)};

    mdev_timetag_now(dev->mapper_dev, &tt);
    mdev_start_queue(dev->mapper_dev, tt);
    switch (msg_type) {
        case 0:
            // note-off message
            msig_release_instance(dev->signals[0][channel],
                                  data[0], tt);
            msig_release_instance(dev->signals[1][channel],
                                  data[0], tt);
            // TODO: release aftertouch instance?
            break;
        case 1:
            // note-on message
            if (data[1]) {
                msig_update_instance(dev->signals[0][channel],
                                     data[0], &data[0], 1, tt);
                msig_update_instance(dev->signals[1][channel],
                                     data[0], &data[1], 1, tt);
            }
            else {
                msig_release_instance(dev->signals[0][channel],
                                      data[0], tt);
                msig_release_instance(dev->signals[1][channel],
                                      data[0], tt);
            }
            break;
        case 2:
            // aftertouch
            msig_update_instance(dev->signals[2][channel],
                                 data[0], &data[1], 1, tt);
            break;
        case 3:
            // control change
            msig_update(dev->signals[3][channel], (void *)data, 1, tt);
            break;
        case 4:
            // program change
            msig_update(dev->signals[4][channel], (void *)data, 1, tt);
            break;
        case 5:
            // channel pressure
            msig_update(dev->signals[5][channel], (void *)data, 1, tt);
            break;
        case 6:
        {
            // pitch wheel
            int value = data[0] + (data[1] << 8);
            msig_update(dev->signals[6][channel], &value, 1, tt);
            break;
        }
        default:
            break;
    }
    mdev_send_queue(dev->mapper_dev, tt);
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
            dev->mapper_dev = mdev_new(devname, port, 0);
            dev->midiin = new RtMidiIn();
            dev->next = outputs;
            outputs = dev;
            // TODO: check if device is linked/connected (perhaps per-channel?)
            add_output_signals(dev);
            dev->midiin->openPort(i);
            dev->midiin->setCallback(&parse_midi, dev);
            dev->midiin->ignoreTypes(true, true, true);
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
            dev->mapper_dev = mdev_new(devname, port, 0);
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
    if (dev->name) {
        free(dev->name);
    }
    if (dev->mapper_dev) {
        mdev_free(dev->mapper_dev);
    }
    if (dev->midiin) {
        delete dev->midiin;
    }
    if (dev->midiout) {
        delete dev->midiout;
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
            mdev_poll(temp->mapper_dev, 0);
            temp = temp->next;
        }
        // poll libmapper inputs
        temp = inputs;
        while (temp) {
            mdev_poll(temp->mapper_dev, 0);
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