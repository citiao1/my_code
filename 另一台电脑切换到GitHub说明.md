# 另一台电脑把 VS Code / Git 从 Gitee 切到 GitHub

目标：让另一台电脑上的 `my_code` 仓库以后默认推送到 GitHub：

```text
https://github.com/citiao1/my_code.git
```

原来的 Gitee 远端可以先保留为 `gitee`，避免误删配置；以后普通 `git push` 会推到 GitHub。

## 1. 打开正确的仓库目录

在 VS Code 里打开真正的仓库目录，不要只打开上一级目录。

常见路径类似：

```powershell
cd D:\my_code\my_code
```

如果不确定位置，可以在 VS Code 终端里运行：

```powershell
git status -sb
```

如果看到 `fatal: not a git repository`，说明当前目录不是仓库，需要进入包含 `.git` 的那一层目录。

## 2. 查看当前远端

```powershell
git remote -v
```

如果看到类似：

```text
origin  https://gitee.com/fsdd/my_code.git
```

说明当前 `origin` 还指向 Gitee。

## 3. 把 Gitee 改名保留，再把 GitHub 设为 origin

如果当前只有 `origin`，并且它是 Gitee，执行：

```powershell
git remote rename origin gitee
git remote add origin https://github.com/citiao1/my_code.git
```

如果已经有 `origin`，但你只想直接改成 GitHub，也可以执行：

```powershell
git remote set-url origin https://github.com/citiao1/my_code.git
```

推荐第一种，因为它会把旧 Gitee 保留成 `gitee`，以后要查旧仓库也方便。

## 4. 拉取 GitHub 分支信息

```powershell
git fetch origin main
```

如果网络不好，先设置代理再执行。代理配置见下面第 7 节。

## 5. 设置 main 默认跟踪 GitHub

```powershell
git branch --set-upstream-to=origin/main main
```

如果提示本地没有 `main`，先看当前分支：

```powershell
git branch --show-current
```

如果当前分支就是 `master` 或别的名字，需要按实际分支名替换命令里的 `main`。

## 6. 验证结果

```powershell
git remote -v
git status -sb
```

正确结果应该类似：

```text
origin  https://github.com/citiao1/my_code.git (fetch)
origin  https://github.com/citiao1/my_code.git (push)
gitee   https://gitee.com/fsdd/my_code.git (fetch)
gitee   https://gitee.com/fsdd/my_code.git (push)

## main...origin/main
```

看到 `main...origin/main` 就说明以后普通提交同步会走 GitHub。

## 7. 配置代理 7897

如果另一台电脑也使用 `127.0.0.1:7897` 代理，可以设置 Git 全局代理：

```powershell
git config --global http.proxy socks5://127.0.0.1:7897
git config --global https.proxy socks5://127.0.0.1:7897
```

确认代理：

```powershell
git config --global --get http.proxy
git config --global --get https.proxy
```

应该输出：

```text
socks5://127.0.0.1:7897
socks5://127.0.0.1:7897
```

如果那台电脑的代理软件只提供 HTTP 代理，不提供 SOCKS5，可以改用：

```powershell
git config --global http.proxy http://127.0.0.1:7897
git config --global https.proxy http://127.0.0.1:7897
```

## 8. 配置 VS Code 自身代理

打开 VS Code 设置 JSON：

```text
Preferences: Open User Settings (JSON)
```

加入或修改：

```json
{
  "http.proxy": "http://127.0.0.1:7897",
  "http.proxySupport": "on"
}
```

如果原本 JSON 里已经有很多设置，不要新建第二个最外层 `{}`，只需要把这两项加到已有对象里，并注意逗号。

改完后重载 VS Code：

```text
Developer: Reload Window
```

## 9. 以后怎么用

以后正常在 VS Code 里提交、同步即可。

普通命令：

```powershell
git add .
git commit -m "你的提交说明"
git push
```

只要 `git status -sb` 显示的是：

```text
## main...origin/main
```

就会推到 GitHub，不会推到 Gitee。

只有手动执行下面这种命令时，才会推到 Gitee：

```powershell
git push gitee main
```

## 10. 不想保留 Gitee 的话

确认 GitHub 已经能正常推送后，可以彻底删除 Gitee 远端：

```powershell
git remote remove gitee
```

删除后再看：

```powershell
git remote -v
```

应该只剩：

```text
origin  https://github.com/citiao1/my_code.git
```

