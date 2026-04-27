# LightlyShaders (helton-godoy fork)

Fork focused on keeping LightlyShaders usable on both:

- `Qt5 + KDE Plasma 5.27` (legacy but still widely used)
- `Qt6 + KDE Plasma 6.x` (current upstream focus)

## Compatibility Matrix

- `master` -> Qt6 / KF6 / Plasma 6.x
- `v2.2.1` (tag from upstream history) -> Qt5 / KF5 / Plasma 5.27.x

For Plasma 5.27 users, this fork includes helper scripts so you do not need to manually switch repos.

## Quick Install

### Auto-detect (recommended)

```bash
chmod +x scripts/install-auto.sh
./scripts/install-auto.sh
```

### Force Qt6 / Plasma 6 path

```bash
chmod +x scripts/install-qt6.sh
./scripts/install-qt6.sh
```

### Force Qt5 / Plasma 5.27 path

```bash
chmod +x scripts/install-qt5.sh
./scripts/install-qt5.sh
```

## Notes

- The Qt5 path uses a git worktree based on tag `v2.2.1` and applies the old `libkwin.so` symlink workaround required on some Plasma 5 package layouts.
- After Plasma/KWin updates, rebuild the effect.
- On Plasma 5.27, disable stock Blur and enable `LightlyShaders Blur` in KWin Effects for best corner/outline results.
