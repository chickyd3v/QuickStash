// Quick Stash — PoeFixer plugin (SDK v6)
// Ctrl+click transfer from main inventory when the backpack is open.

#include "sdk/PluginSDK.h"

#include "config/Settings.h"
#include "game/PanelDetector.h"
#include "game/TransferState.h"
#include "overlay/TransferButtonOverlay.h"
#include "ui/ExclusionGrid.h"

#include <imgui.h>
#include <chrono>
#include <optional>

class QuickStashPlugin : public PluginSDK::Plugin {
public:
    const char* GetName() const override { return "Quick Stash"; }

    bool WantsOverlay() const override { return m_settings.enabled; }

    void OnEnable(bool /*isGameAttached*/) override {
        if (ctx()->ImGuiContext)
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));

        m_settings.Load(DirectoryPath());
        m_lastScan = std::chrono::steady_clock::now();

        auto& events = const_cast<PluginSDK::EventsService&>(ctx()->Events);
        m_frameToken = events.OnFrame([this] { OnFrameTick(); });

        ctx()->Log.Info("Quick Stash plugin enabled");
    }

    void OnDisable() override {
        m_transfer.Abort();
        if (m_frameToken.Valid()) {
            auto& events = const_cast<PluginSDK::EventsService&>(ctx()->Events);
            events.Unsubscribe(m_frameToken);
            m_frameToken = {};
        }
        m_overlayCapturePending = false;
        ctx()->Overlay.SetWantsOverlayInput(false);
        SaveSettings();
        ctx()->Log.Info("Quick Stash plugin disabled");
    }

    void DrawSettings() override {
        if (ctx()->ImGuiContext)
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));

        ImGui::Checkbox("Enable Quick Stash", &m_settings.enabled);
        ImGui::Separator();

        ImGui::SliderInt("Click delay (ms)", &m_settings.clickDelayMs, 10, 500);
        ImGui::SliderInt("Post-click delay (ms)", &m_settings.postClickDelayMs, 0, 300);
        ImGui::SliderInt("Cursor settle (ms)", &m_settings.cursorSettleMs, 0, 100);
        ImGui::SliderInt("Hold Ctrl after last click (ms)", &m_settings.completionHoldMs, 0, 500);
        ImGui::SliderFloat("Button offset X", &m_settings.buttonOffsetX, -40.f, 80.f, "%.0f");
        ImGui::SliderFloat("Button offset Y", &m_settings.buttonOffsetY, -40.f, 80.f, "%.0f");
        ImGui::Checkbox("Cancel on right-click", &m_settings.cancelOnRightClick);
        ImGui::Checkbox("Stop if inventory closes", &m_settings.verifyPanelsOpen);

        ImGui::Separator();
        ImGui::TextWrapped(
            "Shows a Transfer button whenever your inventory grid is open. "
            "Open stash, vendor, trade, etc. first, then click Transfer.");

        ImGui::Separator();
        QuickStashUi::DrawExclusionGrid(m_settings);
    }

    void DrawUI() override {
        if (!m_settings.enabled) return;
        if (!ctx()->Game.IsInGame()) return;
        if (ctx()->ImGuiContext)
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));

        if (!ctx()->Game.GetSnapshot().GameWindowForeground) {
            m_overlayCapturePending = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            return;
        }

        RefreshInventoryIfNeeded();

        if (m_transfer.IsRunning()) {
            m_overlayCapturePending = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            QuickStashOverlay::DrawTransferProgress(m_transfer.ProgressIndex(),
                                                    m_transfer.ProgressTotal());
            return;
        }

        if (!QuickStashGame::IsInventoryOpen(ctx()) || !m_backpack || !m_backpack->Grid.Valid) {
            m_overlayCapturePending = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            return;
        }

        const bool mouseOverBtn =
            QuickStashOverlay::IsMouseOverTransferButton(*m_backpack, m_settings);
        m_overlayCapturePending = mouseOverBtn;
        ctx()->Overlay.SetWantsOverlayInput(mouseOverBtn || m_overlayCaptureApplied);

        const auto btn = QuickStashOverlay::DrawTransferButton(*m_backpack, m_settings);

        bool activated = QuickStashOverlay::TransferButtonActivated(btn);
        if (!activated && (mouseOverBtn || btn.hovered))
            activated = QuickStashOverlay::Win32LeftClickOnRect(btn.btnP0, btn.btnP1);

        if (activated) {
            m_overlayCapturePending = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            m_transfer.Start(ctx(), m_settings, *m_backpack);
        }
    }

    void SaveSettings() override { m_settings.Save(DirectoryPath()); }

private:
    QuickStashConfig::Settings m_settings;
    QuickStashGame::TransferState m_transfer;
    PluginSDK::EventsService::Token m_frameToken{};
    std::optional<PluginSDK::Inventory> m_backpack;
    std::chrono::steady_clock::time_point m_lastScan{};
    bool m_overlayCapturePending = false;
    bool m_overlayCaptureApplied = false;

    void RefreshInventoryIfNeeded() {
        const auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastScan).count() < 150)
            return;
        m_lastScan = now;
        ctx()->Inventory.Scan(-1);
        m_backpack = QuickStashGame::FindMainInventory(ctx());
    }

    void OnFrameTick() {
        if (m_transfer.IsRunning()) {
            m_overlayCapturePending = false;
            m_overlayCaptureApplied = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            RefreshInventoryIfNeeded();
            m_transfer.Tick(ctx());
            return;
        }

        if (!m_settings.enabled || !ctx()->Game.IsInGame()) {
            m_overlayCapturePending = false;
            m_overlayCaptureApplied = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            return;
        }

        if (!ctx()->Game.GetSnapshot().GameWindowForeground) {
            m_overlayCapturePending = false;
            m_overlayCaptureApplied = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            return;
        }

        RefreshInventoryIfNeeded();
        if (!QuickStashGame::IsInventoryOpen(ctx()) || !m_backpack || !m_backpack->Grid.Valid) {
            m_overlayCapturePending = false;
            m_overlayCaptureApplied = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            return;
        }

        m_overlayCaptureApplied = m_overlayCapturePending;
        ctx()->Overlay.SetWantsOverlayInput(m_overlayCaptureApplied);
    }
};

extern "C" PLUGIN_API PluginSDK::Plugin* CreatePlugin() { return new QuickStashPlugin(); }

extern "C" PLUGIN_API void DestroyPlugin(PluginSDK::Plugin* p) { delete p; }
