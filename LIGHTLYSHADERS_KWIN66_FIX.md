# Documentação de Correções - LightlyShaders para KWin 6.6+ (CachyOS)

## Visão Geral

Este documento contém todas as correções necessárias para compilar e instalar o LightlyShaders (sombras + cantos arredondados + blur) no KDE Plasma 6.6+ com KWin 6.6.0 no CachyOS.

---

## Problema Original

O KWin 6.6+ introduziu mudanças significativas na API:

- `Output*` → `LogicalOutput*`
- `QRegion` → `KWin::Region`
- `prePaintWindow` ganhou parâmetro adicional (`RenderView*`)
- `WindowPrePaintData.opaque` → `deviceOpaque`
- `WindowPrePaintData.paint` → `devicePaint`

Além disso, há um bug nos headers do KWin que impede compilação com GCC 15.2.1 (uso de C++26 não suportado).

---

## Correção 1: Patch no Header do KWin (OBRIGATÓRIO)

**Arquivo:** `/usr/include/kwin/utils/xcbutils.h`

**Linhas:** 1059-1064

**Problema:** O KWin usa `.transform()` em `std::optional` (C++26), não suportado pelo GCC 15.

**Correção - Substituir:**

```cpp
        inline SizeHints *sizeHints()
        {
            return array<SizeHints>(32, XCB_ATOM_WM_SIZE_HINTS).transform([](std::span<SizeHints> span) {
                return span.empty() ? nullptr : span.data();
            }).value_or(nullptr);
        }
```

**Por:**

```cpp
        inline SizeHints *sizeHints()
        {
            auto opt = array<SizeHints>(32, XCB_ATOM_WM_SIZE_HINTS);
            if (opt.has_value() && !opt->empty()) {
                return opt->data();
            }
            return nullptr;
        }
```

**Comando sed para aplicar:**

```bash
sudo sed -i '1059,1064c\        inline SizeHints *sizeHints()\n        {\n            auto opt = array<SizeHints>(32, XCB_ATOM_WM_SIZE_HINTS);\n            if (opt.has_value() \&\& !opt->empty()) {\n                return opt->data();\n            }\n            return nullptr;\n        }' /usr/include/kwin/utils/xcbutils.h
```

---

## Correção 2: Arquivos do LightlyShaders

### 2.1 CMakeLists.txt Principal

**Arquivo:** `/tmp/LightlyShaders/CMakeLists.txt`

Adicionar `CorePrivate` aos componentes do Qt6:

```cmake
find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Gui
    Core
    CorePrivate  # ADICIONAR
    DBus
    UiTools
    Widgets
    OpenGL
    Network
    Xml
)
```

### 2.2 lightlyshaders.h

**Arquivo:** `/tmp/LightlyShaders/src/lightlyshaders/lightlyshaders.h`

**Alterações:**

```cpp
// Linha 42: Output* → LogicalOutput*
void setRoundness(const int r, LogicalOutput *s);

// Linha 45: QRegion → Region, Output* → LogicalOutput*
void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const Region &region, LogicalOutput *s) override;

// Linha 46: Adicionar RenderView* como primeiro parâmetro
void prePaintWindow(RenderView *view, EffectWindow* w, WindowPrePaintData& data, std::chrono::milliseconds time) override;

// Linha 47: QRegion → Region
void drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow* w, int mask, const Region& region, WindowPaintData& data) override;

// Linha 85: Output* → LogicalOutput*
std::unordered_map<LogicalOutput *, LSScreenStruct> m_screens;
```

### 2.3 lightlyshaders.cpp

**Arquivo:** `/tmp/LightlyShaders/src/lightlyshaders/lightlyshaders.cpp`

**Alterações:**

```cpp
// Linha 142: Output* → LogicalOutput*
LightlyShadersEffect::setRoundness(const int r, LogicalOutput *s)

// Linha 182: Output* → LogicalOutput*
for(LogicalOutput *s : screens)

// Linha 198: QRegion → Region, Output* → LogicalOutput*
LightlyShadersEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const Region &region, LogicalOutput *s)

// Linha 223: Adicionar RenderView* e alterar Output* → LogicalOutput*
LightlyShadersEffect::prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds time)

// Linha 231: Output* → LogicalOutput*
LogicalOutput *s = w->screen();

// Linha 257: data.opaque → data.deviceOpaque
data.deviceOpaque -= Region(reg);

// Linha 277: QRegion → Region, Output* → LogicalOutput*
LightlyShadersEffect::drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &region, WindowPaintData &data)

// Linha 279: Remover .toRect() (RenderViewport::renderRect() retorna RectF, não precisa converter)
QRectF screen = viewport.renderRect();

// Linha 287: Output* → LogicalOutput*
LogicalOutput *s = w->screen();
```

### 2.4 blur.h

**Arquivo:** `/tmp/LightlyShaders/src/blur/blur.h`

**Alterações:**

```cpp
// Linha 42: Output* → LogicalOutput*
std::unordered_map<LogicalOutput *, BlurRenderData> render;

// Linha 60: Adicionar RenderView* e alterar QRegion → Region
void prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds presentTime) override;

// Linha 61: QRegion → Region
void drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &region, WindowPaintData &data) override;

// Linha 78: Output* → LogicalOutput*
void slotScreenRemoved(KWin::LogicalOutput *screen);

// Linha 91: QRegion → Region
void blur(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &region, WindowPaintData &data);

// Linha 129-130: QRegion → Region
Region m_paintedArea;
Region m_currentBlur;

// Linha 131: Output* → LogicalOutput*
LogicalOutput *m_currentScreen = nullptr;
```

### 2.5 blur.cpp

**Arquivo:** `/tmp/LightlyShaders/src/blur/blur.cpp`

**Alterações (muitas - список completo):**

```cpp
// Linha 256: Conversão explícita
content = QRegion(surf->blur()->region());

// Linha 322: Output* → LogicalOutput*
void BlurEffect::slotScreenRemoved(KWin::LogicalOutput *screen)

// Linha 424-425: QRegion() → Region()
m_paintedArea = Region();
m_currentBlur = Region();

// Linha 431: Adicionar RenderView*
void BlurEffect::prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds presentTime)

// Linha 437: QRegion → Region
const Region oldOpaque = data.deviceOpaque;

// Linha 440-442: Region com rects()
Region newOpaque;
for (const Rect &rect : data.deviceOpaque.rects()) {
    newOpaque += Region(rect.adjusted(m_expandSize, m_expandSize, -m_expandSize, -m_expandSize));
}

// Linha 457: Conversão usando Rect intermediário
const Rect blurRect = blurRegion(w).boundingRect().translated(w->pos().toPoint());
const Region blurArea = Region(blurRect);

// Linha 577-593: Iteration com rects() e conversões
// (ver código fonte para detalhes completos)

// Linha 629-631: Region com QRegion()
const Region dirtyRegion = region & Region(QRegion(backgroundRect));
for (const Rect &dirtyRect : dirtyRegion.rects()) {

// Linha 578: infiniteRegion() → Region::infinite()
if (region != Region::infinite()) {

// Linha 496: QRegion → Region
void BlurEffect::drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &region, WindowPaintData &data)

// Linha 540: QRegion → Region
void BlurEffect::blur(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &region, WindowPaintData &data)
```

---

## Script de Aplicação Automática

Para facilitar, crie um script:

```bash
#!/bin/bash
# apply_lightlyshaders_fixes.sh

set -e

echo "=== Aplicando correção no header do KWin ==="

# Backup
sudo cp /usr/include/kwin/utils/xcbutils.h /usr/include/kwin/utils/xcbutils.h.bak 2>/dev/null || true

# Aplicar patch
sudo sed -i '1059,1064c\        inline SizeHints *sizeHints()\n        {\n            auto opt = array<SizeHints>(32, XCB_ATOM_WM_SIZE_HINTS);\n            if (opt.has_value() \&\& !opt->empty()) {\n                return opt->data();\n            }\n            return nullptr;\n        }' /usr/include/kwin/utils/xcbutils.h

echo "=== Correção aplicada com sucesso ==="
echo "Agora compile o LightlyShaders:"
echo "  cd /tmp/LightlyShaders"
echo "  rm -rf build && mkdir build && cd build"
echo "  cmake ../ -DCMAKE_INSTALL_PREFIX=/usr -DECM_DIR=/usr/share/ECM/cmake"
echo "  make -j\$(nproc)"
echo "  sudo make install"
```

---

## Compilação e Instalação

```bash
# 1. Clonar repositório
cd /tmp
git clone https://github.com/a-parhom/LightlyShaders.git
cd LightlyShaders

# 2. Aplicar correções manuais nos arquivos fonte
# (use as edições listadas acima)

# 3. Configurar e compilar
rm -rf build && mkdir build && cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=/usr -DECM_DIR=/usr/share/ECM/cmake
make -j$(nproc)

# 4. Instalar
sudo make install
```

---

## Verificação e Ativação

```bash
# Verificar instalação
ls -la /usr/lib/qt6/plugins/kwin/effects/plugins/ | grep lightly

# Ativar efeito
# Configurações do Sistema → Composição → LightlyShaders (ativar)
# Reiniciar KWin: Alt+Shift+Esc (cuidado!) ou fazer logout/login
```

---

## Notas

1. **Reinstalação após updates**: Após atualizar o KWin, pode ser necessário reaplicar a correção do header (`xcbutils.h`).

2. **Backup**: O backup do header original fica em `/usr/include/kwin/utils/xcbutils.h.bak`

3. **Funciona com**:
   
   - CachyOS com KDE Plasma 6.6+
   - KWin 6.6.0
   - GCC 15.2.1

---

## Troubleshooting

**Erro: `.transform() not found`**

- Significa que o patch do header não foi aplicado corretamente
- Verifique as linhas 1059-1064 em `/usr/include/kwin/utils/xcbutils.h`

**Erro: `no matching function for intersects`**

- Falta conversão entre QRegion e Region
- Verifique as edições no arquivo blur.cpp

**Efeito não aparece**

- Reinicie o KWin completamente (logout/login)
- Verifique se o arquivo .so está em `/usr/lib/qt6/plugins/kwin/effects/plugins/`

---

*Documento criado em: 21/02/2026*
*Para: CachyOS + KDE Plasma 6.6+*
