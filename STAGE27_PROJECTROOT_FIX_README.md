# Stage 27 保存路径修正版

## 修正原因

上一版直接使用：

```cpp
QCoreApplication::applicationDirPath()
```

所以保存到了：

```text
项目根目录/build/Desktop_Qt_xxx-Debug/custom_levels
```

这不是你想要的外层文件夹。

## 现在改成

程序会自动从运行目录：

```text
项目根目录/build/Desktop_Qt_xxx-Debug
```

向上回到：

```text
项目根目录
```

然后保存到：

```text
项目根目录/custom_levels
项目根目录/levels
```

## 同步修改

为了保证“保存后关卡选择能看到新关卡”，本次同时修改了：

- `leveleditordialog.h`
- `leveleditordialog.cpp`
- `levelmanager.cpp`

现在保存路径和读取路径保持一致。

## 验收方式

1. 运行程序
2. 打开关卡设计师
3. 生成地图并放置起点、终点
4. 保存到 `custom_levels`
5. 弹窗路径应该类似：

```text
D:/code/QTDraftCooperation/kristenLab/custom_levels/xxx.json
```

而不是：

```text
D:/code/QTDraftCooperation/kristenLab/build/Desktop_Qt_xxx-Debug/custom_levels/xxx.json
```

6. 回到关卡选择界面，新关卡应能显示出来。
