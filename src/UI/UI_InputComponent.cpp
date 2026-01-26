#include "UI_InputComponent.h"
#include "UISceneManager.h"  // UIWorld 정의 포함
#include "UITransform.h"
#include "Core/InputSystem.h"
#include "Core/Logger.h"
#include <d2d1_1.h>
#include <DirectXMath.h>
#include <ctime>
#include <cstdio>

using namespace DirectX;

void UI_InputComponent::Update(UIWorld& world, Alice::InputSystem& input)
{
    if (OwnerID == 0) return;

    // Owner의 Transform 가져오기
    UITransform* transform = world.TryGetComponent<UITransform>(OwnerID);
    if (!transform) return;

    // 마우스 스크린 좌표 가져오기 (윈도우 클라이언트 좌표)
    POINT mousePos = input.GetMousePosition();
    XMFLOAT2 mousePosClient{ static_cast<float>(mousePos.x), static_cast<float>(mousePos.y) };

    // 뷰포트 정보 가져오기 (Owner의 UIRenderStruct를 통해)
    UIRenderStruct* renderStruct = nullptr;
    if (Owner)
    {
        renderStruct = Owner->GetRenderStruct();
    }

    // 뷰포트 오프셋 보정: 마우스 좌표를 뷰포트 로컬 좌표로 변환 (=RenderTarget 좌표)
    float localX = mousePosClient.x;
    float localY = mousePosClient.y;
    
    if (renderStruct)
    {
        localX = mousePosClient.x - renderStruct->m_viewportX;
        localY = mousePosClient.y - renderStruct->m_viewportY;

        // 뷰포트 밖이면 UI 판정에서 제외
        if (localX < 0.0f || localY < 0.0f || 
            localX > renderStruct->m_viewportWidth || localY > renderStruct->m_viewportHeight)
        {
            // 뷰포트 밖이면 hover/press 상태 해제
            if (bIsHovered)
            {
                bIsHovered = false;
                if (OnHoverEnd) OnHoverEnd();
            }
            if (bIsPressed)
            {
                bIsPressed = false;
                bPressedInside = false;
            }
            return; // 뷰포트 밖이면 이벤트 처리 중단
        }
    }

    // RenderTarget 좌표를 Unity 좌표로 변환
    // Unity 좌표: 중앙 (0, 0), 위가 양수, 아래가 음수
    float viewportW = renderStruct ? renderStruct->m_viewportWidth : 1280.0f;
    float viewportH = renderStruct ? renderStruct->m_viewportHeight : 720.0f;
    
    XMFLOAT2 unityMousePos = {
        localX - viewportW * 0.5f,
        -(localY - viewportH * 0.5f)  // Y축 반전: 위가 양수, 아래가 음수
    };

    // 충돌 판정 (Unity 좌표계 사용)
    bool inside = IsPointInside(unityMousePos, *transform, world);

    // #region agent log
    FILE* logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
    if (logFile) {
        fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"E\",\"location\":\"UI_InputComponent.cpp:69\",\"message\":\"UI_InputComponent::Update hit test\",\"data\":{\"ownerID\":%lu,\"inside\":%d,\"bIsHovered\":%d},\"timestamp\":%lld}\n",
            OwnerID, inside ? 1 : 0, bIsHovered ? 1 : 0, (long long)time(nullptr) * 1000);
        fclose(logFile);
    }
    // #endregion agent log

    // Hover 상태 전이 처리
    if (inside && !bIsHovered)
    {
        // false → true: Hover 시작
        bIsHovered = true;
        if (OnHoverBegin)
        {
            OnHoverBegin();
        }
        #ifdef _DEBUG
        ALICE_LOG_INFO("[UI_InputComponent] HoverBegin: OwnerID=%lu", OwnerID);
        #endif
    }
    else if (!inside && bIsHovered)
    {
        // true → false: Hover 종료
        bIsHovered = false;
        if (OnHoverEnd)
        {
            OnHoverEnd();
        }
        #ifdef _DEBUG
        ALICE_LOG_INFO("[UI_InputComponent] HoverEnd: OwnerID=%lu", OwnerID);
        #endif
    }

    // 마우스 버튼 상태 확인 (왼쪽 버튼, 인덱스 0)
    const int LEFT_MOUSE_BUTTON = 0;
    bool isMouseDown = input.IsLeftButtonDown();
    bool isMousePressed = input.IsMouseButtonPressed(LEFT_MOUSE_BUTTON);
    bool isMouseReleased = input.IsMouseButtonReleased(LEFT_MOUSE_BUTTON);

    // Press 처리 (Down 순간 1회)
    if (isMousePressed && inside)
    {
        bIsPressed = true;
        bPressedInside = true; // inside 상태에서 눌렀음을 기록
        if (OnPressed)
        {
            OnPressed();
        }
        //#ifdef _DEBUG
        //ALICE_LOG_INFO("[UI_InputComponent] Pressed: OwnerID=%lu", OwnerID);
        //#endif
    }

    // Release 처리
    if (isMouseReleased)
    {
        bool wasPressed = bIsPressed;
        bIsPressed = false;

        if (OnReleased)
        {
            OnReleased();
        }
        //#ifdef _DEBUG
        //ALICE_LOG_INFO("[UI_InputComponent] Released: OwnerID=%lu", OwnerID);
        //#endif

        // Click 처리: inside 상태에서 Down 후 Up이면 Click
        if (wasPressed && bPressedInside && inside)
        {
            ALICE_LOG_INFO("[UI_InputComponent] Click detected: OwnerID=%lu, OnClicked valid=%d", 
                OwnerID, OnClicked ? 1 : 0);
            
            // #region agent log
            FILE* logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
            if (logFile) {
                fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"CLICK\",\"location\":\"UI_InputComponent.cpp:143\",\"message\":\"Click detected\",\"data\":{\"ownerID\":%lu,\"onClickedValid\":%d,\"wasPressed\":%d,\"bPressedInside\":%d,\"inside\":%d},\"timestamp\":%lld}\n",
                    OwnerID, OnClicked ? 1 : 0, wasPressed ? 1 : 0, bPressedInside ? 1 : 0, inside ? 1 : 0, (long long)time(nullptr) * 1000);
                fclose(logFile);
            }
            // #endregion agent log
            
            if (OnClicked)
            {
                ALICE_LOG_INFO("[UI_InputComponent] Calling OnClicked callback: OwnerID=%lu", OwnerID);
                
                // #region agent log
                logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
                if (logFile) {
                    fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"CLICK_CALL\",\"location\":\"UI_InputComponent.cpp:147\",\"message\":\"Calling OnClicked\",\"data\":{\"ownerID\":%lu},\"timestamp\":%lld}\n",
                        OwnerID, (long long)time(nullptr) * 1000);
                    fclose(logFile);
                }
                // #endregion agent log
                
                OnClicked();
                
                // #region agent log
                logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
                if (logFile) {
                    fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"CLICK_DONE\",\"location\":\"UI_InputComponent.cpp:149\",\"message\":\"OnClicked completed\",\"data\":{\"ownerID\":%lu},\"timestamp\":%lld}\n",
                        OwnerID, (long long)time(nullptr) * 1000);
                    fclose(logFile);
                }
                // #endregion agent log
            }
            else
            {
                ALICE_LOG_WARN("[UI_InputComponent] OnClicked callback is null! OwnerID=%lu", OwnerID);
            }
        }
        else
        {
            // Click 조건 불만족 로그 (디버깅용)
            #ifdef _DEBUG
            if (wasPressed && bPressedInside)
            {
                ALICE_LOG_INFO("[UI_InputComponent] Click condition not met: OwnerID=%lu, wasPressed=%d, bPressedInside=%d, inside=%d", 
                    OwnerID, wasPressed ? 1 : 0, bPressedInside ? 1 : 0, inside ? 1 : 0);
            }
            #endif
        }

        bPressedInside = false; // Reset
    }

    // 마우스가 밖으로 나가면 Press 상태 해제
    if (!inside && bIsPressed)
    {
        bIsPressed = false;
        bPressedInside = false;
    }

    // 이전 프레임 상태 저장
    bWasMouseDownPrev = isMouseDown;
}

bool UI_InputComponent::IsPointInside(const XMFLOAT2& unityPoint, UITransform& transform, UIWorld& world) const
{
    // unityPoint는 Unity 좌표계 (중앙 0,0 / 위가 양수, 아래가 음수)
    // 회전 여부와 관계없이 m_invWorldTrans를 사용하여 통일된 좌표계로 처리
    return IsPointInsideRotated(unityPoint, transform);
}

bool UI_InputComponent::IsPointInsideAABB(const XMFLOAT2& unityPoint, UITransform& transform) const
{
    // AABB도 m_invWorldTrans를 사용하여 Unity 좌표계로 통일
    return IsPointInsideRotated(unityPoint, transform);
}

bool UI_InputComponent::IsPointInsideRotated(const XMFLOAT2& unityPoint, UITransform& transform) const
{
    // unityPoint는 Unity 좌표계 (중앙 0,0 / 위가 양수, 아래가 음수)
    // m_invWorldTrans를 사용하여 로컬 좌표로 역변환
    float lx =
        unityPoint.x * transform.m_invWorldTrans._11 +
        unityPoint.y * transform.m_invWorldTrans._21 +
        transform.m_invWorldTrans._31;

    float ly =
        unityPoint.x * transform.m_invWorldTrans._12 +
        unityPoint.y * transform.m_invWorldTrans._22 +
        transform.m_invWorldTrans._32;

    // pivot 보정 (렌더와 동일한 방식)
    float px = transform.m_size.x * transform.m_pivot.x;
    float py = transform.m_size.y * transform.m_pivot.y;
    lx += px;
    ly += py;

    // 로컬 좌표 기준 Rect 범위 체크 (0 ~ size)
    return (lx >= 0.0f && lx <= transform.m_size.x &&
            ly >= 0.0f && ly <= transform.m_size.y);
}
