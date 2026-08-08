# MyPokemonDemo

## 项目简介

- 基于 Unreal Engine 5.4 构建的 3D 宝可梦风格回合制战斗 Demo，采用 Data Asset 驱动物种与技能数据，并通过 C++ 与 Blueprint 协作完成探索、组队、战斗、存档和结算闭环。

## 核心架构亮点

- **Data Asset 数据驱动物种与技能配置**
  - 使用 `DA_PokemonSpecies` 描述物种名称、属性、基础战斗参数和模型相关配置。
  - 使用 `DA_move` 及具体 Move Data Asset 描述技能属性、威力和技能分类。
  - 将静态配置与战斗执行逻辑分离，新增物种或技能时主要通过创建数据资产和配置引用完成，减少 Blueprint 分支复制。

- **结构体数据与 Runtime Actor 分层**
  - 使用 `ST_PlayerPokemonEntry`、`ST_TrainerPokemonEntry` 等结构体保存玩家和训练师的队伍配置。
  - `BP_TurnManager` 根据结构体数据构建 `RuntimePlayerParty` 与 `RuntimeEnemyParty`，战斗阶段只操作当前运行时 Actor。
  - 将持久化数据与场景表现解耦，避免把已经销毁的战斗 Actor 当作长期队伍数据保存或复用。

- **SaveGame 持久化玩家进度**
  - 使用 `SG_PlayerProgress` 保存玩家队伍及战斗状态相关数据。
  - 通过结构体序列化队伍配置和实时状态，支持退出游戏后重新加载存档，并为后续扩展多存档槽、道具和进度标记保留数据边界。

- **集中式回合战斗状态机**
  - `BP_TurnManager` 管理 `BattlePhase`、当前回合、当前战斗双方、队伍切换、技能执行、阵亡处理和结算流程。
  - 通过 `CurrentPlayer`、`CurrentEnemy`、队伍数组和战斗阶段控制合法操作，避免 UI 直接修改战斗核心状态。
  - 玩家和敌方均支持多成员队伍、阵亡后的强制切换以及回合中的主动切换。

- **C++ 速度排序与同速规则处理**
  - `UMyCombatLibrary::SortActorsBySpeed` 作为 Blueprint Function Library 暴露给蓝图。
  - C++ 通过反射读取 Actor 的 `Speed` 属性，先按速度降序排序，再对同速 Actor 做组内随机打乱，保证速度优先级规则统一且同速结果符合战斗设计。
  - 排序前移除无效 Actor，降低已销毁对象进入排序器导致访问异常的风险，并将高频数组处理逻辑集中到可复用的 C++ 工具函数中。

- **BattleStage 与 View Target 战斗演出解耦**
  - `BP_BattleStage` 提供玩家和敌方战斗 Slot、战斗地面检测以及出场位置计算。
  - 进入战斗时通过 `Set View Target with Blend` 将镜头切换到战斗视角，结束战斗后恢复玩家 Pawn 视角。
  - 使用向下 Line Trace 获取战斗地面，并可结合 `SpeciesData` 中的离地偏移处理不同模型的贴地、悬浮和飞行高度。
  - 战斗阶段只刷新战斗 Actor 的位置和可见性，不在每个回合移动整个战斗舞台，降低镜头倾斜和场景位移风险。

## 系统流程图（文字版）

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

## 技术栈与引擎特性

- **引擎**：Unreal Engine 5.4，Windows，DirectX 12 / Shader Model 6 配置。
- **C++**：
  - `UBlueprintFunctionLibrary`。
  - C++ 反射读取 Blueprint Actor 的 `Speed` 属性。
  - `TArray` 有效性清理、速度排序和同速组内随机化。
- **Blueprint**：
  - `BP_TurnManager`：战斗状态、回合和队伍生命周期。
  - `BP_BattleStage`：Slot、战斗地面和战斗位置。
  - `BP_pokemon`：运行时战斗 Actor、属性、技能、生命值和阵亡状态。
  - `BP_Trainer`：训练师队伍和训练师战斗触发。
  - `BP_Pokeball`：投掷物运动与碰撞事件。
- **数据系统**：
  - Data Asset 风格的物种和技能配置。
  - 用户自定义结构体数组。
  - `SaveGame` / `SG_PlayerProgress` 存档对象。
  - `E_BattlePhase`、移动类型和技能分类枚举。
  - 当前类型和战斗阶段使用枚举建模，未引入 GameplayTags；如果扩展状态效果、抗性标签或复杂技能条件，可将对应字段迁移到 GameplayTags。
- **UI 与输入**：
  - UMG Widget Blueprint：`WBP_BattleUI`、`WBP_PartyStatus`。
  - 动态文本、血条、技能按钮、战斗日志和状态面板。
  - Enhanced Input Player Input 配置，并结合传统 Input Key 处理部分演示按键。
- **镜头与演出**：
  - `PlayerController::SetViewTargetWithBlend`。
  - Camera Shake：`BP_HitCameraShake`。
  - Timeline / Blueprint 动画式 UI 和伤害数字表现。
- **碰撞与空间定位**：
  - Projectile Collision。
  - `Line Trace Single for Objects` / World Static 地面检测。
  - Actors to Ignore 过滤当前战斗 Actor，避免射线命中自身。
- **内容资源**：
  - `SimpleDamageText` 纯 Blueprint 伤害数字资源。
  - Mannequin、ArenaAsset 和 MonsterForSurvivalGame 内容资源。

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
│  └─ SimpleDamageText/
├─ Source/
│  └─ MyPokemonDemo/
│     ├─ MyCombatLibrary.h
│     ├─ MyCombatLibrary.cpp
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

- **启动关卡**
  - 默认关卡：`/Game/scene/Demo_Map`。
  - 点击 Unreal Editor 工具栏中的 Play，在 PIE 模式运行。

- **操作方式**
  - `WASD`：移动。
  - 鼠标：控制探索镜头。
  - `Q`：投掷 Pokeball，命中野生精灵后进入战斗。
  - `E`：与附近训练师交互并进入训练师战斗。
  - 战斗中：使用 UMG 技能按钮选择行动，使用 Switch 面板切换当前精灵。
  - `B`：查看或关闭玩家当前队伍状态。

- **运行注意事项**
  - 首次打开项目时建议等待 Shader、Blueprint 和 Asset Registry 完成加载。
  - 如果修改了 `Source` 中的 C++ 文件，需要重新编译 Editor Target 后再运行。
  - 战斗需要场景中存在 `BP_TurnManager` 和 `BP_BattleStage` 实例，并正确配置战斗 Slot 和地面碰撞。
  - 使用自定义物种时，需要在 `Content/DA/PokemonSpecies` 中配置物种 Data Asset，并为队伍结构体设置对应的 `SpeciesData`、等级和技能引用。

## 视频演示

- 视频链接：

## 面试关注点

- **数据与逻辑解耦**：物种、技能和队伍配置通过 Data Asset / Struct 管理，战斗流程不依赖具体物种 Blueprint 子类。
- **运行时与持久化分离**：存档结构体负责长期数据，Runtime Actor 负责当前战斗表现，降低 Actor 生命周期变化带来的引用风险。
- **战斗状态集中管理**：Turn Manager 统一处理回合、行动顺序、切换、阵亡和结算，UMG 负责输入与展示。
- **C++ / Blueprint 边界清晰**：C++ 提供可复用的通用排序工具，Blueprint 负责战斗流程编排和内容配置。
- **生命周期清理**：战斗退出阶段清理 UI、运行时数组和无效对象，野生遭遇对象按胜利结果销毁，避免重复触发和悬空引用。
