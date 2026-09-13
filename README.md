
A foobar2000 component decodes AdLib series formats.
Based on [AdPlug](https://github.com/adplug/adplug), customized a little.
Some formats needs companion file(s). If you find the output is adnormal, please check foobar console. You may miss companion file(s) and file provider will send errors to the console.

## Compatibility
- foobar2000 **v2.0 or newer**, Windows, 32-bit and 64-bit.
- Built against the foobar2000 SDK (2025-03-07).

## Building from source

### Prerequisites
- Visual Studio 2022 with the **Desktop development with C++** workload (and a Windows 10/11 SDK).
- The official **foobar2000 SDK (2025-03-07 or compatible)**.
- **WTL 10** (Windows Template Library).

### Directory layout
The project files reference the SDK with system environment variable `FB2KSDK`. So, you just need to add the variable with SDK loacation to your system then the project will recognize it. 

WTL **MUST** be placed as below:

```
├─ Foobar2000SDK-2025\
│  ├─ foobar2000\
│  │  ├─ SDK\
│  │  ├─ helpers\
│  │  ├─ shared\
│  │  └─ foobar2000_component_client\
│  ├─ libPPUI\
│  ├─ pfc\
│  └─ WTL10_01_Release\Include\        <- WTL goes here
```

### Build steps
1. Open `Adlib_Decoder.sln`.
2. Add all .vcxproj in foobar2000SDK (except `foo_sample`) to the solution. Then add all these as references to `foo_input_adlib_opl`.
3. Build **Release | Win32** and **Release | x64** (both are required to package).
4. The Release|x64 post-build step runs `fb2k_component_pack.ps1`, producing `dist\foo_input_adlib_opl.fb2k-component`
