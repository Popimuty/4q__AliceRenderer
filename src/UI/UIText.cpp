#include "UIText.h"
#include "UIRenderStruct.h"
#include "Core/Logger.h"
#include <cassert>

UIText::UIText()
	: m_textDirty(true)
	, m_formatDirty(true)
{
}

UIText::~UIText()
{
	// ComPtr이 자동으로 해제됨
}

void UIText::Initalize(UIRenderStruct& UIRenderStruct, CompDelegates& tmpDelegate)
{
	UIBase::Initalize(UIRenderStruct, tmpDelegate);
	m_renderStruct = &UIRenderStruct;

	// 텍스트 브러시 생성
	if (m_renderStruct->m_d2DdevCon)
	{
		HRESULT hr = m_renderStruct->m_d2DdevCon->CreateSolidColorBrush(
			m_textColor,
			m_textBrush.GetAddressOf()
		);
		if (FAILED(hr))
		{
			ALICE_LOG_ERRORF("[UIText] Failed to create text brush: HRESULT=0x%08X", hr);
		}
	}

	// 초기 텍스트 포맷 생성
	m_formatDirty = true;
	UpdateTextLayout();
}

void UIText::Update(float deltaTime)
{
	// 텍스트나 포맷이 변경되었으면 레이아웃 갱신
	if (m_textDirty || m_formatDirty)
	{
		UpdateTextLayout();
		m_textDirty = false;
		m_formatDirty = false;
	}
}

void UIText::Render()
{
	if (!m_renderStruct || !m_renderStruct->m_d2DdevCon)
	{
		ALICE_LOG_WARN("[UIText] Render skipped: render struct or device context is null");
		return;
	}

	if (!m_textLayout || !m_textBrush)
	{
		return;
	}

	// Transform을 통해 월드 위치 계산
	auto& transform = GetTransform();
	D2D1_POINT_2F position = D2D1::Point2F(
		transform.m_translation.x,
		transform.m_translation.y
	);

	// 텍스트 레이아웃 렌더링
	m_renderStruct->m_d2DdevCon->DrawTextLayout(
		position,
		m_textLayout.Get(),
		m_textBrush.Get()
	);
}

void UIText::SetText(const std::wstring& text)
{
	if (m_text != text)
	{
		m_text = text;
		m_textDirty = true;
	}
}

void UIText::SetFontName(const std::wstring& fontName)
{
	if (m_fontName != fontName)
	{
		m_fontName = fontName;
		m_formatDirty = true;
	}
}

void UIText::SetFontSize(float fontSize)
{
	if (m_fontSize != fontSize && fontSize > 0.0f)
	{
		m_fontSize = fontSize;
		m_formatDirty = true;
	}
}

void UIText::SetTextColor(const D2D1_COLOR_F& color)
{
	m_textColor = color;
	if (m_textBrush)
	{
		m_textBrush->SetColor(color);
	}
}

void UIText::SetTextColor(float r, float g, float b, float a)
{
	SetTextColor(D2D1::ColorF(r, g, b, a));
}

void UIText::SetMaxWidth(float maxWidth)
{
	if (m_maxWidth != maxWidth)
	{
		m_maxWidth = maxWidth;
		m_textDirty = true; // 레이아웃 재계산 필요
	}
}

void UIText::SetMaxHeight(float maxHeight)
{
	if (m_maxHeight != maxHeight)
	{
		m_maxHeight = maxHeight;
		m_textDirty = true; // 레이아웃 재계산 필요
	}
}

void UIText::UpdateTextLayout()
{
	if (!m_renderStruct || !m_renderStruct->m_D3DWFactory)
	{
		ALICE_LOG_WARN("[UIText] UpdateTextLayout skipped: render struct or DWrite factory is null");
		return;
	}

	// TextFormat이 없거나 폰트 설정이 변경되었으면 재생성
	if (!m_textFormat || m_formatDirty)
	{
		m_textFormat.Reset();

		HRESULT hr = m_renderStruct->m_D3DWFactory->CreateTextFormat(
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
			ALICE_LOG_ERRORF("[UIText] Failed to create text format: HRESULT=0x%08X, fontName=%S", hr, m_fontName.c_str());
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

	// 최대 너비/높이 계산 (Transform의 size 사용 또는 설정된 값)
	float layoutWidth = m_maxWidth > 0.0f ? m_maxWidth : GetTransform().m_size.x;
	float layoutHeight = m_maxHeight > 0.0f ? m_maxHeight : GetTransform().m_size.y;

	// 크기가 0이면 제한 없음으로 처리
	if (layoutWidth <= 0.0f) layoutWidth = FLT_MAX;
	if (layoutHeight <= 0.0f) layoutHeight = FLT_MAX;

	HRESULT hr = m_renderStruct->m_D3DWFactory->CreateTextLayout(
		m_text.c_str(),
		static_cast<UINT32>(m_text.length()),
		m_textFormat.Get(),
		layoutWidth,
		layoutHeight,
		m_textLayout.GetAddressOf()
	);

	if (FAILED(hr))
	{
		ALICE_LOG_ERRORF("[UIText] Failed to create text layout: HRESULT=0x%08X", hr);
		m_textLayout.Reset();
		return;
	}
}
