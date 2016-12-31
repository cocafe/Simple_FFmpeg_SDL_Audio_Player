# _Simple FFMPEG and SDL Music Player_

------

This is an experimental toy project written in C, in order to learn something
about audio decoding and playback.

------

Require *Visual Studio 2015*

------

_FIXME_:

> * User interface looks simple and hardcoded
> * Does not provide a proper timestamp in milliseconds
> * Seeking should use a time length instead of numbers of frames
> * Backward seeking sometimes is broken
> * High resolution audio playback is broken
> * Internal audio buffer doesn't look nice
> * Audio buffer length should use time length instead of size
> * SDL playback is broken during system switches audio device (e.g plug/unplug line output)
> * APE audio format playback is broken

_TODO_:

> * Get rid of Software Resampler
> * Read ffmpeg ffplay source code, etc
> * Implement better liner buffer
> * Add a timer to track playback timestamp and use this timer for seeking
> * Better UI? Nope, I don't wanna works Windows native UI in C programming. ww
