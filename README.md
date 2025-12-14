# CrunchLabs Label Maker

A modified version of Mark Rober's CrunchLabs Label Maker project. This Arduino-based label maker allows you to create custom labels by drawing text on tape using a pen controlled by stepper motors and a servo.

## Modifications

This version includes the following enhancements over the original:
- **Dual-Row Text Support**: Create labels with text on two rows (original only supported single row)
- **Improved User Interface**: Enhanced editing workflow with row switching via joystick button
- **Automatic Scaling**: Two-row labels are automatically scaled to half-size and positioned vertically

## Features

- **Dual-Row Text Support**: Create labels with text on two rows
- **Interactive Editing**: Use a joystick to navigate and edit text
- **Character Set**: Supports letters, numbers, and special characters
- **LCD Display**: 16x2 LCD screen for text editing and menu navigation
- **Automatic Scaling**: Two-row labels are automatically scaled to half-size and positioned vertically
- **Precise Drawing**: Uses Bresenham's line algorithm for smooth character rendering

## Hardware Requirements

### Components
- Arduino (Uno or compatible)
- 2x Stepper Motors (with drivers)
- 1x Servo Motor
- 16x2 LCD Display with I2C backpack (address 0x27)
- Joystick module with button
- Pen holder mechanism
- Tape feed mechanism

### Pin Connections

#### Stepper Motors
- **X-Axis Stepper**: Pins 6, 8, 7, 9
- **Y-Axis Stepper**: Pins 2, 4, 3, 5

#### Other Components
- **Servo**: Pin 13
- **Joystick Button**: Pin 14
- **Joystick X-Axis**: Analog Pin A2
- **Joystick Y-Axis**: Analog Pin A1
- **LCD I2C**: Uses I2C bus (SDA/SCL)

## Software Requirements

### Required Libraries
Install the following libraries via Arduino Library Manager:

- `Wire.h` (included with Arduino IDE)
- `LiquidCrystal_I2C.h`
- `Stepper.h` (included with Arduino IDE)
- `ezButton.h`
- `Servo.h` (included with Arduino IDE)

### Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/LabelMaker.git
   cd LabelMaker
   ```

2. Open `sketch/CrunchLabsLabelMaker.ino` in Arduino IDE

3. Install required libraries (if not already installed):
   - Go to `Sketch` → `Include Library` → `Manage Libraries`
   - Search for and install:
     - `LiquidCrystal_I2C`
     - `ezButton`

4. Upload the sketch to your Arduino

## Usage

### Basic Operation

1. **Start the Device**: Power on the Arduino. The LCD will display "Initializing..."

2. **Main Menu**: 
   - Use the joystick to navigate
   - Press the joystick button to start editing

3. **Editing Mode**:
   - **Up/Down**: Scroll through characters (A-Z, 0-9, special characters)
   - **Right**: Add the selected character to the current row
   - **Left**: Backspace (remove last character)
   - **Button Click (First)**: Move to editing the second row
   - **Button Click (Second)**: Go to print confirmation

4. **Print Confirmation**:
   - **Left/Right**: Select YES or NO
   - **Button Click**: Confirm or cancel printing

5. **Printing**:
   - The device will automatically draw your label
   - After printing, it returns to editing mode

### Two-Row Labels

When you have text in both rows:
- The first click of the joystick button moves you to editing the second row
- The second click goes to print confirmation
- Both rows are automatically scaled to half-size and positioned vertically
- Row 1 is drawn above Row 2

### Supported Characters

- Letters: A-Z (uppercase and lowercase)
- Numbers: 0-9
- Special characters: `!`, `?`, `,`, `.`, `#`, `@`
- Space: Use underscore `_` character (displays as space)

## Configuration

### Adjusting Scale and Spacing

Edit these variables in the code to adjust drawing size and spacing:

```cpp
int x_scale = 230;  // Horizontal scale (motor steps per coordinate unit)
int y_scale = 230;  // Vertical scale (motor steps per coordinate unit)
int space = x_scale * 5;  // Space between characters
int row_spacing = y_scale * 1.5;  // Vertical spacing between rows
```

### Joystick Sensitivity

Adjust the joystick threshold if your joystick is too sensitive or not sensitive enough:

```cpp
const int joystickButtonThreshold = 200;  // Adjust this value (0-512)
```

### LCD Address

If your LCD uses a different I2C address, change it here:

```cpp
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Change 0x27 to your LCD's address
```

## How It Works

### Character Rendering

Characters are defined as vector paths using a coordinate system:
- Each character is stored as a series of coordinates
- The drawing algorithm uses Bresenham's line algorithm for smooth lines
- Coordinates are scaled and converted to stepper motor steps

### Two-Row Printing

When both rows contain text:
1. The system scales all drawing parameters to 50%
2. Row 1 is positioned at the top
3. Row 2 is positioned below with proper spacing
4. Both rows are drawn in a single pass

### Motor Control

- **X-Axis**: Controls horizontal movement (left/right)
- **Y-Axis**: Controls vertical movement (up/down)
- **Servo**: Controls pen up/down position
- Motors are released (powered off) when not in use to prevent overheating

## Troubleshooting

### LCD Not Displaying
- Check I2C connections (SDA/SCL)
- Verify LCD address (try 0x3F if 0x27 doesn't work)
- Check power connections

### Motors Not Moving
- Verify stepper motor connections
- Check motor driver power supply
- Ensure motors are properly wired (4-wire steppers)

### Text Overlapping
- Adjust `row_spacing` value
- Check `x_scale` and `y_scale` values
- Verify motor step counts match your hardware

### Joystick Not Responding
- Check analog pin connections
- Adjust `joystickButtonThreshold` value
- Verify joystick is receiving power

## License

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the [LICENSE](LICENSE) file for more details.

### Original Code

Copyright (c) 2025 Crunchlabs LLC (LabelMaker Code)

### Modifications

Copyright (c) 2025 [Your Name/Organization]

This modified version is based on the original CrunchLabs Label Maker code and is released under GPL v2.

## Credits

- **Original Project**: Mark Rober's CrunchLabs Label Maker
- **Original Code**: Copyright (c) 2025 Crunchlabs LLC
- **Modifications**: Enhanced with dual-row support and improved user interface

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Support

For issues, questions, or suggestions, please open an issue on GitHub.

