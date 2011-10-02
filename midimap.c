
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "midimap.h"

#include <unistd.h>
#include <signal.h>

#define INPUT_BUFFER_SIZE 100
#define OUTPUT_BUFFER_SIZE 0
#define DRIVER_INFO NULL
#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)
#define TIME_INFO NULL
#define TIME_START Pt_Start(1, 0, 0) /* timer started w/millisecond accuracy */

int32_t latency = 0;

int done = 0;
int port = 9000;

typedef struct _midimap_device {
    mapper_device   dev;
    PmStream        *stream;
    int             is_linked;
    mapper_signal   signals[8][16];
    struct _midimap_device *next;
} *midimap_device;

struct _midimap_device *inputs = 0;
struct _midimap_device *outputs = 0;

void signal_handler(mapper_signal sig, mapper_db_signal props,
                    mapper_timetag_t *timetag, void *value)
{
    if (value) {
        printf("--> destination got %s", props->name);
        float *v = value;
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
    int *v = value;
    Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0x80),
                             (uint8_t)v[0], (uint8_t)v[1]));    
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
    int *v = value;
    Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0x90),
                             (uint8_t)v[0], (uint8_t)v[1]));    
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
    int *v = value;
    Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0xA0),
                             (uint8_t)v[0], (uint8_t)v[1]));
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
    int *v = value;
    Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0xB0),
                             (uint8_t)v[0], (uint8_t)v[1]));
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
    int *v = value;
    Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0xC0),
                             (uint8_t)v[0], (uint8_t)v[1]));
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
    int *v = value;
    Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0xD0),
                             (uint8_t)v[0], (uint8_t)v[1]));
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
    int *v = value;
    Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message((uint8_t)(channel_num + 0xE0),
                             (uint8_t)v[0], (uint8_t)v[0] >> 8));
}

void add_input_signals(midimap_device dev)
{
    char signame[64];
    int i, min = 0, max7bit = 127, max14bit = 16383;
    for (i = 1; i < 17; i++) {
        snprintf(signame, 64, "/channel.%i/noteoff", i);
        dev->signals[0][i] = mdev_add_input(dev->dev, signame, 2, 'i', "midi",
                                            &min, &max7bit, noteoff_handler, dev);
        snprintf(signame, 64, "/channel.%i/noteon", i);
        dev->signals[1][i] = mdev_add_input(dev->dev, signame, 2, 'i', "midi",
                                            &min, &max7bit, noteon_handler, dev);
        snprintf(signame, 64, "/channel.%i/aftertouch", i);
        dev->signals[2][i] = mdev_add_input(dev->dev, signame, 2, 'i', "midi",
                                            &min, &max7bit, aftertouch_handler, dev);
        snprintf(signame, 64, "/channel.%i/control_change", i);
        dev->signals[3][i] = mdev_add_input(dev->dev, signame, 2, 'i', "midi",
                                            &min, &max7bit, control_change_handler, dev);
        snprintf(signame, 64, "/channel.%i/program_change", i);
        dev->signals[4][i] = mdev_add_input(dev->dev, signame, 2, 'i', "midi",
                                            &min, &max7bit, program_change_handler, dev);
        snprintf(signame, 64, "/channel.%i/channel_pressure", i);
        dev->signals[5][i] = mdev_add_input(dev->dev, signame, 2, 'i', "midi",
                                            &min, &max7bit, channel_pressure_handler, dev);
        snprintf(signame, 64, "/channel.%i/pitch_wheel", i);
        dev->signals[6][i] = mdev_add_input(dev->dev, signame, 1, 'i', "midi",
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
        dev->signals[0][i] = mdev_add_output(dev->dev, signame, 2,
                                             'i', "midi", &min, &max7bit);
        snprintf(signame, 64, "/channel.%i/noteon", i);
        dev->signals[1][i] = mdev_add_output(dev->dev, signame, 2,
                                             'i', "midi", &min, &max7bit);
        snprintf(signame, 64, "/channel.%i/aftertouch", i);
        dev->signals[2][i] = mdev_add_output(dev->dev, signame, 2,
                                             'i', "midi", &min, &max7bit);
        snprintf(signame, 64, "/channel.%i/control_change", i);
        dev->signals[3][i] = mdev_add_output(dev->dev, signame, 2,
                                             'i', "midi", &min, &max7bit);
        snprintf(signame, 64, "/channel.%i/program_change", i);
        dev->signals[4][i] = mdev_add_output(dev->dev, signame, 2,
                                             'i', "midi", &min, &max7bit);
        snprintf(signame, 64, "/channel.%i/channel_pressure", i);
        dev->signals[5][i] = mdev_add_output(dev->dev, signame, 2,
                                             'i', "midi", &min, &max7bit);
        snprintf(signame, 64, "/channel.%i/pitch_wheel", i);
        dev->signals[6][i] = mdev_add_output(dev->dev, signame, 1,
                                             'i', "midi", &min, &max14bit);
    }
}

// Check if any MIDI ports are available on the system
void search_midi()
{
    PmEvent buffer[1];
    int i;
    char devname[128], *position;
    if (Pm_CountDevices() == 0) {
        printf("No MIDI devices found!\n");
        return;
    }
    for (i = 0; i < Pm_CountDevices(); i++) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        snprintf(devname, 128, "%s", info->name);
        while (position = strchr(devname, ' ')) {
            *position = '_';
        }
        midimap_device dev = (midimap_device) calloc(1, sizeof(struct _midimap_device));
        dev->dev = mdev_new(devname, port, 0);
        if (info->input) {
            printf("Got MIDI input %d %s %s...", i, info->interf, info->name);
            // TODO: Should only open input if it is mapped
            Pm_OpenInput(&dev->stream, i, DRIVER_INFO, INPUT_BUFFER_SIZE, TIME_PROC, TIME_INFO);
            Pm_SetFilter(dev->stream, PM_FILT_ACTIVE | PM_FILT_CLOCK | PM_FILT_SYSEX);
            /* empty the buffer after setting filter, just in case anything
             got through */
            while (Pm_Poll(dev->stream)) {
                Pm_Read(dev->stream, buffer, 1);
            }
            dev->next = outputs;
            outputs = dev;
            add_output_signals(dev);
            printf("added.\n");
        }
        if (info->output) {
            printf("Got MIDI output %d %s %s...", i, info->interf, info->name);
            // TODO: Should only open output if it is mapped
            Pm_OpenOutput(&dev->stream, i, DRIVER_INFO, OUTPUT_BUFFER_SIZE, TIME_PROC, TIME_INFO, latency);
            dev->next = inputs;
            inputs = dev;
            add_input_signals(dev);
            printf("added.\n");
        }
    }
}

void parse_midi(midimap_device dev, PmEvent buffer)
{
    int msg_type = (Pm_MessageStatus(buffer.message) - 0x80) / 0x0F;
    int channel = (Pm_MessageStatus(buffer.message) - 0x80) % 0x0F;
    int data[2] = {Pm_MessageData1(buffer.message),
                   Pm_MessageData2(buffer.message)};

    /* TODO: add array of signal pointers to device, use array index instead
     * of mdev_get_output_by_name()  */

    switch (msg_type) {
        case 0:
            // note-off message
            msig_update(dev->signals[0][channel], data);
            break;
        case 1:
            // note-on message
            msig_update(dev->signals[1][channel], data);
            break;
        case 2:
            // aftertouch
            msig_update(dev->signals[2][channel], data);
            break;
        case 3:
            // control change
            msig_update(dev->signals[3][channel], data);
            break;
        case 4:
            // program change
            msig_update(dev->signals[4][channel], data);
            break;
        case 5:
            // channel pressure
            msig_update(dev->signals[5][channel], data);
            break;
        case 6:
            // pitch wheel
            data[1] = data[1] + (data[2] << 8);
            msig_update(dev->signals[6][channel], data);
            break;
        default:
            break;
    }
}

void cleanup_device(midimap_device dev)
{
    if (dev->dev) {
        mdev_free(dev->dev);
    }
    if (dev->stream) {
        Pm_Close(dev->stream);
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
    Pm_Terminate();
}

void loop()
{
    midimap_device temp;
    PmEvent buffer[1];
    search_midi();
    int i;
    while (!done) {
        // TODO: check periodically for new/dropped  MIDI devices
        // poll libmapper outputs
        temp = outputs;
        while (temp) {
            mdev_poll(temp->dev, 0);
            i = 0;
            while (Pm_Poll(temp->stream) && (i++ < 10)) {
                if (Pm_Read(temp->stream, buffer, 1) > 0) {
                    parse_midi(temp, buffer[0]);
                }
            }
            temp = temp->next;
        }
        // poll libmapper inputs
        temp = inputs;
        while (temp) {
            mdev_poll(temp->dev, 0);
            temp = temp->next;
        }
        usleep(10 * 1000);
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
    
done:
    cleanup_all_devices();
    return 0;
}