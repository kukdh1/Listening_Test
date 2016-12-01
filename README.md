# Listening_Test
Program for school project (Cognitive Acoustics)

## Description
This program testing human acoustics.  

- Compare Hi-Fi audio (192kHz or 96kHz 24bit audio) and normal audio (48kHz 24bit audio).  
- Compare normat audio (48kHz 24bit audio) and low-bitdepth audio (48kHz 8bit audio).  

Does human can determine which one is better quality sound?  

## Dependency
- Qt 5.7
- libxlsxwriter 0.4.2
- ffmpeg 3.2
- portaudio v19.20161030

## Note
You should change default output audio format to 192kHz (or 96kHz) 24bit audio in:
- macOS: Application > Audio MIDI Setup
- Windows: Control Panel > Sound > (Select output device) Properties > Advanced > Default Format
