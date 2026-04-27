# LightlyShaders KWin 6.6+ - Correções Complementares (Arch Linux)

## Visão Geral

Este documento complementa o `LIGHTLYSHADERS_KWIN66_FIX.md` e documenta as correções específicas realizadas para **Arch Linux/CachyOS** com KDE Plasma 6.6.1.

---

## Diferenças do Documento Original

| Item               | Documento Original | Correção Realizada               |
| ------------------ | ------------------ | -------------------------------- |
| `Qt6::CorePrivate` | Adicionar          | **Remover** (não existe no Arch) |
| Blur effect        | Incluído           | **Desabilitado**                 |

### Motivo

O Arch Linux/CachyOS não possui o pacote `qt6-base-private-headers` ou este não inclui `Qt6::CorePrivate`. A correção correta foi **remover** essa dependência ao invés de adicionar.

---

## Correções Realizadas

### 1. CMakeLists.txt - Remoção de CorePrivate

**Arquivo:** `src/liblshelper/CMakeLists.txt`

```diff
 target_link_libraries(lshelper
     Qt6::Core
-    Qt6::CorePrivate
     Qt6::Gui
     Qt6::DBus
```

**Arquivo:** `src/lightlyshaders/CMakeLists.txt`

```diff
 target_link_libraries(${LIGHTLYSHADERS}
     Qt6::Core
-    Qt6::CorePrivate
     Qt6::Gui
     Qt6::DBus
```

**Arquivo:** `src/lightlyshaders/kcm/CMakeLists.txt`

```diff
 target_link_libraries(kwin_lightlyshaders_config
     KWin::kwin
     Qt6::Widgets
     Qt6::Core
-    Qt6::CorePrivate
     Qt6::DBus
     Qt6::Gui
```

### 2. Desabilitação do Blur Effect

**Arquivo:** `src/CMakeLists.txt`

```diff
 add_subdirectory(lightlyshaders)
 add_subdirectory(liblshelper)
-add_subdirectory(blur)
+# add_subdirectory(blur)  # Temporarily disabled - needs more KWin 6.6+ API updates
```

### 3. Correções em lightlyshaders.h

```diff
 void setRoundness(const int r, LogicalOutput *s);

 void reconfigure(ReconfigureFlags flags) override;
-void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *s) override;
-void prePaintWindow(EffectWindow* w, WindowPrePaintData& data, std::chrono::milliseconds time) override;
-void drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow* w, int mask, const QRegion& region, WindowPaintData& data) override;
+void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const Region &region, LogicalOutput *s) override;
+void prePaintWindow(RenderView *view, EffectWindow* w, WindowPrePaintData& data, std::chrono::milliseconds time) override;
+void drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow* w, int mask, const Region& region, WindowPaintData& data) override;

-std::unordered_map<Output *, LSScreenStruct> m_screens;
+std::unordered_map<LogicalOutput *, LSScreenStruct> m_screens;
```

### 4. Correções em lightlyshaders.cpp

```diff
- LightlyShadersEffect::setRoundness(const int r, Output *s)
+ LightlyShadersEffect::setRoundness(const int r, LogicalOutput *s)

- for(Output *s : screens)
+ for(LogicalOutput *s : screens)

- LightlyShadersEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *s)
+ LightlyShadersEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const Region &region, LogicalOutput *s)

- LightlyShadersEffect::prePaintWindow(EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds time)
+ LightlyShadersEffect::prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds time)

- effects->prePaintWindow(w, data, time);
+ effects->prePaintWindow(view, w, data, time);

- Output *s = w->screen();
+ LogicalOutput *s = w->screen();

- data.opaque -= reg;
+ data.deviceOpaque -= Region(reg);

- LightlyShadersEffect::drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const QRegion &region, WindowPaintData &data)
+ LightlyShadersEffect::drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &region, WindowPaintData &data)

- QRectF screen = viewport.renderRect().toRect();
+ QRectF screen = viewport.renderRect();
```

---

## Status de Compatibilidade

| Componente     | Status         | Notas                                     |
| -------------- | -------------- | ----------------------------------------- |
| LightlyShaders | ✅ FUNCIONAL    | Sombras + cantos arredondados             |
| Blur Effect    | ❌ INCOMPATÍVEL | Precisa de mais conversões QRegion→Region |

---

## Problema com Blur Effect

O efeito Blur separado não compilou devido a incompatibilidades na função `roundBlurRegion()`:

- `lshelper.h` define: `void roundBlurRegion(EffectWindow *w, QRegion *region)`
- O código do blur usa: `Region` (KWin::Region)
- A conversão entre tipos é complexa e requer mudanças significativas

**Solução temporária:** O Blur foi desabilitado para permitir a compilação do LightlyShaders principal.

---

## Compilação e Instalação

```bash
# 1. Configurar
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr

# 2. Compilar
cmake --build build -j$(nproc)

# 3. Instalar
sudo cmake --install build
```

---

## Verificação

```bash
# Verificar arquivos instalados
ls -la /usr/lib/qt6/plugins/kwin/effects/plugins/ | grep lightly
ls -la /usr/lib/qt6/plugins/kwin/effects/configs/ | grep lightly

# Ativar efeito
# Configurações do Sistema → Composição → LightlyShaders (ativar)
```

---

## Ambiente Testado

- **Sistema:** CachyOS (Arch Linux)
- **Plasma:** 6.6.1
- **KWin:** 6.6.1
- **Qt:** 6.10.2
- **KF6:** 6.23.0
- **GCC:** 15.2.1

---

## Notas

1. O patch no header do KWin (`xcbutils.h`) ainda pode ser necessário dependendo da versão
2. Após atualizações do KWin, pode ser necessário reaplicar o patch
3. O LightlyShaders principal funciona perfeitamente sem o Blur separado

---

*Documento criado em: 26/02/2026*
*Para: Arch Linux / CachyOS + KDE Plasma 6.6.1*
