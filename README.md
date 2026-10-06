**AudioVisualizer** 
<u>*Advanced Audio Visualizer for STM32F746G-DISCOVERY*</u> 

An embedded audio visualization tool built for the STM32F746G-DISCOVERY board. This project processes real-time audio input from the internal digital microphone or audio jack and renders responsive, hardware-accelerated graphics on the built-in LCD screen .

### Core Features
* **Real-Time Processing:** Utilizes `arduinoFFT` for fast Fourier transform calculations directly on the board .
* **Extensive View Library:** Features 15 distinct audio visualization modes .
* **Interactive UI:** Touchscreen sidebar for on-the-fly parameter adjustments without recompiling .
* **Persistent Settings:** Saves user configurations (like gain and view modes) to an SD card .
* **Auto-Cycle Mode:** Automatically cycles through all visualization modes every 60 seconds for hands-free display .

### Visualization Modes
The visualizer includes a diverse set of modes designed for different audio analysis needs:
* **Equalizers (8, 16, 32, and 48 Bands):** Features adjustable color themes (Standard, VU, Winamp) and a toggleable peak-hold function.
* **Spectrogram:** Waterfall frequency visualization with adjustable rendering resolutions (Normal, Fast, Slow) .
* **Waveform:** Classic oscilloscope-style wave view with selectable trace colors (Green, Cyan, Yellow).
* **Pitch & Note Detector:** Analyzes the dominant frequency to display the exact Hz, musical note (A-G), octave, and MIDI value.
* **Lissajous XY:** Plots the left and right audio channels against each other on an X/Y axis.
* **Beat Meter:** Tracks audio energy and visualizes kicks/beats against a moving average line.
* **Mesh 3D:** A 3D scrolling history of frequency bins.
* **Persistence:** A heatmap-based trail decay visualization showing frequency intensity over time.
* **Additional Views:** Linear Spectrum, L/R VU Meters, Waterfall Peak, and Analog VU Needle .

### Controls & Interface
The right side of the screen features a dedicated touch panel to control the visualizer in real-time :
* **VIEW +/-**: Manually navigate through the 15 visualization modes .
* **GAIN +/-**: Adjust the input sensitivity/scale for the current view .
* **AUTO ON/OFF**: Toggle the automatic 60-second view rotation .
* **COLOR / RES**: Change the color palette (in Waveform/EQ modes) or the time resolution (in Spectrogram mode) .
* **PK ON/OFF**: Toggle the peak-hold bars when using Equalizer modes .

*Hardware Shortcut:* You can also toggle the "Auto-Cycle" mode by pressing the physical USER button on the STM32 board .

### Hardware Requirements & Dependencies
* **Board:** STM32F746G-DISCOVERY.
* **Libraries:** Requires `arduinoFFT` for audio processing and the official BSP (Board Support Package) libraries for LCD, Touchscreen, and Audio input handling (`stm32746g_discovery_lcd.h`, `stm32746g_discovery_audio.h`, etc.) .
