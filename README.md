# ac

the ac can be described as the set of tools for game development. it solves common problems and hides platform dependent details (not in a way you won't expect) so you can concentrate on writing code.

for more information see the [docs](./docs/docs.md)

## features and goals

at the moment, the ac provides the following components, which are independable and can be used separately:

* core - ac core utils like time, fs, threads, etc.. the only one module which you always should use if you are using ac
* input - abstraction for polling input, keep in mind that this is not all-in-one  input lib it was made for games
* main - links some libs for you, setup app, you are free to use your own but make sure you looked at ac one
* renderer - implements a common abstraction layer over multiple graphics APIs (GAPIs): DirectX12, Vulkan 1.3, Metal, GNM, AGC
* render_graph - manages passes and barriers for you. for more info look at perfect explanation written by themaister [render-graphs-and-vulkan](https://themaister.net/blog/2017/08/15/render-graphs-and-vulkan-a-deep-dive/)
* window - manages os window creation for you. again this isn't universal window abstraction. it has limitations and designed only for games

and

* ac-shader-compiler - shader compiler developed specially for ac. can compile shaders written in ac shader language which is same as hlsl but with more strict rules and limitations to be multiplatform

## building sources

building ac requires:

on windows:

* [Visual Studio 2019 or later](https://www.visualstudio.com/downloads) - select the following workloads:
    * Desktop Development with C++
* [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/)

on linux:

* make

on macos:

* xcode

for environment setup see the [environment-setup.md](./docs/environment-setup.md)

## running
it's recommended to have latest graphics drivers installed. ac supports enhanced barriers for dx12 which are not supported in the old drivers

## making changes

to make contributions, see the [contributing.md](./contributing.md) file in this project.

## documentation

you can find documentation for this project in the `docs` directory. these contain the ac documentation files, as well as ac-shader-compiler documentation

## license

ac is distributed under the terms of the Apache 2.0 License.

see [LICENSE](LICENSE) for details.
