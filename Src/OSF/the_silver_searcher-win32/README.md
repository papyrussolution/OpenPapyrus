[![Build status](https://ci.appveyor.com/api/projects/status/1ldgvviqqbkdav3m/branch/master?svg=true)](https://ci.appveyor.com/project/k-takata/the-silver-searcher-win32/branch/master)

# The silver searcher Win32 unofficial daily builds

This project provides daily builds of [the silver searcher](https://github.com/ggreer/the_silver_searcher) for Win32/64.

You can download the zip packages from the [releases](https://github.com/k-takata/the_silver_searcher-win32/releases) page.

Two types of packages are provided:

* `ag-YYYY-MM-DD_{CommitID}-x64.zip`: x64 release build
* `ag-YYYY-MM-DD_{CommitID}-x86.zip`: x86 release build

`{CommitID}` is a commit ID or a tag name of [the official repository](https://github.com/ggreer/the_silver_searcher).

# Note

Starting from the build [2020-02-26/2.2.0-24-gb93c271](https://github.com/k-takata/the_silver_searcher-win32/releases/tag/2020-02-26%2F2.2.0-24-gb93c271), the silver searcher win32 uses [the UTF-8 code page](https://docs.microsoft.com/en-us/windows/uwp/design/globalizing/use-utf8-code-page) on Windows 10 version 1903 or later. This allows you to search UTF-8 strings.

# License

The silver searcher itself (which is in the ag subdirectory) is licensed under the Apache 2 license. See the [LICENSE](LICENSE) file.

The build scripts in this repository are based on the [vim-win32-installer](https://github.com/vim/vim-win32-installer) project, and [the Vim license](http://vimhelp.appspot.com/uganda.txt.html#license) applies to them.
