// ==WindhawkMod==
// @id              custom-corner-radius
// @name            Custom Window Corner Radius
// @description     Increases window corner radius beyond the default 8px in Windows 11
// @version         1.0.7
// @author          custom
// @include         dwm.exe
// @architecture    x86-64
// ==/WindhawkMod==

// ==WindhawkModSettings==
/*
- radius: 15
  $name: Corner Radius
  $description: Corner radius in pixels (default Win11 is 8, try 10-16 for more rounded)
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>
#include <windows.h>

struct {
    int radius;
} settings;

typedef float (*FloatFunc_t)(void* pThis);

FloatFunc_t GetRadiusFromCornerStyle_orig = nullptr;
FloatFunc_t GetFloatCornerRadiusForCurrentStyle_orig = nullptr;
FloatFunc_t GetDpiAdjustedFloatCornerRadius_orig = nullptr;

// Default Win11 radius values
#define DEFAULT_LARGE_RADIUS 8.0f
#define DEFAULT_SMALL_RADIUS 4.0f
#define EPSILON 0.5f

float GetRadiusFromCornerStyle_hook(void* pThis) {
    float orig = GetRadiusFromCornerStyle_orig(pThis);
    // Only override if it's the large radius (regular windows)
    // Leave small radius (popups/flyouts = 4px) unchanged
    if (orig >= DEFAULT_LARGE_RADIUS - EPSILON) {
        return (float)settings.radius;
    }
    return orig;
}

float GetFloatCornerRadius_hook(void* pThis) {
    float orig = GetFloatCornerRadiusForCurrentStyle_orig(pThis);
    if (orig >= DEFAULT_LARGE_RADIUS - EPSILON) {
        return (float)settings.radius;
    }
    return orig;
}

float GetDpiAdjustedFloatCornerRadius_hook(void* pThis) {
    float orig = GetDpiAdjustedFloatCornerRadius_orig(pThis);
    // DPI adjusted 8px at 115% = ~9.2, DPI adjusted 4px = ~4.6
    // Threshold at ~6.0 to distinguish large vs small
    if (orig >= 6.0f) {
        // Scale proportionally: new_radius / 8.0 * orig
        float scale = (float)settings.radius / DEFAULT_LARGE_RADIUS;
        return orig * scale;
    }
    return orig; // leave popup/flyout shadow alone
}

void LoadSettings() {
    settings.radius = Wh_GetIntSetting(L"radius");
    if (settings.radius < 1) settings.radius = 8;
    if (settings.radius > 60) settings.radius = 60;
}

BOOL Wh_ModInit() {
    LoadSettings();
    Wh_Log(L"Target radius: %d", settings.radius);

    HMODULE hUDWM = GetModuleHandleW(L"uDWM.dll");
    if (!hUDWM) {
        Wh_Log(L"uDWM.dll not found");
        return FALSE;
    }

    WH_FIND_SYMBOL_OPTIONS opts = { sizeof(opts) };
    WH_FIND_SYMBOL sym = {};

    void* addr_1 = nullptr;
    void* addr_2 = nullptr;
    void* addr_3 = nullptr;

    HANDLE hFind = Wh_FindFirstSymbol(hUDWM, &opts, &sym);
    if (hFind) {
        do {
            if (sym.symbol) {
                if (wcsstr(sym.symbol, L"GetRadiusFromCornerStyle"))
                    addr_1 = sym.address;
                else if (wcsstr(sym.symbol, L"GetFloatCornerRadiusForCurrentStyle"))
                    addr_2 = sym.address;
                else if (wcsstr(sym.symbol, L"GetDpiAdjustedFloatCornerRadius"))
                    addr_3 = sym.address;
            }
        } while (Wh_FindNextSymbol(hFind, &sym));
        Wh_FindCloseSymbol(hFind);
    }

    int hooked = 0;

    if (addr_1 && Wh_SetFunctionHook(addr_1, (void*)GetRadiusFromCornerStyle_hook, (void**)&GetRadiusFromCornerStyle_orig)) {
        Wh_Log(L"Hooked GetRadiusFromCornerStyle"); hooked++;
    }
    if (addr_2 && Wh_SetFunctionHook(addr_2, (void*)GetFloatCornerRadius_hook, (void**)&GetFloatCornerRadiusForCurrentStyle_orig)) {
        Wh_Log(L"Hooked GetFloatCornerRadiusForCurrentStyle"); hooked++;
    }
    if (addr_3 && Wh_SetFunctionHook(addr_3, (void*)GetDpiAdjustedFloatCornerRadius_hook, (void**)&GetDpiAdjustedFloatCornerRadius_orig)) {
        Wh_Log(L"Hooked GetDpiAdjustedFloatCornerRadius"); hooked++;
    }

    Wh_Log(L"Hooked %d functions. Radius: %d", hooked, settings.radius);
    return hooked > 0 ? TRUE : FALSE;
}

void Wh_ModSettingsChanged() {
    LoadSettings();
    Wh_Log(L"Settings updated. New radius: %d", settings.radius);
}

void Wh_ModUninit() {
    Wh_Log(L"Unloaded.");
}