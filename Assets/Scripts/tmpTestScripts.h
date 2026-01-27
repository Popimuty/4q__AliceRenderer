#pragma once
#include "UI/IUIScript.h"
#include <DirectXMath.h>

#include <string>

// 전방 선언으로 컴파일 속도 향상 및 순환 참조 방지
class UIBase;
class UIButton;
class UIGaugeBar;


class MyUIScript : public IUIScript
{
public:
    MyUIScript() = default;
    virtual ~MyUIScript() = default;

    // 인터페이스 구현
    virtual void OnAdded(UIBase& owner) override;
    virtual void OnStart() override;
    virtual void Update(float dt) override;
    virtual void OnRemoved() override;

private:
    // 참조할 UI 객체 포인터
    UIButton* m_button = nullptr;
    UIGaugeBar* m_gauge = nullptr;

    // 상태 변수
    float m_target = 0.5f;   // 목표 수치
    bool  m_bound = false;   // 이벤트 바인딩 여부
};