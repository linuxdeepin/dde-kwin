# dde-kwin
------------
dde-kwin是一个基于Qt开发的kwin插件库。

## 特性
1. 使用`cmake`进行项目管理。
2. 使用`QTest`进行单元测试。
3. 使用`reuse`进行开源协议检查。
4. 支持生成单元测试覆盖率报告。
5. 使用`g++`编译器。
6. 兼容`Qt6`与`Qt5`。
7. 支持编译为`debian`平台安装包。

## 项目目录规范
 **目录**           | **描述**
------------------|---------------------------------------------------------
 .reuse/          | license声明文件，项目中应使用reuse工具进行license声明
 .gitignore       | git过滤列表，用来过滤不需要推送到仓库中的文件
 README.md        | 项目概述文档
 LICENSE          | 许可协议文件，该文件给github这种仓库使用，项目应使用reuse工具，但协议必须统一，一般为GPL-v3 
 CMakeLists.txt   | 项目文件可放置在最外层
 [debian/]        | debian打包所需文件
 LINCENSES/       | 许可协议目录，存放该项目所有的许可协议
 [translations/ ] | 存放本地化相关文件，如 .ts
 [tests/]         | 单元测试相关代码存放目录

## 风格指南
本风格指南遵循[deepin开源风格指南](https://github.com/linuxdeepin/deepin-styleguide/releases)，在其基础上进行细化拆分，形成适用于开发库的风格指南。

### 基本约定
1. 开发库必须基于`Qt`，可以依赖`dtkcore`，但不允许依赖`dtkgui`与`dtkwidget`，以最小化依赖为原则，若无需依赖`dtkcore`，则不应依赖`dtkcore`。
2. 尽量使用前置声明，此处与[deepin开源风格指南](https://github.com/linuxdeepin/deepin-styleguide/releases)不同，作为开发库，尽量使头文件的依赖最小化。

### 命名约定

#### 通用规则
1. 尽可能使用描述性的命名，尽可能使用全单词组合的命名，务必不要使用缩写或简写。
