#pragma once

#include "../config/Settings.h"
#include <imgui.h>

namespace QuickStashUi {

inline void DrawExclusionGrid(QuickStashConfig::Settings& settings) {
    ImGui::Text("Excluded cells (click to toggle):");
    ImGui::TextDisabled("Columns = X (0=left), Rows = Y (0=top)");

    if (ImGui::Button("Clear all")) {
        settings.ClearIgnoredCells();
    }
    ImGui::SameLine();
    if (ImGui::Button("Weapon column (col 0)")) {
        settings.SetWeaponColumnExcluded();
    }

    const float cellSize = 22.f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    for (int y = 0; y < QuickStashConfig::kGridRows; ++y) {
        for (int x = 0; x < QuickStashConfig::kGridCols; ++x) {
            ImVec2 p0(origin.x + x * cellSize, origin.y + y * cellSize);
            ImVec2 p1(p0.x + cellSize - 2.f, p0.y + cellSize - 2.f);
            bool ignored = settings.ignoredCells[static_cast<size_t>(y)][static_cast<size_t>(x)];
            ImU32 fill = ignored ? IM_COL32(180, 60, 60, 220) : IM_COL32(60, 120, 60, 180);
            dl->AddRectFilled(p0, p1, fill);
            dl->AddRect(p0, p1, IM_COL32(120, 120, 120, 255));

            ImGui::SetCursorScreenPos(p0);
            char id[32];
            snprintf(id, sizeof(id), "##ex_%d_%d", x, y);
            ImGui::InvisibleButton(id, ImVec2(cellSize - 2.f, cellSize - 2.f));
            if (ImGui::IsItemClicked())
                settings.ignoredCells[static_cast<size_t>(y)][static_cast<size_t>(x)] = !ignored;
        }
    }

    ImGui::SetCursorScreenPos(
        ImVec2(origin.x, origin.y + QuickStashConfig::kGridRows * cellSize + 4.f));
    ImGui::Dummy(ImVec2(QuickStashConfig::kGridCols * cellSize,
                        QuickStashConfig::kGridRows * cellSize));
}

} // namespace QuickStashUi
