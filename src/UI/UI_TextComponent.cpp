#include "UI_TextComponent.h"
#include "Core/Logger.h"
#include "UI/UITransform.h"
#include "UI/UIBase.h"
#include <cassert>

void UI_TextComponent::Initalize(UIRenderStruct& UIRenderStruct)
{
	m_UIRenderStruct = &UIRenderStruct;

	// 텍스트 브러시 생성
	if (m_UIRenderStruct->m_d2DdevCon)
	{
		HRESULT hr = m_UIRenderStruct->m_d2DdevCon->CreateSolidColorBrush(
			m_textColor,
			m_textBrush.GetAddressOf()
		);
		if (FAILED(hr))
		{
			ALICE_LOG_ERRORF("[UI_TextComponent] Failed to create text brush: HRESULT=0x%08X", hr);
		}
	}

	// 초기 텍스트 포맷 생성
	m_formatDirty = true;
	UpdateTextLayout();
}

void UI_TextComponent::Update()
{
	// 텍스트나 포맷이 변경되었으면 레이아웃 갱신
	if (m_textDirty || m_formatDirty)
	{
		UpdateTextLayout();
		m_textDirty = false;
		m_formatDirty = false;
	}
}

void UI_TextComponent::Render()
{
	if (!m_UIRenderStruct || !m_UIRenderStruct->m_d2DdevCon)
	{
		ALICE_LOG_WARN("[UI_TextComponent] Render skipped: render struct or device context is null");
		return;
	}

	if (!m_textLayout || !m_textBrush)
	{
		return;
	}

	// Owner의 Transform을 통해 월드 위치 계산
	if (!Owner)
	{
		ALICE_LOG_WARN("[UI_TextComponent] Render skipped: Owner is null");
		return;
	}

	auto& transform = Owner->GetTransform();
	
	// pivot 보정: 텍스트 레이아웃의 위치를 pivot에 맞게 조정
	// DrawTextLayout의 position은 텍스트의 좌상단 위치
	// pivot offset을 빼서 올바른 위치에 그리기
	float pivotOffsetX = transform.m_pivot.x * transform.m_size.x;
	float pivotOffsetY = transform.m_pivot.y * transform.m_size.y;
	
	D2D1_POINT_2F position = D2D1::Point2F(
		transform.m_translation.x - pivotOffsetX,
		transform.m_translation.y - pivotOffsetY
	);

	// 텍스트 레이아웃 렌더링
	m_UIRenderStruct->m_d2DdevCon->DrawTextLayout(
		position,
		m_textLayout.Get(),
		m_textBrush.Get()
	);
}

void UI_TextComponent::SetText(const std::wstring& text)
{
	if (m_text != text)
	{
		m_text = text;
		m_textDirty = true;
	}
}

void UI_TextComponent::SetFontName(const std::wstring& fontName)
{
	if (m_fontName != fontName)
	{
		m_fontName = fontName;
		m_formatDirty = true;
	}
}

void UI_TextComponent::SetFontSize(float fontSize)
{
	if (m_fontSize != fontSize && fontSize > 0.0f)
	{
		m_fontSize = fontSize;
		m_formatDirty = true;
	}
}

void UI_TextComponent::SetTextColor(const D2D1_COLOR_F& color)
{
	m_textColor = color;
	if (m_textBrush)
	{
		m_textBrush->SetColor(color);
	}
}

void UI_TextComponent::SetTextColor(float r, float g, float b, float a)
{
	SetTextColor(D2D1::ColorF(r, g, b, a));
}

void UI_TextComponent::SetMaxWidth(float maxWidth)
{
	if (m_maxWidth != maxWidth)
	{
		m_maxWidth = maxWidth;
		m_textDirty = true; // 레이아웃 재계산 필요
	}
}

void UI_TextComponent::SetMaxHeight(float maxHeight)
{
	if (m_maxHeight != maxHeight)
	{
		m_maxHeight = maxHeight;
		m_textDirty = true; // 레이아웃 재계산 필요
	}
}

void UI_TextComponent::UpdateTextLayout()
{
	if (!m_UIRenderStruct || !m_UIRenderStruct->m_D3DWFactory)
	{
		ALICE_LOG_WARN("[UI_TextComponent] UpdateTextLayout skipped: render struct or DWrite factory is null");
		return;
	}

	// TextFormat이 없거나 폰트 설정이 변경되었으면 재생성
	if (!m_textFormat || m_formatDirty)
	{
		m_textFormat.Reset();

		HRESULT hr = m_UIRenderStruct->m_D3DWFactory->CreateTextFormat(
			m_fontName.c_str(),
			nullptr, // 폰트 컬렉션 (nullptr = 시스템 폰트)
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			m_fontSize,
			L"", // 로케일 (빈 문자열 = 시스템 기본)
			m_textFormat.GetAddressOf()
		);

		if (FAILED(hr))
		{
			ALICE_LOG_ERRORF("[UI_TextComponent] Failed to create text format: HRESULT=0x%08X, fontName=%S", hr, m_fontName.c_str());
			m_textLayout.Reset();
			return;
		}

		// 텍스트 정렬 설정 (왼쪽 정렬)
		m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	}

	// TextLayout 생성/갱신
	if (m_text.empty())
	{
		m_textLayout.Reset();
		return;
	}

	// 최대 너비/높이 계산 (Owner의 Transform size 사용 또는 설정된 값)
	float layoutWidth = m_maxWidth > 0.0f ? m_maxWidth : 0.0f;
	float layoutHeight = m_maxHeight > 0.0f ? m_maxHeight : 0.0f;

	// Owner가 있고 크기가 설정되지 않았으면 Transform의 size 사용
	if (Owner && (layoutWidth <= 0.0f || layoutHeight <= 0.0f))
	{
		auto& transform = Owner->GetTransform();
		if (layoutWidth <= 0.0f) layoutWidth = transform.m_size.x;
		if (layoutHeight <= 0.0f) layoutHeight = transform.m_size.y;
	}

	// 크기가 0이면 제한 없음으로 처리
	if (layoutWidth <= 0.0f) layoutWidth = FLT_MAX;
	if (layoutHeight <= 0.0f) layoutHeight = FLT_MAX;

	HRESULT hr = m_UIRenderStruct->m_D3DWFactory->CreateTextLayout(
		m_text.c_str(),
		static_cast<UINT32>(m_text.length()),
		m_textFormat.Get(),
		layoutWidth,
		layoutHeight,
		m_textLayout.GetAddressOf()
	);

	if (FAILED(hr))
	{
		ALICE_LOG_ERRORF("[UI_TextComponent] Failed to create text layout: HRESULT=0x%08X", hr);
		m_textLayout.Reset();
		return;
	}
}
