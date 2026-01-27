#include "tmpTestScripts.h"
#include "UI/UIScriptFactory.h"
#include "UI/UIBase.h"
#include "UI/UITransform.h"
#include "UI/UI_ImageComponent.h"
#include "Core/Logger.h"



#include "UI/UIButton.h"
#include "UI/UIGaugeBar.h"
#include "UI/UISceneManager.h"

#include <algorithm>
#include <cmath>
#include <ctime>

// 클래스 정의 후에 등록! (중요)
// 이 정적 변수는 이 파일이 링크될 때 자동으로 초기화됩니다
REGISTER_UI_SCRIPT(MyUIScript);

void MyUIScript::OnAdded(UIBase& owner)
{
    // 컴포넌트가 추가될 때 호출
    // Owner는 이미 설정되어 있습니다
    ALICE_LOG_INFO("[MyUIScript] OnAdded called for UI ID: %lu", OwnerID);
}


void MyUIScript::OnStart()
{
    // 1. 기본 포인터 유효성 검사
    if (!Owner || !World)
    {
      
        return;
    }

    // 2. 대상 UI 객체 탐색 (게이지바 ID: 1, 버튼 ID: 6)
    // DLL 경계 문제 발생 시 static_cast로 변경 고려
    UIBase* gaugeBase = World->Get(1);
    UIBase* buttonBase = World->Get(6);

    if (!gaugeBase || !buttonBase)
    {
        ALICE_LOG_WARN("[MyUIScript] Required UI (ID 1 or 6) not found!");
        return;
    }

    // 3. 타입 캐스팅 및 유효성 확인
    m_button = static_cast<UIButton*>(buttonBase);
    m_gauge = static_cast<UIGaugeBar*>(gaugeBase);

    if (!m_button || !m_gauge)
    {
         return;
    }

    ALICE_LOG_INFO("[MyUIScript] Successfully linked Button(6) and Gauge(1)");

    // 4. 클릭 이벤트 바인딩
    if (!m_bound)
    {
        m_button->SetOnClicked([this]() {
            m_gauge->SetTarget01(m_gauge->GetTarget() - 0.1f);
            ALICE_LOG_INFO("[MyUIScript] Button clicked! New target: %.2f", m_target);
            });

        m_bound = true;
    }
}

void MyUIScript::Update(float dt)
{
    // 대상 객체가 없으면 연산 중단
    if (!m_gauge) return;

}

void MyUIScript::OnRemoved()
{
    ALICE_LOG_INFO("[MyUIScript] OnRemoved called for UI ID: %lu", OwnerID);

    // 버튼 이벤트 해제 및 포인터 정리
    if (m_button)
    {
        m_button->SetOnClicked(nullptr);
    }

    m_bound = false;
    m_button = nullptr;
    m_gauge = nullptr;
}
