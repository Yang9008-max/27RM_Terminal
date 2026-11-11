# 2027 RMUL 开发的 Git 工作流

2026-11-11，用户明确要求：以后每一步沿用版本管理流程，在重大改动前先保存 Git 版本，必要时新开分支。

## 初始开发基线

- 仓库：`D:\MyTrain\projects\27RM_Terminal`。
- 上游：`https://github.com/ClearWei/RM26_Client.git`。
- 上游起点：`e291f2f65f5a5924509e4e2fe1b148ff9d4b24af`。
- 开发分支：`rmul2027/develop`。
- 基线标签：`rmul2027-baseline-20261111`（附注标签，以 `git show` 查看具体提交）。
- 保存范围：原有客户端和模拟器源码、Windows 构建与启动脚本、依赖说明、2027 RMUL 调研及字段审计、项目协作约定。
- 适配状态：业务源码仍为上游版本，尚未实施 2027 通信或 RMUL 界面改造。
- 基线验证：2026-11-11 在本机已有 Release 构建上运行 `ctest --preset windows-ucrt64`，27/27 项通过；本次未修改业务源码。

SDK、编译产物、虚拟环境和本机运行配置继续排除在 Git 外。Windows CMake 本机预设的可恢复副本保存在 `tools/windows/CMakeUserPresets.example.json`，恢复步骤见 [Windows 环境说明](windows-setup.md)。Git 标签保存工程文件，运行环境版本见该说明。

官方 PDF、知识库和完整答疑快照位于 `D:\MyTrain\robomaster\文档`，属于独立资料目录，不包含在本仓库的 Git 标签中。资料来源和 PDF 哈希见 [调研索引](../research/rmul2027/README.md)。官方答疑入口：`D:\MyTrain\robomaster\文档\比赛规则\2027赛季\官方答疑\README.md`。

## 每一步的执行顺序

1. 检查当前分支、工作区与已有差异，确定本阶段范围。
2. 在重大改动之前，把应保存的现有文件提交为检查点；若工作区已干净，复用当前提交。
3. 创建新的附注标签。使用 `rmul2027-pre-<阶段编号>-<主题>`，例如第一阶段可用 `rmul2027-pre-01-2027-data`。同名标签存在时核对后复用或递增名称，不强制覆盖。
4. 根据隔离需要开功能分支，例如 `rmul2027/01-2027-data`。一次只推进明确的阶段，开始前记录验收方法。
5. 完成实现和针对性验证；记录结果及未验证事项，提交阶段成果。验收通过的里程碑可另打 `rmul2027-step-<阶段编号>-done` 标签。

本地提交、标签和合理的开发分支已获用户授权。远端发布按用户后续要求处理。

## 查看与安全回查

```powershell
git status --short --branch
git log --oneline --decorate -10
git tag --list 'rmul2027-*'
git show --stat rmul2027-baseline-20261111
git diff rmul2027-baseline-20261111 -- src sim
```

需要单独查看基线时，可在项目旁建立独立 worktree，原开发工作区不受影响：

```powershell
git worktree add --detach ../27RM_Terminal-baseline rmul2027-baseline-20261111
```

先保存当前改动再切换工作状态；不使用强制覆盖标签或清空工作区的方式回退。
