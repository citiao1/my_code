# Unity 日系校园模拟游戏入门教程

版本：v1.0  
适合：完全没用过 Unity，但想做“日系校园模拟游戏”的新手  
目标：先做出一个能移动、能看见校园雏形、能和 NPC 互动的可玩原型。

---

## 0. 你可以怎么和我配合

以后你装好 Unity 后，我可以继续帮你做这些事：

- 帮你规划项目结构和玩法系统。
- 帮你写 Unity C# 脚本。
- 帮你检查 Console 报错。
- 帮你一步步操作 Unity：创建物体、挂脚本、调整 Inspector 参数。
- 帮你把普通方块场景慢慢替换成校园、教室、角色、UI。

你需要做的事情很少：

1. 先安装 Unity Hub 和 Unity Editor。
2. 把项目创建到电脑上的固定位置。
3. 需要我操作 Unity 时，告诉我“可以开始操作 Unity”。

推荐项目位置：

```text
C:\Users\28097\Desktop\my_code\my_code\JapaneseSchoolSim
```

---

## 1. 游戏方向：日系校园模拟

这个游戏可以先做成轻开放校园模拟：

玩家扮演一名转学生，每天在校园里移动、上课、午休、参加社团、和同学互动，慢慢解锁角色剧情。

第一版不要追求大地图和复杂系统，先做“竖切原型”。竖切原型的意思是：游戏虽然小，但包含最核心的一条完整体验。

第一版目标：

- 玩家可以控制角色在校园里走动。
- 摄像机跟随玩家。
- 校园里有简单教学楼、操场、走廊、教室入口。
- 玩家靠近 NPC 后可以按键互动。
- NPC 能显示几句对话。
- 游戏有简单时间段：早晨、上课、午休、放学、傍晚。

---

## 2. 安装 Unity

### 2.1 下载 Unity Hub

打开 Unity 官网：

```text
https://unity.com/download
```

下载并安装 Unity Hub。

Unity Hub 是管理 Unity 版本和项目的工具。你以后打开项目、安装 Unity 版本，都通过它完成。

### 2.2 安装 Unity Editor

打开 Unity Hub 后：

1. 登录 Unity 账号。
2. 进入 `Installs`。
3. 点击 `Install Editor`。
4. 推荐选择 `Unity 2022.3 LTS` 或 `Unity 6 LTS`。

如果你不知道选哪个，优先选择 `Unity 2022.3 LTS`。LTS 代表长期支持版，更稳定，教程也更多。

安装模块建议勾选：

- `Microsoft Visual Studio Community`
- `Windows Build Support`
- `Documentation` 可选

### 2.3 安装时不要急

Unity 安装比较大，下载和安装都可能花一段时间。中途如果卡住，不一定是出错，可以等几分钟。

---

## 3. 创建第一个项目

打开 Unity Hub：

1. 点击 `New project`。
2. 选择模板：`3D Core`。
3. 项目名：`JapaneseSchoolSim`。
4. 项目路径：

```text
C:\Users\28097\Desktop\my_code\my_code
```

最终项目目录应该类似：

```text
C:\Users\28097\Desktop\my_code\my_code\JapaneseSchoolSim
```

点击 `Create project`。

第一次打开项目会比较慢，等 Unity 编辑器完全打开即可。

---

## 4. 认识 Unity 界面

刚打开 Unity 会看到很多窗口。你先记住这些就够了：

| 窗口 | 用途 |
| --- | --- |
| Scene | 编辑游戏世界的地方 |
| Game | 玩家实际看到的画面 |
| Hierarchy | 当前场景里所有物体的列表 |
| Inspector | 查看和修改选中物体的属性 |
| Project | 项目文件、脚本、材质、模型都在这里 |
| Console | 报错、警告、日志都在这里 |

最常用操作：

- 鼠标左键：选择物体。
- 右键拖动：旋转 Scene 视角。
- 鼠标滚轮：缩放视角。
- 按 `W`：移动工具。
- 按 `E`：旋转工具。
- 按 `R`：缩放工具。
- 按 `Ctrl + S`：保存场景。

---

## 5. 建立项目文件夹

在 Unity 的 `Project` 窗口中，进入 `Assets`，创建这些文件夹：

```text
Assets/
  Scenes/
  Scripts/
    Player/
    Camera/
    Interaction/
    Dialogue/
    TimeSystem/
    NPC/
  Prefabs/
    Player/
    NPC/
    UI/
  Materials/
  Models/
  UI/
  Audio/
```

这样做的好处是：后面文件不会乱。游戏越做越大，项目结构越重要。

如果你不想手动创建，之后我也可以直接帮你创建。

---

## 6. 创建第一个场景

### 6.1 保存场景

1. 点击菜单 `File > Save As...`。
2. 保存到：

```text
Assets/Scenes/MainCampus.unity
```

场景名字叫 `MainCampus`，意思是主校园。

### 6.2 创建地面

在 `Hierarchy` 空白处右键：

```text
3D Object > Plane
```

把它改名为：

```text
Ground
```

在 Inspector 里设置：

```text
Position: X 0, Y 0, Z 0
Scale:    X 8, Y 1, Z 8
```

这就是第一个校园地面。

### 6.3 创建玩家

在 `Hierarchy` 空白处右键：

```text
3D Object > Capsule
```

改名：

```text
Player
```

设置：

```text
Position: X 0, Y 1, Z 0
```

这颗胶囊体先临时代表主角。以后可以换成真正的日系学生角色模型。

### 6.4 创建教学楼占位

先用 Cube 搭出校园感觉：

```text
3D Object > Cube
```

改名：

```text
SchoolBuilding_Blockout
```

设置：

```text
Position: X 0, Y 2, Z 12
Scale:    X 12, Y 4, Z 3
```

这只是占位块，目的是先确定地图布局。

### 6.5 创建操场占位

再创建一个 Cube：

```text
Name: Playground_Blockout
Position: X 8, Y 0.02, Z -6
Scale:    X 8, Y 0.05, Z 6
```

后面可以给它红色或绿色材质，当操场。

---

## 7. 添加玩家移动脚本

在 Unity 中创建脚本：

```text
Assets/Scripts/Player/PlayerController.cs
```

脚本内容：

```csharp
using UnityEngine;

[RequireComponent(typeof(CharacterController))]
public class PlayerController : MonoBehaviour
{
    [SerializeField] private float moveSpeed = 4.5f;
    [SerializeField] private float turnSpeed = 12f;
    [SerializeField] private float gravity = -20f;

    private CharacterController characterController;
    private Vector3 verticalVelocity;

    private void Awake()
    {
        characterController = GetComponent<CharacterController>();
    }

    private void Update()
    {
        float horizontal = Input.GetAxisRaw("Horizontal");
        float vertical = Input.GetAxisRaw("Vertical");

        Vector3 inputDirection = new Vector3(horizontal, 0f, vertical).normalized;

        if (inputDirection.sqrMagnitude > 0.01f)
        {
            Quaternion targetRotation = Quaternion.LookRotation(inputDirection);
            transform.rotation = Quaternion.Slerp(transform.rotation, targetRotation, turnSpeed * Time.deltaTime);
        }

        Vector3 move = inputDirection * moveSpeed;

        if (characterController.isGrounded && verticalVelocity.y < 0f)
        {
            verticalVelocity.y = -2f;
        }

        verticalVelocity.y += gravity * Time.deltaTime;
        characterController.Move((move + verticalVelocity) * Time.deltaTime);
    }
}
```

给 `Player` 添加组件：

1. 选中 `Player`。
2. 在 Inspector 点击 `Add Component`。
3. 搜索 `Character Controller` 并添加。
4. 再把 `PlayerController` 脚本拖到 Player 上。

按 Play 后，用键盘移动：

```text
W / S / A / D
```

---

## 8. 添加摄像机跟随

创建脚本：

```text
Assets/Scripts/Camera/CameraFollow.cs
```

脚本内容：

```csharp
using UnityEngine;

public class CameraFollow : MonoBehaviour
{
    [SerializeField] private Transform target;
    [SerializeField] private Vector3 offset = new Vector3(0f, 6f, -7f);
    [SerializeField] private float smoothTime = 0.12f;

    private Vector3 velocity;

    private void LateUpdate()
    {
        if (target == null)
        {
            return;
        }

        Vector3 targetPosition = target.position + offset;
        transform.position = Vector3.SmoothDamp(transform.position, targetPosition, ref velocity, smoothTime);
        transform.LookAt(target.position + Vector3.up * 1.2f);
    }
}
```

操作步骤：

1. 选中 `Main Camera`。
2. 把 `CameraFollow` 脚本拖到相机上。
3. 在 Inspector 中，把 `Player` 从 Hierarchy 拖到 `Target` 字段。
4. 设置相机位置可以先不用管，脚本会跟随玩家。

---

## 9. 创建 NPC 和互动

### 9.1 创建 NPC

在场景中创建一个 Capsule：

```text
Name: Classmate_Npc
Position: X 2, Y 1, Z 3
```

它先代表同班同学。

### 9.2 创建可互动脚本

创建脚本：

```text
Assets/Scripts/Interaction/InteractableNpc.cs
```

脚本内容：

```csharp
using UnityEngine;

public class InteractableNpc : MonoBehaviour
{
    [SerializeField] private string npcName = "同班同学";
    [SerializeField] private string[] dialogueLines;

    public string NpcName => npcName;

    public void Interact()
    {
        if (dialogueLines == null || dialogueLines.Length == 0)
        {
            Debug.Log(npcName + ": 今天也要加油哦。");
            return;
        }

        foreach (string line in dialogueLines)
        {
            Debug.Log(npcName + ": " + line);
        }
    }
}
```

把这个脚本挂到 `Classmate_Npc` 上。

在 Inspector 里设置：

```text
Npc Name: 同桌
Dialogue Lines Size: 2
Element 0: 早上好，今天也一起去教室吧。
Element 1: 听说放学后社团楼会开放。
```

### 9.3 创建玩家互动检测脚本

创建脚本：

```text
Assets/Scripts/Interaction/PlayerInteraction.cs
```

脚本内容：

```csharp
using UnityEngine;

public class PlayerInteraction : MonoBehaviour
{
    [SerializeField] private float interactionDistance = 2.2f;
    [SerializeField] private LayerMask interactableLayers;

    private void Update()
    {
        if (Input.GetKeyDown(KeyCode.E))
        {
            TryInteract();
        }
    }

    private void TryInteract()
    {
        Collider[] hits = Physics.OverlapSphere(transform.position, interactionDistance, interactableLayers);

        if (hits.Length == 0)
        {
            Debug.Log("附近没有可以互动的人。");
            return;
        }

        InteractableNpc npc = hits[0].GetComponent<InteractableNpc>();
        if (npc != null)
        {
            npc.Interact();
        }
    }
}
```

操作步骤：

1. 给 NPC 设置 Layer，例如新建 `NPC` Layer。
2. 把 `Classmate_Npc` 的 Layer 改成 `NPC`。
3. 把 `PlayerInteraction` 脚本挂到 `Player` 上。
4. 在 `Interactable Layers` 里勾选 `NPC`。

进入 Play 模式后，靠近 NPC 按 `E`，Console 会显示对话。

---

## 10. 添加校园时间系统

创建脚本：

```text
Assets/Scripts/TimeSystem/SchoolTimeManager.cs
```

脚本内容：

```csharp
using UnityEngine;

public enum SchoolTimePeriod
{
    Morning,
    ClassTime,
    LunchBreak,
    AfterSchool,
    Evening
}

public class SchoolTimeManager : MonoBehaviour
{
    [SerializeField] private SchoolTimePeriod currentPeriod = SchoolTimePeriod.Morning;
    [SerializeField] private float secondsPerPeriod = 60f;

    private float timer;

    public SchoolTimePeriod CurrentPeriod => currentPeriod;

    private void Update()
    {
        timer += Time.deltaTime;

        if (timer >= secondsPerPeriod)
        {
            timer = 0f;
            AdvancePeriod();
        }
    }

    private void AdvancePeriod()
    {
        int nextValue = ((int)currentPeriod + 1) % System.Enum.GetValues(typeof(SchoolTimePeriod)).Length;
        currentPeriod = (SchoolTimePeriod)nextValue;
        Debug.Log("当前时间段：" + currentPeriod);
    }
}
```

使用方法：

1. 在 Hierarchy 中创建空物体：`GameManager`。
2. 把 `SchoolTimeManager` 挂到 `GameManager` 上。
3. Play 后每隔 60 秒会切换时间段。

这就是日程系统的最小版本。

---

## 11. 日系校园第一版地图规划

先用方块搭地图，不要急着找漂亮模型。

建议第一版地图：

```text
校门
  |
主路
  |
教学楼入口 ---- 操场
  |
走廊 ---- 教室
  |
社团楼 ---- 天台
```

第一版场景物体：

- `SchoolGate_Blockout`：校门
- `MainRoad_Blockout`：主路
- `SchoolBuilding_Blockout`：教学楼
- `Classroom_Blockout`：教室
- `Rooftop_Blockout`：天台
- `ClubRoom_Blockout`：社团活动室
- `Playground_Blockout`：操场

先完成空间布局，再逐步替换资产。

---

## 12. 推荐第一版角色设定

为了让游戏更有日系校园味，可以先设计 4 个 NPC：

| 角色 | 定位 | 第一版互动 |
| --- | --- | --- |
| 班长 | 认真可靠 | 提醒上课时间，介绍学校 |
| 同桌 | 亲近开朗 | 午休聊天，触发校园传闻 |
| 老师 | 规则引导 | 检查是否按时到教室 |
| 社团前辈 | 目标引导 | 邀请玩家加入社团 |

不要一开始写太多剧情。第一版每个 NPC 2 到 5 句台词就够。

---

## 13. 第一周学习路线

### 第 1 天：安装和创建项目

目标：能打开 Unity，能创建 `JapaneseSchoolSim` 项目。

完成标准：

- Unity Editor 能正常打开。
- 项目路径固定。
- 能看到 Scene、Game、Hierarchy、Inspector、Project、Console。

### 第 2 天：搭建第一个场景

目标：创建地面、玩家、教学楼占位、操场占位。

完成标准：

- 场景保存为 `MainCampus.unity`。
- Play 模式能正常进入。

### 第 3 天：玩家移动

目标：让玩家胶囊体能用 WASD 移动。

完成标准：

- Player 有 `CharacterController`。
- Player 有 `PlayerController`。
- 按 Play 后可以移动。

### 第 4 天：摄像机跟随

目标：让相机跟着玩家移动。

完成标准：

- Main Camera 有 `CameraFollow`。
- Target 指向 Player。
- 移动时画面不会丢失玩家。

### 第 5 天：NPC 互动

目标：靠近 NPC 按 E，Console 出现对话。

完成标准：

- NPC 有 `InteractableNpc`。
- Player 有 `PlayerInteraction`。
- NPC Layer 设置正确。

### 第 6 天：时间系统

目标：游戏自动切换校园时间段。

完成标准：

- GameManager 有 `SchoolTimeManager`。
- Console 能显示时间段变化。

### 第 7 天：整理和复盘

目标：把项目整理好，准备进入 UI 和正式对话框。

完成标准：

- 文件夹结构清楚。
- 场景能正常运行。
- 没有红色 Console 报错。

---

## 14. 常见问题

### 14.1 Play 后玩家不动

检查：

- `PlayerController` 是否挂在 Player 上。
- Player 是否有 `CharacterController`。
- Console 是否有红色报错。
- 游戏窗口是否被点击激活。

### 14.2 相机没有跟随

检查：

- `CameraFollow` 是否挂在 Main Camera 上。
- Target 字段是否拖入 Player。
- Player 是否在场景中。

### 14.3 按 E 没有互动

检查：

- NPC 是否挂了 `InteractableNpc`。
- Player 是否挂了 `PlayerInteraction`。
- NPC 是否设置到了 `NPC` Layer。
- PlayerInteraction 的 `Interactable Layers` 是否勾选了 NPC。
- 玩家是否离 NPC 足够近。

### 14.4 Console 出现红色错误怎么办

不要慌。红色报错通常比想象中好解决。

你可以把报错截图或文字发给我，我可以帮你判断是哪一个脚本、哪一行、怎么改。

---

## 15. 下一阶段可以做什么

完成第一版原型后，下一阶段可以加：

- 正式对话框 UI。
- 任务系统。
- 好感度系统。
- 上课触发器。
- 教室、天台、社团楼传送点。
- NPC 简单巡逻。
- 手机菜单。
- 存档系统。
- 日系校园美术资源替换。

推荐第二阶段目标：

```text
玩家早上进入校园 -> 和班长对话 -> 去教室上课 -> 午休和同桌聊天 -> 放学去社团楼
```

这条流程短，但已经像一个真正的校园模拟游戏。

---

## 16. 你下一步只要做什么

现在你只需要做一件事：

安装 Unity Hub，并创建 `JapaneseSchoolSim` 项目。

项目创建好之后告诉我：

```text
Unity 项目已经创建好了，路径是 C:\Users\28097\Desktop\my_code\my_code\JapaneseSchoolSim
```

然后我就可以继续帮你创建脚本、整理目录，并一步一步把日系校园原型做起来。
