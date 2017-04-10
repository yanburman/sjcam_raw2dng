# sjcam_raw2dng
Convert SJCAM raw files to Adobe DNG and TIFF while also getting metadata
such as date and ISO from JPEGs that correlate to the RAW files

Currently supported cameras:
- SJ5000X Elite (with Adobe camera and lens profile for professional level color calibration)
- SJ6 LEGEND (with Adobe camera and lens profile for professional level color calibration)
- M20 (with Adobe lens profile only)

Features:
--------------------
- Using official Adobe SDK for maximum compatibility and extendability
- Parallel conversion speeding up batch conversion
- Adobe camera profile
- Adobe lens profile
- Support for Linux, Windows and Mac (on Mac only CLI was tested on 10.11, although GUI should work as well)
- Best possible quality
- EXIF embedding from original JPEG files
- Full EXIF support


Camera profile usage:
---------------------
In order to be able to utilize camera profiles, you will need to put them in
C:\Users\\{you}\AppData\Roaming\Adobe\CameraRaw\CameraProfiles
After that you will need to restart Lightroom in order to be able to use the profile.
Now you can choose the new profile (SJ5000x Recipe or SJ6 LEGEND Recipe) in Camera Calibration

Lens profile usage:
---------------------
In order to be able to utilize lens profiles, you will need to put them in
C:\Users\\{you}\AppData\Roaming\Adobe\CameraRaw\LensProfiles\1.0
After that you will need to restart Lightroom in order to be able to use the profile.
Now you can select "Enable Profile Corrections" under lens correction and have LR/ACR correct geometric distortions

GUI:
---------------------
Enables selecting source and optionally destination directories and convert
Also optional is ability to convert to TIFF as well as DNG

![Image](https://github.com/yanburman/sjcam_raw2dng/blob/master/resources/gui.png)


CLI:
---------------------
This is allows most control by supporting list of files/directories to convert.
Note that if you drag-n-drop directory or file into sjcam_raw2dng.exe on windows, it will do the batch conversion.


Troubleshooting:
---------------------
Q: I get "unsupported format" error when trying to convert a photo taken with FOV crop or zoom</br>
A: Only some of FOV crops are currently supported for SJ5000x Elite and none for M20. This is partly due to</br>
crop raw files having some apparently extra pixels that are being discarded by the camera's firmware</br>
thus making it difficult to find the actual width and height of the image.</br>
Zoom settings are not planned to be supported as well.</br>
Anyway, using crop/zoom setting with RAW have no real benefit, since shooting in full resolution</br>
and then cropping in post-processing will yield identical results.

Q: Windows returns an error about missing MSVCP140.dll<br/>
A: Install: https://www.microsoft.com/en-us/download/details.aspx?id=48145

Q: Windows reports an error: "This app can't run on your PC. To find a version for your PC, check with the software publisher."<br/>
A: The binary is for 64-bit windows, while you are probably running 32-bit

Q: Windows is unable to convert files in directory containing non ASCII characters</br>
A: Adobe libraries do not support unicode file names on Windows, so as result the same is true for sjcam_raw2dng

Q: On Mac OSX GUI does not start<br/>
A: Install wxPython and put sjcam_raw2dng in the same folder as the GUI

Q: On Mac OSX running the executable results in "permission denied"<br/>
A: Type "cd" [drag sjcam_raw2dng_v1.0.1_macosx folder here]</br>
Type "ls -l"</br>
Hit return.</br>
Then type "chmod 755" [Drag the sjcam_raw2dng here]</br>
Hit return.</br>

Now you will be able to etither run the sjcam_raw2dng binary from console</br>
or drag and drop files/folders into the sjcam_raw2dng binary

Q: Why does a cropped FOV raw have lower resolution while jpeg has higher resolution?<br/>
A: Cropped FOV resolution uses only part of the sensor. While JPEG engine inside the camera</br>
extrapolates to a higher resolution, raw format has only the pixels taken from the sensor

