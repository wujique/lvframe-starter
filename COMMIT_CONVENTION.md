# Git Commit 规范

基于 [Conventional Commits](https://www.conventionalcommits.org/) 规范。

## 格式

```
<type>(<scope>): <subject>

[body]

[footer]
```

### type（必填）

| type | 说明 |
|------|------|
| `feat` | 新功能 |
| `fix` | Bug 修复 |
| `refactor` | 重构（不新增功能，不修复 bug） |
| `style` | 代码格式调整（空格、缩进等，不影响逻辑） |
| `docs` | 文档变更 |
| `chore` | 构建脚本、工具配置、依赖更新等杂项 |
| `perf` | 性能优化 |
| `test` | 测试相关 |
| `revert` | 回滚某次提交 |

### scope（可选）

指明影响范围，使用以下约定值：

| scope | 说明 |
|-------|------|
| `lvframe` | lvframe 框架核心代码 |
| `platform` | 平台适配层（通用） |
| `platform/sdl` | SDL 平台实现 |
| `platform/rk3506` | RK3506 Linux 平台实现 |
| `platform/rtos` | RTOS 平台实现 |
| `box86` | box86 应用工程 |
| `build` | CMake 构建系统 |
| `docs` | 文档 |

### subject（必填）

- 使用中文或英文均可，团队统一即可
- 动词开头，简明描述做了什么
- 不超过 72 个字符
- 结尾不加句号

### body（可选）

- 说明"为什么"这样改，而不是"改了什么"
- 与 subject 之间空一行

### footer（可选）

- 关联 issue：`Closes #123`
- 破坏性变更：`BREAKING CHANGE: <描述>`

---

## 示例

```
feat(lvframe): 新增页面缓存 FIFO 淘汰策略

fix(platform/sdl): 修复 SDL flush 回调未调用 lv_display_flush_ready 的问题

refactor(lvframe): 适配 LVGL v9.4 API

docs: 新增项目结构规划文档 PROJECT_STRUCTURE.md

chore(build): 添加顶层 CMakeLists.txt 和 box86 工程构建配置

feat(box86): 新增首页页面骨架

fix(lvframe): 修复 event_bus 在设备移除时的野指针问题

BREAKING CHANGE: lv_task_t 已替换为 lv_timer_t，需同步更新调用方
```

---

## 分支命名约定

| 分支 | 用途 |
|------|------|
| `main` | 稳定版本，只接受 PR 合入 |
| `dev` | 日常开发主分支 |
| `feat/<name>` | 新功能开发，如 `feat/box86-home-page` |
| `fix/<name>` | Bug 修复，如 `fix/sdl-flush-crash` |
| `refactor/<name>` | 重构分支 |
