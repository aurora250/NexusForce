# CHANGELOG

## [1.0.0] - 2026-3-7

### 🚀 New Features
- PCRE2 正则表达式支持
- WebSocket 支持

### 🔧 Improvements
- 优化 network 结构
- 统一使用 vcpkg 包管理
- 使用外部配置 cmake 选项
- WinSock 初始化优化

### 📚 Documentation
- 除 db 与 network 外的大部分 API 文档
- README 与 构建指南

### 🐛 Bug Fixes
- 修复 network socket 处理问题
- 修复线程池交叉模式下的线程局部操作问题
