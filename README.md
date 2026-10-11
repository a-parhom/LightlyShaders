# Support Ukraine:
  - Via United24 platform (the initiative of the President of Ukraine):
    - [One click donation (credit card, bank transfer or crypto)](https://u24.gov.ua/)
  - Via National Bank of Ukraine:
    - [Ukrainian army](https://bank.gov.ua/en/about/support-the-armed-forces)
    - [Humanitarian aid to Ukraine](https://bank.gov.ua/en/about/humanitarian-aid-to-ukraine)

# LightlyShaders v3.0
 This is a fork of Luwx's [LightlyShaders](https://github.com/Luwx/LightlyShaders), which in turn is a fork of [ShapeCorners](https://sourceforge.net/projects/shapecorners/).  

 **It now also includes a fork of KWin Blur effect to fix the "korner bug".** It is disabled by default, you will have to enter Effects settings, disable the stock blur and enable the one with the "LightlyShaders" label. Both use the same blur settings, so they carry over when switching.

 This effect works correctly with stock Plasma effects.

 ![default](screenshot.png)

# Dependencies:

Plasma >= 6.7.

Arch Linux:
```
sudo pacman -S --needed base-devel cmake extra-cmake-modules kwin kdecoration qt6-base kconfig kcmutils ki18n kcoreaddons kwidgetsaddons libxcb
```

Other distributions need a C++ compiler, CMake, extra-cmake-modules and the development packages of KWin, KDecoration, Qt 6 and KDE Frameworks 6 (KConfig, KCMUtils, KI18n, KCoreAddons, KWidgetsAddons), usually named like `kwin-dev` or `kwin-devel`.

If CMake reports `Could not find a package configuration file provided by "ECM"`, extra-cmake-modules is missing.

# Manual installation
```
git clone https://github.com/akano12/LightlyShaders

cd LightlyShaders

cmake -B build -DCMAKE_INSTALL_PREFIX=/usr && cmake --build build && sudo cmake --install build
```

Log out and back in afterwards, KWin only loads effects at startup.

## Tests
`ctest --test-dir build` checks the settings and renders the shaders on the GPU, so run it after changing them or after a KWin update. The shader tests are skipped when no GPU is available.

## Note
After some updates of Plasma (or Qt) this plugin needs to be recompiled in order to work with changes introduced to KWin. If the effects disappear from the Desktop Effects list after an update, rebuild and reinstall.

Older versions installed a `liblshelper` library to `/usr/lib`, it is no longer needed and can be removed:
```
sudo rm /usr/lib/liblshelper.so*
```
 
