#include "UITextObject.h"
#include "Core/Logger.h"
#include <cassert>

// 정적 멤버 초기화
const std::wstring UITextObject::m_emptyText = L"";

UITextObject::UITextObject()
{
	// TextComponent 직접 생성
	m_textComponent = new UI_TextComponent();
}

UITextObject::~UITextObject()
{
	// TextComponent 직접 삭제
	if (m_textComponent)
	{
		delete m_textComponent;
		m_textComponent = nullptr;
	}
}

void UITextObject::Initalize(UIRenderStruct& UIRenderStruct, CompDelegates& tmpDelegate)
{
	// 부모 초기화 (Transform 생성)
	UIBase::Initalize(UIRenderStruct, tmpDelegate);

	// TextComponent 초기화
	if (m_textComponent)
	{
		m_textComponent->Owner = this;
		m_textComponent->OwnerID = ID;
		m_textComponent->Initalize(UIRenderStruct);
		m_textComponent->OnAdded();
	}
}

void UITextObject::Update(float deltaTime)
{
	// TextComponent 업데이트
	if (m_textComponent)
	{
		m_textComponent->Update();
	}
}

void UITextObject::Render()
{
	// TextComponent 렌더링
	if (m_textComponent)
	{
		m_textComponent->Render();
	}
}
