#pragma once
#include "UIBase.h"
#include "UI_InputComponent.h"
#include "UIImage.h"
#include <DirectXMath.h>


// 아직 프로토타입
class UIButton : public UIBase
{
public:
	UIButton();
	virtual ~UIButton() override;
	
	const char* GetTypeName() const override { return "UIButton"; }

	void Initalize(UIRenderStruct& UIRenderStruct, CompDelegates& tmpDelegate) override;
	void Update(float deltaTime) override;
	void Render() override;

	// 자식 UIImage 연결
	void SetupParts(UIImage* normal, UIImage* hover, UIImage* pressed, UIImage* clicked);

	// 이미지 경로 설정
	void SetNormalImage(const std::wstring& path);
	void SetHoverImage(const std::wstring& path);
	void SetPressedImage(const std::wstring& path);
	void SetClickedImage(const std::wstring& path);

	// InputComponent 이벤트 콜백 설정
	void SetOnHoverBegin(std::function<void()> callback);
	void SetOnHoverEnd(std::function<void()> callback);
	void SetOnPressed(std::function<void()> callback);
	void SetOnReleased(std::function<void()> callback);
	void SetOnClicked(std::function<void()> callback);

	// 상태 확인
	bool IsHovered() const { return m_input ? m_input->bIsHovered : false; }
	bool IsPressed() const { return m_input ? m_input->bIsPressed : false; }

	// InputComponent 업데이트 (UIWorld와 InputSystem 필요)
	void UpdateInput(UIWorld& world, Alice::InputSystem& input);

private:
	// InputComponent (멤버로 직접 관리)
	UI_InputComponent* m_input = nullptr;

	// 자식 UIImage 4개 참조
	UIImage* m_imgNormal = nullptr;
	UIImage* m_imgHover = nullptr;
	UIImage* m_imgPressed = nullptr;
	UIImage* m_imgClicked = nullptr;

	// 이전 상태 추적 (상태 변경 시에만 업데이트)
	bool m_prevIsPressed = false;
	bool m_prevIsHovered = false;
};
