#pragma once

/**
 * @brief ホールド機能を管理するクラス
 */
class HoldManager {
public:
    /**
     * @brief BtnA押下時に呼び出す。ホールド開始/解除をトグルする。
     * @param currentTmp 現在の温度値
     * @param currentPrs 現在の気圧値
     */
    void toggle(float currentTmp, float currentPrs) {
        holding_ = !holding_;
        if (holding_) {
            heldTmp_ = currentTmp;
            heldPrs_ = currentPrs;
        }
    }

    bool  isHolding()    const { return holding_; }
    float heldTemp()     const { return heldTmp_; }
    float heldPressure() const { return heldPrs_; }

private:
    bool  holding_ = false;
    float heldTmp_ = 0.0f;
    float heldPrs_ = 0.0f;
};
