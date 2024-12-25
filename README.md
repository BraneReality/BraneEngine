# BraneEngine

<img alt="Engine Logo" src="https://github.com/WireWhiz/BraneEngine/blob/Main/media/branelogo.png?raw=true" width="30%" />
 A metaverse framework, built on a custom engine. Inspired by the String Theory version of a multiverse, where each universe rests upon a membrane or "brane"

# Mission Statement
To create a framework for a creator-focused metaverse by creating an engine and server framework that caters to what creators need to build AAA-quality immersive worlds.

# Get involved
Check out the [Contribution Guidelines](https://github.com/WireWhiz/BraneEngine/wiki/Contributing) and join our [discord server](https://discord.gg/T3Td7PMDFX) to hang out.

# Building
Brane Engine is built using [vckpkg](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started) & cmake

First run vcpkg to install packages locally, vcpkg will use our vcpkg.json, so there is no need to specify packages
```bash
vcpkg install
```

make a build folder and scope to it

```bash 
mkdir build && cd build
```

next we need to configure cmake, if you have VCPKG_ROOT set you can copy this command in verbatim, however if not you'll have to replace `%VCPKG_ROOT%` with the path to your local vcpkg install.
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" -DBUILD_TESTS=ON

```

then we can build all targets with:

```bash

cmake --build .

```

Brane Engine has 4 build targets, Editor, Client, AssetServer, and tests. You can choose to build only one of them with:
```bash

cmake --build . --target <target name>

```

Now, none of these except for tests will run in the build folder as they require relative file placements like `config.json`s and media so we need to run cmake install before using them.
We also need to make sure to only install one build target to any given install directory, since things like their default configs will overwrite eachother.
The install components we can choose from are Editor, Client, and AssetServer.
```bash
cmake --install . --component Editor --prefix ./editor
cmake --install . --component Client --prefix ./client
cmake --install . --component AssetServer --prefix ./assetServer

```
now if you ran all those commands in order, you should have a runnable executable in `build/<component name>`!

## More examples
We have example vscode/nvim(with overseer) build commands provided in `.vscode/tasks.json` and `.vscode/launch.json` 
For ninja to funciton on windows it needs to be run in a Visual Studio Developer command prompt, or in a [shell with msvc env vars set up](https://wirewhiz.com/running-ninja-in-windows-powershell/).

