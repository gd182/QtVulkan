<h1 align="center">QtVulkan</h1>

<p align="center">
  3D-вьюер реального времени с физической симуляцией на Qt 6 и Vulkan
</p>

###

<div align="center">
  <img src="https://skillicons.dev/icons?i=cpp" height="40" alt="C++ logo" />
  <img width="12" />
  <img src="https://skillicons.dev/icons?i=cmake" height="40" alt="CMake logo" />
  <img width="12" />
  <img src="https://skillicons.dev/icons?i=git" height="40" alt="git logo" />
  <img width="12" />
  <img src="https://skillicons.dev/icons?i=windows" height="40" alt="windows logo" />
</div>

###

## О проекте

**QtVulkan** — десктопное приложение, встраивающее поверхность Vulkan-рендеринга в окно Qt Quick. Рендерится 3D-сфера, отскакивающая от плоскости под действием простой физической симуляции. Боковая панель QML отображает FPS в реальном времени, слайдер упругости и кнопку сброса — всё это связано с Vulkan-движком через `SceneController`.

## Возможности

- **Vulkan-рендерер** — swapchain, буфер глубины, render pass, descriptor sets и graphics pipeline с нуля
- **Физическая симуляция** — гравитация и упругий отскок от плоскости с настраиваемым коэффициентом восстановления
- **Орбитальная камера** — перетаскивание левой кнопкой мыши для вращения, колесо для зума
- **QML-оверлей** — счётчик FPS, слайдер упругости (0.0 – 1.0), кнопка сброса
- **3D-меши** — процедурные генераторы сферы и пирамиды
- **GLSL-шейдеры** — компилируются во время сборки через `glslc` и копируются рядом с исполняемым файлом
- **Слои валидации** — `VK_LAYER_KHRONOS_validation` и `VK_EXT_debug_utils` активны в debug-сборках

## Стек технологий

| Компонент | Технология |
|-----------|------------|
| Язык | C++20 |
| GUI / QML | Qt 6.8 (Quick, Gui, QuickControls2) |
| Графический API | Vulkan 1.0 |
| Математика | GLM (header-only, через vcpkg) |
| Шейдеры | GLSL → SPIR-V через glslc |
| Система сборки | CMake 3.20+ |

## Структура проекта

```
QtVulkan/
├── CMakeLists.txt           # Конфигурация сборки, компиляция шейдеров
├── Main.qml                 # QML-окно: FPS, слайдер упругости, подсказки
├── main.cpp                 # Точка входа: QVulkanInstance, VulkanWindow, QML engine
├── build_and_run.bat        # Сборка и запуск одной командой (Windows)
└── src/
    ├── VulkanCore.cpp/h     # Полный Vulkan pipeline (device, swapchain, renderpass…)
    ├── VulkanWindow.cpp/h   # Подкласс QVulkanWindow, цикл рендеринга
    ├── SceneController.cpp/h # QObject-мост между QML и Vulkan-движком
    ├── Camera.cpp/h         # Орбитальная камера (азимут / угол / радиус)
    ├── Physics.cpp/h        # Физика мяча: гравитация + отскок (BallPhysics)
    ├── vk3dSphere.cpp/h     # Процедурный меш сферы
    ├── vk3dPyramid.cpp/h    # Процедурный меш пирамиды
    ├── vk3dObject.cpp/h     # Базовый 3D-объект
    ├── Transform.cpp/h      # Данные трансформации (позиция, поворот, масштаб)
    └── TypesData.h          # Общие структуры вершин и UBO
```

## Требования

- Windows 10/11 (64-bit)
- **Qt 6.8+** с модулями Quick и QuickControls2
- **Vulkan SDK** (1.0+) — предоставляет `glslc` для компиляции шейдеров
- **GLM** — установить через vcpkg: `vcpkg install glm`
- **CMake 3.20+**
- GPU с поддержкой Vulkan 1.0

## Сборка и запуск

### 1. Клонирование репозитория

```bash
git clone <repo-url>
cd QtVulkan
```

### 2. Конфигурация и сборка

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

CMake автоматически скомпилирует `sphere.vert` / `sphere.frag` в SPIR-V через `glslc` и скопирует `.spv`-файлы рядом с исполняемым файлом.

### 3. Запуск

```bash
./build/Release/appQtVulkan.exe
```

Или использовать готовый скрипт:

```bat
build_and_run.bat
```

## Управление

| Действие | Ввод |
|----------|------|
| Вращение камеры | Перетаскивание левой кнопкой мыши |
| Зум | Колесо мыши |
| Сброс физики | `R` или `Space` (или кнопка на панели) |
| Упругость отскока | Слайдер на панели (0.0 = пластик, 1.0 = упругий) |

## Шейдеры

Исходники шейдеров берутся из `../Vulcan/shaders/` относительно корня проекта и компилируются в `shaders/compiled/`:

| Файл | Роль |
|------|------|
| `sphere.vert` | Вершинный шейдер — MVP-преобразование + нормаль |
| `sphere.frag` | Фрагментный шейдер — диффузное + фоновое освещение |
