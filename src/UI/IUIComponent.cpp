#include "IUIComponent.h"
#include "UIBase.h"

// 편의 함수: Owner 이름 조회 구현
const std::string& IUIComponent::GetOwnerName() const
{
    static const std::string emptyString;
    return Owner ? Owner->GetName() : emptyString;
}
