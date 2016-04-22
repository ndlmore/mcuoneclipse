/*
 * ReadyMIDI.c
 *
 *  Created on: 31.12.2014
 *      Author: Erich Styger
 */

#include "Platform.h"
#if PL_HAS_MUSIC
#include "Floppy.h"
#include "FRTOS1.h"
#include "MidiMusic.h"
#include "MidiMusicReady.h"
#include "MidiPirate.h"

static bool PlayTrackItem(MIDI_MusicTrack *track, unsigned int currTimeMs, uint8_t channel) {
  uint32_t beatsPerSecond = 2; /* 120 bpm */
  uint32_t currentMillis;
  uint32_t ppqn; /* ticks per beat/quarter note */
  uint32_t tempo = 110;
  uint8_t event;
  unsigned int itemNo;

  ppqn = track->nofTicksPerQuarterNote;
  for(;;) { /* breaks */
    itemNo = track->currLine;
    if (itemNo>=track->nofLines) { /* out of list */
      return FALSE; /* do not continue any more */
    }
    currentMillis = ((uint32_t)track->lines[itemNo].beat*1000UL)/beatsPerSecond;
    currentMillis += ((uint32_t)track->lines[itemNo].tick*60000UL)/ppqn/tempo;
    if (currentMillis>currTimeMs) {
      return TRUE; /* continue */
    }
    event = track->lines[itemNo].event;
    switch(event) {
      case MIDI_BANK:
        FLOPPY_MIDI_SetBank(channel, track->lines[itemNo].val1);
        break;
      case MIDI_NOTE_ON:
        FLOPPY_MIDI_NoteOn(channel, track->lines[itemNo].val1, track->lines[itemNo].val2);
        break;
      case MIDI_NOTE_OFF:
        FLOPPY_MIDI_NoteOff(channel, track->lines[itemNo].val1, track->lines[itemNo].val2);
        break;
      case MIDI_PATCH:
        FLOPPY_MIDI_SetInstrument(channel, track->lines[itemNo].val1);
        break;
      case MIDI_VOLUME:
        FLOPPY_MIDI_SetVolume(channel, track->lines[itemNo].val1);
        break;
      case MIDI_PAN:
        FLOPPY_MIDI_SetPan(channel, track->lines[itemNo].val1);
        break;
      case MIDI_END_OF_TRACK:
        //VS1_MIDI_AllSoundOff(channel);
        FLOPPY_MIDI_AllSoundOff(channel);
        break;
    }
    track->currLine++;
  } /* for */
  return TRUE;
}

static void Play(MIDI_MusicTrack *tracks, unsigned int nofTracks) {
  int itemNo;
  uint8_t channel;
  uint32_t currTimeMs;
  TickType_t startTicks;
  unsigned int nofFinished;

  /* init defaults */
  for(channel=0;channel<nofTracks;channel++) {
    FLOPPY_MIDI_SetBank(channel, 0);
    FLOPPY_MIDI_SetInstrument(channel, 0);
    FLOPPY_MIDI_SetVolume(channel, 127);
  }
  startTicks = FRTOS1_xTaskGetTickCount();
  itemNo = 0;
  for(;;) { /* breaks */
    currTimeMs = (FRTOS1_xTaskGetTickCount()-startTicks)/portTICK_RATE_MS;
    nofFinished = 0;
    for(channel=0;channel<nofTracks;channel++) {
      if (!PlayTrackItem(&tracks[channel], currTimeMs, channel)) {
        nofFinished++;
      }
    }
    if (nofFinished==nofTracks) { /* all finished */
      break;
    }
    FRTOS1_vTaskDelay(1/portTICK_RATE_MS);
    itemNo++;
  }
}

void MM_Play(void) {
  MIDI_MusicTrack tracks[4];
  uint8_t res;

  //res = MMReady_GetMidiMusicInfo(&tracks[0], sizeof(tracks)/sizeof(tracks[0]));
  res = MPirate_GetMidiMusicInfo(&tracks[0], sizeof(tracks)/sizeof(tracks[0]));
  if (res==ERR_OK) {
    Play(tracks, sizeof(tracks)/sizeof(tracks[0]));
  }
}#endif /* PL_HAS_MUSIC */
