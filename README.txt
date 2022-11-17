Files Used in Project

To Compile the project you need Microsoft Visual Studio 2005 + latest OpenCV library.

FaceHandInteract.cpp, FaceHandInteractDlg.cpp - MFC files for the GUI and interaction.
BitmapViewer.cpp - a custom control in MFC to place OpenCV images (IplImage) on the dialog view.
ImageProcessor.cpp - class with all algorithmic part of the project.

XML files are trained cascades to detect hand on image using HaarClassifiers. Images must be specially segmented images.

shockwaveflash1.cpp - flash plugin for MFC.

In the carousel folder you can find the carousel flash files.

All other files are not really necessary.

Code is partly commented.