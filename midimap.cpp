
#include <iostream>
#include <cstdlib>
#include "RtMidi.h"
#include "mapper/mapper.h"

int done = 0;
int port = 9000;

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

void cleanup_device(midimap_device dev);

void signal_handler(mapper_signal sig, mapper_db_signal props,
                    mapper_timetag_t *timetag, void *value)
{
    if (value) {
        printf("--> destination got %s", props->name);
        float *v = (float *)value;
        for (int i = 0; i < props->length; i++) {
            printf(" %f", v[i]);
        }
        printf("\n");
    }
}

void noteoff_handler(mapper_signal sig, mapper_db_signal props,
                     mapper_timetag_t *timetag, void *value)
{
    // noteoff messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev)
        return;
    char channel[4] = {0, 0, 0, 0};
    int channel_num = 0;
    if (props->name[8] != '.')
        return;
    // extract channel number from signal name
    strncpy(channel, &props->name[9], 3);
    channel[strchr(channel, '/') - channel] = 0;
    channel_num = atoi(channel);
    if (channel_num < 1 || channel_num > 16)
        return;
    //int *v = (int *)value;
    /*Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0x80),
                             (uint8_t)v[0], (uint8_t)v[1]));*/
}

void noteon_handler(mapper_signal sig, mapper_db_signal props,
                    mapper_timetag_t *timetag, void *value)
{
    // noteon messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev)
        return;
    char channel[4] = {0, 0, 0, 0};
    int channel_num = 0;
    if (props->name[8] != '.')
        return;
    // extract channel number from signal name
    strncpy(channel, &props->name[9], 3);
    channel[strchr(channel, '/') - channel] = 0;
    channel_num = atoi(channel);
    if (channel_num < 1 || channel_num > 16)
        return;
    //int *v = (int *)value;
    /*Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0x90),
                             (uint8_t)v[0], (uint8_t)v[1]));*/
}

void aftertouch_handler(mapper_signal sig, mapper_db_signal props,
                        mapper_timetag_t *timetag, void *value)
{
    // aftertouch messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev)
        return;
    char channel[4] = {0, 0, 0, 0};
    int channel_num = 0;
    if (props->name[8] != '.')
        return;
    // extract channel number from signal name
    strncpy(channel, &props->name[9], 3);
    channel[strchr(channel, '/') - channel] = 0;
    channel_num = atoi(channel);
    if (channel_num < 1 || channel_num > 16)
        return;
    //int *v = (int *)value;
    /*Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0xA0),
                             (uint8_t)v[0], (uint8_t)v[1]));*/
}

void control_change_handler(mapper_signal sig, mapper_db_signal props,
                            mapper_timetag_t *timetag, void *value)
{
    // control change messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev)
        return;
    char channel[4] = {0, 0, 0, 0};
    int channel_num = 0;
    if (props->name[8] != '.')
        return;
    // extract channel number from signal name
    strncpy(channel, &props->name[9], 3);
    channel[strchr(channel, '/') - channel] = 0;
    channel_num = atoi(channel);
    if (channel_num < 1 || channel_num > 16)
        return;
    //int *v = (int *)value;
    /*Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0xB0),
                             (uint8_t)v[0], (uint8_t)v[1]));*/
}

void program_change_handler(mapper_signal sig, mapper_db_signal props,
                            mapper_timetag_t *timetag, void *value)
{
    // program change messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev)
        return;
    char channel[4] = {0, 0, 0, 0};
    int channel_num = 0;
    if (props->name[8] != '.')
        return;
    // extract channel number from signal name
    strncpy(channel, &props->name[9], 3);
    channel[strchr(channel, '/') - channel] = 0;
    channel_num = atoi(channel);
    if (channel_num < 1 || channel_num > 16)
        return;
    //int *v = (int *)value;
    /*Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0xC0),
                             (uint8_t)v[0], (uint8_t)v[1]));*/
}

void channel_pressure_handler(mapper_signal sig, mapper_db_signal props,
                              mapper_timetag_t *timetag, void *value)
{
    // channel pressure messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev)
        return;
    char channel[4] = {0, 0, 0, 0};
    int channel_num = 0;
    if (props->name[8] != '.')
        return;
    // extract channel number from signal name
    strncpy(channel, &props->name[9], 3);
    channel[strchr(channel, '/') - channel] = 0;
    channel_num = atoi(channel);
    if (channel_num < 1 || channel_num > 16)
        return;
    //int *v = (int *)value;
    /*Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0xD0),
                             (uint8_t)v[0], (uint8_t)v[1]));*/
}

void pitch_wheel_handler(mapper_signal sig, mapper_db_signal props,
                         mapper_timetag_t *timetag, void *value)
{
    // pitch wheel messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev)
        return;
    char channel[4] = {0, 0, 0, 0};
    int channel_num = 0;
    if (props->name[8] != '.')
        return;
    // extract channel number from signal name
    strncpy(channel, &props->name[9], 3);
    channel[strchr(channel, '/') - channel] = 0;
    channel_num = atoi(channel);
    if (channel_num < 1 || channel_num > 16)
        return;
    //int *v = (int *)value;
    /*Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0xE0),
                             (uint8_t)v[0], (uint8_t)v[0] >> 8));*/
}

void add_input_signals(midimap_device dev)
{
    char signame[64];
    int i, min = 0, max7bit = 127, max14bit = 16383;
    for (i = 1; i < 17; i++) {
        snprintf(signame, 64, "/channel.%i/noteoff", i);
        dev->signals[0][i] = mdev_add_input(dev->mapper_dev, signame, 2, 'i', "midi",
                                            &min, &max7bit, noteoff_handler, dev);
        snprintf(signame, 64, "/channel.%i/noteon", i);
        dev->signals[1][i] = mdev_add_input(dev->mapper_dev, signame, 2, 'i', "midi",
                                            &min, &max7bit, noteon_handler, dev);
        snprintf(signame, 64, "/channel.%i/aftertouch", i);
        dev->signals[2][i] = mdev_add_input(dev->mapper_dev, signame, 2, 'i', "midi",
                                            &min, &max7bit, aftertouch_handler, dev);
        snprintf(signame, 64, "/channel.%i/control_change", i);
        dev->signals[3][i] = mdev_add_input(dev->mapper_dev, signame, 2, 'i', "midi",
                                            &min, &max7bit, control_change_handler, dev);
        snprintf(signame, 64, "/channel.%i/program_change", i);
        dev->signals[4][i] = mdev_add_input(dev->mapper_dev, signame, 2, 'i', "midi",
                                            &min, &max7bit, program_change_handler, dev);
        snprintf(signame, 64, "/channel.%i/channel_pressure", i);
        dev->signals[5][i] = mdev_add_input(dev->mapper_dev, signame, 2, 'i', "midi",
                                            &min, &max7bit, channel_pressure_handler, dev);
        snprintf(signame, 64, "/channel.%i/pitch_wheel", i);
        dev->signals[6][i] = mdev_add_input(dev->mapper_dev, signame, 1, 'i', "midi",
                                            &min, &max14bit, pitch_wheel_handler, dev);
    }
}

// Declare output signals
void add_output_signals(midimap_device dev)
{
    char signame[64];
    int i, min = 0, max7bit = 127, max14bit = 16383;
    // TODO: Need to declare these signals for each MIDI channel
    for (i = 1; i < 17; i++) {
        snprintf(signame, 64, "/channel.%i/noteoff", i);
        dev->signals[0][i] = mdev_add_output(dev->mapper_dev, signame, 2,
                                             'i', "midi", &min, &max7bit);
        snprintf(signame, 64, "/channel.%i/noteon", i);
        dev->signals[1][i] = mdev_add_output(dev->mapper_dev, signame, 2,
                                             'i', "midi", &min, &max7bit);
        snprintf(signame, 64, "/channel.%i/aftertouch", i);
        dev->signals[2][i] = mdev_add_output(dev->mapper_dev, signame, 2,
                                             'i', "midi", &min, &max7bit);
        snprintf(signame, 64, "/channel.%i/control_change", i);
        dev->signals[3][i] = mdev_add_output(dev->mapper_dev, signame, 2,
                                             'i', "midi", &min, &max7bit);
        snprintf(signame, 64, "/channel.%i/program_change", i);
        dev->signals[4][i] = mdev_add_output(dev->mapper_dev, signame, 2,
                                             'i', "midi", &min, &max7bit);
        snprintf(signame, 64, "/channel.%i/channel_pressure", i);
        dev->signals[5][i] = mdev_add_output(dev->mapper_dev, signame, 2,
                                             'i', "midi", &min, &max7bit);
        snprintf(signame, 64, "/channel.%i/pitch_wheel", i);
        dev->signals[6][i] = mdev_add_output(dev->mapper_dev, signame, 1,
                                             'i', "midi", &min, &max14bit);
    }
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
            dev->midiin->openPort(i);
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

void parse_midi(midimap_device dev, uint8_t *message)
{
    int msg_type = (message[0] - 0x80) / 0x0F;
    int channel = (message[0] - 0x80) % 0x0F;
    int data[2] = {message[1], message[2]};

    switch (msg_type) {
        case 0:
            // note-off message
            msig_update(dev->signals[0][channel], (void *)data);
            break;
        case 1:
            // note-on message
            msig_update(dev->signals[1][channel], (void *)data);
            break;
        case 2:
            // aftertouch
            msig_update(dev->signals[2][channel], (void *)data);
            break;
        case 3:
            // control change
            msig_update(dev->signals[3][channel], (void *)data);
            break;
        case 4:
            // program change
            msig_update(dev->signals[4][channel], (void *)data);
            break;
        case 5:
            // channel pressure
            msig_update(dev->signals[5][channel], (void *)data);
            break;
        case 6:
            // pitch wheel
            data[1] = data[1] + (data[2] << 8);
            msig_update(dev->signals[6][channel], (void *)data);
            break;
        default:
            break;
    }
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
    std::vector<unsigned char> message;
    int nBytes, i;
    double stamp;
    //int counter = 0;
    midimap_device temp;

    scan_midi_devices();

    while (!done) {
        // poll libmapper outputs
        temp = outputs;
        while (temp) {
            mdev_poll(temp->mapper_dev, 0);
            stamp = temp->midiin->getMessage(&message);
            nBytes = message.size();
            for ( i=0; i<nBytes; i++ )
                std::cout << temp->name << "Byte " << i << " = " << (int)message[i] << ", ";
            if ( nBytes > 0 )
                std::cout << "stamp = " << stamp << std::endl;
            temp = temp->next;
        }
        // poll libmapper inputs
        temp = inputs;
        while (temp) {
            mdev_poll(temp->mapper_dev, 0);
            temp = temp->next;
        }
        usleep(10 * 1000);
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