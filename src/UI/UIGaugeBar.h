#pragma once
#include "UI/UIBase.h"
#include "UI/UIImage.h"

#pragma once
#include <algorithm>
#include <float.h> // FLT_MAX


// 밑의 하위 오브젝트들은 메니저에서 생성해서 연결해줄 예정
class UIGaugeBar : public UIBase
{
public:

    void Initalize(UIRenderStruct& rs, CompDelegates& worldDelegates) override;

    void Update(float dt) override;

    void Render() override {} // UIImage들이 알아서 렌더되면 비워도 OK

    void SetupParts(UIImage* bgi, UIImage* fi, UIImage* ldi, UIImage* lui);

    // 외부에서 게이지 값 설정 (0~1)
    void SetTarget01(float t) { m_target = std::clamp(t, 0.0f, 1.0f); }

    UIImage* GetBackgroundImage() { return backGroundImage; }

    // path setter
    void SetBackgroundImagePath(const std::wstring& path) { if (backGroundImage) backGroundImage->createImage(path); }
    void SetFillImagePath(const std::wstring& path) { if (FillImage)       FillImage->createImage(path); }
    void SetLerpDownImagePath(const std::wstring& path) { if (lerpDownImage)   lerpDownImage->createImage(path); }
    void SetLerpUpImagePath(const std::wstring& path) { if (lerpUpImage)     lerpUpImage->createImage(path); }

private:
    // Scale 기반으로 게이지 값 반영 (Rect 수정 없이 scale.x만 변경)
    void UpdateScaleFromValue();

private:
    UIImage* backGroundImage{ nullptr };
    UIImage* FillImage{ nullptr };
    UIImage* lerpDownImage{ nullptr };
    UIImage* lerpUpImage{ nullptr };


    float m_target = 1.0f;
    float m_current = 1.0f;
    float m_prevTarget = 1.0f;

    float m_velocity = 0.0f;
    float m_smoothTime = 0.08f;

    float m_lerpDown = 1.0f;
    float m_lerpDownVel = 0.0f;
    float m_lerpDownTime = 0.15f;

    float m_lerpUp = 1.0f;
    float m_lerpUpVel = 0.0f;
    float m_lerpUpTime = 0.10f;

    // SmoothDamp (maxSpeed 기본값: FLT_MAX)
    float SmoothDamp(float current, float target, float& currentVelocity, float smoothTime, float dt);
};
