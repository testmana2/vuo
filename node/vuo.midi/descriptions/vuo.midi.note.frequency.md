Outputs the frequency associated with a MIDI note number. 

This node is useful for playing MIDI notes aloud in Vuo. You can connect this node to a `Make Audio Wave` node, then connect that to a `Send Live Audio` node. 

   - `Note Number` — Ranges from 0 to 127. 
   - `Pitch Bend` — Ranges from 0 to 16383, where 8192 is the default.  A value of 8192 means no change to the frequency.  A value of 0 decreases the frequency by `Pitch Bend Range` semitones; a value of 16383 increases the frequency by `Pitch Bend Range` semitones.
   - `Pitch Bend Range` — How far the `Pitch Bend` causes the frequency to change, in semitones.
   - `Frequency` — The frequency, in Hz, that corresponds to the note number, according to the [MIDI Tuning Standard](https://en.wikipedia.org/wiki/MIDI_Tuning_Standard). 

Examples: 

   - For note number 0 (C-1), the frequency is 8.18. 
   - For note number 60 (C4, middle C), the frequency is 261.63. 
   - For note number 69 (A4), the frequency is 440. 
   - For note number 69 (A4), with a pitch bend of 0 and pitch bend range of 12, the frequency is 220. 
   - For note number 127 (G9), the frequency is 12543.85. 
