本程序给你一个普通的终端窗口，也就是跑在 BlackBerry BB10 上的 `sh`，并提供命令行
OpenSSH 套件工具。这些工具只是你在任何 Linux/BSD 系统上所用工具的移植版，所以它们
提供（几乎）相同的功能、接受相同的参数。

免责声明：这些工具会做加密/解密，在某些国家可能属于违法行为，遵守法律是你自己的
责任。

这是一个实验性作品（Qt + BlackBerry BB10 原生开发），它给了我这样一个 *nix 用户
非常想念的普通 shell 终端窗口。目前只提供了一些基本命令，比如 `ls`、`echo`、`uname`
（你可以再翻翻还有什么）。软件仍在"进行中"，按"现状"提供，欢迎任何意见！

这是 QTermWidget（http://qtermwidget.sourceforge.net/）到 BB10 OS 的移植 ——
非常感谢社区带来的 qtermwidget！

功能列表：
    -- 终端窗口，自带 ssh、scp、sftp、ssh-keygen（随包附带，位于 app/native/bin，依赖库在 app/native/lib）
    -- 用于 Alt+ 和 Ctrl+ 组合键的软键盘
    -- 中文输入法：终端内运行的拼音输入法（Alt+Enter），8230 个
       汉字（《通用规范汉字表》的 8105 字加 GB2312 其余部分）与词组，候选词随用随学
    -- SSH 自带 30 秒心跳保活；掉线后屏幕顶部出现一键重连横条
    -- ssh 隧道（应用最小化时也能工作）
    -- SSH over HTTP 代理（同时装在 Work 与 Personal 空间时可用）
    -- SSH 在 BlackBerry Balance 的 Work 空间安装时也能工作

更多关于本项目的信息见 http://bgmot.com/BGShellPlus

不要在评论（Reviews）区提问，那里不会有人回答。请发邮件到 support@bgmot.com

发布说明：

- 0.0.0.5 —— 首个发布版

- 0.0.0.6
    -- 启动时读取 ~/.profile

- 0.0.0.600 —— 移植到 BB 10.0.9

- 0.0.0.604
    -- 字号适配 BB10.0.9.1103

- 0.0.0.605
    -- 修正按钮响应
    -- 修正键盘类型（Enter 键现在显示为 Enter）

- 0.0.0.606
    -- 现在可以生成任意 CTRL+ 代码：先点 Ctrl+ 软键，再在虚拟键盘上按所需按键。
    -- 这个 shell 现在能在后台模式（最小化时）运行 —— 完美的用例是用 ssh 隧道
       安全地为你的 SMTP 和 IMAP 流量做隧道。你只需在终端窗口里建好所需的隧道，例如：
       ssh -p 2222 user@host.domain.eg -L 2225:your.mailserver:25 -L 2143:your.mailserver:143 等等。
       然后在 BB10 邮件账户设置里，SMTP 用 127.0.0.1 的 2225 端口，
       IMAP 用 127.0.0.1 的 2143 端口。当然你也可以为任何流量建隧道。
       这个好主意来自 Nick W.，谢谢他。

- 0.0.0.607
    -- 新增使用 HTTP 代理的 SSH 连接。装在 BlackBerry Balance 的 Personal 空间
       时，它读取当前连接的代理设置；装在 Work 空间时，它读取 BES 下发的代理设置。
       只需下拉并在 Settings 里勾选 Use HTTP Proxy。BETA 功能，非常欢迎反馈，
       请发邮件到 bgmot@tm-k.com。

- 0.0.0.608
    -- 修复 Work 容器里的一个小 bug
    -- 新增控制台字号选项

- 0.0.0.700
    -- 字体管理的小修

- 0.0.0.6060（仅限带物理键盘的设备）
    -- 移植到带物理键盘的设备

- 0.0.0.7070（仅限带物理键盘的设备）
    -- 字体管理
    -- SSH 支持 HTTP 代理（可安装到 BlackBerry Balance 的 Work 容器）
    -- SYM 字符虚拟键盘

- 0.0.1.1
    -- 所有类型设备统一为单一版本
    -- 现在支持 BlackBerry Passport
    -- 可以在任意目录下运行 ssh、scp 和 sftp（修复了 PATH）
    -- 已知 bug：Passport 上的 Shift 键会卡住 —— 需要再按一次才解除

- 0.0.1.2
    -- 菜单栏移到屏幕底部（太多人要求了）
    -- 修复最小化时的运行（对隧道有用）

- 0.0.2.0
    -- 新增 ssh-keygen、ssh-agent、ssh-add 工具
    -- 控制台字号选择更灵活

- 0.0.3.0
    -- 中文拼音输入法：Alt+Enter（或 Ctrl+Space）切换，
       Term50 的引擎和词库跑在 widget 内部 —— shell 拿到的是已提交的
       UTF-8，不涉及系统输入面板（见下文）
    -- 内置从 kit 的 Maple Mono NF CN 裁剪出的 CJK 字体子集，因此在
       未安装任何字体的原厂设备上也能显示中文
    -- 物理 SYM 键打开 Term50 的符号网格，而不是旧的 vk.png 图片；
       Alt+Space 重新变回 Tab（Alt+Enter 接管了 Tab）
    -- 空格不再取第一个候选：组字时把已输入的字母当英文上屏
       （`Hello` -> `Hello `），回车提交高亮候选 —— 打英文不用切中/英
    -- - , [ = . ] 只在候选超过一页时才翻页；单页时恢复成普通标点，
       打英文句子不会丢逗号

- 0.0.3.1
    -- 空格与回车对调：**空格**取当前高亮候选（原先回车），**回车**把
       已输入的字母当英文上屏（原先空格）
    -- 回车不再补空格、不发换行；英文上屏后需再按一次回车才执行命令行
    -- 帮助浮层与中英文 README 同步更新

- 0.0.3.3
    -- 触屏手势：长按选中整行并弹出复制/粘贴菜单（长按后拖动可扩选区），
       两指上下滑翻看回滚缓冲，两指捏合改字号并记住
    -- 首次连接未知主机不再卡在 fingerprint 提问：ssh 统一带
       StrictHostKeyChecking=accept-new
    -- 按键说明改成可滚动窗口，不再被消息框底部截断

- 0.0.3.4
    -- SSH 保活：ServerAliveInterval=30 / ServerAliveCountMax=3 /
       TCPKeepAlive=yes，手机熄屏后不会再安静一分钟就掉线
    -- 掉线时顶部出现「连接已断开」横条，点「重连」用原命令直接重连
       （存过密码的会重新应答）；按任意键横条消失
    -- 粘贴含换行的内容会先确认：原样粘贴（逐行执行）/ 合并成一行 /
       取消 —— 默认按钮是取消，误敲回车不会执行任何东西
    -- 回滚缓冲行数可设（500 / 1000 / 2000 / 5000 / 10000），启动即生效，
       当前会话也会立即调整

- 0.0.3.2
    -- 新增常用命令 / SSH 主机面板（下滑菜单 → Cmds / SSH）
    -- 命令可运行、可仅插入命令行；启动时自带 ls -l / df -h / ps 等示例
    -- SSH 主机可保存 用户 / 主机 / 端口 / 私钥 / 密码；密码在 pty 层
       自动应答密码提示（只应答一次，30 秒超时）
    -- 密码仅做 XOR+Base64 混淆，不是加密；推荐改用密钥登录

## 常用命令与 SSH 主机（下滑菜单 → Cmds / SSH）

下滑菜单现在有两个按钮：**设置** / **命令 / SSH**。
界面文字（菜单、设置窗口、按键说明）已全部汉化；赞赏按钮及其内购代码已移除。

### 命令

* 点一行选中，再点 **运行**（命令 + 回车）或 **仅插入**（只把文本放到当前命令行，
  还能继续改）。双击一行进入编辑，**新增** / **删除** 管理列表。
* 首次启动自带几条示例（`ls -l`、`df -h`、`ps`、`ifconfig`、`uname -a`），
  不要的删掉即可。
* 命令是直接写进 pty 的，所以和手打完全一样：有回显、进 shell 历史。

### SSH 主机

* 保存 名称 / 用户 / 主机 / 端口 / 私钥 / 密码；点 **连接** 后等价于输入
  `ssh -o StrictHostKeyChecking=accept-new -p <端口> -i <私钥> 用户@主机`
  并回车。这个参数专门对付首次连接时的
  `Are you sure you want to continue connecting (yes/no)?`：新主机会自动接受并
  写入 `data/.ssh/known_hosts`，而**指纹变了**的主机依然会被拒绝。
* **密码**：保存后会监听 pty 输出，出现 `password:`（或“密码”）提示时自动
  输入一次，30 秒内没有提示就放弃；之后清出内存。

### 密码保存的边界（请先看这段）

BB10 没有给第三方应用开放的系统钥匙串，所以密码只能存在应用私有目录的
QSettings 里（键 `SshHosts`），并且只做了 XOR+Base64 混淆 —— **这不是加密**，
有 root、能读备份或能拿到应用数据目录的人都能还原。

因此更推荐用密钥：

```sh
ssh-keygen -t ed25519 -f data/.ssh/id_ed25519      # 随包附带
```

ssh-agent / ssh-add 没有随包附带（包里只有 ssh、scp、sftp、ssh-keygen）；
不设口令的密钥可以直接用，设了口令就要每次手输。

然后在主机条目里填私钥路径并把密码留空；需要密码时 ssh 会正常向你提问。

### 自带的 ssh 套件

ssh / scp / sftp / ssh-keygen 随包附带，安装在应用目录：

```
app/native/bin/ssh  scp  sftp  ssh-keygen      约 6.9 MB
app/native/lib/libcrypto.so.3  libz.so.1      约 4.0 MB
```

* `main.cpp` 启动时把 `app/native/bin` 加进 PATH，并给这四个二进制补上执行位；
  `LD_LIBRARY_PATH` 原本就包含 `app/native/lib`。
* BB10 系统只有 `libcrypto.so.2` / `libz.so.2`，与这里的 `.so.3` / `.so.1` 名字不同，
  不会顶替系统库；而 openssh 要的正是这两个名字，所以必须随包带。
* 代价：bar 从 6.4 MB 涨到 11.3 MB。不需要的话，从 `bar-descriptor.xml` 删掉那几条
  `sshbin` 的 asset 重新打包即可。

## 触屏手势

终端自己处理手指输入，短按、长按、两指都有明确分工：

| 手势 | 作用 |
|---|---|
| 轻点 | 把键盘焦点还给终端 |
| 长按（0.45 秒） | 选中手指下面那一整行，并弹出小菜单：**复制本行** / **复制选中文字** / **复制可见屏幕** / **粘贴** |
| 长按后同方向拖动 | 把选区扩到任意范围，菜单里的“复制选中文字”就复制这一段 |
| 两指上下滑 | 按行高逐行翻看回滚缓冲 |
| 两指捏合 / 张开 | 字号 10–60 像素，立刻生效并写进 QSettings |

复制时会去掉屏幕右侧的补位空格，粘到编辑器或命令行里不会拖一串空白。捏合改的
就是设置里那个 `FontSize`，两边不会打架。

## 按键绑定（Term50 风格，无软键条）

原来屏幕上那排按钮（Ctrl+ / Tab / < / > / ^ / v / Esc）已经去掉：终端现在使用
虚拟键盘上方完整的屏幕高度，按键来自物理键盘，外加一套从 Term50 的 metamode
方案移植来的粘滞"CTRL 层"。

### Shift 就是 Ctrl 键

BB10 手机键盘（Q10 / Q30 / Passport / Classic）**没有方向键** —— 印着向上箭头的
那个键是 **Shift** 键，所以它就是要当 Ctrl 用的键：

* **按住右边的 Shift + 某键** -> Ctrl+某键，真正的修饰键行为：
  `c`=Ctrl+C、`d`=Ctrl+D、`z`=Ctrl+Z、`l`=Ctrl+L、`[`=Ctrl+[ ……
  （A-Z 和 `[ \ ] ^ _` 都映射为 `key - 0x40`；方向键/Home/End/Delete/PgUp/PgDn
  以 xterm 形式 `ESC[1;5X` 发送。）
* **双击右边的 Shift**（500 ms 内，且两次之间没有按过别的键）-> 切换粘滞 CTRL 层；
  激活时右下角出现绿色的 `CTRL` 徽标。
* **左 Shift 仍是普通 Shift** —— 它永远不会变成 Ctrl，因此大写输入和 BB10 的
  shift/SYM 变通方案不受影响。连续打大写（`Shift`、`A`、`Shift`、`B` ……）不会
  误触发图层：中间只要按了别的键，待定的双击就被取消。
* 两个 Shift 键都以 `Qt::Key_Shift` 到达；左右靠 `nativeVirtualKey()` /
  `nativeScanCode()` 里携带的 QNX keycode（`KEYCODE_RIGHT_SHIFT`）区分。两者
  都是 0（不带 keycode）时左右无法区分，此时双击任意一个 Shift 都算。如果某台设备的
  移植上报的东西不对，就激活图层并按 `?`：帮助浮层会打印最近一次 Shift 的
  `native`/`scan` keycode，方便把映射固定下来。

CTRL 层内部：

| 按键 | 动作 |
|-----|--------|
| `e` | Esc |
| `s` | Ctrl+L（重刷屏幕） |
| `v` | 粘贴剪贴板 |
| `i` / `o` / `z` | 字体变大 / 变小 / 重置（保存到 QSettings） |
| `r` | 从 QSettings 重新读取设置 |
| `?` | 键位帮助浮层 |
| `h` `j` `k` `l` | 左 / 下 / 上 / 右，留在本层 |
| 方向键 | 继续移动，留在本层（外接键盘） |
| 其他任意键 | Ctrl+&lt;该键&gt; 并离开本层 |

### SYM：Term50 的 symmenu 浮层

物理 **SYM** 键弹出 Term50 的 `main_symmenu` 网格（见 `symbolpanel.cpp`）—— 就是
Term50 的 `DEFAULT_SYMMENU_ENTRIES` / `DEFAULT_SYMMENU_ROW_LENS {10, 9}` 定义的
那 19 个符号、同样两行：

```
 ~    `    {    }    [    ]    <    >    ^    %
 q    w    e    r    t    y    u    i    o    p

 =    -    *    /    \    |    &    '    "
 a    s    d    f    g    h    j    k    l
```

每个格子在角上用绿色显示该符号以及选中它的按键，与 Term50 的渲染完全一致。插入
符号有两种方式：

* **点按该格子**（Q30 / Passport / Classic 触摸屏），或
* **按对应按键** —— 网格弹出时键盘归它所有。

再按 SYM 或 Esc 关闭；按其他任意键也会关闭并吞掉该键，因此误触的按键绝不会漏进
shell。网格会随旋转重新布局，且从不改变终端尺寸（不像 Term50 那样重刷屏幕）。

SYM 通过它的 QNX keycode `0xF0D3`（`KEYCODE_BB_SYM_KEY`，Term50 的
`src/terminal.h`）经 `nativeVirtualKey()`/`nativeScanCode()` 识别；把 SYM 上报为
裸 Escape 的固件也能靠这个回退继续工作，因为手机本身没有 Esc 键（Esc 是 CTRL 层
里的 `e`）。这替换了旧的 `myvk` `vk.png` 图片 —— 那张图只是"显示"符号，还得按个
键才能关掉。

### 中文输入法：终端内的拼音输入

BB10 自带的输入面板对终端没用（pty 需要原始 UTF-8，而 BB10 上的 Qt4 没有能让
全屏 widget 驱动的输入法插件），所以输入法跑在 widget 内部，和 Term50 完全一样：

* **Alt+Enter**（或 **Ctrl+Space**）切换 中文 / EN。开启时左上角有绿色 **中**
  徽标，状态跨重启保留。
* **a-z** 输入拼音，**空格** 提交高亮的候选，**1-8** 取对应编号的候选，
  **- , <** 与 **= . >** 翻页 —— 但只在候选真的超过一页时才翻。只有一页时它们
  就是普通标点：先把已输入的字母上屏，再把 `,` 交给 shell，所以
  `hello, world` 的逗号不会丢。**Esc** 丢弃组字不发任何东西。
* **回车不取候选**：组字时把已输入的字母原样当英文上屏（`Hello` -> `Hello`），
  所以打英文词不用切 中/EN。回车不补空格、也不发换行 —— 组字结束后再按一次
  回车（此时已无组字）才是普通的执行命令行。没在组字时回车照旧就是回车。
  引擎用不了的键（标点、功能键）同样先上屏已输入的字母再放行，
  所以 `Hello!` 送出的就是 `Hello!`。
* 屏幕底边的候选条显示当前页：

  ```
  [中]  nihao   1 你好   2 你   3 尼   4 泥      1/1  < >
  ```

  每个候选都可以点按，所以只靠手机键盘就够用 —— 不需要外接键盘。

  在 **Q20 / Q10 / Q5 / Classic**（没有触摸屏，只有触控板）上，触控板驱动候选条：
  左右移动绿色框从一个候选到下一个，上下翻页，触控板单击提交框中的候选。外接键盘
  的方向键效果相同。步进按按键重复的节奏限速（80 ms），因为触控板每像素就发一个
  事件。没有组字进行时，触控板保持原来的职责，向 shell 发送方向键。

  词候选占两到三个格子，所以一页只显示放得下的那些；该页其余候选仍可用数字键和
  翻页键取到。
* 支持整词，不是一次一个字：`nihao` -> 你好，`zhongwen` -> 中文，
  `xianzai` -> 现在，未收录的词会退化为逐音节输入（`woaini` -> 我爱你）。
* 你选过的候选会被记住（`$HOME/.term/cjk_ime_freq`），所以下次你最常用的字排
  在最前 —— 内置顺序也按词频排过（`tools/order-ime-dict.py`），否则打 `shi`
  会以 世 事 仕 似 开头而不是 是。

引擎是 Term50 的 `cjk_ime.c` / `cjk_ime_dict.inc`，原样拷入（只给头文件加了
`extern "C"` 以便 C++ 调用）—— 8230 个单字加约 2.8 万个词组。

8105 是《通用规范汉字表》的全部汉字。GB2312 —— 老手机和 PC 能产出的字符集 ——
有 125 个字被那张表丢掉了，主要是人们确实会打的偏旁（氵 扌 艹 讠 纟 钅 饣 辶 阝
灬 忄 疒 礻 衤）和少数生僻字（磺 矽 捱 搿 渖 飚 齄）。`tools/add-gb2312-chars.py`
用 `tools/cache` 里 pinyin-data 转储的读音把它们加进去，因此 6763 个 GB2312 汉字
现在全部可打，且每一个都有字形。

已知缺口：8105 字里有 273 个是 BMP 之外的繁体/生僻字（Ext-A、Ext-B/C，例如
㑇 䁖 𠅤）—— Maple Mono 完全没有这些区块的字形，所以从候选条里选中会写入一个
显示成方框的字符。它们在各自读音里排在最后，也没有任何词组用到它们。渲染需要设备
原厂字体没有的字形，因此 `main.cpp` 在启动时注册内置的 `assets/bgscjk.ttf`
（kit 的 Maple Mono NF CN 覆盖本词库的子集），并把该字体族用于终端：中文正好占
两格宽，正如 `Screen.cpp` 所假设的。候选条的调整在 `imepanel.cpp`，按键映射在
`TerminalDisplay::handleImeKey`。

引擎是纯 C、不含 Qt，因此它的行为在构建主机上测试：`tools/test-cjk-ime.sh` 用主机
编译器编译 `cjk_ime.c`，检查词组、候选顺序、翻页、提交以及必须继续送达 shell 的
按键（21 项检查）。改动词库后请跑一遍 —— `tools/order-ime-dict.py` 和
`tools/gen-ime-data.py`（字体子集）是另外两个值得重跑的工具。

widget 层还有两个主机端检查，同样不需要 NDK：
`tests/run-shift-layer-check.sh`（左 Shift 仍是 Shift、Ctrl 层只来自右 Shift）和
`tests/run-ime-page-check.sh`（`, . - [ ] =` 什么时候翻页、什么时候就是普通标点）。
两者都从 `TerminalDisplay.cpp` 原样抽出代码来编译，不会和出货代码走样。

其他组合键：**Alt+Enter = 中文/EN**、**Ctrl+Space = 同上**、
**Alt+Space = Tab**、**Shift+Alt+键 = Ctrl+键**，双击物理右方向键也会切换该层
（仅外接键盘 —— 手机没有方向键）。Term50 把 `ime_toggle` 放在 Alt+Space；这里
它移到了 Alt+Enter，Alt+Space 回到 Tab，因为手机键盘没有 Tab 键，而补全更重要。

Term50 里没有 BBShell 对应物的绑定被有意省略：`tab_new/tab_next/tab_prev/tab_close`
（单会话应用）、`url_pick`、`reload_config`（没有 `.term.lua`；`r` 改为重读
QSettings），以及 Lua/termctl 相关接口。

可调项：`TerminalDisplay.cpp` 中按键图层代码块顶部的 `META_DOUBLE_TAP_MS` 和
`META_DEFAULT_FONT_SIZE`。
