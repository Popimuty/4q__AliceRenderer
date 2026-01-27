#include "UIGaugeBar.h"

void UIGaugeBar::SetupParts(UIImage* bgi, UIImage* fi, UIImage* ldi, UIImage* lui)
{
    backGroundImage = bgi;
    FillImage = fi;
    lerpDownImage = ldi;
    lerpUpImage = lui;

    // SetupParts 호출 시점에도 pivot 설정 (Initalize보다 먼저 호출될 수 있음)
    if (backGroundImage)
        backGroundImage->GetTransform().SetPivot(0.0f, 0.5f);
    if (FillImage)
        FillImage->GetTransform().SetPivot(0.0f, 0.5f);
    if (lerpDownImage)
        lerpDownImage->GetTransform().SetPivot(0.0f, 0.5f);
    if (lerpUpImage)
        lerpUpImage->GetTransform().SetPivot(0.0f, 0.5f);
}


float UIGaugeBar::SmoothDamp(float current, float target, float& currentVelocity, float smoothTime, float dt)
{
    float maxSpeed = 2.0f;

    smoothTime = std::max(0.0001f, smoothTime);
    float omega = 2.0f / smoothTime;
    float x = omega * dt;
    //exp 계산 대신 --테일러 급수--> 근사 --> 다항식 
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

    float change = current - target;                     // 차이
    float originalTarget = target;
    float maxChange = maxSpeed * smoothTime;             // 최대 변화량
    change = std::clamp(change, -maxChange, maxChange);  // 최대 변화량으로 조절

    target = current - change;  // 이번에 변할 거리          
    float temp = (currentVelocity + omega * change) * dt;       //(현재 속도 + 차이/시간)* 시간 = 이번 틱의 예상 이동량
    currentVelocity = (currentVelocity - omega * temp) * exp;   // (현재 속도 - 이동량/시간) * 감쇠
    float output = target + (change + temp) * exp;              // overshoot 방지 
    if ((originalTarget - current > 0.0f) == (output > originalTarget))
    {
        output = originalTarget;
        currentVelocity = (output - originalTarget) / dt;
    }

    return output;
}


void UIGaugeBar::UpdateScaleFromValue()
{
    if (!FillImage || !lerpDownImage || !lerpUpImage)
        return;

   

    float downScaleX = std::clamp(m_lerpDown, 0.0f, 1.0f);
    lerpDownImage->GetTransform().m_scale.x = m_target; 


    float fillScaleX = std::clamp(m_current, 0.0f, 1.0f);
    FillImage->GetTransform().m_scale.x = downScaleX;

    float upScaleX = std::clamp(m_lerpUp, 0.0f, 1.0f);
    lerpUpImage->GetTransform().m_scale.x = 0;
}



// 지금 줄어들고 늘어날때 보이는 색이 안보임....
void UIGaugeBar::Update(float dt)
{
    if (!backGroundImage || !FillImage || !lerpDownImage || !lerpUpImage)
        return;

    // 디버깅: Update 호출 및 값 확인
    // ALICE_LOG_INFO("[UIGaugeBar] Update called - target: %.2f, current: %.2f, dt: %.4f", m_target, m_current, dt);


    // 2. 하락 잔상 (빨간색) 연출
    // 실제 피(m_current)가 잔상보다 아래에 있다면, 잔상이 틱마다 따라 내려감
    m_current = SmoothDamp(m_current, m_target, m_velocity, m_smoothTime, dt);

    // 2. 하락 잔상 (ch색) 연출
    // m_current보다 더 느린 시간(m_lerpDownTime)을 설정하여 뒤늦게 따라오게 함
    if (m_current < m_lerpDown)
    {
        m_lerpDown = SmoothDamp(m_lerpDown, m_current, m_lerpDownVel, m_lerpDownTime, dt);
    }
    else
    {
        m_lerpDown = m_current;
    }

    // 3. 상승 잔상 (초록색) 연출
    if (m_current > m_lerpUp)
    {
        m_lerpUp = SmoothDamp(m_lerpUp, m_current, m_lerpUpVel, m_lerpUpTime, dt);
    }
    else
    {
        m_lerpUp = m_current;
    }

    // Scale 기반으로 게이지 값 반영
    UpdateScaleFromValue();
}

void UIGaugeBar::Initalize(UIRenderStruct& rs, CompDelegates& worldDelegates)
{
    UIBase::Initalize(rs, worldDelegates);
    // parts가 연결되기 전에도 크래시 안나게 가드만 잘 두면 됨
    if (!backGroundImage || !FillImage || !lerpDownImage || !lerpUpImage)
        return;

    // Fill, lerpDown, lerpUp 이미지의 Pivot을 (0, 0.5)로 고정
    // GetTransform()은 참조를 반환하므로 직접 수정 가능
    FillImage->GetTransform().SetPivot(0.0f, 0.5f);
    lerpDownImage->GetTransform().SetPivot(0.0f, 0.5f);
    lerpUpImage->GetTransform().SetPivot(0.0f, 0.5f);

    // 초기 scale 설정 (게이지 값에 맞춰)
    UpdateScaleFromValue();
}

void UIGaugeBar::SetNormalized(float t01)
{
    // 값 클램프
    float clamped = std::clamp(t01, 0.0f, 1.0f);
    
    // target과 current를 모두 설정 (즉시 반영)
    m_target = clamped;
    m_current = clamped;
    m_prevTarget = clamped;
    
    // lerp 값들도 동기화
    m_lerpDown = clamped;
    m_lerpUp = clamped;
    
    // 즉시 scale 반영
    UpdateScaleFromValue();
}