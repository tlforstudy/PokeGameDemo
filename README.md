# MyPokemonDemo

## 项目简介

- 基于 Unreal Engine 5.4 构建的 3D 宝可梦风格回合制战斗 Demo，采用 Data Asset 驱动物种与技能数据，通过 C++ 与 Blueprint 协作完成探索、组队、战斗、存档闭环，并扩展了服务器权威的双人联机对战模式。

## 核心架构亮点

- **数据驱动与持久化分层**：使用 Data Asset 配置物种和技能，Struct 保存队伍状态，SaveGame 负责持久化；Runtime Actor 只承担当前战斗逻辑与场景表现，避免静态配置、存档数据和 Actor 生命周期相互耦合。

- **集中式战斗状态机**：`BP_TurnManager` 统一管理回合阶段、行动、换人、阵亡与结算，UMG 仅提交操作并展示状态；C++ Function Library 负责速度排序、同速随机和无效对象过滤。

- **战斗逻辑与演出解耦**：`BP_BattleStage` 管理双方 Slot、地面检测和物种离地偏移，通过 View Target 切换战斗镜头，使位置计算、镜头和精灵动画能够独立复用。

- **服务器权威联机结算**：客户端通过 Server RPC 提交技能或换人意图，`ANetBattleGameMode` 统一验证并结算行动顺序、命中、伤害、阵亡和胜负；GameState / PlayerState 将权威状态同步至双方客户端。

- **状态同步与表现时序分离**：联机状态使用 `FNetPokemonState`、`FNetBattleAction` 等结构体建模；Replicated State 保证最终一致性，有序 Battle Cue 驱动攻击、受击、缓动掉血、换人与结算表现。

## 系统流程图（文字版）

### 单机探索与战斗

```text
探索状态
  ↓
玩家按 Q
  ↓
获取 FollowCamera Forward Vector
  ↓
生成 BP_Pokeball，并赋予初速度、重力和速度朝向
  ↓
Pokeball 碰撞命中野生 BP_pokemon
  ↓
记录命中的野生精灵引用，构建 Runtime Player Party
  ↓
获取 BP_TurnManager 与 BP_BattleStage
  ↓
传入 Player Party / Enemy Party，设置 CurrentPlayer / CurrentEnemy
  ↓
BattleStage 根据 PlayerSlot / EnemySlot 向下射线检测战斗地面
  ↓
按 SpeciesData 的离地高度计算双方精灵位置
  ↓
Set View Target with Blend 切换战斗镜头
  ↓
创建并刷新 WBP_BattleUI
  ↓
初始化回合与 BattlePhase
  ↓
玩家选择技能或切换精灵
  ↓
C++ 按 Speed 对行动 Actor 排序
  ↓
执行技能：伤害计算 → 伤害数字 → 血量刷新 → Battle Log → 镜头抖动
  ↓
检测阵亡
  ├─ 当前精灵阵亡：从队伍中选择可用精灵并刷新战场
  ├─ 敌方队伍仍有可用精灵：敌方切换并继续回合
  └─ 一方队伍全部阵亡：进入胜利或失败结算
  ↓
保存 Runtime 队伍状态
  ↓
退出战斗：隐藏/移除战斗 UI，清理运行时队伍引用
  ↓
胜利对野生精灵：销毁被命中的野生精灵 Actor，避免重复触发
  ↓
恢复探索镜头与玩家控制
```

### 双人联机对战

```text
玩家从联机入口选择 Host Battle
  ↓
创建 Listen Server，进入等待界面
  ↓
另一名玩家输入主机 IPv4 并选择 Join Battle
  ↓
GameMode 等待两名玩家连接并分别构建 FNetPokemonState 队伍
  ↓
GameState 进入 ChoosingActions，双方客户端创建并刷新 WBP_NetBattleUI
  ↓
玩家点击技能或换人按钮
  ↓
PlayerController 通过 Server RPC 提交 FNetBattleAction
  ↓
服务器验证战斗阶段、索引、阵亡状态和行动合法性
  ↓
GameMode 收集双方行动，全部提交后进入 ResolvingTurn
  ↓
按技能优先级与 Speed 决定顺序
  ↓
服务器计算命中、属性倍率、伤害、阵亡和强制换人
  ↓
更新 PlayerState / GameState 的 Replicated State
  ↓
生成并复制有序 FNetBattleCue
  ↓
双方 Presenter / UMG 依次播放攻击、受击、伤害数字与缓动血条
  ↓
进入下一回合、ChoosingForcedSwitch 或 BattleEnded
  ↓
双方分别显示 Victory / Defeat 结算结果
```

## 已实现功能

- **探索与交互**
  - 3D 第三人称探索场景。
  - 玩家角色移动、跟随摄像机和探索视角。
  - Q 键投掷 Pokeball，支持投掷物碰撞检测。
  - E 键接近训练师并触发训练师战斗。
  - B 键创建/移除玩家队伍状态面板。

- **队伍与数据**
  - 玩家队伍与训练师队伍配置。
  - `ST_PlayerPokemonEntry` 和 `ST_TrainerPokemonEntry` 结构化队伍数据。
  - Runtime 队伍 Actor 生成、初始化、切换和状态同步。
  - 物种、属性、技能、等级、生命值和阵亡状态读取。
  - 战斗位置支持通过 Species Data Asset 扩展物种专属离地高度配置。

- **回合制战斗**
  - 玩家技能选择和敌方自动行动。
  - 速度排序、同速随机顺序和回合推进。
  - 属性类型克制与伤害计算。
  - 阵亡判断、强制切换、主动切换和敌方队伍切换。
  - 战斗日志、技能按钮、血条、伤害数字和结算面板。

- **战斗演出与生命周期**
  - 战斗镜头切换与退出恢复。
  - BattleStage 固定战斗位置和地面射线定位。
  - 攻击镜头抖动和浮动伤害数字。
  - 战斗结束后的 UI 清理、队伍状态保存和野生精灵销毁。

- **双人联机对战**
  - 原生 C++ Host / Join 入口、IPv4 直连和 Listen Server 等待界面。
  - 服务器权威的行动验证、回合顺序、伤害、换人、阵亡与胜负结算。
  - 支持多精灵队伍、主动换人、濒死后手动强制换人以及双方独立胜负界面。
  - GameState / PlayerState 复制状态，Battle Cue 驱动双客户端 UI、日志和战斗演出。

## 技术栈与引擎特性

- **引擎**：Unreal Engine 5.4、C++、Blueprint、UMG。
- **数据与存档**：Data Asset、Struct、Enum、SaveGame。
- **单机战斗**：Blueprint 状态机、C++ Function Library、View Target、Line Trace。
- **联机框架**：Listen Server、Server RPC、属性复制、RepNotify、动态多播委托。
- **网络职责**：GameMode 权威结算，GameState / PlayerState 状态同步，Presenter / Battle Cue 驱动客户端表现。

## 目录结构

```text
MyPokemonDemo/
├─ Config/
│  ├─ DefaultEngine.ini
│  ├─ DefaultGame.ini
│  └─ DefaultInput.ini
├─ Content/
│  ├─ Blueprint/
│  │  ├─ Character/
│  │  ├─ Gamemode/
│  │  ├─ Input/
│  │  ├─ Pokemon/
│  │  ├─ Save/
│  │  └─ UI/
│  ├─ DA/
│  │  ├─ Moves/
│  │  └─ PokemonSpecies/
│  ├─ NetBattle/
│  │  ├─ BattleMenuMap
│  │  ├─ NetBattleMap
│  │  └─ WBP_NetBattleUI
│  └─ SimpleDamageText/
├─ Source/
│  └─ MyPokemonDemo/
│     ├─ MyCombatLibrary.h
│     ├─ MyCombatLibrary.cpp
│     ├─ NetBattleTypes.h
│     ├─ NetBattleGameMode.h / .cpp
│     ├─ NetBattleGameState.h / .cpp
│     ├─ NetBattlePlayerController.h / .cpp
│     ├─ NetBattlePlayerState.h / .cpp
│     ├─ NetBattlePresenter.h / .cpp
│     ├─ NetBattleVisualInterface.h
│     ├─ NetBattleMenuWidget.h / .cpp
│     ├─ NetBattleWaitingWidget.h / .cpp
│     └─ MyPokemonDemo.Build.cs
├─ MyPokemonDemo.uproject
└─ README.md
```

## 如何运行

- **环境要求**
  - Unreal Engine `5.4`。
  - Windows 开发环境。
  - 如需重新编译 C++ 模块，安装 Visual Studio 2022，并启用 Desktop development with C++ 和 Windows SDK。

- **打开项目**
  - 双击根目录的 `MyPokemonDemo.uproject`。
  - 如果编辑器提示重新编译 `MyPokemonDemo`，选择编译并等待 Editor Target 构建完成。
  - 项目已在 `.uproject` 中启用 `ModelingToolsEditorMode` 编辑器插件。
  - `SimpleDamageText` 作为 Content 资源随项目提供，不需要单独安装插件。

- **启动方式**
  - 启动菜单提供 `Single Player Demo`、`Host Battle` 和 `Join Battle` 三个入口。
  - 单机地图：`/Game/scene/Demo_Map`。
  - 联机地图：`/Game/NetBattle/NetBattleMap`。
  - 点击 Unreal Editor 工具栏中的 Play，可在 PIE 中测试单机或双客户端联机。

- **操作方式**
  - `WASD`：移动。
  - 鼠标：控制探索镜头。
  - `Q`：投掷 Pokeball，命中野生精灵后进入战斗。
  - `E`：与附近训练师交互并进入训练师战斗。
  - 战斗中：使用 UMG 技能按钮选择行动，使用 Switch 面板切换当前精灵。
  - `B`：查看或关闭玩家当前队伍状态。

- **联机操作**
  - 主机选择 `Host Battle` 创建 Listen Server，并停留在等待对手界面。
  - 客户端填写主机 IPv4 地址后选择 `Join Battle`；本机双开测试可使用 `127.0.0.1`。
  - 两名玩家连接完成后自动进入联机战斗。
  - 战斗中通过技能按钮提交攻击，通过 `Switch` 面板选择队伍成员。
  - 双方都提交行动后由服务器统一结算并广播结果。

- **PIE 双客户端测试**
  - 打开 `/Game/NetBattle/NetBattleMap`。
  - Play 设置中将 `Number of Players` 设为 `2`。
  - `Net Mode` 选择 `Play As Client` 或 `Play As Listen Server`。
  - 分别操作两个窗口，验证攻击、换人、血量、日志及胜负状态同步。

- **运行注意事项**
  - 首次打开项目时建议等待 Shader、Blueprint 和 Asset Registry 完成加载。
  - 如果修改了 `Source` 中的 C++ 文件，需要重新编译 Editor Target 后再运行。
  - 战斗需要场景中存在 `BP_TurnManager` 和 `BP_BattleStage` 实例，并正确配置战斗 Slot 和地面碰撞。
  - 使用自定义物种时，需要在 `Content/DA/PokemonSpecies` 中配置物种 Data Asset，并为队伍结构体设置对应的 `SpeciesData`、等级和技能引用。

## 视频演示

- [Bilibili：UE5 回合制战斗 Demo（单机闭环 + 双人联机）](https://www.bilibili.com/video/BV1ufuB6SErE/?share_source=copy_web&vd_source=316a67c1c2fe977667654c279e9aacee)
