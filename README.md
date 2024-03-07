# Support Ukraine:
  - Via United24 platform (the initiative of the President of Ukraine):
    - [One click donation (credit card, bank transfer or crypto)](https://u24.gov.ua/)
  - Via National Bank of Ukraine:
    - [Ukrainian army](https://bank.gov.ua/en/about/support-the-armed-forces)
    - [Humanitarian aid to Ukraine](https://bank.gov.ua/en/about/humanitarian-aid-to-ukraine)

# LightlyShaders v2.2
 Round window corners and outline effect for KWin.
 
 This is a fork of Luwx's [LightlyShaders](https://github.com/Luwx/LightlyShaders), which in turn is a fork of [ShapeCorners](https://sourceforge.net/projects/shapecorners/).  

 It deals with infamous "korner bug" and works correctly with stock Plasma effects (except WindowHeap-based effects, which is a [bug](https://bugs.kde.org/show_bug.cgi?id=457442) in KWin).

 ![default](https://github.com/a-parhom/LightlyShaders/blob/master/screenshot.png)

# Changelog:
  - Includes a fork of KWin Blur effect to fix the "korner bug" on X11 and Wayland (should be enabled manually instead of the default Blur effect)
  - The settings for outline width and color were added
  - Works with any window decorations
    - Shadow offset parameter can solve issues with window decorations outlines
  - Window shaping does not use additional textures and is done entirely in shader, which potentially improves performance

# Plasma 6:
On Plasma 6 Beta and RC builds you can try experimental branch **plasma6**. Eventually it will become the main branch.

# Notes:
  - After some updates of Plasma this plugin may need to be recompiled in order to work with changes introduced to KWin.
  - This version includes a patched KWin Blur effect, that shapes blur region using the lshelper library that also comes bundled with this plugin. To build it in Plasma 5.27.10, some KWin headers were included and an installer script introduced, which makes a symlink to libkwin.so.5. This is needed, because KWin development packages for Plasma 5.27 does not include that (which is not the case in Plasma 6).
  - You will need to install qt5, kf5, kwin and xcb development packages. Instructions below come mostly from previous version and most likely do not include each required package. Unfortunately, I'm not able to figure out the dependency packages for each linux distro, so pull requests are welcome here.

# Dependencies:
 
Plasma >= 5.27.10.
 
Debian based (Ubuntu, Kubuntu, KDE Neon):
```bash
sudo apt install git cmake g++ gettext extra-cmake-modules qttools5-dev libqt5x11extras5-dev libkf5*-dev libxcb*-dev kinit-dev kwin-dev 
```
Fedora based
```bash
sudo dnf install git cmake gcc-c++ extra-cmake-modules qt5-qttools-devel qt5-qttools-static qt5-qtx11extras-devel kf5-kconfigwidgets-devel kf5-kcrash-devel kf5-kguiaddons-devel kf5-kglobalaccel-devel kf5-kio-devel kf5-ki18n-devel kf5-knotifications-devel kf5-kinit-devel kwin-devel qt5-qtbase-devel libepoxy-devel kdecoration-devel kf5-kiconthemes-devel kf5-kpackage-devel xcb-imdkit-devel xcb-proto xcb-util-cursor-devel xcb-util-devel xcb-util-image-devel xcb-util-keysyms-devel xcb-util-renderutil-devel xcb-util-wm-devel xcb-util-xrm-devel xcb-imdkit xcb-util-xrm
```
Arch based
```bash
sudo pacman -S git make cmake gcc gettext extra-cmake-modules qt5-tools qt5-x11extras kcrash kglobalaccel kde-dev-utils kio knotifications kinit kwin
```
OpenSUSE based
```bash
sudo zypper install git cmake gcc-c++ extra-cmake-modules libqt5-qttools-devel libqt5-qtx11extras-devel kconfigwidgets-devel kcrash-devel kguiaddons-devel kglobalaccel-devel kio-devel ki18n-devel knotifications-devel kinit-devel kwin5-devel libQt5Gui-devel libQt5OpenGL-devel libepoxy-devel
```

# Installation
## Via script
```bash
git clone https://github.com/a-parhom/LightlyShaders

cd LightlyShaders;

chmod +x install.sh
sudo ./install.sh
```
## Manually
If the script doesn't work for you, you can find libkwin.so.5 file manually and make a symlink named libkwin.so to it in the same directory (or in the directory, where `ld` looks for libraries to link with). After that you can build and install like in previous versions:
```bash
mkdir build; cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
sudo make install
```

# Uninstall
```bash
chmod +x uninstall.sh
sudo ./uninstall.sh
```
