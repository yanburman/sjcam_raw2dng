# sjcam_raw2dng
Convert SJCAM raw files to Adobe DNG and TIFF while also getting metadata
such as date and ISO from JPEGs that correlate to the RAW files

Currently supported cameras:
- SJ5000X Elite (with Adobe camera profile for professional level color calibration)
- M20

Features:
--------------------
- Using official Adobe SDK for maximum compatibility and extendability
- Parallel conversion speeding up batch conversion
- Adobe camera profile
- Support for Linux, Windows and Mac (on Mac only CLI was tested on 10.11, although GUI should work as well)
- Best possible quality
- EXIF embedding from original JPEG files
- Full EXIF support


Camera profile usage:
---------------------
In order to be able to utilize camera profiles, you will need to put them in
C:\Users\\{you}\AppData\Roaming\Adobe\CameraRaw\CameraProfiles
After that you will need to restart Lightroom in order to be able to use the profile.
Now you can choose the new profile (SJ5000x Recipe) in Camera Calibration


GUI:
---------------------
Enables selecting source and optionally destination directories and convert
Also optional is ability to convert to TIFF as well as DNG


CLI:
---------------------
This is allows most control by supporing list of files/directories to convert.
Note that if you drag-n-drop directory or file into sjcam_raw2dng.exe on windows, it will do the batch conversion.


Troubleshooting:
---------------------

Q: Windows returns an error about missing MSVCP140.dll<br/>
A: Install: https://www.microsoft.com/en-us/download/details.aspx?id=48145

Q: Windows repors an error: "This app can't run on your PC. To find a version for your PC, check with the software publisher."<br/>
A: The binary is for 64-bit windows, while you are probably running 32-bit

Q: On Mac OSX GUI does not start<br/>
A: Install wxPython and put sjcam_raw2dng in the same folder as the GUI
