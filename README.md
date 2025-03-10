# Support Ukraine:
  - Via United24 platform (the initiative of the President of Ukraine):
    - [One click donation (credit card, bank transfer or crypto)](https://u24.gov.ua/)
  - Via National Bank of Ukraine:
    - [Ukrainian army](https://bank.gov.ua/en/about/support-the-armed-forces)
    - [Humanitarian aid to Ukraine](https://bank.gov.ua/en/about/humanitarian-aid-to-ukraine)

# LightlyShaders v3.0
 This is a fork of Luwx's [LightlyShaders](https://github.com/Luwx/LightlyShaders), which in turn is a fork of [ShapeCorners](https://sourceforge.net/projects/shapecorners/).  

 **It now also includes a fork of KWin Blur effect to fix the "korner bug".** It is disabled by default, you will have to enter Effects settings, disable the stock blur and enable the one with the "LightlyShaders" lable.

 This effect works correctly with stock Plasma effects.

 ![default](https://github.com/a-parhom/LightlyShaders/blob/plasma6/screenshot.png)

# Dependencies:
 
Plasma >= 6.0.
 
You will need qt6, kf6 and kwin development packages.

# Manual installation
```
git clone https://github.com/a-parhom/LightlyShaders

cd LightlyShaders;

mkdir qt6build; cd qt6build; cmake ../ -DCMAKE_INSTALL_PREFIX=/usr && make && sudo make install
```

## Note
After some updates of Plasma this plugin may need to be recompiled in order to work with changes introduced to KWin.
 
