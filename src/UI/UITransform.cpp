#include "UITransform.h"

// 크기 세팅 함수
void UITransform::SetTranslation(float x, float y)
{
	m_translation.x = x; m_translation.y = y;
}

void UITransform::SetRotation(float InRotation)
{
	m_rotation = InRotation;
}

void UITransform::SetScale(float scaleX, float scaleY)
{
	m_scale.x = scaleX; m_scale.y = scaleY;
}

void UITransform::SetPivot(float pivotX, float pivotY) 
{
	m_pivot.x = pivotX; m_pivot.y = pivotY;
}

void UITransform::Reset()
{
	m_scale = { 1.0f, 1.0f };
	m_rotation = { 0.0f };
	m_translation = { 0.0f, 0.0f };
}


void UITransform::LocalMat(D2D1::Matrix3x2F& m_localTrans)
{
	// pivot center 인자 제거: Scale/Rotation은 pivot 없이 계산
	// pivot 보정은 렌더 단계에서 Rect 오프셋으로 처리
	m_localTrans =
		D2D1::Matrix3x2F::Scale(m_scale.x, m_scale.y) *
		D2D1::Matrix3x2F::Rotation(m_rotation) *
		D2D1::Matrix3x2F::Translation(m_translation.x, m_translation.y);
}


D2D1::Matrix3x2F UITransform::WorldMatrix(const D2D1::Matrix3x2F& WorldMat)
{
	D2D1::Matrix3x2F LocalMatrix;
	LocalMat(LocalMatrix);
	m_worldTrans = LocalMatrix * WorldMat;

	// Unity 좌표계 기준 역행렬 계산
	m_invWorldTrans = m_worldTrans;
	BOOL invertSuccess = m_invWorldTrans.Invert();
	if (!invertSuccess)
	{
		// 역행렬 계산 실패 시 Identity로 초기화
		m_invWorldTrans = D2D1::Matrix3x2F::Identity();
	}

	// D2D 좌표계 변환 행렬 계산 (ConVertD2DPos의 역행렬용)
	// toD2D = Scale(1,-1) * Translation(w/2,h/2) (한 번만 적용)
	D2D1::Matrix3x2F toD2D =
		D2D1::Matrix3x2F::Scale(1.0f, -1.0f) *
		D2D1::Matrix3x2F::Translation(
			m_screenSize.x * 0.5f,
			m_screenSize.y * 0.5f
		);

	D2D1::Matrix3x2F d2dTransform = m_worldTrans * toD2D;
	m_invD2DTrans = d2dTransform;
	invertSuccess = m_invD2DTrans.Invert();
	if (!invertSuccess)
	{
		// 역행렬 계산 실패 시 Identity로 초기화
		m_invD2DTrans = D2D1::Matrix3x2F::Identity();
	}

	return m_worldTrans;
}


// 유니티 -> D2D  : Render용 좌표 계산
D2D1::Matrix3x2F UITransform::ConVertD2DPos() {
	// Unity 좌표계 -> D2D 좌표계 변환: Y-flip + 화면 중앙 이동
	// toD2D = Scale(1,-1) * Translation(w/2,h/2) (한 번만 적용)
	D2D1::Matrix3x2F toD2D =
		D2D1::Matrix3x2F::Scale(1.0f, -1.0f) *
		D2D1::Matrix3x2F::Translation(
			m_screenSize.x * 0.5f,
			m_screenSize.y * 0.5f
		);

	return m_worldTrans * toD2D;
}


void UITransform::GetTransformMat(D2D1::Matrix3x2F& tmp)
{
	tmp = m_worldTrans;
}

