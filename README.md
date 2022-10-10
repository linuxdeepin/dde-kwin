# dde-kwin
------------
dde-kwin is a kwin plugin library developed based on Qt.

## Features
1. Use `cmake` for project management.
2. Use `QTest` for unit testing.
3. Use `reuse` for open source protocol checking.
4. Support generating unit test coverage reports.
5. `g++` compilers are use.
6. Compatible with `Qt6` and `Qt5`.
7. Support for compiling as a `debian` platform installer.

## Catalog Specification
 **Catalog**           | **Description**
------------------|---------------------------------------------------------
 .reuse/          | license declaration file, the project should use the reuse tool for license declaration
 .gitignore       | The git filter list, used to filter files that don't need to be pushed to the repository
 README.md        | Project Overview Document
 LICENSE          | License agreement file, the file to github such repository use, the project should use reuse tool, but the agreement must be unified, generally GPL-v3 
 CMakeLists.txt   | Project files can be placed in the outermost
 [debian/]        | Required files for debian packaging
 LINCENSES/       | License agreement directory, where all license agreements for the project are stored
 [translations/ ] | Store localization-related files, such as .ts

## Style Guide
This style guide follows the [deepin-styleguide](https://github.com/linuxdeepin/deepin-styleguide/releases) and is refined and split on its basis to form a style guide applicable to development libraries.

### Basic
1. The development library must be based on `Qt` and can depend on `dtkcore`, but is not allowed to depend on `dtkgui` and `dtkwidget`, in order to minimize dependencies, and should not depend on `dtkcore` if it does not need to.
2. Try to use pre-declarations, unlike [deepin-tyleguide](https://github.com/linuxdeepin/deepin-styleguide/releases) here, as a development library, to try to minimize dependencies on header files.

### Naming

#### General Rules
1. Use descriptive naming whenever possible, use full word combinations whenever possible, and be sure not to use abbreviations or shorthand.

#### Project Name
The project name should start with the three letters `dtk` and be all lowercase, no ligatures are allowed, e.g. `dtkpower`, `dtkpowermanager`.

