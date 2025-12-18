# Cyberpunk 2077 RED4ext / RED4ext.SDK / RedLib – “built-in functions” quick reference (AI-friendly)

> Scope note: the RED4ext/SDK surface area is **very large** (thousands of types/functions across patches).  
> This file focuses on the **core built-in APIs you actually use when authoring a plugin**, plus **how to discover the full native surface** (NativeDB + RTTI dumps).

---

## Repos and docs covered

- **RED4ext Plugin Dev docs (GitBook):** plugin lifecycle, native class/function registration, logging, config  
  - https://docs.red4ext.com/ (entry)
- **RED4ext.SDK examples (WopsS):** function registration, native globals, calling into game functions
  - https://github.com/WopsS/RED4ext.SDK
- **RedLib (psiberx) README:** header-only helpers for RTTI definitions, calling functions, type info helpers, logging wrapper
  - https://github.com/psiberx/cp2077-red-lib

---

## 1) Plugin entrypoints (exports) and lifecycle

Your plugin DLL is loaded by RED4ext. You export:

### `Supports() -> uint32_t`
Return the RED4ext API version your plugin supports.
- For “latest”, return `RED4EXT_API_VERSION_LATEST`.

### `Query(RED4ext::PluginInfo* aInfo)`
Populate plugin metadata; **do not** hook/allocate here.
Common fields in examples:
- `aInfo->name`, `aInfo->author`, `aInfo->version`
- `aInfo->runtime` (game version gate)  
  - recommended: `RED4EXT_RUNTIME_LATEST`  
  - or `RED4EXT_RUNTIME_INDEPENDENT` if you only need the loader and don’t care about the game version.
- `aInfo->sdk` (SDK compatibility gate), often `RED4EXT_SDK_LATEST`

### `Main(RED4ext::PluginHandle aHandle, RED4ext::EMainReason aReason, const RED4ext::Sdk* aSdk) -> bool`
This is your entrypoint (similar to `DllMain`), called after RED4ext checks compatibility.
- You can attach hooks and register RTTI types here.
- Important: memory allocators / full RTTI readiness may not be available immediately; for “do something when RTTI is ready”, use a later custom state (see RED4ext docs “Custom game states”).

Skeleton from docs:

```cpp
#include <RED4ext/RED4ext.hpp>

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(
  RED4ext::PluginHandle aHandle,
  RED4ext::EMainReason aReason,
  const RED4ext::Sdk* aSdk)
{
  switch (aReason)
  {
    case RED4ext::EMainReason::Load:
      // attach hooks, register RTTI, init
      break;
    case RED4ext::EMainReason::Unload:
      break;
  }
  return true;
}
```

---

## 2) Logging (built-in)

RED4ext provides a per-plugin logger via `aSdk->logger`.
Example:

```cpp
aSdk->logger->Trace(aHandle, "Hello World!");
aSdk->logger->TraceF(aHandle, "Hello %s!", "World");
```

Logs are created under:
`<game_dir>/red4ext/logs/<plugin_name>.log`

Global log rotation and level are controlled by RED4ext config (see section 8).

---

## 3) RTTI system basics (RED4ext.SDK)

Most “built-in” functionality you use in SDK land revolves around the RTTI system:

### Get the RTTI system
```cpp
auto rtti = RED4ext::CRTTISystem::Get();
```

### Register callbacks for type/function registration
Typical pattern in examples:

```cpp
rtti->AddRegisterCallback(RegisterTypes);
rtti->AddPostRegisterCallback(PostRegisterTypes);
```

Why two phases?
- `RegisterTypes`: register new **types** (classes/structs).
- `PostRegisterTypes`: add **functions/params**, set flags, link parents, etc.

---

## 4) Adding native functions (RED4ext.SDK)

### 4.1) Native function handler signature (low-level)

The “raw” SDK handler signature is:

```cpp
void MyFunc(
  RED4ext::IScriptable* aContext,
  RED4ext::CStackFrame* aFrame,
  /*ReturnType*/ * aOut,
  int64_t a4);
```

Read arguments from the stack frame in order:

```cpp
RED4ext::GetParameter(aFrame, &arg1);
RED4ext::GetParameter(aFrame, &arg2);
aFrame->code++; // skip ParamEnd
```

Set output by writing into `*aOut` (if not void).

### 4.2) Registering a class function (example pattern)

From the docs “Adding a Native Function”:

```cpp
auto rtti = RED4ext::CRTTISystem::Get();
auto scriptable = rtti->GetClass("IScriptable");

auto getNumber = RED4ext::CClassFunction::Create(
  &customControllerClass,
  "GetNumber", "GetNumber",
  &GetNumber,
  { .isNative = true });

getNumber->AddParam("Vector4", "worldPosition");
getNumber->AddParam("Int32", "count");
getNumber->SetReturnType("Float");

customControllerClass.RegisterFunction(getNumber);
```

Key built-ins used here:
- `CRTTISystem::Get()`
- `CRTTISystem::GetClass("...")`
- `CClassFunction::Create(...)`
- `CBaseFunction::AddParam(typeName, paramName)`
- `CBaseFunction::SetReturnType(typeName)`
- class `RegisterFunction(...)`

### 4.3) Registering global functions

From SDK example “function_registration”:

```cpp
auto func = RED4ext::CGlobalFunction::Create(
  "MyGlobalFunc", "MyGlobalFunc", &MyGlobalFunc);
func->SetReturnType("String");
rtti->RegisterFunction(func);
```

### 4.4) Static class functions

```cpp
auto playerPuppetCls = rtti->GetClass("PlayerPuppet");
auto staticFunc = RED4ext::CClassStaticFunction::Create(
  playerPuppetCls,
  "MyStaticFunc", "MyStaticFunc",
  &MyStaticFunc);
staticFunc->SetReturnType("Bool");
playerPuppetCls->RegisterFunction(staticFunc);
```

---

## 5) Creating custom native classes (RED4ext.SDK)

Minimal pattern from docs “Creating a Custom Native Class”:

1) Define a scriptable type:
```cpp
struct CustomController : RED4ext::IScriptable
{
  RED4ext::CClass* GetNativeType();
};
```

2) Create a typed class wrapper:
```cpp
RED4ext::TTypedClass<CustomController> customControllerClass("CustomController");

RED4ext::CClass* CustomController::GetNativeType()
{
  return &customControllerClass;
}
```

3) Register the type at the right time:
```cpp
void RegisterTypes()
{
  RED4ext::CRTTISystem::Get()->RegisterType(&customControllerClass);
}
```

4) Hook the registration callbacks in `Main` (via RTTI registrator/callbacks).

---

## 6) Calling existing game functions (RED4ext.SDK and RedLib)

### 6.1) Low-level SDK calls (examples)

From “native_globals_redscript” example:
- `RED4ext::ExecuteGlobalFunction("GetPlayer;GameInstance", &handle, gameInstance)`
- `RED4ext::ExecuteFunction(handle, getQuickSlotsManagerFunc, &quickSlotManager)`
- Then `ExecuteFunction` to call methods like `SummonVehicle`.

This is the “call into engine functions” path:
1) resolve function (via RTTI `GetClass` → `GetFunction`)
2) call it with `ExecuteFunction` / `ExecuteGlobalFunction`.

### 6.2) RedLib convenience wrappers

RedLib provides nicer, strongly-typed helpers:

```cpp
float a = 13, b = 78, max;
Red::CallGlobal("MaxF", max, a, b);

Red::Vector4 vec{};
Red::CallStatic("Vector4", "Rand", vec);

Red::ScriptGameInstance game;
Red::Handle<Red::PlayerSystem> system;
Red::Handle<Red::GameObject> player;

Red::CallStatic("ScriptGameInstance", "GetPlayerSystem", system, game);
Red::CallVirtual(system, "GetLocalPlayerControlledGameObject", player);
Red::CallVirtual(player, "Revive", 100.0f);
```

Related built-ins in RedLib:
- `Red::CallGlobal(name, out, args...)`
- `Red::CallStatic(className, funcName, out, args...)`
- `Red::CallVirtual(handleOrPtr, funcName, outOrArgs...)`

---

## 7) RedLib: built-in macros/utilities for RTTI and native exposure

RedLib is a header-only helper layer “to help create RED4ext plugins”.

### 7.1) Declare native globals

```cpp
RTTI_DEFINE_GLOBALS({
  RTTI_FUNCTION(MakeArray);
  RTTI_FUNCTION(SortArray);
  RTTI_FUNCTION(Swap);
});
```

### 7.2) Enums and flags

```cpp
RTTI_DEFINE_ENUM(MyEnum);
RTTI_DEFINE_FLAGS(MyFlags);
```

### 7.3) Classes/structs: methods + properties

```cpp
RTTI_DEFINE_CLASS(MyStruct, {
  RTTI_METHOD(Inc);
  RTTI_PROPERTY(value);
});
```

For `IScriptable` classes:
```cpp
struct MyClass : Red::IScriptable
{
  RTTI_IMPL_TYPEINFO(MyClass);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(MyClass, {
  RTTI_METHOD(AddItem);
  RTTI_METHOD(AddFrom);
  RTTI_METHOD(Create);
  RTTI_GETTER(items);
});
```

### 7.4) Inheritance helpers

```cpp
RTTI_DEFINE_CLASS(ClassA, "A", { RTTI_ABSTRACT(); });
RTTI_DEFINE_CLASS(ClassB, "B", { RTTI_PARENT(ClassA); });
```

### 7.5) Auto-registration

Call once on plugin load:

```cpp
Red::TypeInfoRegistrar::RegisterDiscovered();
```

This registers all RTTI definitions “discovered” in your translation units.

### 7.6) Accessing RTTI type info by C++ type

Compile-time:
```cpp
constexpr auto name = Red::GetTypeName<uint64_t>();          // CName("Uint64")
constexpr auto name2 = Red::GetTypeName<Red::CString>();     // CName("String")
constexpr auto name3 = Red::GetTypeName<Red::DynArray<Red::Handle<MyClass>>>();
constexpr auto str = Red::GetTypeNameStr<RED4ext::CString>(); // "String\0"
```

Runtime:
```cpp
auto stringType = Red::GetType<Red::CString>();
auto enumArrayType = Red::GetType<Red::DynArray<MyEnum>>();
auto entityClass = Red::GetClass<Red::Entity>();
```

### 7.7) Class extensions (add methods to existing game classes)

```cpp
struct MyExtension : Red::GameObject
{
  void AddTag(Red::CName tag) { tags.Add(tag); }
};

RTTI_EXPAND_CLASS(Red::GameObject, {
  RTTI_METHOD_FQN(MyExtension::AddTag);
});
```

### 7.8) Raw handlers when you need `CStackFrame`

RedLib shows the raw pattern and use of:
- `RED4ext::GetParameter(frame, &a)`
- `frame->code` advancement
- reading `frame->func` to learn caller

---

## 8) RED4ext configuration knobs (useful built-ins)

RED4ext has a config file with sections like:
- `[logging] level, flush_on, max_files, max_file_size`
- `[plugins] enabled, ignored`
- `[dev] console, wait_for_debugger`

These affect plugin system behavior and logging.

---

## 9) How to discover “ALL native functions” exposed by the game (practical approach)

Because the engine surface changes every patch, the recommended ways to enumerate available functions/types are:

1) **NativeDB** (browser for SDK/native RTTI)
   - https://nativedb.red4ext.com/ (linked from docs)

2) **RTTI dumps** (maintainer provides dumps per patch, historically shared in the modding Discord)
   - In a RED4ext discussion, WopsS mentions dumping RTTI for each patch and sharing it (example linked for v1.2), and points to Discord threads where updated dumps are posted.

3) **Browse headers + generated natives in RED4ext.SDK**
   - When you include the SDK, you get generated type headers under `RED4ext/Scripting/Natives/Generated/...`
   - Use your IDE’s “Go to Definition” and search for `CClass`, `CBaseFunction`, `ExecuteFunction`, etc.

If you want a *programmatic* index for AI:
- prefer exporting a normalized list from NativeDB / RTTI dump and storing it as JSON (e.g., `{ class:..., functions:[...], params:[...] }`).
- then feed that JSON + this “how to use APIs” reference to your AI tool.

---

## 10) Minimal “starter template” (RED4ext.SDK + RedLib)

```cpp
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
  return RED4EXT_API_VERSION_LATEST;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::PluginInfo* aInfo)
{
  aInfo->name = L"MyMod";
  aInfo->author = L"Me";
  aInfo->version = RED4EXT_SEMVER(1,0,0);
  aInfo->runtime = RED4EXT_RUNTIME_LATEST;
  aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(
  RED4ext::PluginHandle aHandle,
  RED4ext::EMainReason aReason,
  const RED4ext::Sdk* aSdk)
{
  switch (aReason)
  {
    case RED4ext::EMainReason::Load:
      // Register RedLib RTTI definitions
      Red::TypeInfoRegistrar::RegisterDiscovered();

      // Log
      aSdk->logger->Info(aHandle, "Loaded!");
      break;

    case RED4ext::EMainReason::Unload:
      break;
  }
  return true;
}
```

---

## 11) “Cheat sheet” of the most-used built-in calls

### Plugin API / lifecycle
- `Supports()` → return `RED4EXT_API_VERSION_*`
- `Query(PluginInfo*)` → fill metadata
- `Main(handle, reason, sdk)` → hook, register, init

### Logging
- `aSdk->logger->Trace/Debug/Info/Warn/Error/Critical(...)`
- `aSdk->logger->TraceF/...` formatted variants

### RTTI / registration
- `RED4ext::CRTTISystem::Get()`
- `rtti->AddRegisterCallback(fn)`
- `rtti->AddPostRegisterCallback(fn)`
- `rtti->RegisterType(CClass*)`
- `rtti->RegisterFunction(CBaseFunction*)`
- `rtti->GetClass("ClassName")`
- `cls->GetFunction("FuncName")`
- `cls->RegisterFunction(...)`

### Function building
- `CGlobalFunction::Create(longName, shortName, handler)`
- `CClassFunction::Create(ownerClass, longName, shortName, handler, flags)`
- `CClassStaticFunction::Create(ownerClass, longName, shortName, handler)`
- `func->AddParam("Type", "name")`
- `func->SetReturnType("Type")`
- `func->flags = { .isNative = true, ... }` (for some SDK examples)

### Stack frame argument parsing
- `RED4ext::GetParameter(frame, &outVar)`
- `frame->code++` after reading args

### Calling into game functions
- `RED4ext::ExecuteGlobalFunction("Name;Context", out, context)`
- `RED4ext::ExecuteFunction(handle, func, out)`
- RedLib: `Red::CallGlobal / CallStatic / CallVirtual`

---

## References (raw links)

- Creating a Plugin (exports, lifecycle): https://docs.red4ext.com/mod-developers/creating-a-plugin  
- Adding a Native Function (GetParameter, AddParam, SetReturnType): https://docs.red4ext.com/mod-developers/adding-a-native-function  
- Creating a Custom Native Class: https://docs.red4ext.com/mod-developers/creating-a-custom-native-class  
- Logging: https://docs.red4ext.com/mod-developers/logging  
- Config options: https://docs.red4ext.com/getting-started/configuration  
- Creating a plugin with RedLib: https://docs.red4ext.com/mod-developers/creating-a-plugin-with-redlib  
- RedLib repo (README contains macros + calling helpers): https://github.com/psiberx/cp2077-red-lib  
- RED4ext.SDK repo + examples: https://github.com/WopsS/RED4ext.SDK  
- NativeDB: https://nativedb.red4ext.com/

