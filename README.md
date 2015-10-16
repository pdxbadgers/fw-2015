# BSides PDX 2015 Badger Firmware

## Commands

Command               | Action
----------------------|------------------------------------------
/flag                 | the current CTF flag
/flag?newflag=myflag  | set the CTF flag to myflag
/leds                 | how many LEDs and how are they flashing
/leds/                | status of all the LEDs
/leds?m=none          | all LEDs off
/leds?m=all           | all LEDs on
/leds?m=blink         | all LEDs blink
/leds?m=chase         | two LEDs chasing along the badge
/leds?m=twinkle       | random LED flashing
/leds?d=n             | status of LED n
/leds?d=n&s=1         | set LED on (1) or off (2)
/leds?d=n&r=R&g=G&b=B | set an RGB LED to color #RRGGBB. It has to be turned on/off separately.

