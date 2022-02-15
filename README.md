# Biztortion

 Modular distortion plugin for VST-based hosts made with C++ and JUCE framework.
 
 Windows/MacOS installers of the software, both 32 bit and 64 bit, are available for ****DOWNLOAD**** in the [releases section](https://github.com/killbizz/Biztortion/releases/).
 
 ![Biztortion-Screenshot](Biztortion.png)
 
 This repository contains the thesys project for the bachelor degree in Electronic Music of Gabriel Bizzo at the C. Pollini Conservatory of Music in Padua (Italy).
 The thesys document, wrote in Italian language, contains an in-depth study on the phenomenon of audio distortion and a technical analysis of the different algorithms implemented in the plugin.

## Dependencies

- [JUCE Framework](https://github.com/juce-framework/JUCE): v6.1.0 (GNU GPLv3 License)
- [dRowAudio Module](https://github.com/killbizz/drowaudio) (MIT License)
- [ff_meters Module](https://github.com/ffAudio/ff_meters) (BSD 3 clause License)

## Installation

1. Download the required dependencies
2. Git clone this repository
3. Open the `Biztortion.jucer` file and point to the local location of the library dependencies in the module section
4. Click on the `Save and Open in IDE` button, build the project and run it in your favourite IDE

## License

Biztortion is licensed under the GNU GPLv3 license.
