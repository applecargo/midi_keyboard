//
// a hackable midi master keyboard
//
// D. Yi @ 2019 7 19
//

//==============<configurations>==============
// --> uncomment only 1 set of conf.
// (1) USB MIDI keyboard using 'teensy MIDI'
#define PROTOCOL_MIDI // teensy MIDI --> select "USB Type : Serial + MIDI"
// (2) USB SERIAL keyboard speaking 'OSC over SLIP' protocol
#define PROTOCOL_OSC_SLIP
#define SLIP_USBSERIAL
// (3) Wireless keyboard speaking OSC protocol (HC-06)
#define PROTOCOL_OSC_SLIP
#undef SLIP_USBSERIAL
#define SLIP_HWSERIAL1 // --> h/w serial pin 0/1
// (4) Wireless keyboard speaking I2C command string over esp8266 MESH
#define PROTOCOL_I2C_MESH // esp8266 MESH
//==============</configurations>==============

//
#ifdef PROTOCOL_OSC_SLIP
#include <OSCBundle.h>
#ifdef SLIP_USBSERIAL
#include <SLIPEncodedUSBSerial.h>
SLIPEncodedUSBSerial SLIPSerial(Serial);
#endif
#ifdef SLIP_HWSERIAL1
#include <SLIPEncodedSerial.h>
SLIPEncodedSerial SLIPSerial(Serial1);
#endif

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
  #ifdef SLIP_USBSERIAL
  SLIPSerial.begin(57600);
  //Beware!! --> Serial.print will damage SLIP communications..
  #endif

  #ifdef SLIP_HWSERIAL1
  Serial.begin(115200);
  SLIPSerial.begin(57600);
  #endif

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
          // do communications
          #ifdef PROTOCOL_MIDI
          // send MIDI msg.
          if (cur_key == 1) {
            // note 'on'
            usbMIDI.sendNoteOn((key + 24), 60, 1);   // velocity == 60, midi-channel == 1
          } else {
            // note 'off'
            usbMIDI.sendNoteOff((key + 24), 60, 1);   // velocity == 60, midi-channel == 1
          }
          #endif
          #ifdef PROTOCOL_OSC_SLIP
          OSCBundle bndl;
          //NOTE: even though not sure if we can count on this or not,
          //  following order later become a message happening order in pd patch.
          //  so, this means.. if we do [routeOSC /pitch /velocity /onoff]
          //  then, events will happen from right-to-left.
          int oncnt = 0;
          for (int idx = 0; idx < NUMKEYS; idx++) {
            if (key == idx) {
              oncnt += cur_key;
            } else {
              oncnt += keystat[idx];
            }
          }
          bndl.add("/note/oncnt").add(oncnt);
          bndl.add("/note/onoff").add(cur_key);
          bndl.add("/note/velocity").add(60);
          bndl.add("/note/pitch").add(key + 24);
          SLIPSerial.beginPacket();   // mark the beginning of the OSC Packet
          bndl.send(SLIPSerial);
          SLIPSerial.endPacket();
          #endif
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

    // // print out the changes.
    // for (int chg = 0; chg < n_keychg; chg++) {
    //   Serial.print(keychanges[chg]);
    //   Serial.println();
    // }

    // clear 'keychanges' array
    for (int idx = 0; idx < MAXCHANGES; idx++) {
      keychanges[idx] = 0;
      n_keychg = 0;
    }

  }

  #ifdef PROTOCOL_MIDI
  // discard all incoming MIDI msgs.
  while (usbMIDI.read()) { }
  #endif
}
