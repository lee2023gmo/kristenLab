# 激光门元素说明

本次新增地图元素：`L`，名称：激光门。

## 玩法规则

- 激光门周期性亮灭。
- 亮起时：相当于临时墙体，角色不能通过，撞到后会被反弹回去。
- 熄灭时：对角色完全没有影响，可以自由通过。

## 周期配置

在 `constants.h` 中调整：

```cpp
const int LASER_ACTIVE_MS = 1200;
const int LASER_INACTIVE_MS = 3000;
```

## 可测试关卡

已新增：

```text
levels/laser_test.json
```

进入“关卡选择”后选择“激光门测试”，或直接开始游戏进行测试。
