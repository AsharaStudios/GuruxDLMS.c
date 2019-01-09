# Debugging

If you use Visual Studio Code, you can use [Cortex Debug Plugin](https://github.com/Marus/cortex-debug)
for debugging, the configuration file (`launch.json`) may be the one like this [template for Tiva C](.launch.json). Additionally, if you are using also Platformio, you may know that (at time) Platformio overwrites this `launch.json` every time you open the project, so there's also a [script that overwrites this action](.prepareDebug.py) once the `[env:tiva]` ends compiling ([Platformio Advanced Scripting](https://docs.platformio.org/en/latest/projectconf/advanced_scripting.html)).

You need openOCD and arm-none-eabi-gdb installed;
but if you use PlatformIO you can use the PIO packages changing the default path in the plugin:

first install openOCD and arm-none-eabi-gdb packages:

```bash
~/.platformio/penv/bin/platformio platform install --with-package tool-openocd titiva
```

then, configure Cortex Debug Plugin to include custom path for executables:

* `settings.json` (Visual Studio Code user configurations).

```json
    "cortex-debug.armToolchainPath": "~/.platformio/packages/toolchain-gccarmnoneeabi/bin",
    "cortex-debug.openocdPath": "~/.platformio/packages/tool-openocd/bin/openocd",
    ...
```
