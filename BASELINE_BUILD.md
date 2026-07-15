# Baseline build evidence

The initial `shell/main` product branch contains official Notepad++ v8.9.7 source at commit `6634650414ff91220a4c353b7fe5ad741af0f9f9` plus bootstrap documentation only.

On 2026-07-16, the unmodified application source built successfully for x64 Release with Visual Studio Build Tools 2022, MSBuild 17.14.40, v143, and Windows SDK 10.0.26100.0:

```powershell
MSBuild.exe PowerEditor\visual.net\notepadPlus.sln /m /p:Configuration=Release /p:Platform=x64 /v:minimal
```

Produced `PowerEditor\bin64\Notepad++.exe` reports file/product version `8.9.7.0`. This proves a local clean baseline only. It is not a reproducibility, branding, security, or distribution qualification.

An isolated startup smoke with `-multiInst -noPlugin -nosession` and a temporary settings directory also passed; the process remained healthy and accepted a normal main-window close.
