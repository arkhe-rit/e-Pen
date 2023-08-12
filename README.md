# e-Pen
**e-Paper display program**


# Raspberry Pi
## Instructions for use

0. Setup the apeiron WiFi network, and start your instance of Redis. Make sure you note the IP Address of the server.
1. Log into the Raspberry Pi using the username and password. Navigate to the `e-Pen` directory.
2. Launch the program using `sudo ./e-Pen`
    * You can specify a different hostname or IP address (default: `mothership.local`) using the flag `-h [hostname]`
    * You can specify a different port (default `6379`) using the flag `-p [port]`
    * You can skip the splashscreen with `-S` (NOTE: this will not clear the screen on startup)
3. Send commands through the Redis channel `projector/eink`
    * To change images, send the command `{ "type": "command", "command": "change-image", "value": 0 }`
    * There are also maintenece commands that can replace `change-image`, and do not require a value parameter. These commands are:
        * `exit` - Safely quits the application
        * `reboot` - Reboots the Raspberry Pi
        * `shutdown` - Shuts down the Raspberry Pi

## Creating New Images
The e-Paper Display will only display monochrome images provided in the BMP format. [Click here](https://cdn.discordapp.com/attachments/1118597834153930812/1139748401228034048/How_to_Create_BMP_Images_for_ePaper_Display.pdf) for documentation on how to create these images.

## Adding / Removing Images
To switch images, navigate to the `e-Pen.c` file. Around line 20, you will find something like this:
```C
#define NUMBER_OF_IMAGES 5
const char *imgs[NUMBER_OF_IMAGES] = {
    "./imgs/portrait1.bmp",
    "./imgs/portrait2.bmp",
    "./imgs/portrait3.bmp",
    "./imgs/portrait4.bmp",
    "./imgs/portrait5.bmp"};
```
To add, remove, or swap out images, simply change the `NUMBER_OF_IMAGES` define, and modify the array below. On the Raspberry Pi, navigate to the `e-Pen` directory, and run the commands `sudo make clean` to prepare for a clean rebuild, and `sudo make` to compile the changed code.

# ESP32
## Instructions for use
0. Setup the apeiron network, and plug in the ESP32
1. Prepare your image by converting it to a C-array with something like [image2cpp](https://javl.github.io/image2cpp/). I recommend using Atkinson or Floyd-Steinberg dithering.
2. Once an IP Address is displayed on the e-Paper, send it the generated 48,000 byte character array as a raw data stream. You can do this in Node.js by using the `Buffer.from()` and `Socket.write()` from the `net` module.
3. To shutdown the ESP32, send a 48,000 byte array of pure white (`0xFF`). This will trigger the ESP32 code to cleanup memory and disconnect.