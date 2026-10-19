# TUI 快照基准

`*.snap` 是 `tests/unit/test_tui_snapshot.cpp` 用于比对的纯文本帧基准，由 `tui::headless` 在固定终端尺寸下渲染得到一个或多个组件树后导出。

- 校验：随单元测试自动执行，内容不一致会打印期望与实际两侧的完整帧
- 重新生成：`NEFORCE_UPDATE_SNAPSHOTS=1 ./bin/Debug/NexusForceUnitTests --gtest_filter='Tui*'`
- 新增基准：在测试中调用 `NEFORCE_EXPECT_SNAPSHOT("name", app.plaintext())` 并跑一次更新模式

基准只保存字符内容与换行（不含 ANSI 转义）；颜色、反色等样式由 `test_tui_widgets.cpp` 中的断言直接检查单元格属性。
