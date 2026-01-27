#pragma once
#include "UIBase.h"
#include "UI_TextComponent.h"

class UITextObject : public UIBase
{
public:
	UITextObject();
	virtual ~UITextObject() override;

	const char* GetTypeName() const override { return "UITextObject"; }

	void Initalize(UIRenderStruct& UIRenderStruct, CompDelegates& tmpDelegate) override;
	void Update(float deltaTime) override;
	void Render() override;

	// UI_TextComponent 편의 메서드들
	void SetText(const std::wstring& text) { if (m_textComponent) m_textComponent->SetText(text); }
	const std::wstring& GetText() const { return m_textComponent ? m_textComponent->GetText() : m_emptyText; }

	void SetFontName(const std::wstring& fontName) { if (m_textComponent) m_textComponent->SetFontName(fontName); }
	void SetFontSize(float fontSize) { if (m_textComponent) m_textComponent->SetFontSize(fontSize); }

	void SetTextColor(const D2D1_COLOR_F& color) { if (m_textComponent) m_textComponent->SetTextColor(color); }
	void SetTextColor(float r, float g, float b, float a = 1.0f) { if (m_textComponent) m_textComponent->SetTextColor(r, g, b, a); }

	void SetMaxWidth(float maxWidth) { if (m_textComponent) m_textComponent->SetMaxWidth(maxWidth); }
	void SetMaxHeight(float maxHeight) { if (m_textComponent) m_textComponent->SetMaxHeight(maxHeight); }

	UI_TextComponent* GetTextComponent() { return m_textComponent; }

private:
	// TextComponent (멤버로 직접 관리)
	UI_TextComponent* m_textComponent = nullptr;
	static const std::wstring m_emptyText;
};
