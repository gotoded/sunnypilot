# 更新说明 / Changelog

## 2025-07-06

### 用 MM-X/sunnypilot-pc master-rk3588 覆盖本地仓库
- 将仓库重置为 https://github.com/MM-X/sunnypilot-pc.git 的 master-rk3588 分支
- 当前 HEAD: `3ad69b20b` (fix(lon): Change LEAD_DANGER_FACTOR)
- 远程: origin → MM-X/sunnypilot-pc, gotoded → gotoded/sunnypilot

### 修改 opendbc 子模块指向
- `.gitmodules`: opendbc 子模块 URL 改为 `https://github.com/gotoded/opendbc-sny.git`
- commit: `4c4a3c839`

### SSH 默认强制开启
- `system/hardware/base.h`: `get_ssh_enabled()` 从 `return false` 改为 `return true`
- 影响所有继承 HardwareNone 的平台（包括 PC）
- PC 上 `set_ssh_enabled` 为空操作，用户无法通过 UI 关闭

### 简体中文翻译补充
- `selfdrive/ui/translations/main_zh-CHS.ts`: 补充翻译以下上下文中的所有 type="unfinished" 条目：
  - DeveloperPanel（雷达追踪、GitHub runner、错误日志等）
  - DevicePanelSP（非道路模式相关）
  - DriveStats（行程统计）
  - MadsSettings（UEM、刹车后转向模式）
  - NetworkingSP（扫描）
  - OffroadAlert（始终非道路模式提示）
  - PlatformSelector（车辆选择器）
  - SettingsWindowSP（sunnylink、sunnypilot 等标签）
  - SidebarSP（状态显示）
  - SoftwarePanelSP（模型选择器）
  - SunnylinkPanel（sunnylink 功能）
  - SunnylinkSponsorPopup（赞助流程）
  - SunnypilotPanel（MADS 功能）
  - TogglesPanel（动态实验控制）
- commit: `5efb08953`
