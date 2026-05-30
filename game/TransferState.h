#pragma once

#include "../config/Settings.h"
#include "../input/Win32Input.h"
#include "PanelDetector.h"
#include "TransferPlanner.h"
#include "sdk/PluginSDK.h"

#include <chrono>
#include <vector>

namespace QuickStashGame {

class TransferState {
public:
    bool IsRunning() const { return m_running || m_finishing; }
    int  ProgressIndex() const { return m_index; }
    int  ProgressTotal() const { return static_cast<int>(m_queue.size()); }

    void Start(const PluginSDK::Context* ctx, const QuickStashConfig::Settings& settings,
               const PluginSDK::Inventory& inv) {
        if (!ctx || m_running || m_finishing) return;
        m_settings = settings;
        m_queue = BuildClickQueue(inv, settings);
        m_index = 0;
        m_ctrlHeld = false;
        m_finishing = false;
        m_running = !m_queue.empty();
        m_lastClick = std::chrono::steady_clock::now();
        if (m_running) {
            QuickStashInput::CtrlDown();
            m_ctrlHeld = true;
            ctx->Log.Info(("Quick Stash: transferring " + std::to_string(m_queue.size())
                           + " items").c_str());
        }
    }

    void Abort() {
        if (m_ctrlHeld) {
            QuickStashInput::CtrlUp();
            m_ctrlHeld = false;
        }
        m_running = false;
        m_finishing = false;
        m_queue.clear();
        m_index = 0;
    }

    void Tick(const PluginSDK::Context* ctx) {
        if (!ctx) return;

        if (m_finishing) {
            TickFinishing(ctx);
            return;
        }
        if (!m_running) return;

        if (m_settings.cancelOnRightClick && QuickStashInput::IsRightMouseDown()) {
            ctx->Log.Info("Quick Stash: cancelled (right mouse)");
            Abort();
            return;
        }

        if (m_settings.verifyPanelsOpen && !IsInventoryOpen(ctx)) {
            ctx->Log.Info("Quick Stash: cancelled (inventory closed)");
            Abort();
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_lastClick).count();
        if (elapsed < m_settings.clickDelayMs)
            return;

        if (m_index >= static_cast<int>(m_queue.size())) {
            BeginFinishing(now);
            return;
        }

        const auto& target = m_queue[static_cast<size_t>(m_index)];
        QuickStashInput::MoveCursorScreen(
            static_cast<int>(target.screenX + 0.5f),
            static_cast<int>(target.screenY + 0.5f));
        if (m_settings.cursorSettleMs > 0)
            QuickStashInput::SleepMs(m_settings.cursorSettleMs);

        QuickStashInput::LeftClickAtCursor();
        if (m_settings.postClickDelayMs > 0)
            QuickStashInput::SleepMs(m_settings.postClickDelayMs);

        ++m_index;
        m_lastClick = std::chrono::steady_clock::now();

        if (m_index >= static_cast<int>(m_queue.size()))
            BeginFinishing(m_lastClick);
    }

private:
    void BeginFinishing(std::chrono::steady_clock::time_point from) {
        m_running = false;
        m_finishing = true;
        const int hold = m_settings.completionHoldMs > 0
                             ? m_settings.completionHoldMs
                             : m_settings.postClickDelayMs + m_settings.clickDelayMs;
        m_finishAt = from + std::chrono::milliseconds(hold);
    }

    void TickFinishing(const PluginSDK::Context* ctx) {
        if (std::chrono::steady_clock::now() < m_finishAt)
            return;
        ctx->Log.Info("Quick Stash: transfer complete");
        Abort();
    }

    bool m_running = false;
    bool m_finishing = false;
    bool m_ctrlHeld = false;
    int  m_index = 0;
    QuickStashConfig::Settings m_settings{};
    std::vector<ClickTarget> m_queue;
    std::chrono::steady_clock::time_point m_lastClick{};
    std::chrono::steady_clock::time_point m_finishAt{};
};

} // namespace QuickStashGame
