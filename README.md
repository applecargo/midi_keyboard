# midi_keyboard

- a hackable midi master keyboard

- the principles
  - http://blog.komar.be/how-to-make-a-keyboard-the-matrix/

- old 'Miditech MIDIPLUS-61'
  ![](miditech-midiplus-61-427542.jpg)

- only using the 'keybed' + teensy 3.5
  ![](photo_2019-07-21_19-24-54.jpg)

- no velocity support, but you can find relative info.
  - https://github.com/ast/keyboard/blob/master/README.md
  - https://github.com/ast/keyboard/issues/1

- OSC over Serial added, and transmitting Serial over Bluetooth w/ HC-06
  ![](photo_2019-07-24_15-40-03.jpg)

- connecting PD patch added, depends on 'osc', 'comport', 'slip' etc..
  ![](Screen Shot 2019-07-24 at 4.25.24 PM.png)
  ![](Screen Shot 2019-07-24 at 4.25.27 PM.png)
  ![](Screen Shot 2019-07-24 at 4.25.37 PM.png)
