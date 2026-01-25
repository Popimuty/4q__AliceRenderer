#pragma once
#include "IUIComponent.h"
#include <dwrite.h>
#include <d2d1.h>
#include <wrl/client.h>
#include <string>
//#include <wstring>
#include <float.h> // FLT_MAX
#include "UIRenderStruct.h"
#include "UITransform.h"

#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d2d1.lib")

using Microsoft::WRL::ComPtr;

class UI_TextComponent : public IUIComponent
{
public:
	// 복사/이동 방지 (컴포넌트는 unique_ptr로 관리되므로 복사/이동 불필요)
	UI_TextComponent() = default;
	~UI_TextComponent() = default;
	UI_TextComponent(const UI_TextComponent&) = delete;
	UI_TextComponent& operator=(const UI_TextComponent&) = delete;
	UI_TextComponent(UI_TextComponent&&) = delete;
	UI_TextComponent& operator=(UI_TextComponent&&) = delete;

	void Initalize(UIRenderStruct& UIRenderStruct);
	void Update() override;
	void Render() override;

	// 텍스트 설정
	void SetText(const std::wstring& text);
	const std::wstring& GetText() const { return m_text; }

	// 폰트 설정
	void SetFontName(const std::wstring& fontName);
	void SetFontSize(float fontSize);
	
	// 텍스트 색상 설정
	void SetTextColor(const D2D1_COLOR_F& color);
	void SetTextColor(float r, float g, float b, float a = 1.0f);

	// 텍스트 레이아웃 크기 설정 (너비/높이 제한)
	void SetMaxWidth(float maxWidth);
	void SetMaxHeight(float maxHeight);

	UIRenderStruct* m_UIRenderStruct{ nullptr };

private:
	// 텍스트 레이아웃 갱신 (m_text가 변경될 때 호출)
	void UpdateTextLayout();

	// DirectWrite 리소스
	ComPtr<IDWriteTextFormat> m_textFormat;
	ComPtr<IDWriteTextLayout> m_textLayout;
	ComPtr<ID2D1SolidColorBrush> m_textBrush;

	// 텍스트 데이터
	std::wstring m_text;
	std::wstring m_fontName{ L"Arial" };
	float m_fontSize{ 16.0f };
	float m_maxWidth{ 0.0f };  // 0이면 제한 없음
	float m_maxHeight{ 0.0f }; // 0이면 제한 없음
	D2D1_COLOR_F m_textColor{ D2D1::ColorF(D2D1::ColorF::White) };

	// 텍스트가 변경되었는지 추적
	bool m_textDirty{ true };
	bool m_formatDirty{ true };
};
