# 关卡校验逻辑拆分说明

## 本次拆分目标

本次没有大幅改动 `GameScene` 或 `LevelEditorDialog` 的运行行为，而是先做一个低风险拆分：

- 新增 `levelvalidator.h` / `levelvalidator.cpp`。
- 将关卡地图校验、候选点校验、目标反转次数校验集中到 `LevelValidator`。
- `LevelManager` 不再自己维护整套 `validateLevel()` 细节。
- `LevelEditorDialog` 不再自己维护 `validateMapData()` / `validateEditablePoints()` 细节。

## 为什么先拆这个

`GameScene` 和 `LevelEditorDialog` 都很大，但直接拆游戏物理或 UI 绘制风险较高。
关卡校验逻辑比较独立，适合作为第一步拆分：

1. 改动范围清晰。
2. 方便后续统一规则。
3. 能减少多人协作时对大文件的重复修改。

## 新增文件职责

### `levelvalidator.h`

声明统一的关卡校验类 `LevelValidator`。

主要接口：

```cpp
static bool validateLevel(const Level &level, QString *errorMessage, const Options &options);
static bool validateMapData(const QStringList &mapData, QString *errorMessage, const Options &options);
static bool validateEditablePoints(const QStringList &mapData, const QVector<QPoint> &points, QString *errorMessage, const Options &options);
```

### `levelvalidator.cpp`

实现具体校验规则：

- 地图不能为空。
- 每一行长度必须一致。
- 地图字符必须是 `TileDefs` 已知元素。
- 必须有且只有一个起点。
- 至少有一个终点。
- 候选点不能越界、不能重复、不能放在墙/起点/终点/死亡区/数据碎片等关键格子上。

## 两种校验模式

为了减少行为变化，保留了两种配置：

### 运行时模式

```cpp
LevelValidator::runtimeOptions()
```

用于 `LevelManager` 读取关卡。默认继续兼容旧关卡，不强制 5~150 尺寸，也不强制 0~999 目标上限。

### 编辑器模式

```cpp
LevelValidator::editorOptions()
```

用于 `LevelEditorDialog` 保存/导入/校验地图。

规则更严格：

- 宽度 5~150。
- 高度 5~150。
- 目标反转次数 0~999。
- 候选点不能放在旧机关 `5` / `7` 上。
- 错误提示更适合弹窗展示。

## 同步修改文件

- `CMakeLists.txt`：加入 `levelvalidator.cpp` 和 `levelvalidator.h`。
- `levelmanager.cpp`：改用 `LevelValidator::validateLevel()`。
- `levelmanager.h`：移除私有 `validateLevel()` 声明。
- `leveleditordialog.cpp`：改用 `LevelValidator` 做地图和候选点校验。
- `leveleditordialog.h`：移除私有 `validateMapData()` / `validateEditablePoints()` 声明。

## 后续建议

下一步可以继续拆：

1. `LevelJsonService`：把 JSON 读取/保存从 `LevelManager` 和 `LevelEditorDialog` 里抽出来。
2. `EditorMapTableController`：把设计器表格绘制、缩放、拖动画格子拆出来。
3. `MechanismSystem`：把激光门、蹦床、数据碎片从 `GameScene` 中拆出来。


## Fix 1：MinGW 默认参数兼容修正

修复 `levelvalidator.h` 中如下形式导致的 MinGW/GCC 编译错误：

```cpp
const Options &options = Options()
```

原因是 `Options` 是 `LevelValidator` 的嵌套结构体，并且包含成员默认初始化器。
在外层类尚未完全结束前，将 `Options()` 用作默认参数，MinGW/GCC 会报：

```text
default member initializer for 'LevelValidator::Options::xxx' required before the end of its enclosing class
```

修正方式：

```cpp
const Options &options
```

当前项目中的调用点本来都显式传入了 `LevelValidator::runtimeOptions()` 或
`LevelValidator::editorOptions()`，所以移除默认参数不会改变现有行为。


## Fix 2：默认参数顺序修正

上一版移除了 `Options()` 默认参数后，函数声明仍保留了：

```cpp
QString *errorMessage = nullptr,
const Options &options
```

C++ 不允许“前面的参数有默认值，后面的参数没有默认值”，因此 MinGW 报：

```text
default argument missing for parameter ...
```

本次修正改为重载形式：

```cpp
validateLevel(level, errorMessage)
validateLevel(level, errorMessage, options)
```

不带 `options` 的重载默认使用 `runtimeOptions()`；编辑器和关卡管理器仍可显式传入
`editorOptions()` / `runtimeOptions()`。
