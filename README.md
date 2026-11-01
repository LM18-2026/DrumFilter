# DrumFilter

JUCE VST3 drum transient-triggered low-pass filter. Build 001 prototype.

## Build
GitHub Actions Windows workflow builds a VST3 using CMake and JUCE 8. Requires no local JUCE installation.

## Prototype
Kick onset detector (30–150 Hz), threshold, retrigger holdoff, envelope-controlled low-pass with selectable 6/12/18/24 dB/oct, decay and target cutoff. Sample triggering deferred.

Detection parameters are provisional and require testing with real kick recordings.
