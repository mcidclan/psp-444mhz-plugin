# Experimental 444mhz Overclock

## Description
This is an experimental project for technical purposes only, intended for developers and advanced users. Stability is not guaranteed, use with caution.

## Usage

### Prerequisites
Before using this plugin, make sure to:
- Disable all previous versions or similar plugins
- Remove any existing overclocking >333MHz code from your application

### Controls
Press **L_TRIGGER + R_TRIGGER + NOTE** to toggle between 333MHz and 444MHz, or the frequency set in the ms0:/overconfig.txt file.

### Visual Feedback
- **333MHz (standard)**: White square on green background
- **444MHz (overclocked)**: Red square on white background
The plugin auto-starts at 333MHz. In most cases, you should see the square a few seconds after the game/homebrew boots.

### ms0:/overconfig.txt
If the file doesn't exist, the plugin will target 444MHz for the overclock frequency. The value in the file must be between 333 and 444, which could be stable on some 2k and 3k models, but not all of them.

## Compatibility

### 2000 and 3000
Tested on PSP 2000 and 3000. Reaching 444MHz, which appears to be a limit before instabilities.

### 1000
Unfortunately on 1000, the limit seems to already be reached at 370MHz, from which instabilities start to manifest.

### PSP Street
Not tested

### PSP Go
Not tested

## Disclamer
This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

## Special Mention
Thanks to z2442 and st1x51 for testing, and to koutsie whose claims about overclocking possibilities motivated me to investigate and experiment on the subject.

*m-c/d*
