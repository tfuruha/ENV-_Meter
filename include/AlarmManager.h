#pragma once
#include "util.h"
#include <M5Unified.h>

/**
 * @brief 圧力過剰低下警報機能を管理するクラス
 *
 * 状態遷移:
 *   IDLE --(prs <= 700)--> ALARM --(button)--> SNOOZE
 *     ^                      |                    |
 *     +-(prs >= 710)---------+-(prs >= 710 or timeout)+
 */
class AlarmManager {
public:
    static constexpr float    ALARM_THRESHOLD  = 700.0f; // hPa
    static constexpr float    RESET_THRESHOLD  = 710.0f; // hPa
    static constexpr uint32_t SNOOZE_MS        = 60000;  // 60秒
    static constexpr uint32_t BEEP_INTERVAL_MS = 500;    // ms
    static constexpr uint32_t BEEP_FREQ_HZ     = 2000;   // Hz
    static constexpr uint32_t BEEP_DURATION_MS = 100;    // ms

    AlarmManager() : snoozeTimer_(SNOOZE_MS), beepTimer_(BEEP_INTERVAL_MS) {}

    /**
     * @brief センサー値更新時に気圧判定を行う
     * @param prs 最新の気圧値 [hPa]
     * @return true: 表示の再描画が必要
     */
    [[nodiscard]] bool check(float prs) {
        if (prs <= ALARM_THRESHOLD) {
            if (!alarmActive_ && !snoozed_) {
                alarmActive_ = true;
                beepTimer_.init();
                M5.Speaker.tone(BEEP_FREQ_HZ, BEEP_DURATION_MS);
                return true;
            }
        } else if (prs >= RESET_THRESHOLD) {
            if (alarmActive_ || snoozed_) {
                alarmActive_ = false;
                snoozed_ = false;
                snoozeTimer_.stop();
                M5.Speaker.stop();
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 警報鳴動中のボタン押下処理（スヌーズ化）
     * @return true: 表示の再描画が必要
     */
    [[nodiscard]] bool handleButton() {
        if (alarmActive_) {
            alarmActive_ = false;
            snoozed_ = true;
            snoozeTimer_.start();
            M5.Speaker.stop();
            return true;
        }
        return false;
    }

    /**
     * @brief ループごとの更新処理（スヌーズ判定・ビープ判定）
     * @param prs 最新の気圧値 [hPa]
     * @return true: 表示の再描画が必要
     */
    [[nodiscard]] bool update(float prs) {
        bool need_redraw = false;

        // スヌーズタイムアウト判定
        if (snoozed_ && snoozeTimer_.hasExpired()) {
            snoozed_ = false;
            if (prs <= ALARM_THRESHOLD) {
                alarmActive_ = true;
                beepTimer_.init();
                M5.Speaker.tone(BEEP_FREQ_HZ, BEEP_DURATION_MS);
            }
            need_redraw = true;
        }

        // 警報音の発声判定 (間欠ビープ)
        if (alarmActive_ && !snoozed_) {
            if (beepTimer_.hasExpired()) {
                M5.Speaker.tone(BEEP_FREQ_HZ, BEEP_DURATION_MS);
            }
        }

        return need_redraw;
    }

    bool isAlarmActive() const { return alarmActive_; }
    bool isSnoozed()     const { return snoozed_; }

private:
    bool             alarmActive_ = false;
    bool             snoozed_     = false;
    OneShotTrigger_m snoozeTimer_;
    IntervalTrigger_m beepTimer_;
};
