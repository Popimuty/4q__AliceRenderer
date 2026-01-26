#include "UIButton.h"
#include "UISceneManager.h"
#include "Core/InputSystem.h"
#include "Core/Logger.h"
#include <cassert>
#include <ctime>
#include <cstdio>

UIButton::UIButton()
{
	// InputComponent만 직접 생성 (이미지는 자식 UIImage로 관리)
	m_input = new UI_InputComponent();
}

UIButton::~UIButton()
{
	// InputComponent만 직접 삭제 (이미지는 자식 오브젝트로 관리되므로 자동 삭제됨)
	if (m_input) delete m_input;
}

void UIButton::Initalize(UIRenderStruct& UIRenderStruct, CompDelegates& tmpDelegate)
{
	// 부모 초기화 (Transform 생성)
	UIBase::Initalize(UIRenderStruct, tmpDelegate);

	// InputComponent 초기화
	if (m_input)
	{
		m_input->Owner = this;
		m_input->OwnerID = ID;
		m_input->OnAdded();
	}

	// SetupParts로 연결된 자식 UIImage가 있다면 초기 표시 상태 설정
	// Normal만 보이게, 나머지는 scale 0으로 숨김
	if (m_imgNormal)
	{
		m_imgNormal->GetTransform().m_scale = DirectX::XMFLOAT2(1.0f, 1.0f);
	}
	if (m_imgHover)
	{
		m_imgHover->GetTransform().m_scale = DirectX::XMFLOAT2(0.0f, 0.0f);
	}
	if (m_imgPressed)
	{
		m_imgPressed->GetTransform().m_scale = DirectX::XMFLOAT2(0.0f, 0.0f);
	}
	if (m_imgClicked)
	{
		m_imgClicked->GetTransform().m_scale = DirectX::XMFLOAT2(0.0f, 0.0f);
	}

	// 이전 상태 초기화
	m_prevIsPressed = false;
	m_prevIsHovered = false;
}

void UIButton::Update(float deltaTime)
{
	// 즉시 로그 출력 (함수 진입 확인용)
	ALICE_LOG_INFO("[UIButton] Update called: ID=%lu, deltaTime=%f", ID, deltaTime);
	
	// #region agent log
	FILE* logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
	if (logFile) {
		fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"A\",\"location\":\"UIButton.cpp:56\",\"message\":\"UIButton::Update called\",\"data\":{\"buttonID\":%lu,\"m_input\":%p,\"bIsHovered\":%d,\"bIsPressed\":%d,\"m_prevIsHovered\":%d,\"m_prevIsPressed\":%d},\"timestamp\":%lld}\n",
			ID, m_input, m_input ? m_input->bIsHovered : -1, m_input ? m_input->bIsPressed : -1, m_prevIsHovered ? 1 : 0, m_prevIsPressed ? 1 : 0, (long long)time(nullptr) * 1000);
		fclose(logFile);
	}
	// #endregion agent log

	if (!m_input) 
	{
		ALICE_LOG_WARN("[UIButton] Update early return: m_input is null, ID=%lu", ID);
		return;
	}

	// 현재 상태 확인
	bool isPressed = m_input->bIsPressed;
	bool isHovered = m_input->bIsHovered;

	// #region agent log
	logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
	if (logFile) {
		fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"B\",\"location\":\"UIButton.cpp:65\",\"message\":\"UIButton::Update state check\",\"data\":{\"buttonID\":%lu,\"isHovered\":%d,\"isPressed\":%d,\"stateChanged\":%d},\"timestamp\":%lld}\n",
			ID, isHovered ? 1 : 0, isPressed ? 1 : 0, ((isPressed != m_prevIsPressed || isHovered != m_prevIsHovered) ? 1 : 0), (long long)time(nullptr) * 1000);
		fclose(logFile);
	}
	// #endregion agent log

	// 상태 변경 시에만 업데이트
	if (isPressed != m_prevIsPressed || isHovered != m_prevIsHovered)
	{
		// #region agent log
		logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
		if (logFile) {
			fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"F\",\"location\":\"UIButton.cpp:85\",\"message\":\"UIButton::Update state changed\",\"data\":{\"buttonID\":%lu,\"isHovered\":%d,\"isPressed\":%d,\"m_imgNormal\":%p,\"m_imgHover\":%p},\"timestamp\":%lld}\n",
				ID, isHovered ? 1 : 0, isPressed ? 1 : 0, m_imgNormal, m_imgHover, (long long)time(nullptr) * 1000);
			fclose(logFile);
		}
		// #endregion agent log

		// 상태 우선순위: Pressed > Hover > Normal
		if (isPressed)
		{
			// Pressed 상태
			if (m_imgPressed) m_imgPressed->GetTransform().m_scale = DirectX::XMFLOAT2(1.0f, 1.0f);
			if (m_imgHover) m_imgHover->GetTransform().m_scale = DirectX::XMFLOAT2(0.0f, 0.0f);
			if (m_imgNormal) m_imgNormal->GetTransform().m_scale = DirectX::XMFLOAT2(0.0f, 0.0f);
			if (m_imgClicked) m_imgClicked->GetTransform().m_scale = DirectX::XMFLOAT2(0.0f, 0.0f);
		}
		else if (isHovered)
		{
			// Hover 상태
			if (m_imgHover) 
			{
				m_imgHover->GetTransform().m_scale = DirectX::XMFLOAT2(1.0f, 1.0f);
				// #region agent log
				logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
				if (logFile) {
					fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"G\",\"location\":\"UIButton.cpp:99\",\"message\":\"UIButton::Update set hover scale\",\"data\":{\"buttonID\":%lu,\"hoverScaleX\":%f,\"hoverScaleY\":%f},\"timestamp\":%lld}\n",
						ID, m_imgHover->GetTransform().m_scale.x, m_imgHover->GetTransform().m_scale.y, (long long)time(nullptr) * 1000);
					fclose(logFile);
				}
				// #endregion agent log
			}
			if (m_imgNormal) m_imgNormal->GetTransform().m_scale = DirectX::XMFLOAT2(0.0f, 0.0f);
			if (m_imgPressed) m_imgPressed->GetTransform().m_scale = DirectX::XMFLOAT2(0.0f, 0.0f);
			if (m_imgClicked) m_imgClicked->GetTransform().m_scale = DirectX::XMFLOAT2(0.0f, 0.0f);
		}
		else
		{
			// Normal 상태
			if (m_imgNormal) m_imgNormal->GetTransform().m_scale = DirectX::XMFLOAT2(1.0f, 1.0f);
			if (m_imgHover) m_imgHover->GetTransform().m_scale = DirectX::XMFLOAT2(0.0f, 0.0f);
			if (m_imgPressed) m_imgPressed->GetTransform().m_scale = DirectX::XMFLOAT2(0.0f, 0.0f);
			if (m_imgClicked) m_imgClicked->GetTransform().m_scale = DirectX::XMFLOAT2(0.0f, 0.0f);
		}

		m_prevIsPressed = isPressed;
		m_prevIsHovered = isHovered;
	}
}

void UIButton::UpdateInput(UIWorld& world, Alice::InputSystem& input)
{
	ALICE_LOG_INFO("[UIButton] UpdateInput called: buttonID=%lu, m_input=%p", ID, m_input);
	
	if (m_input)
	{
		bool beforeHovered = m_input->bIsHovered;
		bool beforePressed = m_input->bIsPressed;
		bool hadOnClicked = static_cast<bool>(m_input->OnClicked);
		
		ALICE_LOG_INFO("[UIButton] UpdateInput: Before Update - hovered=%d, pressed=%d, OnClicked valid=%d", 
			beforeHovered ? 1 : 0, beforePressed ? 1 : 0, hadOnClicked ? 1 : 0);
		
		m_input->Update(world, input);
		
		bool afterHovered = m_input->bIsHovered;
		bool afterPressed = m_input->bIsPressed;
		
		ALICE_LOG_INFO("[UIButton] UpdateInput: After Update - hovered=%d, pressed=%d", 
			afterHovered ? 1 : 0, afterPressed ? 1 : 0);
	}
	else
	{
		ALICE_LOG_WARN("[UIButton] UpdateInput: m_input is null! buttonID=%lu", ID);
	}
}

void UIButton::Render()
{
	// 자식 UIImage는 UIWorld 트리 렌더에서 그려지므로 여기서는 아무것도 하지 않음
}

void UIButton::SetupParts(UIImage* normal, UIImage* hover, UIImage* pressed, UIImage* clicked)
{
	m_imgNormal = normal;
	m_imgHover = hover;
	m_imgPressed = pressed;
	m_imgClicked = clicked;
}

void UIButton::SetNormalImage(const std::wstring& path)
{
	if (m_imgNormal)
	{
		m_imgNormal->createImage(path);
	}
}

void UIButton::SetHoverImage(const std::wstring& path)
{
	if (m_imgHover)
	{
		m_imgHover->createImage(path);
	}
}

void UIButton::SetPressedImage(const std::wstring& path)
{
	if (m_imgPressed)
	{
		m_imgPressed->createImage(path);
	}
}

void UIButton::SetClickedImage(const std::wstring& path)
{
	if (m_imgClicked)
	{
		m_imgClicked->createImage(path);
	}
}

void UIButton::SetOnHoverBegin(std::function<void()> callback)
{
	if (m_input)
	{
		m_input->OnHoverBegin = callback;
	}
}

void UIButton::SetOnHoverEnd(std::function<void()> callback)
{
	if (m_input)
	{
		m_input->OnHoverEnd = callback;
	}
}

void UIButton::SetOnPressed(std::function<void()> callback)
{
	if (m_input)
	{
		m_input->OnPressed = callback;
	}
}

void UIButton::SetOnReleased(std::function<void()> callback)
{
	if (m_input)
	{
		m_input->OnReleased = callback;
	}
}

void UIButton::SetOnClicked(std::function<void()> callback)
{
	ALICE_LOG_INFO("[UIButton] SetOnClicked called - buttonID=%lu, m_input=%p, callback valid=%d", 
		ID, m_input, callback ? 1 : 0);
	
	if (m_input)
	{
		// 이전 콜백이 있었는지 확인
		bool hadPreviousCallback = static_cast<bool>(m_input->OnClicked);
		
		m_input->OnClicked = callback;
		
		// 바인딩 검증: 콜백이 제대로 설정되었는지 확인
		bool callbackSet = static_cast<bool>(m_input->OnClicked);
		ALICE_LOG_INFO("[UIButton] SetOnClicked: callback set=%d (hadPrevious=%d)", 
			callbackSet ? 1 : 0, hadPreviousCallback ? 1 : 0);
		
		if (!callbackSet && callback)
		{
			ALICE_LOG_WARN("[UIButton] SetOnClicked: WARNING - callback was provided but not set! (DLL boundary issue?)");
		}
	}
	else
	{
		ALICE_LOG_WARN("[UIButton] SetOnClicked: m_input is null! (InputComponent not initialized)");
	}
}

