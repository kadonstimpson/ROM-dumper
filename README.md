# ROM‑Dumper for STM32F072 Discovery  
University of Utah • Embedded Systems Project

## Team Members
Cubby DeBry,  
Joshua Elieson,  
Kadon Stimpson,  
Yunzu Hou  

---

## Table of Contents
1. [About the Project](#about-the-project)  
2. [Hardware](#hardware)  
   * [Discovery‑board Modifications](#discovery-board-modifications)  
3. [Software](#software)  
   * [Supported Cartridge Types](#supported-cartridge-types)  
   * [Dumping Options](#dumping-options)  
4. [Getting Started](#getting-started)  
5. [Usage](#usage)  
6. [Roadmap / To‑Do](#roadmap--to-do)  

---

## About the Project
This project implements a **stand‑alone cartridge dumper** that interfaces classic Nintendo hand‑held game cards with an STM32F072 Discovery board.  
* **Hardware**—custom daughter‑board hosting the game‑card connector, level‑shifters and SD‑card slot.  
* **Firmware**—bank‑aware bus driver plus USB‑CDC and FatFS back‑ends.  
* **Host Tools**—minimal Python/Tk GUI for one‑click dumps on macOS, Windows, and Linux.

All design files live under `Hardware/`; Desktop tools reside in `Tools/`.

---

## Hardware
Design iteration **v1** is complete and electrically validated.  SD‑card and game‑card buses have been verified on real cartridges.

### Discovery‑board Modifications
The stock STM32F072 Discovery must be tweaked to expose additional GPIO lines:
| Component | Action |
|-----------|--------|
| C26, C27, C28 | **Remove** |
| R38, R39, R40 | **Remove** |
| SBx (solder bridges on rear) | **Bridge** matching removed caps/resistors |

> **Orientation tip**: the Discovery board plugs into the daughter‑board with its **USB‑Mini port facing the SD‑card slot**. No directional silk is present, so double‑check before powering.

---

## Software
### Supported Cartridge Types
| System | Status | Notes |
|--------|--------|-------|
| **Game Boy / Game Boy Color** |  | |
| &nbsp;&nbsp;MBC1 | ✔ Implemented *(untested)* |
| &nbsp;&nbsp;MBC2 | ✔ Implemented *(untested)* |
| &nbsp;&nbsp;MBC3 | ✔ Implemented *(untested)* |
| &nbsp;&nbsp;MBC5 | ✔ Implemented & **tested** |
| &nbsp;&nbsp;MBC6 | ✔ Implemented *(untested)* |
| &nbsp;&nbsp;MBC7 | ✔ Implemented *(untested)* |
| &nbsp;&nbsp;Other/rare MBCs | ✖ Not yet |
| **Game Boy Advance** | ✔ Full dumps (32 MiB assumed) |

### Dumping Options
* **USB‑CDC** – streams data over 1 Mbaud VCP; proven with GBA cartridges.  
* **SD‑Card** – direct‐to‑FAT file dumps; already linked into MBC5 flow, pending integration for other types.

---

## Getting Started
```bash
# clone repository
git clone https://github.com/your‑org/ROM‑dumper.git
cd ROM‑dumper

# create virtual environment for host tools
python3 -m venv .venv
source .venv/bin/activate
pip install -r Tools/requirements.txt

# build firmware (requires arm‑gcc & stm32cubemx)
pio run -t upload
```

---

## Usage
### Stand‑alone (SD‑Card)
1. Insert a FAT formatted SD card.  
2. Power the board via USB. 
3. Insert cartridge
6. Press *Reset* button 
7. Retrieve `.gba`/`.gbc`/`.gb` file from SD.

### USB‑CDC
1. Power the board via USB. 
2. Run `dumperGUI.py`.
3. Select corresponding serial port from list.
4. Provide file name for dump output (including `.gba`/`.gbc`/`.gb`).
5. Press *Start Dump* button 
6. Retrieve `.gba`/`.gbc`/`.gb` file

---

## Roadmap / To‑Do
* **Mode selector**—hardware or menu to choose SD vs USB without a PC.  
* **DMA + double buffering** for both back‑ends to maximise throughput.  
* **GBA ROM trimming**—detect true ROM size to avoid 32 MiB paddings.  
* **Integrate SD flow** into all MBC drivers.
* **V2 Hardware** standalone dumper with lcd and onboard STM32.  
* **Automated self‑test** jig for production.

---

© 2025 University of Utah Embedded Systems Group • Licensed under MIT (see `LICENSE`).

