#pragma once

#include "../third_party/json.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>

namespace QuickStashConfig {

inline constexpr int kGridCols = 12;
inline constexpr int kGridRows = 5;

struct Settings {
    bool enabled = true;
    std::array<std::array<bool, kGridCols>, kGridRows> ignoredCells{};
    int  clickDelayMs = 75;
    int  postClickDelayMs = 50;
    int  cursorSettleMs = 20;
    int  completionHoldMs = 125;
    bool cancelOnRightClick = true;
    bool verifyPanelsOpen = true;
    float buttonOffsetX = 0.f;
    float buttonOffsetY = 0.f;

    Settings() { SetWeaponColumnExcluded(); }

    void SetWeaponColumnExcluded() {
        for (int y = 0; y < kGridRows; ++y)
            ignoredCells[static_cast<size_t>(y)][0] = true;
    }

    void ClearIgnoredCells() {
        for (auto& row : ignoredCells)
            row.fill(false);
    }

    bool IsCellIgnored(int x, int y) const {
        if (x < 0 || x >= kGridCols || y < 0 || y >= kGridRows) return true;
        return ignoredCells[static_cast<size_t>(y)][static_cast<size_t>(x)];
    }

    std::filesystem::path SettingsPath(const std::filesystem::path& pluginDir) const {
        return pluginDir / "config" / "settings.json";
    }

    void Load(const std::filesystem::path& pluginDir) {
        const auto path = SettingsPath(pluginDir);
        if (!std::filesystem::exists(path)) return;
        std::ifstream in(path);
        if (!in.is_open()) return;
        nlohmann::json j;
        in >> j;
        enabled = j.value("enabled", enabled);
        clickDelayMs = std::clamp(j.value("click_delay_ms", clickDelayMs), 10, 2000);
        postClickDelayMs = std::clamp(j.value("post_click_delay_ms", postClickDelayMs), 0, 500);
        cursorSettleMs = std::clamp(j.value("cursor_settle_ms", cursorSettleMs), 0, 200);
        completionHoldMs = std::clamp(j.value("completion_hold_ms", completionHoldMs), 0, 1000);
        cancelOnRightClick = j.value("cancel_on_right_click", cancelOnRightClick);
        verifyPanelsOpen = j.value("verify_panels_open", verifyPanelsOpen);
        buttonOffsetX = j.value("button_offset_x", buttonOffsetX);
        buttonOffsetY = j.value("button_offset_y", buttonOffsetY);

        if (j.contains("ignored_cells") && j["ignored_cells"].is_array()) {
            const auto& rows = j["ignored_cells"];
            for (int y = 0; y < kGridRows && y < static_cast<int>(rows.size()); ++y) {
                if (!rows[y].is_array()) continue;
                for (int x = 0; x < kGridCols && x < static_cast<int>(rows[y].size()); ++x)
                    ignoredCells[static_cast<size_t>(y)][static_cast<size_t>(x)] =
                        rows[y][x].get<bool>();
            }
        }
    }

    void Save(const std::filesystem::path& pluginDir) const {
        std::error_code ec;
        std::filesystem::create_directories(pluginDir / "config", ec);

        nlohmann::json j;
        j["enabled"] = enabled;
        j["click_delay_ms"] = clickDelayMs;
        j["post_click_delay_ms"] = postClickDelayMs;
        j["cursor_settle_ms"] = cursorSettleMs;
        j["completion_hold_ms"] = completionHoldMs;
        j["cancel_on_right_click"] = cancelOnRightClick;
        j["verify_panels_open"] = verifyPanelsOpen;
        j["button_offset_x"] = buttonOffsetX;
        j["button_offset_y"] = buttonOffsetY;

        nlohmann::json rows = nlohmann::json::array();
        for (int y = 0; y < kGridRows; ++y) {
            nlohmann::json row = nlohmann::json::array();
            for (int x = 0; x < kGridCols; ++x)
                row.push_back(ignoredCells[static_cast<size_t>(y)][static_cast<size_t>(x)]);
            rows.push_back(std::move(row));
        }
        j["ignored_cells"] = std::move(rows);

        std::ofstream out(SettingsPath(pluginDir));
        if (out.is_open())
            out << j.dump(2);
    }
};

} // namespace QuickStashConfig
