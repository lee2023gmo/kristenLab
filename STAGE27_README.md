# Stage 27 修改说明

本压缩包基于阶段 26 继续修改。

## 已实现

阶段 27：保存为 JSON 文件。

现在在“关卡设计师”窗口中可以：

1. 读取关卡名
2. 读取目标反转次数
3. 从表格生成 `QStringList mapData`
4. 保存前自动校验地图
5. 使用 `QJsonObject / QJsonArray / QJsonDocument` 生成 JSON
6. 保存到程序运行目录下的 `custom_levels` 或 `levels` 文件夹
7. 保存成功后弹窗显示完整路径
8. 保存后的 JSON 可以被 `LevelManager` 自动读取

## 新增功能

新增保存位置下拉框：

- `自定义关卡文件夹 custom_levels`
- `内置关卡文件夹 levels`

默认保存到：

- `custom_levels`

如果你选择 `levels`，文件会保存到程序运行目录下的 `levels` 文件夹。

## 保存格式

```json
{
    "name": "我的设计关卡",
    "targetReverseCount": 6,
    "map": [
        "111111111111",
        "120000000031",
        "100050000001",
        "100000400001",
        "111111111111"
    ]
}
```

## 文件名规则

如果关卡名不为空：

- 使用 `关卡名.json`

如果关卡名为空：

- 使用 `custom_level_时间戳.json`

非法文件名字符会自动替换成 `_`。

## 验收方式

运行程序后：

1. 主菜单点击 `关卡设计师`
2. 点击 `生成地图`
3. 点击 `自动加边框墙`
4. 放置一个起点 `S`
5. 放置一个终点 `END`
6. 点击 `校验地图`
7. 确认校验通过
8. 选择保存位置 `custom_levels` 或 `levels`
9. 点击 `保存关卡`
10. 查看提示中的完整路径
11. 回到主菜单，打开 `关卡选择`
12. 新保存的关卡应该可以显示并进入游戏

## 注意

保存路径使用：

```cpp
QCoreApplication::applicationDirPath()
```

也就是和 `LevelManager` 读取路径一致。
