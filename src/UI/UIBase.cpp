#include "UIBase.h"
#include "UITransform.h"
#include "../Core/Delegate.h"
#include "UICompDelegate.h"

void UIBase::Initalize(UIRenderStruct& UIRenderStruct, CompDelegates& worldDelegates)
{
	m_UIRenderStruct = &UIRenderStruct;

	// World 쪽 CompDelegates 포인터 저장
	m_world = &worldDelegates;

	// Transform 필수 생성 + 캐시 (UIBase가 책임)
	if (!Transform)
	{
		Transform = AddComponent<UITransform>();
		if (Transform)
		{
			Transform->m_screenSize = { (float)m_UIRenderStruct->m_width, (float)m_UIRenderStruct->m_height };
		}
	}
	assert(Transform && "UIBase::Initalize: Transform must be set");
}



bool UIBase::IsMouseOverUIAABB(DirectX::XMFLOAT2& unityPoint, std::vector<long unsigned>& IDStorage)
{
	auto& m_transform = this->GetTransform();
	
	// unityPoint는 Unity 좌표계 (중앙 0,0 / 위가 양수, 아래가 음수)
	// m_invWorldTrans를 사용하여 로컬 좌표로 역변환
	float lx =
		unityPoint.x * m_transform.m_invWorldTrans._11 +
		unityPoint.y * m_transform.m_invWorldTrans._21 +
		m_transform.m_invWorldTrans._31;

	float ly =
		unityPoint.x * m_transform.m_invWorldTrans._12 +
		unityPoint.y * m_transform.m_invWorldTrans._22 +
		m_transform.m_invWorldTrans._32;

	// pivot 보정 (렌더와 동일한 방식)
	float px = m_transform.m_size.x * m_transform.m_pivot.x;
	float py = m_transform.m_size.y * m_transform.m_pivot.y;
	lx += px;
	ly += py;

	// 로컬 좌표 기준 Rect 범위 체크 (0 ~ size)
	if (lx >= 0.0f && lx <= m_transform.m_size.x &&
		ly >= 0.0f && ly <= m_transform.m_size.y)
	{
		IDStorage.push_back(ID);
		return true;
	}
	
	return false;
}


bool UIBase::IsMouseOverUIRot(DirectX::XMFLOAT2& unityPoint)
{
	auto& tr = this->GetTransform();
	// unityPoint는 Unity 좌표계 (중앙 0,0 / 위가 양수, 아래가 음수)
	// m_invWorldTrans를 사용하여 로컬 좌표로 역변환
	float lx =
		unityPoint.x * tr.m_invWorldTrans._11 +
		unityPoint.y * tr.m_invWorldTrans._21 +
		tr.m_invWorldTrans._31;

	float ly =
		unityPoint.x * tr.m_invWorldTrans._12 +
		unityPoint.y * tr.m_invWorldTrans._22 +
		tr.m_invWorldTrans._32;

	// pivot 보정 (렌더와 동일한 방식)
	float px = tr.m_pivot.x * tr.m_size.x;
	float py = tr.m_pivot.y * tr.m_size.y;
	lx += px;
	ly += py;
	if(lx >= 0.0f && lx <= tr.m_size.x &&
		ly >= 0.0f && ly <= tr.m_size.y)
	int a = 0;
	// 로컬 좌표계 (0, 0) ~ (size.x, size.y) 범위 체크
	return (lx >= 0.0f && lx <= tr.m_size.x &&
		ly >= 0.0f && ly <= tr.m_size.y);
}
