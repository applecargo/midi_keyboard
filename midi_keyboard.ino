//
// a hackable midi master keyboard
//
// D. Yi @ 2019 7 19
//

//
// studied principles & examples & repos
// --> https://github.com/ast/keyboard/blob/master/README.md
// --> http://blog.komar.be/how-to-make-a-keyboard-the-matrix/
//

//
// took a keybed from old 'Miditech MIDIPLUS-61'
// --> https://medias.audiofanzine.com/images/normal/miditech-midiplus-61-427542.jpg
//

#define NUMKEYS 61
// --> how many keys are available for this master keyboard

int keystat[NUMKEYS] = {0, };
// --> a buffer to store statuses of all keys
// --> needed to extract only 'the change' of key statuses

#define MAXCHANGES 10
int keychanges[MAXCHANGES] = {0, };
int n_keychg = 0;
// --> for monitoring only 'changed' keys.

#define NUMCOLS 8
int pins_cols[NUMCOLS] = {29, 30, 31, 32, 33, 34, 35, 36};
// --> cols is w/ diodes.
// --> to be driven as output.

#define NUMROWS 16
int pins_rows[NUMROWS] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 24, 25, 26, 27, 28};
// --> rows is w/o diodes.
// --> to be read as input.
// --> our circuit requires these pins to be __pulled-down__
// --> 2 s/w for 1 key. time difference of the 2 s/w could be used to estimate & predict the 'velocity'

#define NUMOCTAVES 8
int pins_oct1[NUMOCTAVES] = {3, 5, 7, 9, 11, 24, 26, 28}; // --> preceeding switches
int pins_oct2[NUMOCTAVES] = {2, 4, 6, 8, 10, 12, 25, 27}; // --> following switches

void setup()
{
  //
  Serial.begin(115200);

  // cols : to be driven as output. (scanning)
  for (int idx = 0; idx < NUMCOLS; idx++) {
    pinMode(pins_cols[idx], OUTPUT);
    digitalWrite(pins_cols[idx], LOW);
  }

  // rows : to be read as input. (PULL-DOWN)
  for (int idx = 0; idx < NUMROWS; idx++) {
    pinMode(pins_rows[idx], INPUT_PULLDOWN); // INPUT_PULLDOWN is available, expecially for teensy boards !
  }

}

void loop()
{
  // perform a full scan of the key statuses!

  // update cols
  static int col = 0;
  digitalWrite(pins_cols[col], LOW); // let old 'on' column goes 'off' first.
  col++; // next pin!
  if (col >= NUMCOLS) { // a looper
    col = 0;
  }
  digitalWrite(pins_cols[col], HIGH); // let new 'off' column goes 'on'.. -> ready to scan.

  // 100us ~ 200us, waiting for the electricity ready...
  delayMicroseconds(200);

  // read all octaves
  for (int oct = 0; oct < NUMOCTAVES; oct++) {
    int key = oct * 8 + col;
    if (key < NUMKEYS) {
      int cur_key = digitalRead(pins_oct1[oct]);

      // if it is 'changed',
      if (cur_key != keystat[key]) {
        // if there is a room to record this change,
        if (n_keychg < MAXCHANGES) {
          // make a memo.
          keychanges[n_keychg] = (key + 24) * 10 + cur_key; // for example : C4 note-on --> 601 ( == 60*10 + 1)
          n_keychg++;
          // send MIDI msg.
          if (cur_key == 1) {
            // note 'on'
            usbMIDI.sendNoteOn((key + 24), 60, 1); // velocity == 60, midi-channel == 1
          } else {
            // note 'off'
            usbMIDI.sendNoteOff((key + 24), 60, 1); // velocity == 60, midi-channel == 1
          }
        }
      }

      // okay. good. now, apply the change.
      keystat[key] = cur_key;
    }
  }

  // at the moment of 'full scan done'
  if (col == NUMCOLS - 1) {

    // // print out the buffer status.
    // for (int key = 0; key < NUMKEYS; key++) {
    //   Serial.print(keystat[key]); Serial.print(" ");
    // }
    // Serial.println();

    // print out the changes.
    for (int chg = 0; chg < n_keychg; chg++) {
      Serial.print(keychanges[chg]);
      Serial.println();
    }

    // clear 'keychanges' array
    for (int idx = 0; idx < MAXCHANGES; idx++) {
      keychanges[idx] = 0;
      n_keychg = 0;
    }

  }

  // discard all incoming MIDI msgs.
  while (usbMIDI.read()) { }
}
