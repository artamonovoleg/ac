# common prebuild steps

these are the steps you should do before start if you are planning to develop ac
and not just build it. if this is the case for you skip it and navigate to [building](#building) section

create the directory where you will clone all ac related repositories.
will be named ac-wks in this example. clone all repositories that you need in this
directory

# building

## using ide

probably the most common way but not the ac developing way. however you are free
to use any tools you like.

depending on the ac project you want to build, open the terminal in the premake
folder of this project. if you are not sure how to do it open terminal and execute next
command.

**windows**

``` bash
cd <full-path-to-project>/premake
```

**unix**

``` bash
cd <full-path-to-project>/premake
```

**visual studio**

``` bash
..\..\ac\premake\bin\windows\premake5.exe vs2022
```
you can also use vs2017, vs2018 or vs2019 if you want. see all supported versions by
executing

``` bash
..\..\ac\premake\bin\windows\premake5.exe  --help
```

**linux**

```bash
../../ac/premake/bin/linux/premake5 gmake2
```

there is also gmake and codelite available but they are not tested and
won't be fixed if any bugs founded

**xcode**

```bash
../../ac/premake/bin/macos/premake5 xcode4
```

you can use gmake2 as well if you want but it's not mantained and a lot
of required build options provided only for xcode

## using command line

```bash
../../ac/premake/bin/<system>/premake5<.exe> --os=<target os> --cli-config=<release/debug/dist> --cli-project=<project name or just skip this to build everything> cli-build
```

## using vscode

ac was developed in vscode mostly so it has the rich set of tools for developing
in vscode

install these extensions first:

* [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) - will use it for debuggin on windows and linux
* [CodeLLDB](https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb) - lldb debugger extension
* [Better C Syntax](https://marketplace.visualstudio.com/items?itemName=jeff-hykin.better-c-syntax)
* [Better C++ Syntax](https://marketplace.visualstudio.com/items?itemName=jeff-hykin.better-cpp-syntax)
* [Better Objective-C Syntax](https://marketplace.visualstudio.com/items?itemName=jeff-hykin.better-objc-syntax)
* [Better Objective-C++ Syntax](https://marketplace.visualstudio.com/items?itemName=jeff-hykin.better-objcpp-syntax)
* [Clang-Format](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format) - formatting extension, keep in mind that clang-format executable not bundled in it, you should install it yourself [LLVM](https://releases.llvm.org/)
* [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) - language extension, for completions, syntax highlighting
* [Shader languages support for VS Code](https://marketplace.visualstudio.com/items?itemName=slevesque.shader) - syntax highlight for shaders

misc:

some extensions which are not required but they are just cool:

* [Task Explorer](https://marketplace.visualstudio.com/items?itemName=spmeesseman.vscode-taskexplorer) - nice gui to launch tasks
* [TODO Highlight](https://marketplace.visualstudio.com/items?itemName=wayou.vscode-todo-highlight) - hightlights TODO, NOTE and other tags in code
* [WSL](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-wsl) - if you are using wsl to build this is very convenient for developing. also check Remote Tunnels, Remote Explorer and Dev Containers if you are building on some remote machine
* [Git Graph](https://marketplace.visualstudio.com/items?itemName=mhutchie.git-graph) - git graph in vscode
* [ac-theme](https://marketplace.visualstudio.com/items?itemName=acidev.ac-theme) - theme for C languages family. treat it as template. most themes on market have pure support for C languages. you can take ac-theme-template.json which has comments to easily create own themes

before building you should change settings.json. you can make it globally or for ac-wks only
however second option is preffered

for the first approach press ctrl+p on windows or cmd+p on mac and type ```> Open User Settings (JSON)``` and press enter
for the second approach create folder .vscode in ac-wks and settings.json file in it

put next in your settings.json

```json
{
  // disable C/C++ plugin intellisense as we will use clangd for completions and highlighting
  "C_Cpp.intelliSenseEngine": "disabled",
  "clangd.arguments": [
    // let clangd generate more detailed logs
    "--log=error",
    // the output json file is more beautiful, disable it by default if you are not planning to look at clangd log
    // "--pretty",
    // global completion (the pop-up suggestions during input will provide possible symbols in all files, and the header file will be automatically, supplemented)
    "--all-scopes-completion",
    // more detailed completion
    "--completion-style=detailed",
    // automatically analyze files in the background
    "--background-index",
    // disable header insertion
    "--header-insertion=never",
    // Enable clang tidy to provide static checking
    "--clang-tidy",
    // disable provide placeholder for function
    "--function-arg-placeholders=false",
    // pch optimized location (memory or disk, selecting memory will increase memory overhead, but will improve performance)
    "--pch-storage=memory",
    // number of tasks opened at the same time, typically you want to write <number of cores> * 2 if you are not sure about it remove it
    "-j=32",
    // path to compile_commands.json, ac premake plugin will generate it and put in this folder
    "--compile-commands-dir=${workspaceFolder}/.vscode",
  ],
  "editor.tabSize": 2,
  "editor.insertSpaces": true,
  "files.insertFinalNewline": true,
  "files.eol": "\n",
  "files.trimTrailingWhitespace": true,
  // you can setup formatting of file or selected region on shortcut if you want
  "editor.formatOnSave": true,
  // treat ac shader language as hlsl
  "file.associations": {
    "*.acsl": "hlsl"
  },
  // task explorer settings, ac premake plugin will generate tasks which won't be displayed nicely if you skip this
  "taskExplorer.showLastTasks": false,
  "taskExplorer.enableSideBar": true,
  "taskExplorer.groupWithSeparator": true,
  "taskExplorer.groupMaxLevel": 4,
  "taskExplorer.groupSeparator": "_",
  "taskExplorer.autoRefresh": true,
  "taskExplorer.useAnt": false,
  "taskExplorer.useGulp": false,
}

```

next open terminal, navigate to premake folder of choosen project and
execute next command


``` bash
../../ac/premake/bin/<os>/premake5<.exe> vscode
```

this will create tasks.json and launch.json which are needed to build project and
launch debug

now you should be able to open debug tab and launch any project

keep in mind that ac premake plugin doesn't include non executable targets to
avoid panel polution. if you will execute it in ac folder you won't see build
and clean sections

if this is first launch press ctrl/cmd+p and type ```> Restart language server``` after building

for console platforms see the [environment-setup-consoles.md](./private/environment-setup-consoles.md)
