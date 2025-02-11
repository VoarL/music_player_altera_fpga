# music_player_altera_fpga
[![Build status](https://ci.appveyor.com/api/projects/status/yc3leb1t5t6ue01i?svg=true)]()

This is a a Repository containing the design ot the FPGA SoC Music Player

[![License](https://img.shields.io/badge/License-GNU%20GPL-blue.svg)](https://opensource.org/licenses/MIT)
[![GitHub Issues](https://img.shields.io/github/issues/VoarL/music_player_altera_fpga.svg)](https://github.com/VoarL/music_player_altera_fpga/issues)
[![GitHub Stars](https://img.shields.io/github/stars/VoarL/music_player_altera_fpga.svg)](https://github.com/VoarL/music_player_altera_fpga/stargazers)

## Table of Contents

- [Introduction](#introduction)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Introduction

For the second lab in ECE 224, we developed a digital audio player using an Altera FPGA-based NIOS II system. This project focused on integrating various hardware IP cores and software components to enable audio playback from a MicroSD card, controlled through an LCD-based user interface and push-button inputs. By leveraging embedded programming and hardware description languages (HDL), we implemented a fully functional music player while gaining practical experience with system-on-chip (SoC) design and software-hardware integration.

## Features

- FPGA-Based SoC Design: Implemented an MCU system on an Altera FPGA using HDL, incorporating NIOS II for embedded processing.
- Audio Playback & Processing: Developed software to handle WAV file playback, leveraging audio IP cores for decoding and output.
- File System Management: Integrated a FAT32 file system to read audio files from an SD card using the FatFS library.
- User Interface & Controls: Designed an LCD-based UI displaying track names and playback status, with push-button controls for navigation and playback management.
- Peripheral Communication: Managed interactions with UART, I2C, and JTAG for debugging and data exchange.
- Performance Optimization: Ensured seamless playback by efficiently handling audio buffering and FIFO operations to prevent underflow or overflow issues.

## Installation

- Run Quartus Prime and open the project
- Use the Max10 Altera FPGA to run or simply simulate.

## Usage

- After running, customize accordingly.
  
## Contributing

We welcome contributions! If you'd like to contribute to this project, please follow the guidelines in [CONTRIBUTING.md](CONTRIBUTING.md).

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

