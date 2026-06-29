#ifndef SONGS_H
#define SONGS_H

#include <Arduino.h>

// Pitch definitions
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define REST     0

struct Note {
  uint16_t pitch;
  uint8_t duration; // Note duration: 4 = quarter note, 8 = eighth note, etc.
};

// 1. Super Mario Theme (Intro)
const Note mario_melody[] = {
  {NOTE_E7, 12}, {NOTE_E7, 12}, {REST, 12}, {NOTE_E7, 12}, {REST, 12}, {NOTE_C7, 12}, {NOTE_E7, 12}, {REST, 12},
  {NOTE_G7, 12}, {REST, 12}, {REST, 12}, {NOTE_G6, 12}, {REST, 12}, {REST, 12},
  {NOTE_C7, 12}, {REST, 12}, {REST, 12}, {NOTE_G6, 12}, {REST, 12}, {REST, 12}, {NOTE_E6, 12}, {REST, 12}, {REST, 12},
  {NOTE_A6, 12}, {REST, 12}, {NOTE_B6, 12}, {REST, 12}, {NOTE_AS6, 12}, {NOTE_A6, 12}, {REST, 12},
  {NOTE_G6, 9}, {NOTE_E7, 9}, {NOTE_G7, 9}, {NOTE_A7, 12}, {REST, 12}, {NOTE_F7, 12}, {NOTE_G7, 12},
  {REST, 12}, {NOTE_E7, 12}, {REST, 12}, {NOTE_C7, 12}, {NOTE_D7, 12}, {NOTE_B6, 12}
};
const int mario_length = sizeof(mario_melody) / sizeof(Note);
const int mario_tempo = 150;

// 2. Despacito (Chorus/Intro hook)
const Note despacito_melody[] = {
  {NOTE_D5, 2}, {NOTE_CS5, 4}, {NOTE_B4, 8}, {NOTE_FS4, 8}, {NOTE_B4, 8}, {NOTE_D5, 8},
  {NOTE_B4, 8}, {NOTE_CS5, 8}, {NOTE_D5, 8}, {NOTE_E5, 8}, {NOTE_CS5, 4}, {NOTE_B4, 8}, {NOTE_A4, 8},
  {NOTE_A4, 8}, {NOTE_CS5, 8}, {NOTE_E5, 8}, {NOTE_A4, 8}, {NOTE_B4, 8}, {NOTE_CS5, 8}, {NOTE_D5, 8},
  {NOTE_B4, 4}, {NOTE_A4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_B4, 8}, {NOTE_D5, 8}, {NOTE_G4, 8},
  {NOTE_A4, 8}, {NOTE_B4, 8}, {NOTE_CS5, 8}, {NOTE_A4, 4}, {NOTE_G4, 8}, {NOTE_FS4, 8}, {NOTE_FS4, 8},
  {NOTE_A4, 8}, {NOTE_CS5, 8}, {NOTE_FS4, 8}, {NOTE_G4, 8}, {NOTE_A4, 8}, {NOTE_B4, 8}
};
const int despacito_length = sizeof(despacito_melody) / sizeof(Note);
const int despacito_tempo = 100;

// 3. Jingle Bells (Chorus)
const Note jingle_bells_melody[] = {
  {NOTE_E5, 8}, {NOTE_E5, 8}, {NOTE_E5, 4},
  {NOTE_E5, 8}, {NOTE_E5, 8}, {NOTE_E5, 4},
  {NOTE_E5, 8}, {NOTE_G5, 8}, {NOTE_C5, 8}, {NOTE_D5, 8},
  {NOTE_E5, 2},
  {NOTE_F5, 8}, {NOTE_F5, 8}, {NOTE_F5, 8}, {NOTE_F5, 8},
  {NOTE_F5, 8}, {NOTE_E5, 8}, {NOTE_E5, 8}, {NOTE_E5, 16}, {NOTE_E5, 16},
  {NOTE_E5, 8}, {NOTE_D5, 8}, {NOTE_D5, 8}, {NOTE_E5, 8},
  {NOTE_D5, 4}, {NOTE_G5, 4}
};
const int jingle_bells_length = sizeof(jingle_bells_melody) / sizeof(Note);
const int jingle_bells_tempo = 140;

#endif
