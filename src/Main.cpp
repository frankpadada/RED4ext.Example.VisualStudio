#include <bit>
#include <cstdint>
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/ResourceLoader.hpp>
#include <RED4ext/SystemUpdate.hpp>
#include <RedLib.hpp>


const RED4ext::Sdk* g_Sdk = nullptr;
RED4ext::PluginHandle g_pluginHandle = nullptr;

constexpr auto kUpdateName = "AimSplitTick";

class AimSplitSystem : public Red::game::IGameSystem
{
public:
    AimSplitSystem()
    {
        s_instance = this;

        if (g_Sdk)
        {
            g_Sdk->logger->Trace(g_pluginHandle, "AimSplitSystem constructed");
        }
    }

    ~AimSplitSystem() override
    {
        s_instance = nullptr;

        if (g_Sdk)
        {
            g_Sdk->logger->Trace(g_pluginHandle, "AimSplitSystem destroyed");
        }
    }

    static AimSplitSystem* Get()
    {
        return s_instance;
    }

    void ToggleMode()
    {
        m_shootMode = !m_shootMode;

        if (g_Sdk)
        {
            g_Sdk->logger->TraceF(g_pluginHandle, "AimSplit mode toggled to %s", m_shootMode ? "Shoot" : "Look");
        }
    }

    void OnRegisterUpdates(RED4ext::UpdateRegistrar* aRegistrar) override
    {
        // Register a per-frame callback to drive split aim logic.
        aRegistrar->RegisterUpdate(
            RED4ext::UpdateTickGroup::PlayerAimUpdate,
            this,
            kUpdateName,
            [this](RED4ext::FrameInfo& aFrame, RED4ext::JobQueue& aJobQueue)
            {
                OnUpdate(aFrame, aJobQueue);
            });

        if (g_Sdk)
        {
            g_Sdk->logger->Trace(g_pluginHandle, "AimSplitSystem update registered");
        }
    }

private:
    void OnUpdate(RED4ext::FrameInfo& aFrame, RED4ext::JobQueue& aJobQueue)
    {
        RED4EXT_UNUSED_PARAMETER(aFrame);
        RED4EXT_UNUSED_PARAMETER(aJobQueue);

        // TODO: drive camera/aim split each frame using m_shootMode.
        if (g_Sdk && m_updateLogCount < 3)
        {
            g_Sdk->logger->TraceF(g_pluginHandle, "AimSplit OnUpdate tick; mode=%s", m_shootMode ? "Shoot" : "Look");
            ++m_updateLogCount;
        }
    }

    inline static AimSplitSystem* s_instance = nullptr;
    bool m_shootMode{false};
    uint32_t m_updateLogCount{0};

    RTTI_IMPL_TYPEINFO(AimSplitSystem);
    RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(AimSplitSystem, {
    RTTI_PARENT(Red::game::IGameSystem);
});

void AimSplit_OnAction()
{
    if (auto* system = AimSplitSystem::Get())
    {
        system->ToggleMode();
    }
    else if (g_Sdk)
    {
        g_Sdk->logger->Trace(g_pluginHandle, "AimSplit_OnAction called but system instance is null");
    }
}

RTTI_DEFINE_GLOBALS({
    RTTI_FUNCTION(AimSplit_OnAction);
});

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::PluginHandle aHandle, RED4ext::EMainReason aReason,
                                        const RED4ext::Sdk* aSdk)
{
    switch (aReason)
    {
    case RED4ext::EMainReason::Load:
    {
        g_Sdk = aSdk;
        g_pluginHandle = aHandle;

        aSdk->logger->Trace(aHandle, "Loading AimSplit mod and registering updates.");
        /*
         * Here you can register your custom functions, initalize variable, create hooks and so on.
         *
         * Be sure to store the plugin handle and the interface because you cannot get it again later. The plugin handle
         * is what identify your plugin through the extender.
         *
         * Returning "true" in this function loads the plugin, returning "false" will unload it and "Main" will be
         * called with "Unload" reason.
         */
        Red::TypeInfoRegistrar::RegisterDiscovered();
        break;
    }
    case RED4ext::EMainReason::Unload:
    {
        /*
         * Here you can free resources you allocated during initalization or during the time your plugin was executed.
         */
        break;
    }
    }

    /*
     * For more information about this function see https://docs.red4ext.com/mod-developers/creating-a-plugin#main.
     */

    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::PluginInfo* aInfo)
{
    /*
     * This function supply the necessary information about your plugin, like name, version, support runtime and SDK. DO
     * NOT do anything here yet!
     *
     * You MUST have this function!
     *
     * Make sure to fill all of the fields here in order to load your plugin correctly.
     *
     * Runtime version is the game's version, it is best to let it set to "RED4EXT_RUNTIME_LATEST" if you want to target
     * the latest game's version that the SDK defined, if the runtime version specified here and the game's version do
     * not match, your plugin will not be loaded. If you want to use RED4ext only as a loader and you do not care about
     * game's version use "RED4EXT_RUNTIME_INDEPENDENT".
     *
     * For more information about this function see https://docs.red4ext.com/mod-developers/creating-a-plugin#query.
     */

    aInfo->name = L"RED4ext.Example.VisualStudio";
    aInfo->author = L"WopsS";
    aInfo->version = RED4EXT_SEMVER(1, 0, 0);
    aInfo->runtime = RED4EXT_RUNTIME_LATEST;
    aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    /*
     * This functions returns only what API version is support by your plugins.
     * You MUST have this function!
     *
     * For more information about this function see https://docs.red4ext.com/mod-developers/creating-a-plugin#supports.
     */
    return RED4EXT_API_VERSION_LATEST;
}
