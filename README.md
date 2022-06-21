# Godot Space Mouse : Plug-In For Godot Engine
Plug-in to add support for 3Dconnexion Space Mouse and Space Navigator 3D mice input devices. Control the Godot Engine editor viewport camera with 6DOF (6 degrees of freedom). Freely translate the camera position and also rotate along the yaw, pitch, and roll axes.

![Screenshot](screenshots/Godot_Space_Mouse_Promo.jpg)

## INSTALLATION

[Click Here to Download the Latest Release](https://github.com/cybereality/godot-space-mouse/releases/latest)

For manual install, download the `addons` folder from this repository and copy it into your Godot project.

## CONFIGURATION

* Place the `addons` folder, with all contents, into your Godot project root directory.
* To enable the plug-in, click `Project` `Project Settings` `Plugins` and check `Enable` next to the `Godot Space Mouse` entry.
* On Linux you will need to allow HID access via udev rules. First, download the file `70-space-mouse.rules` and place this file in the correct directory for your distro, such as `/etc/udev/rules.d/`. Then replug your Space Mouse hardware or run `sudo udevadm control --reload-rules && sudo udevadm trigger` to update the rules.
* For macOS, you must uninstall the 3Dconnexion drivers, as they're incompatible with the plug-in.
* Please note, a Space Mouse or Space Navigator hardware device from 3Dconnexion is required.

## LICENSE

MIT License

Copyright (c) 2022 Andres Hernandez

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
