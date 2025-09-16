# Advanced User Experience: Connecting LLMs to Physical Machines

## 1. Program Installation

To use this project, you’ll need access to an LLM API and a compatible machine (such as an ESP32 or Arduino). For best results, we recommend using a paid API key.

This project uses the ChatGPT API by default, but you can easily modify a few lines to use a different provider. Check the commit history for examples.

**Project Structure:**

- `index.py` — Run this on your PC.
- `esp.c` — Upload this to your ESP32 or Arduino using the Arduino IDE.

## 2. How to Run the Program

1. Open your command prompt (CMD) on Windows, or Terminal on Mac.
2. Navigate to the folder containing `index.py`:
   ```
   cd path/to/your/project
   ```
3. Set your LLM API key as an environment variable:

   ```
   set OPENAI_API_KEY=yourapikey
   ```

   _(On Mac/Linux, use `export OPENAI_API_KEY=yourapikey` instead.)_

4. **Edit the serial port setting in `index.py`:**
   - Open `index.py` in a text editor.
   - Find the line that looks like `ser = serial.Serial('COMx', ...)` (Windows) or `'/dev/ttyUSBx'` (Mac/Linux).
   - Change `'COMx'` to your actual port name (e.g., `'COM3'` for Windows, `'/dev/ttyUSB0'` for Mac/Linux).
   - You can check your port in the Arduino IDE (Tools > Port) or Device Manager.
5. Upload `esp.c` to your ESP32 or Arduino using the Arduino IDE.
6. Run `index.py` on your PC.

---

Feel free to check the commit history for more details or troubleshooting tips. If you have any questions, open an issue or contact the maintainer.
