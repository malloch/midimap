
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

void noteon_handler(mapper_signal sig, mapper_db_signal props,
                    mapper_timetag_t *timetag, void *value)
{
    // normal MIDI noteon messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev)
        return;
    int *v = value;
    Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message(0x90, (uint8_t)v[0], (uint8_t)v[1]));    
}

void noteoff_handler(mapper_signal sig, mapper_db_signal props,
                    mapper_timetag_t *timetag, void *value)
{
    // normal MIDI noteoff messages passed straight through with no instances
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev)
        return;
    int *v = value;
    Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                  Pm_Message(0x80, (uint8_t)v[0], (uint8_t)v[1]));    
}

void pitch_handler(mapper_signal sig, mapper_db_signal props,
                   mapper_timetag_t *timetag, void *value)
{
    midimap_device dev = (midimap_device)props->user_data;
    if (!dev)
        return;
    int *v = value;
    uint8_t b = v[0];
    if (value) {
        Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                      Pm_Message(0x90, b, 100));
    }
    else {
        Pm_WriteShort(dev->stream, TIME_PROC(TIME_INFO),
                      Pm_Message(0x90, b, 0));
    }

}

void velocity_handler(mapper_signal sig, mapper_db_signal props,
                      mapper_timetag_t *timetag, void *value)
{
}

void aftertouch_handler(mapper_signal sig, mapper_db_signal props,
                        mapper_timetag_t *timetag, void *value)
{
}

void channel_aftertouch_handler(mapper_signal sig, mapper_db_signal props,
                                mapper_timetag_t *timetag, void *value)
{
}

void pitchbend_handler(mapper_signal sig, mapper_db_signal props,
                       mapper_timetag_t *timetag, void *value)
{
}

void control_handler(mapper_signal sig, mapper_db_signal props,
                     mapper_timetag_t *timetag, void *value)
{
}

void add_input_signals(midimap_device dev)
{
    int i, min = 0, max7bit = 127, max14bit = 16383;
    float minf = 0;
    char signame[128];
    for (i = 1; i < 2; i++) {
        snprintf(signame, 128, "/channel.%i/noteon", i);
        mdev_add_input(dev->dev, signame, 2, 'i', "midi",
                       &min, &max7bit, noteon_handler, dev);
        snprintf(signame, 128, "/channel.%i/noteoff", i);
        mdev_add_input(dev->dev, signame, 2, 'i', "midi",
                       &min, &max7bit, noteoff_handler, dev);
        snprintf(signame, 128, "/channel.%i/note/pitch", i);
        mdev_add_input(dev->dev, signame, 1, 'i', "midi",
                       &min, &max7bit, pitch_handler, dev);
        snprintf(signame, 128, "/channel.%i/note/velocity", i);
        mdev_add_input(dev->dev, signame, 1, 'i', "midi",
                       &min, &max7bit, velocity_handler, dev);
        //snprintf(signame, 128, "/channel.%i/note/duration", i);
        //mdev_add_input(dev->dev, signame, 1, 'f', "midi",
        //               &minf, 0, duration_handler, dev);
        snprintf(signame, 128, "/channel.%i/note/aftertouch", i);
        mdev_add_input(dev->dev, signame, 1, 'i', "midi",
                       &min, &max7bit, aftertouch_handler, dev);
        snprintf(signame, 128, "/channel.%i/aftertouch", i);
        mdev_add_input(dev->dev, signame, 1, 'i', "midi",
                       &min, &max7bit, channel_aftertouch_handler, dev);
        snprintf(signame, 128, "/channel.%i/pitchbend", i);
        mdev_add_input(dev->dev, signame, 1, 'i', "midi",
                       &min, &max14bit, pitchbend_handler, dev);
    }
}

// Declare output signals
void add_output_signals(midimap_device dev)
{
    int i, min = 0, max7bit = 127, max14bit = 16383;
    float minf = 0;
    char signame[128];
    // TODO: Need to declare these signals for each MIDI channel
    for (i = 1; i < 2; i++) {
        snprintf(signame, 128, "/channel.%i/noteon", i);
        mdev_add_output(dev->dev, signame, 2, 'i', "midi", &min, &max7bit);
        snprintf(signame, 128, "/channel.%i/noteoff", i);
        mdev_add_output(dev->dev, signame, 2, 'i', "midi", &min, &max7bit);
        snprintf(signame, 128, "/channel.%i/note/pitch", i);
        mdev_add_output(dev->dev, signame, 1, 'i', "midi", &min, &max7bit);
        snprintf(signame, 128, "/channel.%i/note/velocity", i);
        mdev_add_output(dev->dev, signame, 1, 'i', "midi", &min, &max7bit);
        //snprintf(signame, 128, "/channel.%i/note/duration", i);
        //mdev_add_output(dev->dev, signame, 1, 'f', "midi", &minf, 0);
        snprintf(signame, 128, "/channel.%i/note/aftertouch", i);
        mdev_add_output(dev->dev, signame, 1, 'i', "midi", &min, &max7bit);
        snprintf(signame, 128, "/channel.%i/aftertouch", i);
        mdev_add_output(dev->dev, signame, 1, 'i', "midi", &min, &max7bit);
        snprintf(signame, 128, "/channel.%i/pitchbend", i);
        mdev_add_output(dev->dev, signame, 1, 'i', "midi", &min, &max14bit);
    }
}

// MIDI -> mapper

// Process a pitch wheel message
// Process a master volume message
// Process a transport control message
// Process a control change message
// Process a MIDI time code message
// Process a sysex message

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
            printf("Got MIDI input %d %s %s\n", i, info->interf, info->name);
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
            printf("added device!\n");
        }
        if (info->output) {
            printf("Got MIDI output %d %s %s\n", i, info->interf, info->name);
            // TODO: Should only open output if it is mapped
            Pm_OpenOutput(&dev->stream, i, DRIVER_INFO, OUTPUT_BUFFER_SIZE, TIME_PROC, TIME_INFO, latency);
            dev->next = inputs;
            inputs = dev;
            add_input_signals(dev);
        }
    }
}

void parse_midi()
{
}

void cleanup_devices()
{
    printf("\nCleaning up!\n");
    midimap_device temp = inputs;
    while (temp) {
        if (temp->dev) {
            mdev_free(temp->dev);
        }
        if (temp->stream) {
            Pm_Close(temp->stream);
        }
        temp = temp->next;
    }
    temp = outputs;
    while (temp) {
        if (temp->dev) {
            mdev_free(temp->dev);
        }
        if (temp->stream) {
            Pm_Close(temp->stream);
        }
        temp = temp->next;
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
        // poll libmapper outputs
        temp = outputs;
        while (temp) {
            mdev_poll(temp->dev, 0);
            temp = temp->next;
        }
        // poll libmapper inputs
        temp = inputs;
        while (temp) {
            mdev_poll(temp->dev, 0);
            while (Pm_Poll(temp->stream) && (i++ < 10)) {
                if (Pm_Read(temp->stream, buffer, 1)) {
                    parse_midi(buffer[0]);
                }
            }
            temp = temp->next;
        }
        usleep(100 * 1000);
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
    cleanup_devices();
    return 0;
}