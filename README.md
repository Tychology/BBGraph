# BBGraph
![grafik](https://user-images.githubusercontent.com/38350366/187028920-67659b35-762e-4b06-8ea9-0b4eaafb4399.png)


Synthesizer based on Bytebeat using a node graph interface. See these links for a general explanation of the concept:  
http://canonical.org/~kragen/bytebeat/  
https://nightmachines.tv/the-absolute-beginners-guide-to-coding-bytebeats.html

Use context menu to add new nodes. Write expressions in infix or postfix notation. See presets for examples.  
To install the VST3-plugin, place it in C:\Program Files\Common Files\VST3.  
Check your DAWs manual on how to open .vstpreset presets.
  
  
Expression reference:  
Arithmetic operators `+`, `-`, `*`, `/`, `%`, unary minus `_` , power `**`  
Bitwise operators `~`, `&`, `|`, `^`, `<<`, `>>`  
Logical operators `!`, `&&`, `||`  
Comparison operators `==`, `!=`, `<`, `<=`, `>`, `>=`  
Mathematical funtions `sqrt`, `cbrt`, `exp`, `exp2`, `log`, `log2`, `log10`, `abs`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`  
Math constants `pi`, `twoPi`, `halfPi`, `e`  

The four inputs of the expression node from left to right `a`, `b`, `c`, `d`  
`rand` - random number between 0 and 1  
`fs` - counts seconds  
`f` - counts samples  
`ps` - position of playhead in timeline in seconds  
`p` - postion pf playhead in timeline in samples  
`rs` - seconds since note start  
`r` - samples since note start  
`n` - equal to `rs` * `nf` * 256  
`sr` - sample rate  
`bps` - tempo in beats per seconds  

