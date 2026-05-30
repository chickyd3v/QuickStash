#pragma once

#include "../config/Settings.h"
#include "sdk/PluginSDK.h"

#include <cstring>
#include <optional>
#include <string>

namespace QuickStashGame {

inline std::optional<PluginSDK::Inventory> FindMainInventory(
    const PluginSDK::Context* ctx) {
    if (!ctx) return std::nullopt;
    static const char* kNames[] = {
        "MainInventory1", "Main Inventory", "Backpack", "Player Inventory"};
    for (const char* want : kNames) {
        for (const auto& inv : ctx->Inventory.GetAll()) {
            const char* name = ctx->Inventory.GetName(inv.InventoryId);
            if (name && strcmp(name, want) == 0 && inv.Grid.Valid)
                return inv;
        }
    }
    for (const auto& inv : ctx->Inventory.GetAll()) {
        const char* name = ctx->Inventory.GetName(inv.InventoryId);
        if (!inv.Grid.Valid || !name) continue;
        if (inv.TotalBoxesX == QuickStashConfig::kGridCols
            && inv.TotalBoxesY == QuickStashConfig::kGridRows)
            return inv;
    }
    return std::nullopt;
}

inline bool IsInventoryOpen(const PluginSDK::Context* ctx) {
    auto inv = FindMainInventory(ctx);
    return inv.has_value() && inv->Grid.Valid;
}

} // namespace QuickStashGame
