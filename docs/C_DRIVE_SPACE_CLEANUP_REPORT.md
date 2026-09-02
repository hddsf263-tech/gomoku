# C 盘安全空间清理报告

**日期**: 2026-09-01
**扫描范围**: `C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku`
**模式**: Safe Mode，只删除可再生成的构建产物

## 保护目录（禁止删除）

- `C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku_cleanup_backup_20260901_200248`
- `C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku_ai_archive_20260901`
- `C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku_ai_archive_20260901\src\SearchEngine.cpp.fixed_from_C_20260901`

## 扫描结果

总文件数：49

总大小：521,666 字节（约 509.4 KB）

其中 `build/` 构建产物：18 个文件，368,038 字节（约 359.4 KB）

### 可删除的构建产物（SAFE_TO_DELETE）

| 路径 | 类型 | 大小 | 是否可重新生成 | 是否允许删除 |
| --- | --- | --: | --- | --- |
| `build/.qt/QtDeploySupport.cmake` | 构建缓存 | 2,776 | 是 | SAFE_TO_DELETE |
| `build/.qt/QtDeployTargets.cmake` | 构建缓存 | 317 | 是 | SAFE_TO_DELETE |
| `build/build.ninja` | 构建生成 | 62,423 | 是 | SAFE_TO_DELETE |
| `build/cmake_install.cmake` | 构建生成 | 2,533 | 是 | SAFE_TO_DELETE |
| `build/CMakeCache.txt` | 构建缓存 | 57,606 | 是 | SAFE_TO_DELETE |
| `build/CMakeFiles/3.30.5/CMakeCXXCompiler.cmake` | 构建缓存 | 8,863 | 是 | SAFE_TO_DELETE |
| `build/CMakeFiles/3.30.5/CMakeDetermineCompilerABI_CXX.bin` | 编译器探测 | 44,778 | 是 | SAFE_TO_DELETE |
| `build/CMakeFiles/3.30.5/CMakeRCCompiler.cmake` | 构建缓存 | 250 | 是 | SAFE_TO_DELETE |
| `build/CMakeFiles/3.30.5/CMakeSystem.cmake` | 构建缓存 | 395 | 是 | SAFE_TO_DELETE |
| `build/CMakeFiles/3.30.5/CompilerIdCXX/a.exe` | 编译器探测 | 44,862 | 是 | SAFE_TO_DELETE |
| `build/CMakeFiles/3.30.5/CompilerIdCXX/CMakeCXXCompilerId.cpp` | 编译器探测 | 29,376 | 是 | SAFE_TO_DELETE |
| `build/CMakeFiles/clean_additional.cmake` | 构建生成 | 284 | 是 | SAFE_TO_DELETE |
| `build/CMakeFiles/cmake.check_cache` | 构建缓存 | 86 | 是 | SAFE_TO_DELETE |
| `build/CMakeFiles/CMakeConfigureLog.yaml` | 构建日志 | 58,277 | 是 | SAFE_TO_DELETE |
| `build/CMakeFiles/Gomoku_autogen.dir/AutogenInfo.json` | 自动生成 | 33,406 | 是 | SAFE_TO_DELETE |
| `build/CMakeFiles/rules.ninja` | 构建生成 | 2,524 | 是 | SAFE_TO_DELETE |
| `build/CMakeFiles/TargetDirectories.txt` | 构建缓存 | 821 | 是 | SAFE_TO_DELETE |
| `build/Gomoku_autogen/moc_predefs.h` | 自动生成 | 18,461 | 是 | SAFE_TO_DELETE |

### 保留文件（KEEP）

| 类别 | 文件数 | 大小 | 判断 |
| --- | --: | --: | --- |
| 源码 `src/`、`include/` | 21 | 88,197 | KEEP：历史源码 |
| 文档与状态 `docs/`、README、PROJECT_STATE 等 | 9 | 60,795 | KEEP：历史记录 |
| 工程文件 `CMakeLists.txt`、`build.ps1` | 2 | 5,844 | KEEP：历史工程配置 |
| 归档标记 `ARCHIVED_DO_NOT_DEVELOP_HERE.md` | 1 | 316 | KEEP：目录标记 |

## 预计释放空间

```text
预计删除文件数：18
预计释放空间：368,038 字节（约 359.4 KB）
```

## 执行前 C 盘状态

```text
C 盘可用空间：249,456,054,272 字节（约 232.3 GiB）
```

## 执行状态

本报告生成后，仅删除上表中标记 `SAFE_TO_DELETE` 的 `build/` 目录内容。

## 执行结果

- 删除文件数：18
- 逻辑释放空间：368,038 字节（约 359.4 KB）
- 删除对象：`C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku\build`（唯一 SAFE_TO_DELETE 项）
- C 盘可用空间：249,456,054,272 -> 249,456,394,240 字节（实际约 +339,968 字节）

## 最终检查

```text
D:\gomoku
= 完全未修改（工程代码与构建产物未动；仅按指令新增本报告）

gomoku_cleanup_backup_20260901_200248
= 完整保留（149 个文件）

gomoku_ai_archive_20260901
= 完整保留（22 个文件，含 SearchEngine.cpp.fixed_from_C_20260901）

C盘历史源码
= 保留（31 个文件）

可再生成构建产物
= 已清理
```