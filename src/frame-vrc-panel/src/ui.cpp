// Copyright (c) 2026 FrameClash contributors. MIT License.
// Layout and helpers follow frameclash-panel (sidebar + cards). Colors: palette.h. The drawing helpers (Pen, fonts,
// text layout) come from frameeyeosc.
#include "ui.h"

#include "palette.h"
#include "third_party_frameeyeosc/text_layout.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

constexpr double kSideW = 180;
constexpr double kX0 = 200, kX1 = kPanelWidth - 20, kCW = kX1 - kX0;  // content area
constexpr double kTop = 76;                                           // cards start (the page title is above)
constexpr double kFootY = 640, kFootH = 44;
constexpr double kGap = 16;
constexpr double kHalfW = (kCW - kGap) / 2;
constexpr double kEnvRowH = 40, kEnvHeadH = 34, kEnvBottom = kFootY - 12;
constexpr int64_t kArmMs = 4000;

/** Button text for a doctor fix. */
std::string fixLabel(const std::string& fix, bool ja) {
    if (fix == "install-deps") return ja ? "Steam でインストール" : "Install in Steam";
    if (fix == "fex eac") return ja ? "eac 版に切り替え" : "Switch to eac";
    if (fix == "proton dfr") return ja ? "DFR 版 Proton を用意" : "Set up DFR Proton";
    if (fix == "proton stock") return ja ? "Proton Exp. を使う" : "Use Proton Exp.";
    if (fix == "launch-options set") return ja ? "起動オプションを設定" : "Set launch options";
    if (fix == "setup") return ja ? "まとめて設定" : "Set up everything";
    return fix;
}

/** doctor's group names (the row texts come from the CLI in English) */
std::string groupLabel(const std::string& g, bool ja) {
    if (!ja) return g;
    if (g == "device") return "デバイス";
    if (g == "steam") return "Steam";
    return g;
}

Color levelColor(const std::string& level) {
    return level == "ok" ? cGreen : level == "warn" ? cAmber : cRed;
}

}  // namespace

int countLevel(const Snapshot& s, const std::string& level) {
    return static_cast<int>(std::count_if(s.checks.begin(), s.checks.end(),
                                          [&](const Check& c) { return c.level == level; }));
}

PanelUi::PanelUi(Model& model, const FontSet& fonts) : model_(model), fonts_(fonts) {}

void PanelUi::addHit(const std::string& id, double x, double y, double w, double h, bool enabled) {
    hits_.push_back({id, x, y, w, h, enabled});
}

const Hit* PanelUi::hitAt(double x, double y) const {
    for (auto it = hits_.rbegin(); it != hits_.rend(); ++it)
        if (x >= it->x && x < it->x + it->w && y >= it->y && y < it->y + it->h) return &*it;
    return nullptr;
}

void PanelUi::button(const Pen& pen, const std::string& id, double x, double y, double w, double h,
                     const std::string& label, bool enabled, bool selected, double size) {
    const bool hot = enabled && (hover_ == id || pressed_ == id);
    const bool armed = isArmed(id);
    Color f = !enabled ? cCtl : armed ? cRedTint : selected ? cAccentTint : cCtl;
    if (hot) f = blendColor(hexColor(0xffffff), f, pressed_ == id ? 0.18 : 0.08);
    pen.color(f);
    pen.roundedRect(x, y, w, h, 8);
    cairo_fill(pen.cr);
    if (selected) {
        pen.color(cAccent);
        cairo_set_line_width(pen.cr, 2);
        pen.roundedRect(x + 1, y + 1, w - 2, h - 2, 8);
        cairo_stroke(pen.cr);
    }
    const bool restarts = (id == "run:setup" || id == "run:proton dfr") && last_.status.proton != "dfr";
    const std::string text = !armed ? label
                             : restarts ? tr("SteamVR restarts – tap again", "SteamVR 再起動 – もう一度")
                                        : tr("Tap again to confirm", "もう一度タップで実行");
    const std::string fit = ellipsize(pen, text, size, true, w - 20, false);
    const double tw = pen.measure(fit, size, true);
    const Color tc = !enabled ? cDim : armed ? cRed : selected ? cAccent : cText;
    pen.text(x + (w - tw) / 2, y + h / 2 + size * 0.36, fit, size, tc, true);
    addHit(id, x, y, w, h, enabled);
}

void PanelUi::mainButton(const Pen& pen, const std::string& id, double x, double y, double w, double h,
                         const std::string& label, bool enabled, double size) {
    const bool hot = enabled && (hover_ == id || pressed_ == id);
    const bool armed = isArmed(id);
    Color f = !enabled ? cCtl : armed ? cAmber : cAccent;
    if (hot) f = blendColor(hexColor(0xffffff), f, pressed_ == id ? 0.25 : 0.12);
    pen.color(f);
    pen.roundedRect(x, y, w, h, 10);
    cairo_fill(pen.cr);
    const std::string text = !armed ? label
                             : id == "run:setup" && last_.status.proton != "dfr"
                                 ? tr("SteamVR restarts once – tap again", "SteamVR が 1 回再起動 – もう一度タップ")
                                 : tr("Tap again to confirm", "もう一度タップで実行");
    const std::string fit = ellipsize(pen, text, size, true, w - 24, false);
    const double tw = pen.measure(fit, size, true);
    pen.text(x + (w - tw) / 2, y + h / 2 + size * 0.36, fit, size, enabled ? cOnAccent : cDim, true);
    addHit(id, x, y, w, h, enabled);
}

void PanelUi::card(const Pen& pen, double x, double y, double w, double h, const std::string& label) {
    pen.color(cCard);
    pen.roundedRect(x, y, w, h, 12);
    cairo_fill(pen.cr);
    pen.color(cLine);
    cairo_set_line_width(pen.cr, 1);
    pen.roundedRect(x + 0.5, y + 0.5, w - 1, h - 1, 12);
    cairo_stroke(pen.cr);
    if (!label.empty()) pen.text(x + 20, y + 34, label, 17, cMuted, true);
}

void PanelUi::hint(const Pen& pen, double x, double y, double w, const std::string& text, int lines) {
    for (const std::string& line : wrapText(pen, text, 15, false, w, static_cast<size_t>(lines))) {
        pen.text(x, y, line, 15, cMuted);
        y += 21;
    }
}

bool PanelUi::needsConfirm(const std::string& id) const {
    if (id == "run:uninstall" || id == "run:uninstall --purge") return true;
    // the first DFR Proton restarts Steam (and its VR dashboard) once
    if ((id == "run:setup" || id == "run:proton dfr") && last_.status.proton != "dfr") return true;
    // replaces options the user wrote (kept, and put back by "clear")
    return (id == "run:launch-options set" || id == "run:setup") && last_.status.launch == "other";
}

bool PanelUi::isArmed(const std::string& id) const {
    return armed_ == id && nowMs() - armedAt_ < kArmMs;
}

std::string PanelUi::setting(const Snapshot& s, const std::string& key, const std::string& current) const {
    const auto it = s.pending.find(key);
    return it != s.pending.end() ? it->second : current;
}

int PanelUi::settingInt(const Snapshot& s, const std::string& key, int current) const {
    const auto it = s.pending.find(key);
    return it != s.pending.end() ? std::atoi(it->second.c_str()) : current;
}

bool PanelUi::canRun(const std::string& args) const {
    if (last_.running) return false;
    const bool swaps = args.rfind("fex", 0) == 0 || args.rfind("proton", 0) == 0 || args == "setup" ||
                       args.rfind("uninstall", 0) == 0 || args == "update";
    return !(swaps && last_.status.gameRunning);
}

void PanelUi::draw(cairo_t* cr, const Snapshot& s) {
    last_ = s;
    hits_.clear();
    const Pen pen {cr, &fonts_};
    pen.color(cBg);
    cairo_paint(cr);
    drawSidebar(pen, s);
    const std::string titles[] = {tr("Home", "ホーム"), tr("Environment", "環境チェック"), tr("Advanced", "詳細設定"),
                                  tr("Monitor", "モニター"), tr("Log", "ログ")};
    pen.text(kX0, 52, titles[static_cast<int>(view_)], 28, cText, true);
    switch (view_) {
        case View::Home: drawHome(pen, s); break;
        case View::Environment: drawEnvironment(pen, s); break;
        case View::Advanced: drawAdvanced(pen, s); break;
        case View::Monitor: drawMonitor(pen, s); break;
        case View::Log: drawLog(pen, s); break;
    }
    drawFooter(pen, s);
}

void PanelUi::drawSidebar(const Pen& pen, const Snapshot& s) {
    pen.color(cSide);
    cairo_rectangle(pen.cr, 0, 0, kSideW, kPanelHeight);
    cairo_fill(pen.cr);
    pen.color(cLine);
    cairo_rectangle(pen.cr, kSideW - 1, 0, 1, kPanelHeight);
    cairo_fill(pen.cr);
    pen.color(cAccent);
    pen.roundedRect(22, 33, 14, 14, 3);
    cairo_fill(pen.cr);
    pen.text(44, 48, "frame-vrc", 21, cText, true);
    const struct {
        View v;
        const char* id;
        std::string label;
    } items[] = {{View::Home, "view:home", tr("Home", "ホーム")},
                 {View::Environment, "view:env", tr("Environment", "環境チェック")},
                 {View::Advanced, "view:advanced", tr("Advanced", "詳細設定")},
                 {View::Monitor, "view:monitor", tr("Monitor", "モニター")},
                 {View::Log, "view:log", tr("Log", "ログ")}};
    const int fails = countLevel(s, "fail");
    double y = 92;
    for (const auto& it : items) {
        const bool sel = view_ == it.v;
        const bool hot = hover_ == it.id || pressed_ == it.id;
        if (sel || hot) {
            pen.color(sel ? cAccentTint : cCtl);
            pen.roundedRect(12, y, kSideW - 24, 50, 8);
            cairo_fill(pen.cr);
        }
        if (sel) {
            pen.color(cAccent);
            pen.roundedRect(12, y + 12, 4, 26, 2);
            cairo_fill(pen.cr);
        }
        pen.text(32, y + 32, it.label, 18, sel ? cAccent : cText, sel);
        if (it.v == View::Environment && fails > 0) pen.dot(160, y + 25, cRed);
        if (it.v == View::Log && s.running) pen.dot(160, y + 25, cAccent);
        addHit(it.id, 12, y, kSideW - 24, 50, true);
        y += 58;
    }
    button(pen, "lang", 12, kPanelHeight - 100, kSideW - 24, 40, ja_ ? "English" : "日本語", true, false, 17);
    pen.text(22, kPanelHeight - 28, "v" + (s.status.version.empty() ? std::string(FRAME_VRC_PANEL_VERSION) : s.status.version),
             15, cMuted);
}

void PanelUi::drawHome(const Pen& pen, const Snapshot& s) {
    const Status& st = s.status;
    // ---- what is set now ----
    double y = kTop;
    card(pen, kX0, y, kHalfW, 236, tr("Now", "現在の設定"));
    struct Row {
        std::string label, value;
        Color color;
    };
    const std::string fexValue = st.fex == "eac" ? tr("patched (eac)", "パッチ版 (eac)")
                                 : st.fex == "valve" ? tr("Valve's build", "Valve 版") : st.fex;
    const std::string launch = st.launch == "set" ? tr("set by frame-vrc", "frame-vrc が設定済み")
                               : st.launch == "stale" ? tr("out of date", "古い")
                               : st.launch == "other" ? tr("your own", "ユーザー独自") : tr("not set", "未設定");
    const int fails = countLevel(s, "fail"), warns = countLevel(s, "warn");
    std::string env = !s.checked ? tr("checking…", "確認中…")
                      : fails > 0 ? std::to_string(fails) + tr(" to fix", " 件要修正")
                      : warns > 0 ? std::to_string(warns) + tr(warns > 1 ? " warnings" : " warning", " 件の警告")
                      : tr("all good", "問題なし");
    std::string dfr = st.proton != "dfr" ? tr("off (stock Proton)", "オフ（通常の Proton）")
                      : st.dfr == "auto" ? (st.dfrEffective == "off" ? tr("auto (off now)", "自動（現在オフ）")
                                                                     : tr("auto (on now)", "自動（現在オン）"))
                      : st.dfr;
    const Row rows[] = {
        {"FEX", st.known ? fexValue : "—", st.fex == "eac" ? cGreen : cAmber},
        {"Proton", st.proton == "dfr" ? "Experimental + DFR" : "Experimental", cText},
        {tr("Resolution", "解像度"),
         st.resolution > 0 ? std::to_string(st.resolution) : tr("SteamVR did not answer", "SteamVR の応答なし"), cText},
        {tr("Foveated", "注視点レンダリング"), dfr, cText},
        {tr("Launch options", "起動オプション"), launch, st.launch == "set" ? cGreen : cAmber},
        {tr("Environment", "環境"), env, !s.checked ? cMuted : fails > 0 ? cRed : warns > 0 ? cAmber : cGreen},
    };
    double ry = y + 66;
    for (const Row& r : rows) {
        pen.text(kX0 + 20, ry, r.label, 17, cMuted);
        pen.text(kX0 + kHalfW - 20, ry, ellipsize(pen, r.value, 18, true, kHalfW - 190, false), 18,
                 st.known || &r == &rows[5] ? r.color : cMuted, true, true);
        ry += 28;
    }
    if (fails > 0) addHit("view:env", kX0, ry - 46, kHalfW, 30, true);

    // ---- the one-click setup ----
    const double rx = kX0 + kHalfW + kGap;
    card(pen, rx, y, kHalfW, 236, tr("Recommended setup", "おすすめ設定"));
    pen.text(rx + 20, y + 72, tr("1920 + foveated rendering", "1920 + 注視点レンダリング"), 24, cText, true);
    hint(pen, rx + 20, y + 102, kHalfW - 40,
         tr("Patched FEX, a Proton copy with eye-tracked DFR, resolution 1920 and the tested memory options. "
            "Downloads ~100 MB, copies Proton (1.5 GB), restarts SteamVR once.",
            "パッチ版 FEX、視線追跡 DFR 入り Proton の複製、解像度 1920、検証済みのメモリ設定。約 100 MB を"
            "ダウンロード、Proton を複製（1.5 GB）、SteamVR を 1 回再起動。"), 3);
    mainButton(pen, "run:setup", rx + 20, y + 168, kHalfW - 40, 50, tr("Set up everything", "まとめて設定"), canRun("setup"));

    // ---- resolution and DFR ----
    y += 236 + kGap;
    card(pen, kX0, y, kHalfW, 150, tr("Render resolution", "レンダリング解像度"));
    const struct {
        int res;
        std::string sub;
    } res[] = {{1056, tr("performance", "パフォーマンス")}, {1440, tr("balanced", "バランス")}, {1920, tr("quality", "画質")}};
    const double bw = (kHalfW - 40 - 20) / 3;
    const int resNow = settingInt(s, "resolution", st.resolution);
    const std::string dfrNow = setting(s, "dfr", st.dfr);
    for (int i = 0; i < 3; ++i) {
        const double bx = kX0 + 20 + i * (bw + 10);
        const std::string id = "set:resolution=" + std::to_string(res[i].res);
        button(pen, id, bx, y + 52, bw, 54, std::to_string(res[i].res), st.resolution > 0, resNow == res[i].res, 22);
        const double tw = pen.measure(res[i].sub, 14);
        pen.text(bx + (bw - tw) / 2, y + 128, res[i].sub, 14, cMuted);
    }
    card(pen, rx, y, kHalfW, 150, tr("Foveated rendering (DFR)", "注視点レンダリング (DFR)"));
    const struct {
        const char* level;
        std::string label;
    } lv[] = {{"auto", tr("Auto", "自動")}, {"off", tr("Off", "オフ")}, {"def", tr("On", "オン")}};
    for (int i = 0; i < 3; ++i) {
        const double bx = rx + 20 + i * (bw + 10);
        button(pen, std::string("set:dfr=") + lv[i].level, bx, y + 52, bw, 54, lv[i].label, true,
               dfrNow == lv[i].level, 20);
    }
    hint(pen, rx + 20, y + 128, kHalfW - 40,
         st.proton == "dfr" ? tr("Auto: on from 1440 up. Applies the next time VRChat starts.",
                                 "自動：1440 以上でオン。次に VRChat を起動したときから有効。")
                            : tr("Needs the DFR Proton: Set up everything, or Advanced.",
                                 "DFR 版 Proton が必要：まとめて設定、または詳細設定から。"), 1);

    // ---- play ----
    y += 150 + kGap;
    mainButton(pen, "run:launch", kX0, y, kCW, 70,
               st.gameRunning ? tr("VRChat is running", "VRChat 実行中") : tr("Launch VRChat", "VRChat を起動"), canRun("") && !st.gameRunning, 26);
}

void PanelUi::drawEnvironment(const Pen& pen, const Snapshot& s) {
    button(pen, "refresh", kX1 - 170, 22, 170, 40, s.refreshing ? tr("Checking…", "確認中…") : tr("Check again", "再チェック"), !s.refreshing);
    if (!s.checked) {
        card(pen, kX0, kTop, kCW, 120, "");
        pen.text(kX0 + 24, kTop + 66, s.error.empty() ? tr("Checking the environment…", "環境を確認中…") : s.error, 20, cMuted);
        return;
    }
    // flat list: group headers and rows
    struct Item {
        bool head;
        const Check* c;
        std::string group;
    };
    std::vector<Item> items;
    std::string group;
    for (const Check& c : s.checks) {
        if (c.group != group) items.push_back({true, nullptr, group = c.group});
        items.push_back({false, &c, ""});
    }
    // how many items fit from each offset decides the last offset
    double total = 0;
    int fitFromEnd = 0;
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        total += items[i].head ? kEnvHeadH : kEnvRowH;
        if (total > kEnvBottom - kTop) break;
        ++fitFromEnd;
    }
    maxEnvOffset_ = std::max(0, static_cast<int>(items.size()) - fitFromEnd);
    envOffset_ = std::min(envOffset_, maxEnvOffset_);
    double y = kTop;
    for (size_t i = envOffset_; i < items.size(); ++i) {
        const Item& it = items[i];
        const double h = it.head ? kEnvHeadH : kEnvRowH;
        if (y + h > kEnvBottom) break;
        if (it.head) {
            pen.text(kX0 + 4, y + 24, groupLabel(it.group, ja_), 16, cMuted, true);
        } else {
            const Check& c = *it.c;
            pen.color(cCard);
            pen.roundedRect(kX0, y, kCW, h - 4, 8);
            cairo_fill(pen.cr);
            const Color lc = levelColor(c.level);
            pen.color(lc);
            pen.roundedRect(kX0 + 12, y + 8, 52, h - 20, 5);
            cairo_fill(pen.cr);
            const std::string tag = c.level == "ok" ? "OK" : c.level == "warn" ? "WARN" : "FAIL";
            const double tw = pen.measure(tag, 13, true);
            pen.text(kX0 + 38 - tw / 2, y + h / 2 + 3, tag, 13, cOnAccent, true);
            const double textW = kCW - 90 - (c.fix.empty() ? 0 : 220);
            pen.text(kX0 + 78, y + h / 2 + 4, ellipsize(pen, c.text, 17, false, textW, false), 17, cText);
            if (!c.fix.empty())
                button(pen, "run:" + c.fix, kX1 - 214, y + 3, 206, h - 10, fixLabel(c.fix, ja_), canRun(c.fix), false, 16);
        }
        y += h;
    }
    if (maxEnvOffset_ > 0) {
        const std::string more = std::to_string(envOffset_ + 1) + "–" +
                                 std::to_string(std::min<size_t>(items.size(), envOffset_ + fitFromEnd)) + " / " +
                                 std::to_string(items.size()) + tr(" · scroll", " · スクロール");
        pen.text(kX1 - 190, 52, more, 14, cMuted, false, true);
    }
}

void PanelUi::tuneRow(const Pen& pen, double x, double y, double w, const std::string& label, const std::string& key,
                      int value, int down, int up, const std::string& unit) {
    pen.text(x, y + 28, label, 17, cText);
    const double bx = x + w - 200;
    const bool ok = value > 0;
    button(pen, "set:" + key + "=" + std::to_string(down), bx, y + 4, 44, 36, "−", ok && down > 0, false, 22);
    const std::string v = value > 0 ? std::to_string(value) + unit : "—";
    const double tw = pen.measure(v, 18, true);
    pen.text(bx + 100 - tw / 2, y + 29, v, 18, cText, true);
    button(pen, "set:" + key + "=" + std::to_string(up), bx + 156, y + 4, 44, 36, "+", ok && up > 0, false, 22);
}

void PanelUi::drawAdvanced(const Pen& pen, const Snapshot& s) {
    const Status& st = s.status;
    const double rx = kX0 + kHalfW + kGap, bw = (kHalfW - 50) / 2;
    const int resNow = settingInt(s, "resolution", st.resolution);
    const std::string dfrNow = setting(s, "dfr", st.dfr);
    double y = kTop;
    card(pen, kX0, y, kHalfW, 150, tr("FEX-Emu (x86 emulator)", "FEX-Emu（x86 エミュレーター）"));
    button(pen, "run:fex valve", kX0 + 20, y + 52, bw, 48, tr("Valve's build", "Valve 版"), canRun("fex"), st.fex == "valve");
    button(pen, "run:fex eac", kX0 + 30 + bw, y + 52, bw, 48, tr("Patched (eac)", "パッチ版 (eac)"), canRun("fex"), st.fex == "eac");
    hint(pen, kX0 + 20, y + 126, kHalfW - 40, st.gameRunning ? tr("Quit VRChat to switch.", "切り替えるには VRChat を終了してください。")
                                                       : tr("EAC needs the patched build.", "EAC にはパッチ版が必要です。"), 1);
    card(pen, rx, y, kHalfW, 150, "Proton");
    button(pen, "run:proton stock", rx + 20, y + 52, bw, 48, "Experimental", canRun("proton"), st.proton == "stock");
    button(pen, "run:proton dfr", rx + 30 + bw, y + 52, bw, 48, "Experimental + DFR", canRun("proton"), st.proton == "dfr");
    hint(pen, rx + 20, y + 126, kHalfW - 40, tr("DFR: a 1.5 GB Proton copy with the foveated DXVK.", "DFR：注視点 DXVK 入りの Proton の複製（1.5 GB）。"), 1);

    y += 150 + kGap;
    const double bw3 = (kHalfW - 60) / 3;
    card(pen, kX0, y, kHalfW, 150, tr("More resolutions", "その他の解像度"));
    button(pen, "set:resolution=1600", kX0 + 20, y + 52, bw3, 48, "1600", st.resolution > 0, resNow == 1600);
    button(pen, "set:resolution=2160", kX0 + 30 + bw3, y + 52, bw3, 48, "2160", st.resolution > 0, resNow == 2160);
    button(pen, "run:resolution reset", kX0 + 40 + 2 * bw3, y + 52, bw3, 48, tr("Reset", "元に戻す"), canRun("") && st.resolution > 0);
    hint(pen, kX0 + 20, y + 126, kHalfW - 40, tr("Reset: the value SteamVR had before frame-vrc.", "元に戻す：frame-vrc 導入前の SteamVR の値。"), 1);
    card(pen, rx, y, kHalfW, 150, tr("DFR periphery", "DFR の周辺部"));
    button(pen, "set:dfr=def", rx + 20, y + 52, bw3, 48, "def", true, dfrNow == "def");
    button(pen, "set:dfr=wide", rx + 30 + bw3, y + 52, bw3, 48, "wide", true, dfrNow == "wide");
    button(pen, "set:dfr=ext", rx + 40 + 2 * bw3, y + 52, bw3, 48, "ext", true, dfrNow == "ext");
    hint(pen, rx + 20, y + 126, kHalfW - 40, tr("Shading skipped: def 66%, wide 77%, ext 91% (for 2160 and up).",
                                                  "削減率：def 66%、wide 77%、ext 91%（ext は 2160 以上）"), 1);

    y += 150 + kGap;
    const double h3 = kFootY - 12 - y;
    card(pen, kX0, y, kHalfW, h3, tr("Tuning", "チューニング"));
    button(pen, "set:cpus=16,dxvk-threads=4,vram=1024", kX0 + kHalfW - 120, y + 10, 100, 36, tr("Defaults", "既定値"), st.known,
           false, 16);
    // CPUs: 8, 16 or 32 only - a multiple of the 8 cores maps evenly onto all of them (fewer would leave out the fast
    // cores 5-7, as frame-vrc lists the cores from 0)
    const int cpus = settingInt(s, "cpus", st.cpus), threads = settingInt(s, "dxvk-threads", st.dxvkThreads),
              vram = settingInt(s, "vram", st.vram);
    tuneRow(pen, kX0 + 20, y + 50, kHalfW - 40, tr("CPUs reported", "報告する CPU 数"), "cpus", cpus,
            cpus > 16 ? 16 : cpus > 8 ? 8 : 0, cpus < 16 ? 16 : cpus < 32 ? 32 : 0, "");
    tuneRow(pen, kX0 + 20, y + 96, kHalfW - 40, tr("Shader threads", "シェーダースレッド"), "dxvk-threads", threads,
            threads > 1 ? threads - 1 : 0, threads < 8 ? threads + 1 : 0, "");
    tuneRow(pen, kX0 + 20, y + 142, kHalfW - 40, tr("VRAM reported", "報告する VRAM"), "vram", vram,
            vram > 512 ? vram - 256 : 0, vram < 4096 ? vram + 256 : 0, " MB");
    card(pen, rx, y, kHalfW, h3, tr("Maintenance", "メンテナンス"));
    button(pen, "run:launch-options set", rx + 20, y + 52, bw, 44, tr("Set launch options", "起動オプションを設定"), canRun("launch-options set"));
    button(pen, "run:update", rx + 30 + bw, y + 52, bw, 44, tr("Update frame-vrc", "frame-vrc を更新"), canRun("update"));
    button(pen, "run:uninstall", rx + 20, y + 106, bw, 44, tr("Restore official", "公式版に戻す"), canRun("uninstall"));
    button(pen, "run:uninstall --purge", rx + 30 + bw, y + 106, bw, 44, tr("…and delete data", "…データも削除"), canRun("uninstall"));
    hint(pen, rx + 20, y + 176, kHalfW - 40, tr("Restore: Valve's FEX, Proton Experimental, your launch options and "
                                                "resolution. Removes frame-vrc and this panel.",
                                                "公式版に戻す：Valve 版 FEX、Proton Experimental、元の起動オプションと解像度。"
                                                "frame-vrc とこのパネルも削除します。"), 2);
}

void PanelUi::drawFooter(const Pen& pen, const Snapshot& s) {
    std::string text;
    Color c = cMuted;
    const bool restartArmed = (isArmed("run:setup") || isArmed("run:proton dfr")) && s.status.proton != "dfr";
    if (restartArmed) {
        text = tr("SteamVR restarts once: the view disappears for about a minute, then returns. Setup carries on; "
                  "reopen frame-vrc.",
                  "SteamVR が 1 回再起動：画面が 1 分ほど消えて戻ります。設定は続くので frame-vrc を開き直してください。");
        c = cAmber;
    } else if (s.applyAtMs != 0 && !s.running) {
        std::string what;
        for (const auto& kv : s.pending) what += (what.empty() ? "" : ", ") + kv.first + " " + kv.second;
        const int64_t secs = std::max<int64_t>(0, (s.applyAtMs - nowMs() + 999) / 1000);
        text = tr("Applying in ", "適用まで ") + std::to_string(secs) + tr(" s: ", " 秒：") + what;
        c = cAccent;
    } else if (s.running) {
        const int64_t secs = (nowMs() - s.runSinceMs) / 1000;
        text = tr("Running frame-vrc ", "実行中 frame-vrc ") + s.runLabel + " (" + std::to_string(secs) + tr(" s)", " 秒)") +
               (s.runLine.empty() ? "" : ": " + s.runLine);
        c = cAccent;
    } else if (s.haveResult) {
        text = (s.resultOk ? tr("Done: ", "完了：") : tr("Failed: ", "失敗：")) + s.resultLabel + (s.resultLine.empty() ? "" : " — " + s.resultLine);
        c = s.resultOk ? cGreen : cRed;
    } else if (!s.error.empty()) {
        text = s.error;
        c = cRed;
    } else if (s.status.gameRunning) {
        text = tr("VRChat is running: FEX and Proton can be switched after it exits.",
                  "VRChat 実行中：FEX と Proton は終了後に切り替えられます。");
    }
    if (text.empty()) return;
    const bool toLog = view_ != View::Log && (s.running || s.haveResult) && !restartArmed;
    pen.color(toLog && (hover_ == "view:log" || pressed_ == "view:log") ? cCtlHot : cCard);
    pen.roundedRect(kX0, kFootY, kCW, kFootH, 8);
    cairo_fill(pen.cr);
    const double more = toLog ? pen.measure(tr("Log ›", "ログ ›"), 16, true) + 24 : 0;
    pen.text(kX0 + 16, kFootY + 28, ellipsize(pen, text, 16, false, kCW - 32 - more, false), 16, c);
    if (toLog) {
        pen.text(kX1 - 16, kFootY + 28, tr("Log ›", "ログ ›"), 16, cText, true, true);
        addHit("view:log", kX0, kFootY, kCW, kFootH, true);
    }
}

void PanelUi::drawThumbnail(cairo_t* cr, const Snapshot& s) {
    drawAppIcon(cr, fonts_);
    const Pen pen {cr, &fonts_};
    const Color c = !s.checked ? cDim : countLevel(s, "fail") > 0 ? cRed : s.running ? cAccent : cGreen;
    cairo_arc(cr, 214, 42, 24, 0, 2 * M_PI);
    pen.color(c);
    cairo_fill(cr);
}

void drawAppIcon(cairo_t* cr, const FontSet& fonts) {
    const Pen pen {cr, &fonts};
    pen.color(cCard);
    pen.roundedRect(0, 0, kThumbnailSize, kThumbnailSize, 48);
    cairo_fill(cr);
    const double tw = pen.measure("VRC", 92, true);
    pen.text((kThumbnailSize - tw) / 2, 160, "VRC", 92, cText, true);
    pen.color(cAccent);
    cairo_rectangle(cr, 48, 200, kThumbnailSize - 96, 10);
    cairo_fill(cr);
}

bool PanelUi::pointerMove(double x, double y) {
    const Hit* h = hitAt(x, y);
    const std::string id = h != nullptr && h->enabled ? h->id : "";
    if (id == hover_) return false;
    hover_ = id;
    return true;
}

bool PanelUi::pointerDown(double x, double y) {
    const Hit* h = hitAt(x, y);
    pressed_ = h != nullptr && h->enabled ? h->id : "";
    hover_ = pressed_;
    return true;
}

bool PanelUi::pointerUp(double x, double y) {
    const Hit* h = hitAt(x, y);
    const std::string id = h != nullptr && h->enabled ? h->id : "";
    const std::string was = pressed_;
    pressed_.clear();
    if (!id.empty() && id == was) activate(id);  // only a press and release on the same target counts
    return true;
}

bool PanelUi::pointerLeave() {
    const bool changed = !hover_.empty() || !pressed_.empty();
    hover_.clear();
    pressed_.clear();
    return changed;
}

bool PanelUi::scroll(double dy) {
    if (view_ == View::Log) {  // up = older lines
        scrollAccum_ += dy;
        bool changed = false;
        for (; scrollAccum_ >= 26; scrollAccum_ -= 26)
            if (logOffset_ > 0) --logOffset_, changed = true;
        for (; scrollAccum_ <= -26; scrollAccum_ += 26)
            if (logOffset_ < maxLogOffset_) ++logOffset_, changed = true;
        return changed;
    }
    if (view_ != View::Environment) return false;
    scrollAccum_ += dy;
    bool changed = false;
    for (; scrollAccum_ >= kEnvRowH; scrollAccum_ -= kEnvRowH)
        if (envOffset_ < maxEnvOffset_) ++envOffset_, changed = true;
    for (; scrollAccum_ <= -kEnvRowH; scrollAccum_ += kEnvRowH)
        if (envOffset_ > 0) --envOffset_, changed = true;
    return changed;
}

bool PanelUi::press(const std::string& id) {
    for (const Hit& h : hits_)
        if (h.id == id && h.enabled) {
            activate(id);
            return true;
        }
    return false;
}

void PanelUi::activate(const std::string& id) {
    if (id == "view:home") view_ = View::Home;
    else if (id == "view:env") {
        view_ = View::Environment;
        model_.refresh();
    }
    else if (id == "view:advanced") view_ = View::Advanced;
    else if (id == "view:log") {
        view_ = View::Log;
        logOffset_ = 0;
    } else if (id == "view:monitor") {
        view_ = View::Monitor;
        monitorAt_ = 0;  // sample now
    } else if (id == "lang") {
        ja_ = !ja_;
        if (!langFile_.empty()) {
            std::FILE* f = std::fopen(langFile_.c_str(), "w");
            if (f != nullptr) {
                std::fputs(ja_ ? "ja\n" : "en\n", f);
                std::fclose(f);
            }
        }
    }
    else if (id == "refresh") model_.refresh();
    model_.setFullChecks(view_ == View::Environment);
    if (id.rfind("set:", 0) == 0) {  // settings: shown at once, applied together once the taps settle
        const std::string list = id.substr(4);
        for (size_t from = 0; from < list.size();) {
            const size_t comma = std::min(list.find(',', from), list.size());
            const std::string kv = list.substr(from, comma - from);
            const size_t eq = kv.find('=');
            if (eq != std::string::npos) model_.stage(kv.substr(0, eq), kv.substr(eq + 1));
            from = comma + 1;
        }
        return;
    }
    if (id.rfind("run:", 0) != 0) return;
    if (needsConfirm(id) && !isArmed(id)) {
        armed_ = id;
        armedAt_ = nowMs();
        return;
    }
    armed_.clear();
    std::vector<std::string> args;
    const std::string cmd = id.substr(4);
    for (size_t from = 0; from < cmd.size();) {
        const size_t sp = std::min(cmd.find(' ', from), cmd.size());
        args.push_back(cmd.substr(from, sp - from));
        from = sp + 1;
    }
    model_.run(args);
    // long, many-step ones: show their output as it comes
    if (args[0] == "setup" || args[0] == "update" || args[0] == "uninstall" || cmd == "proton dfr" || cmd == "fex eac" ||
        args[0] == "install-deps") {
        view_ = View::Log;
        logOffset_ = 0;
    }
}

bool PanelUi::tick() {
    if (view_ != View::Monitor || nowMs() - monitorAt_ < 1000) return false;
    monitorAt_ = nowMs();
    monitor_.sample();
    return true;
}

void PanelUi::graph(const Pen& pen, double x, double y, double w, double h, const std::deque<double>& values,
                    Color c) {
    pen.color(cCtl);
    pen.roundedRect(x, y, w, h, 6);
    cairo_fill(pen.cr);
    if (values.size() < 2) return;
    const double step = w / (SystemMonitor::kHistory - 1);
    const double x0 = x + w - step * (values.size() - 1);  // newest at the right edge
    cairo_move_to(pen.cr, x0, y + h);
    for (size_t i = 0; i < values.size(); ++i)
        cairo_line_to(pen.cr, x0 + step * i, y + h - h * std::clamp(values[i], 0.0, 100.0) / 100);
    cairo_line_to(pen.cr, x + w, y + h);
    cairo_close_path(pen.cr);
    pen.color(c, 0.25);
    cairo_fill(pen.cr);
    for (size_t i = 0; i < values.size(); ++i) {
        const double px = x0 + step * i, py = y + h - h * std::clamp(values[i], 0.0, 100.0) / 100;
        if (i == 0) cairo_move_to(pen.cr, px, py);
        else cairo_line_to(pen.cr, px, py);
    }
    pen.color(c);
    cairo_set_line_width(pen.cr, 2);
    cairo_stroke(pen.cr);
}

void PanelUi::drawMonitor(const Pen& pen, const Snapshot& s) {
    if (monitorAt_ == 0) tick();  // a picture before the first second passed (--dump-png)
    const MonitorSample& m = monitor_.last();
    char buf[160];
    auto gb = [](double mb) { return mb / 1024; };
    const double rx = kX0 + kHalfW + kGap;
    double y = kTop;
    const double topH = 262;

    // ---- memory: the limit on this headset ----
    card(pen, kX0, y, kHalfW, topH, tr("Memory", "メモリ"));
    const double usedMb = m.memTotalMb - m.memAvailMb;
    const Color memC = m.memAvailMb < 1000 ? cRed : m.memAvailMb < 2000 ? cAmber : cAccent;
    std::snprintf(buf, sizeof(buf), "%.1f / %.1f GB", gb(usedMb), gb(m.memTotalMb));
    pen.text(kX0 + 20, y + 80, buf, 30, cText, true);
    std::snprintf(buf, sizeof(buf), ja_ ? "空き %.2f GB" : "%.2f GB free", gb(m.memAvailMb));
    pen.text(kX0 + kHalfW - 20, y + 78, buf, 20, memC, true, true);
    std::snprintf(buf, sizeof(buf), ja_ ? "スワップ (zram) %.1f GB 使用、RAM %.2f GB を占有" : "swap (zram) %.1f GB used, takes %.2f GB of RAM",
                  gb(m.swapUsedMb), gb(m.zramMemMb));
    pen.text(kX0 + 20, y + 112, buf, 16, cMuted);
    graph(pen, kX0 + 20, y + 128, kHalfW - 40, topH - 148, monitor_.memHistory(), memC);

    // ---- CPU ----
    card(pen, rx, y, kHalfW, topH, "CPU");
    std::snprintf(buf, sizeof(buf), "%.0f%%", m.cpuPct);
    pen.text(rx + 20, y + 80, buf, 30, cText, true);
    std::snprintf(buf, sizeof(buf), ja_ ? "最高温度 %.0f °C" : "max temperature %.0f °C", m.tempC);
    if (m.tempC > 0)
        pen.text(rx + kHalfW - 20, y + 78, buf, 20, m.tempC >= 90 ? cRed : m.tempC >= 75 ? cAmber : cText, true, true);
    if (m.gpuMaxMhz > 0) {
        std::snprintf(buf, sizeof(buf), ja_ ? "GPU クロック %.0f / %.0f MHz" : "GPU clock %.0f / %.0f MHz", m.gpuMhz,
                      m.gpuMaxMhz);
        pen.text(rx + 20, y + 112, buf, 16, cMuted);
    }
    graph(pen, rx + 20, y + 128, kHalfW - 40, topH - 148, monitor_.cpuHistory(), cAccent);

    // ---- per core ----
    y += topH + kGap;
    const double lowH = kFootY - 12 - y;
    card(pen, kX0, y, kHalfW, lowH, tr("Cores", "コアごと"));
    const size_t n = m.corePct.size();
    if (n > 0) {
        const double gap = 8, bw = (kHalfW - 40 - gap * (n - 1)) / n, bh = lowH - 90;
        for (size_t i = 0; i < n; ++i) {
            const double bx = kX0 + 20 + i * (bw + gap), by = y + 50;
            pen.color(cCtl);
            pen.roundedRect(bx, by, bw, bh, 4);
            cairo_fill(pen.cr);
            const double fill = bh * std::clamp(m.corePct[i], 0.0, 100.0) / 100;
            pen.color(m.corePct[i] >= 90 ? cAmber : cAccent);
            pen.roundedRect(bx, by + bh - fill, bw, fill, 4);
            cairo_fill(pen.cr);
            std::snprintf(buf, sizeof(buf), "%.0f", m.corePct[i]);
            const double tw = pen.measure(buf, 14);
            pen.text(bx + (bw - tw) / 2, by + bh + 22, buf, 14, cMuted);
        }
    }

    // ---- the game ----
    card(pen, rx, y, kHalfW, lowH, "VRChat");
    if (m.gamePid > 0) {
        std::snprintf(buf, sizeof(buf), "%.2f GB", gb(m.gameRssMb));
        pen.text(rx + 20, y + 84, buf, 30, cText, true);
        pen.text(rx + kHalfW - 20, y + 82, tr("resident memory", "常駐メモリ"), 16, cMuted, false, true);
        std::snprintf(buf, sizeof(buf), ja_ ? "%.1f コア分" : "%.1f cores", m.gameCores);
        pen.text(rx + 20, y + 132, buf, 30, cText, true);
        pen.text(rx + kHalfW - 20, y + 130, tr("CPU (8 = all)", "CPU（8 = 全部）"), 16, cMuted, false, true);
        std::snprintf(buf, sizeof(buf), "pid %d · %s", m.gamePid,
                      s.status.proton == "dfr" ? ("DFR " + s.status.dfrEffective).c_str() : "DFR off");
        pen.text(rx + 20, y + lowH - 24, buf, 15, cMuted);
    } else {
        pen.text(rx + 20, y + 84, tr("not running", "起動していません"), 22, cMuted, true);
    }
}

void PanelUi::drawLog(const Pen& pen, const Snapshot& s) {
    const std::string label = s.running ? s.runLabel : s.resultLabel;
    std::string state;
    Color sc = cMuted;
    if (s.running) {
        state = tr("running ", "実行中 ") + std::to_string((nowMs() - s.runSinceMs) / 1000) + tr(" s", " 秒");
        sc = cAccent;
    } else if (s.haveResult) {
        state = s.resultOk ? tr("done", "完了") : tr("failed", "失敗");
        sc = s.resultOk ? cGreen : cRed;
    }
    if (!label.empty()) {
        const double w = pen.measure(state, 18, true);
        pen.text(kX1, 50, state, 18, sc, true, true);
        pen.text(kX1 - w - 16, 50, ellipsize(pen, "frame-vrc " + label, 18, false, 420, false), 18, cMuted, false, true);
    }
    const double y0 = kTop, h = kFootY - 12 - kTop, lineH = 24;
    pen.color(hexColor(0x0E0E0E));
    pen.roundedRect(kX0, y0, kCW, h, 10);
    cairo_fill(pen.cr);
    if (s.runLog.empty()) {
        pen.text(kX0 + 20, y0 + 40, label.empty() ? tr("Nothing has run yet.", "まだ何も実行していません。")
                                                  : tr("Waiting for output…", "出力を待っています…"), 17, cMuted);
        return;
    }
    const int fit = static_cast<int>((h - 24) / lineH);
    const int n = static_cast<int>(s.runLog.size());
    maxLogOffset_ = std::max(0, n - fit);
    logOffset_ = std::min(logOffset_, maxLogOffset_);
    const int first = std::max(0, n - fit - logOffset_);
    double y = y0 + 30;
    for (int i = first; i < n && i < first + fit; ++i) {
        const std::string& line = s.runLog[i];
        const bool head = line.rfind("==> ", 0) == 0;
        const bool bad = line.find("FAIL") != std::string::npos || line.rfind("frame-vrc: ", 0) == 0;
        const Color c = head ? cAccent : bad ? cRed : line.find("WARN") != std::string::npos ? cAmber : cText;
        if (head) pen.text(kX0 + 16, y, ellipsize(pen, line, 16, true, kCW - 32, false), 16, c, true);
        else pen.textMono(kX0 + 16, y, line.size() > 110 ? line.substr(0, 109) + "…" : line, 15, c);
        y += lineH;
    }
    if (logOffset_ > 0) pen.text(kX1 - 16, y0 + h - 10, tr("scrolled up · newest below", "スクロール中 · 最新は下"), 13,
                                 cMuted, false, true);
}
