#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <termios.h>
#include <sys/fcntl.h>
#include <iostream>

//for timing
#include <ctime>
clock_t midistart,midifinish;
double mymiditime;

//#include "arduino_serial.h"

#include "midi.h"

using namespace std;

//extern void processMidiMsg(PmEvent buffer);

#define MIDI_SYSEX 0xf0
#define MIDI_EOX 0xf7

/* active is set true when midi processing should start */
int active = FALSE;
/* process_midi_exit_flag is set when the timer thread shuts down */
int process_midi_exit_flag;

PmStream *midi_in;
PmStream *midi_out;

/* shared queues */
#define IN_QUEUE_SIZE 1024
#define OUT_QUEUE_SIZE 1024
PmQueue *in_queue;
PmQueue *out_queue;
PmTimestamp current_timestamp = 0;
int thru_sysex_in_progress = FALSE;
int app_sysex_in_progress = FALSE;
PmTimestamp last_timestamp = 0;

int fd;

//callback for mysdlvu:
midiCallbackFuncType *myMidiCallback_ptr;
void *midiCallbackData_p;


/* time proc parameter for Pm_MidiOpen */
long midithru_time_proc(void *info)
{
    return current_timestamp;
}


/* timer interrupt for processing midi data.
   Incoming data is delivered to main program via in_queue.
   Outgoing data from main program is delivered via out_queue.
   Incoming data from midi_in is copied with low latency to  midi_out.
   Sysex messages from either source block messages from the other.
 */
void process_midi(PtTimestamp timestamp, void *userData)
{
    PmError result;
    PmEvent buffer; /* just one message at a time */

    current_timestamp++; /* update every millisecond */
    /* if (current_timestamp % 1000 == 0) 
        printf("time %d\n", current_timestamp); */

    /* do nothing until initialization completes */
    if (!active) {
        /* this flag signals that no more midi processing will be done */
        process_midi_exit_flag = TRUE;
        return;
    }

    /* see if there is any midi input to process */
    if (!app_sysex_in_progress) {
        do {
            result = Pm_Poll(midi_in);
            if (result) {
                long status;
                PmError rslt = (PmError)Pm_Read(midi_in, &buffer, 1);
                if (rslt == pmBufferOverflow) 
                    continue;
                assert(rslt == 1);

                /* record timestamp of most recent data */
                last_timestamp = current_timestamp;

                /* the data might be the end of a sysex message that
                   has timed out, in which case we must ignore it.
                   It's a continuation of a sysex message if status
                   is actually a data byte (high-order bit is zero). */
                status = Pm_MessageStatus(buffer.message);
                if (((status & 0x80) == 0) && !thru_sysex_in_progress) {
                    continue; /* ignore this data */
                }

                /* implement midi thru */
                /* note that you could output to multiple ports or do other
                   processing here if you wanted
                 */
//                printf("midi: %08x\n", buffer.message);

//                 if (Pm_MessageStatus(buffer.message) == 0x90 &&
//                     Pm_MessageData2(buffer.message) != 0) {
//                   printf("Note: %d\n", Pm_MessageData1(buffer.message));
//                 }
                  
                //call static function in MySDLVU:
//                processMidiMsg(buffer);
                myMidiCallback_ptr(midiCallbackData_p, &buffer);

                unsigned char midimsg[3];
                midimsg[0] = Pm_MessageStatus(buffer.message);
                midimsg[1] = Pm_MessageData1(buffer.message);
                midimsg[2] = Pm_MessageData2(buffer.message);
//                int n = serialport_write(fd, midimsg, 3);
//                if (n == -1) {
//                  perror("Serial write() error\n");
//                  exit(1);
//                }
                Pm_Write(midi_out, &buffer, 1);

                /* send the message to the application */
                /* you might want to filter clock or active sense messages here
                   to avoid sending a bunch of junk to the application even if
                   you want to send it to MIDI THRU
                 */
                Pm_Enqueue(in_queue, &buffer);

                /* sysex processing */
                if (status == MIDI_SYSEX) thru_sysex_in_progress = TRUE;
                else if ((status & 0xF8) != 0xF8) {
                    /* not MIDI_SYSEX and not real-time, so */
                    thru_sysex_in_progress = FALSE;
                }
                if (thru_sysex_in_progress && /* look for EOX */
                    (((buffer.message & 0xFF) == MIDI_EOX) ||
                     (((buffer.message >> 8) & 0xFF) == MIDI_EOX) ||
                     (((buffer.message >> 16) & 0xFF) == MIDI_EOX) ||
                     (((buffer.message >> 24) & 0xFF) == MIDI_EOX))) {
                    thru_sysex_in_progress = FALSE;
                }
            }
        } while (result);
    }


    /* see if there is application midi data to process */
    while (!Pm_QueueEmpty(out_queue)) {
        /* see if it is time to output the next message */
        PmEvent *next = (PmEvent *) Pm_QueuePeek(out_queue);
        assert(next); /* must be non-null because queue is not empty */
        if (next->timestamp <= current_timestamp) {
            /* time to send a message, first make sure it's not blocked */
            long status = Pm_MessageStatus(next->message);
            if ((status & 0xF8) == 0xF8) {
                ; /* real-time messages are not blocked */
            } else if (thru_sysex_in_progress) {
                /* maybe sysex has timed out (output becomes unblocked) */
                if (last_timestamp + 5000 < current_timestamp) {
                    thru_sysex_in_progress = FALSE;
                } else break; /* output is blocked, so exit loop */
            }
            Pm_Dequeue(out_queue, &buffer);
            Pm_Write(midi_out, &buffer, 1);

            /* inspect message to update app_sysex_in_progress */
            if (status == MIDI_SYSEX) app_sysex_in_progress = TRUE;
            else if ((status & 0xF8) != 0xF8) {
                /* not MIDI_SYSEX and not real-time, so */
                app_sysex_in_progress = FALSE;
            }
            if (app_sysex_in_progress && /* look for EOX */
                (((buffer.message & 0xFF) == MIDI_EOX) ||
                 (((buffer.message >> 8) & 0xFF) == MIDI_EOX) ||
                 (((buffer.message >> 16) & 0xFF) == MIDI_EOX) ||
                 (((buffer.message >> 24) & 0xFF) == MIDI_EOX))) {
                app_sysex_in_progress = FALSE;
            }
        } else break; /* wait until indicated timestamp */
    }
}

int initialize()
/* set up midi processing thread and open midi streams */
{
    /* note that it is safe to call PortMidi from the main thread for
       initialization and opening devices. You should not make any
       calls to PortMidi from this thread once the midi thread begins.
       to make PortMidi calls.
     */

    /* note that this routine provides minimal error checking. If
       you use the PortMidi library compiled with PM_CHECK_ERRORS,
       then error messages will be printed and the program will exit
       if an error is encountered. Otherwise, you should add some
       error checking to this code.
     */

    const PmDeviceInfo *info;
    int id;

    /* make the message queues */
    in_queue = Pm_QueueCreate(IN_QUEUE_SIZE, sizeof(PmEvent));
    assert(in_queue != NULL);
    out_queue = Pm_QueueCreate(OUT_QUEUE_SIZE, sizeof(PmEvent));
    assert(out_queue != NULL);

    /* always start the timer before you start midi */
    Pt_Start(1, &process_midi, 0); /* start a timer with millisecond accuracy */
    /* the timer will call our function, process_midi() every millisecond */
    
    Pm_Initialize();

//     id = Pm_GetDefaultOutputDeviceID();
//     info = Pm_GetDeviceInfo(id);
//     if (info == NULL) {
//         printf("Could not open default output device (%d).", id);
//         exit_with_message("");
//     }
//     printf("Opening output device %s %s\n", info->interf, info->name);

    /* use zero latency because we want output to be immediate */
    Pm_OpenOutput(&midi_out, 
                  id, 
                  NULL /* driver info */,
                  OUT_QUEUE_SIZE,
                  &midithru_time_proc,
                  NULL /* time info */,
                  0 /* Latency */);

    id = Pm_GetDefaultInputDeviceID();
    info = Pm_GetDeviceInfo(id);
    if (info == NULL) {
        printf("Could not open default input device (%d).\n", id);
        return -1;
        //exit(1);
    }
    printf("Opening input device %s %s\n", info->interf, info->name);
    Pm_OpenInput(&midi_in, 
                 id, 
                 NULL /* driver info */,
                 0 /* use default input size */,
                 &midithru_time_proc,
                 NULL /* time info */);
    /* Note: if you set a filter here, then this will filter what goes
       to the MIDI THRU port. You may not want to do this.
     */
    Pm_SetFilter(midi_in, PM_FILT_ACTIVE | PM_FILT_CLOCK);

    active = TRUE; /* enable processing in the midi thread -- yes, this
                      is a shared variable without synchronization, but
                      this simple assignment is safe */

}


void finalize()
{
    printf("terminating midi stuff...\n");
    /* the timer thread could be in the middle of accessing PortMidi stuff */
    /* to detect that it is done, we first clear process_midi_exit_flag and
       then wait for the timer thread to set it
     */
    process_midi_exit_flag = FALSE;
    active = FALSE;
    /* busy wait for flag from timer thread that it is done */
    //while (!process_midi_exit_flag) ;
    midistart = clock();
		while (!process_midi_exit_flag) {
		  midifinish = clock();
		  mymiditime = (double(midifinish)-double(midistart))/CLOCKS_PER_SEC;
		  if(mymiditime > 5) {
        cout << "midi timer thread is taking too long. something's gonna break" << endl;
        break;
		  }
		}
		printf("midi shutdown took: %f\n",mymiditime);
		
    /* at this point, midi thread is inactive and we need to shut down
     * the midi input and output
     */
    Pt_Stop(); /* stop the timer */
    Pm_QueueDestroy(in_queue);
    Pm_QueueDestroy(out_queue);

    Pm_Close(midi_in);
    Pm_Close(midi_out);

    Pm_Terminate();    
    printf("terminated\n");
}

//typedef void midiCallbackFuncType(void *userdata);

int startMidi(midiCallbackFuncType *CBFptr, void *userdata)
{
  PmTimestamp last_time = 0;
  PmEvent buffer;
  
  myMidiCallback_ptr = CBFptr;
  midiCallbackData_p = userdata;
  //CBFptr(userdata);
//  if (argc < 2) {
//    printf("Usage: %s <serialport>\n", argv[0]);
//    exit(1);
//  }

//  fd = serialport_open(argv[1], 115200);
//  if (fd == -1) {
//    fprintf(stderr, "Unable to open serial port %s\n", argv[1]);
//    exit(1);
//  }
//  sleep(2); // Wait for bootloader to finish
//  serialport_write(fd, (unsigned char *)"init", 4);

  /* determine what type of test to run */
  printf("begin PortMidi midithru program...\n");
  
  if(initialize() == -1) { /* set up and start midi processing */
    return -1;
  }

  //FIXME   finalize never gets called in this hacky callback setup
  //finalize();
  return 0;
}


