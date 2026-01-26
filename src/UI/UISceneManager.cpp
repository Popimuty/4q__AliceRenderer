#include "UISceneManager.h"
#include <dxgi1_2.h>
#include <d2d1_1.h>
#include <dwrite.h>
#pragma comment(lib, "dwrite.lib")
#include <d2d1.h>
#pragma comment(lib, "d2d1.lib")
#include <wrl/client.h>
#include <string>
#include <stdexcept>
#include "Core/InputSystem.h"
#include "Core/Logger.h"
#include "UITransform.h"
#include "UI_ImageComponent.h"
#include "UI_ScriptComponent.h"
#include "IUIComponent.h"
#include "UIBase.h"
#include "UIScriptSystem.h"
#include "UIScriptFactory.h"
#include "UI_InputComponent.h"
#include "UIButton.h"
#include "imgui.h"
#include <cmath>
#include <typeinfo>
#include <ctime>
// ============================================================================
// UIWorld 구현 ����
// ============================================================================
void UIWorld::Initialize(UIRenderStruct* UIRst)
{
	assert(UIRst && "UIWorld::Initialize: UIRst must not be null");
	m_UIRenderStruct = UIRst;
	nowInteger = 1;

	// ��������Ʈ ���ε� (���ø� ���� ���� ����)
	m_worldDelegates.AddComponent.BindLambda(
		[this](ObjectID id, std::type_index type, std::function<void* ()> factory) -> void*
		{
			void* raw = nullptr;

			// UITextComponent ?�음 (추후 추�? ??복구)
			// if (type == typeid(UITextComponent))
			// {
			// 	if (out) *out = static_cast<void*>(this->CreateTextComponent(static_cast<unsigned long>(id)));
			// 	return;
			// }

			// 1) type �?고정 ?�성(?�드 ?�유)
			if (type == typeid(UITransform))
			{
				raw = static_cast<void*>(this->CreateTransformComponent(static_cast<unsigned long>(id)));
			}
			else if (type == typeid(UI_ImageComponent))
			{
				raw = static_cast<void*>(this->CreateImageComponent(static_cast<unsigned long>(id)));
			}
			else if (type == typeid(UI_ScriptComponent))
			{
				raw = static_cast<void*>(this->CreateScriptComponent(static_cast<unsigned long>(id)));
			}
			// 2) 기�? ?�?? factory ?�성(?�재???�유�?TODO)
			else if (factory)
			{
				raw = factory();
			}

			// 3) owner ?�팅 (공통)
			if (raw)
			{
				UIBase* owner = this->Get(static_cast<unsigned long>(id)); // UIWorld::Get
				// owner가 ?�을 ?�도 ?�으??체크
				if (owner)
				{
					// raw가 IUIComponent?�면 ?�팅
					IUIComponent* comp = static_cast<IUIComponent*>(raw);
					if (comp != nullptr)
					{
						comp->Owner = owner;
						comp->OwnerID = static_cast<unsigned long>(id);
						comp->World = this; // World ?�인??주입
						comp->OnAdded();
					}
				}
			}

			return raw;
		}
	);

	m_worldDelegates.FindComponent.BindLambda(
		[this](ObjectID id, std::type_index type) -> void*
		{

			// UITextComponent ?�음 (추후 추�? ??복구)
			// if (type == typeid(UITextComponent))
			// {
			// 	if (out) *out = static_cast<void*>(this->FindTextComponent(static_cast<unsigned long>(id)));
			// 	return;
			// }

			// UITransform 조회
			if (type == typeid(UITransform))
			{
				return static_cast<void*>(this->FindTransformComponent(static_cast<unsigned long>(id)));
			}
			// UI_ImageComponent 조회
			if (type == typeid(UI_ImageComponent))
			{
				return static_cast<void*>(this->FindImageComponent(static_cast<unsigned long>(id)));
			}
			if (type == typeid(UI_ScriptComponent))
			{
				return static_cast<void*>(this->FindScriptComponent(static_cast<unsigned long>(id)));
			}
			return nullptr;
		}
	);

	// 추후??bool type?�로 고치�?!
	m_worldDelegates.RemoveComponent.BindLambda(
		[this](ObjectID id, std::type_index type)
		{
			// UITextComponent ?�음 (추후 추�? ??복구)
			// if (type == typeid(UITextComponent))
			// {
			// 	this->RemoveTextComponent(static_cast<unsigned long>(id));
			// 	return;
			// }

			// UITransform ??��
			if (type == typeid(UITransform))
			{
				this->RemoveTransformComponent(static_cast<unsigned long>(id));
				return;
			}
			// UI_ImageComponent ??��
			if (type == typeid(UI_ImageComponent))
			{
				this->RemoveImageComponent(static_cast<unsigned long>(id));
				return;
			}
			if (type == typeid(UI_ScriptComponent))
			{
				this->RemoveScriptComponent(static_cast<unsigned long>(id));
				return;
			}
		}
	);
}


// UIGaugeBar ?�플�??�수?? 배경 ?��?지?� 게이지 �??��?지�??�동 ?�성
template<>
UIGaugeBar* UIWorld::CreateEntity<UIGaugeBar>()
{
	assert(m_UIRenderStruct && "UIWorld::Initialize() must be called before CreateEntity");

	// 
	auto pUIObj = std::unique_ptr<UIGaugeBar>(new UIGaugeBar());
	UIGaugeBar* ObjPtr = pUIObj.get();
	long unsigned gaugeBarID = this->nowInteger;
	ObjPtr->SetID(gaugeBarID);

	pUIObjStorage.emplace(gaugeBarID, std::move(pUIObj));
	m_rootID.push_back(gaugeBarID); // 루트 
	
	// 
	std::string defaultName = "UI_" + std::to_string(gaugeBarID);
	RegisterName(gaugeBarID, defaultName);
	ObjPtr->SetName(defaultName);
	
	this->nowInteger++;

	//
	UIImage* bgImage = CreateChildEntity<UIImage>(gaugeBarID);
	UIImage* barImage = CreateChildEntity<UIImage>(gaugeBarID);
	UIImage* lerpDImage = CreateChildEntity<UIImage>(gaugeBarID);
	UIImage* lerpUImage = CreateChildEntity<UIImage>(gaugeBarID);

	// UI_ImageComponent
	// Background:
	if (bgImage)
	{
		if (auto* imgComp = TryGetComponent<UI_ImageComponent>(bgImage->getID()))
		{
			imgComp->SetFallbackColor(D2D1::ColorF(0.2f, 0.2f, 0.2f, 1.0f)); // 회색
		}
	}
	if (barImage)
	{
		if (auto* imgComp = TryGetComponent<UI_ImageComponent>(barImage->getID()))
		{
			imgComp->SetFallbackColor(D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f)); // 초록
		}
	}
	if (lerpDImage)
	{
		if (auto* imgComp = TryGetComponent<UI_ImageComponent>(lerpDImage->getID()))
		{
			imgComp->SetFallbackColor(D2D1::ColorF(1.0f, 0.0f, 0.0f, 1.0f)); // 빨간
		}
	}
	if (lerpUImage)
	{
		if (auto* imgComp = TryGetComponent<UI_ImageComponent>(lerpUImage->getID()))
		{
			imgComp->SetFallbackColor(D2D1::ColorF(0.0f, 0.0f, 1.0f, 1.0f)); // 파란
		}
	}

	
	ObjPtr->SetupParts(bgImage, barImage, lerpUImage, lerpDImage);

	
	ObjPtr->Initalize(*m_UIRenderStruct, m_worldDelegates);

	
	if (ObjPtr->Transform)
	{
		ObjPtr->Transform->m_size = DirectX::XMFLOAT2(400.0f, 50.0f);
	}

	// 
	DirectX::XMFLOAT2 childSize(400.0f, 50.0f);
	if (bgImage && bgImage->Transform)
	{
		bgImage->Transform->m_size = childSize;
	}
	if (barImage && barImage->Transform)
	{
		barImage->Transform->m_size = childSize;
	}
	if (lerpDImage && lerpDImage->Transform)
	{
		lerpDImage->Transform->m_size = childSize;
	}
	if (lerpUImage && lerpUImage->Transform)
	{
		lerpUImage->Transform->m_size = childSize;
	}

	return ObjPtr;
}

// UIButton
template<>
UIButton* UIWorld::CreateEntity<UIButton>()
{
	assert(m_UIRenderStruct && "UIWorld::Initialize() must be called before CreateEntity");

	// 
	auto pUIObj = std::unique_ptr<UIButton>(new UIButton());
	UIButton* ObjPtr = pUIObj.get();
	long unsigned buttonID = this->nowInteger;
	ObjPtr->SetID(buttonID);

	pUIObjStorage.emplace(buttonID, std::move(pUIObj));
	m_rootID.push_back(buttonID); // 루트 
	
	// 
	std::string defaultName = "UI_" + std::to_string(buttonID);
	RegisterName(buttonID, defaultName);
	ObjPtr->SetName(defaultName);
	
	this->nowInteger++;

	//  UIImage 
	UIImage* normalImage = CreateChildEntity<UIImage>(buttonID);
	UIImage* hoverImage = CreateChildEntity<UIImage>(buttonID);
	UIImage* pressedImage = CreateChildEntity<UIImage>(buttonID);
	UIImage* clickedImage = CreateChildEntity<UIImage>(buttonID);

	// �??��?지??UI_ImageComponent???�로 ?�른 기본 ?�상 ?�정
	// Normal: ?�색, Hover: 밝�? ?�색, Pressed: ?�두???�색, Clicked: ?��???
	if (normalImage)
	{
		if (auto* imgComp = TryGetComponent<UI_ImageComponent>(normalImage->getID()))
		{
			imgComp->SetFallbackColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f)); // ?�색
		}
	}
	if (hoverImage)
	{
		if (auto* imgComp = TryGetComponent<UI_ImageComponent>(hoverImage->getID()))
		{
			imgComp->SetFallbackColor(D2D1::ColorF(0.8f, 0.8f, 0.8f, 1.0f)); // 밝�? ?�색
		}
	}
	if (pressedImage)
	{
		if (auto* imgComp = TryGetComponent<UI_ImageComponent>(pressedImage->getID()))
		{
			imgComp->SetFallbackColor(D2D1::ColorF(0.4f, 0.4f, 0.4f, 1.0f)); // ?�두???�색
		}
	}
	if (clickedImage)
	{
		if (auto* imgComp = TryGetComponent<UI_ImageComponent>(clickedImage->getID()))
		{
			imgComp->SetFallbackColor(D2D1::ColorF(1.0f, 1.0f, 0.0f, 1.0f)); // ?��???
		}
	}

	// ?�성???��?지?�을 UIButton 멤버 변?�에 ?�당
	ALICE_LOG_INFO("[UIWorld] CreateEntity<UIButton>: calling SetupParts - buttonID=%lu", buttonID);
	ObjPtr->SetupParts(normalImage, hoverImage, pressedImage, clickedImage);

	// UIButton 초기화
	ALICE_LOG_INFO("[UIWorld] CreateEntity<UIButton>: calling Initalize - buttonID=%lu", buttonID);
	ObjPtr->Initalize(*m_UIRenderStruct, m_worldDelegates);

	ALICE_LOG_INFO("[UIWorld] CreateEntity<UIButton>: completed - buttonID=%lu, rootID count=%zu", 
		buttonID, m_rootID.size());
	
	// 무한 루프 방�? ?�래�??�제
	
	return ObjPtr;
}


//UITextComponent* UIWorld::CreateTextComponent(unsigned long ownerID)
//{
//	// ?��? ?�당 ownerID??컴포?�트가 ?�다�?그�?�?반환
//	auto it = m_compStorage.find(ownerID);
//	if (it != m_compStorage.end())
//		return it->second.get();
//
//	// ??컴포?�트 ?�성 ???�??
//	auto comp = std::make_unique<UITextComponent>();
//	comp->owner = ownerID;
//	UITextComponent* raw = comp.get();
//	m_compStorage.emplace(ownerID, std::move(comp));
//	return raw;
//}

UITransform* UIWorld::CreateTransformComponent(unsigned long ownerID)
{
	// �̹� �ش� ownerID�� ������Ʈ�� �ִٸ� �״��?��ȯ
	auto it = m_transformStorage.find(ownerID);
	if (it != m_transformStorage.end())
		return it->second.get();

	// �� ������Ʈ ���� �� ������ ���� ����
	auto comp = std::make_unique<UITransform>();
	comp->owner = ownerID;
	UITransform* raw = comp.get();
	
	// Owner/OwnerID??AddComponent ?�리게이?�에???�정??
	// 직접 ?�출 ?�에???�기???�정 (?�리게이?��? 거치지 ?�는 경우)
	if (UIBase* owner = this->Get(ownerID))
	{
		raw->Owner = owner;
		raw->OwnerID = ownerID;
	}
	
	m_transformStorage.emplace(ownerID, std::move(comp));
	return raw;
}

/*
UITextComponent* UIWorld::FindTextComponent(unsigned long ownerID)
{
	auto it = m_compStorage.find(ownerID);
	return (it == m_compStorage.end()) ? nullptr : it->second.get();
}
*/

UITransform* UIWorld::FindTransformComponent(unsigned long ownerID)
{
	auto it = m_transformStorage.find(ownerID);
	return (it == m_transformStorage.end()) ? nullptr : it->second.get();
}

const UITransform* UIWorld::FindTransformComponent(unsigned long ownerID) const
{
	auto it = m_transformStorage.find(ownerID);
	return (it == m_transformStorage.end()) ? nullptr : it->second.get();
}

/*
void UIWorld::RemoveTextComponent(unsigned long ownerID)
{
	m_compStorage.erase(ownerID);
}
*/

void UIWorld::RemoveTransformComponent(unsigned long ownerID)
{
	m_transformStorage.erase(ownerID);
}

UI_ImageComponent* UIWorld::CreateImageComponent(unsigned long ownerID)
{
	// ?��? ?�당 ownerID??컴포?�트가 ?�다�?그�?�?반환
	auto it = m_imageComponentStorage.find(ownerID);
	if (it != m_imageComponentStorage.end())
		return it->second.get();

	// ??컴포?�트 ?�성 ???�??
	auto comp = std::make_unique<UI_ImageComponent>();
	comp->Initalize(*m_UIRenderStruct);
	UI_ImageComponent* raw = comp.get();
	
	// Owner/OwnerID??AddComponent ?�리게이?�에???�정??
	// 직접 ?�출 ?�에???�기???�정 (?�리게이?��? 거치지 ?�는 경우)
	if (UIBase* owner = this->Get(ownerID))
	{
		raw->Owner = owner;
		raw->OwnerID = ownerID;
	}
	m_imageComponentStorage.emplace(ownerID, std::move(comp));
	return raw;
}

UI_ImageComponent* UIWorld::FindImageComponent(unsigned long ownerID)
{
	auto it = m_imageComponentStorage.find(ownerID);
	return (it == m_imageComponentStorage.end()) ? nullptr : it->second.get();
}

const UI_ImageComponent* UIWorld::FindImageComponent(unsigned long ownerID) const
{
	auto it = m_imageComponentStorage.find(ownerID);
	return (it == m_imageComponentStorage.end()) ? nullptr : it->second.get();
}

void UIWorld::RemoveImageComponent(unsigned long ownerID)
{
	m_imageComponentStorage.erase(ownerID);
}

UI_ScriptComponent* UIWorld::CreateScriptComponent(unsigned long ownerID)
{
	// ?��? ?�당 ownerID??컴포?�트가 ?�다�?그�?�?반환
	auto it = m_scriptComponentStorage.find(ownerID);
	if (it != m_scriptComponentStorage.end())
		return it->second.get();

	auto comp = std::make_unique<UI_ScriptComponent>();
	UI_ScriptComponent* raw = comp.get();
	
	// Owner/OwnerID??AddComponent ?�리게이?�에???�정??
	// 직접 ?�출 ?�에???�기???�정 (?�리게이?��? 거치지 ?�는 경우)
	if (UIBase* owner = this->Get(ownerID))
	{
		raw->Owner = owner;
		raw->OwnerID = ownerID;
	}
	m_scriptComponentStorage.emplace(ownerID, std::move(comp));
	return raw;
}

UI_ScriptComponent* UIWorld::FindScriptComponent(unsigned long ownerID)
{
	auto it = m_scriptComponentStorage.find(ownerID);
	return (it == m_scriptComponentStorage.end()) ? nullptr : it->second.get();
}

const UI_ScriptComponent* UIWorld::FindScriptComponent(unsigned long ownerID) const
{
	auto it = m_scriptComponentStorage.find(ownerID);
	return (it == m_scriptComponentStorage.end()) ? nullptr : it->second.get();
}

void UIWorld::RemoveScriptComponent(unsigned long ownerID)
{
	auto it = m_scriptComponentStorage.find(ownerID);
	if (it != m_scriptComponentStorage.end())
	{
		if (it->second)
		{
			it->second->OnRemoved();
		}
		m_scriptComponentStorage.erase(it);
	}
}

// ============================================================================
// UI Script 관�?구현 (?�러 ?�크립트 지??
// ============================================================================
UIScriptEntry& UIWorld::AddUIScript(unsigned long ownerID, const std::string& scriptName)
{
	UIScriptEntry entry{};
	entry.scriptName = scriptName;
	
	// UIBase 참조 가?�오�?
	UIBase* owner = Get(ownerID);
	if (!owner)
	{
		ALICE_LOG_WARN("[UIWorld] AddUIScript: Owner UIBase not found for ID=%lu", ownerID);
		// owner가 ?�어??entry??추�? (?�중??EnsureInstance?�서 처리)
	}
	
	// ============================================================================
	// Vector reallocation 방�?: reserve�?메모�??�전 ?�당
	// ============================================================================
	auto& scriptVec = m_scripts[ownerID];
	if (scriptVec.capacity() == scriptVec.size())
	{
		// ?�할??직전?�면 미리 ?�장 (최소 4�??�는 ?�재 ?�기??2�?
		scriptVec.reserve(std::max<size_t>(4, scriptVec.size() * 2));
	}
	
	
	// DLL 
	entry.instance = UIScriptSystem::CreateUIScriptInstance(scriptName);
	if (entry.instance)
	{

		// 포인터 주입 및 검증 (EXE 주소 공간의 포인터를 DLL 객체에 주입)
		ALICE_LOG_INFO("[UIWorld] AddUIScript: Injecting pointers (EXE address space) - owner=%p, world=%p", owner, this);
		
		entry.instance->Owner = owner;
		entry.instance->OwnerID = ownerID;
		entry.instance->World = this; // EXE의 World 포인터를 DLL 객체에 주입
		
		// 포인터 검증: 주입된 포인터가 제대로 설정되었는지 확인
		ALICE_LOG_INFO("[UIWorld] AddUIScript: Pointer verification - instance->Owner=%p, instance->OwnerID=%lu, instance->World=%p",
			entry.instance->Owner, entry.instance->OwnerID, entry.instance->World);
		
		if (entry.instance->Owner != owner)
		{
			ALICE_LOG_ERRORF("[UIWorld] AddUIScript: CRITICAL - Owner pointer mismatch! (expected=%p, actual=%p)", 
				owner, entry.instance->Owner);
		}
		
		if (entry.instance->World != this)
		{
			ALICE_LOG_ERRORF("[UIWorld] AddUIScript: CRITICAL - World pointer mismatch! (expected=%p, actual=%p)", 
				this, entry.instance->World);
		} // World
		
		// OnAdded 
		if (owner)
		{
			ALICE_LOG_INFO("[UIWorld] AddUIScript: Will call OnAdded on Tick for script '%s' (owner ID=%lu, instance=%p)", scriptName.c_str(), ownerID, entry.instance.get());
			
			// vtable
			if (entry.instance)
			{
				void** vtable = *(void***)entry.instance.get();
				ALICE_LOG_INFO("[UIWorld] AddUIScript: vtable=%p, vtable[0]=%p", vtable, vtable ? vtable[0] : nullptr);
			}
			
			// DLL 경계 문제
		
		}
		else
		{
			ALICE_LOG_WARN("[UIWorld] AddUIScript: Owner is null, skipping OnAdded for script '%s'", scriptName.c_str());
		}
		

		ALICE_LOG_INFO("[UIWorld] AddUIScript: Created script '%s' for UI ID=%lu", scriptName.c_str(), ownerID);
	}
	else
	{
		ALICE_LOG_WARN("[UIWorld] AddUIScript: Failed to create script '%s' for UI ID=%lu (will be created later)", scriptName.c_str(), ownerID);
	}
	
	
	scriptVec.push_back(std::move(entry));
	
	
	return scriptVec.back();
}

std::vector<UIScriptEntry>* UIWorld::GetUIScripts(unsigned long ownerID)
{
	auto it = m_scripts.find(ownerID);
	if (it == m_scripts.end())
		return nullptr;
	return &it->second;
}

const std::vector<UIScriptEntry>* UIWorld::GetUIScripts(unsigned long ownerID) const
{
	auto it = m_scripts.find(ownerID);
	if (it == m_scripts.end())
		return nullptr;
	return &it->second;
}

void UIWorld::RemoveUIScript(unsigned long ownerID, std::size_t index)
{
	auto it = m_scripts.find(ownerID);
	if (it == m_scripts.end())
		return;
	
	auto& list = it->second;
	if (index >= list.size())
		return;
	
	// OnRemoved ?�출
	if (list[index].instance)
	{
		list[index].instance->OnRemoved();
	}
	
	list.erase(list.begin() + static_cast<std::ptrdiff_t>(index));
	if (list.empty())
		m_scripts.erase(it);
}

UIBase* UIWorld::Get(long unsigned int ID)
{
	auto it = pUIObjStorage.find(ID);
	if (it == pUIObjStorage.end())
		return nullptr;
	return it->second.get();
}

const UIBase* UIWorld::Get(long unsigned int ID) const
{
	auto it = pUIObjStorage.find(ID);
	if (it == pUIObjStorage.end())
		return nullptr;
	return it->second.get();
}

bool UIWorld::DestroyEntity(long unsigned int ID)
{
	UIBase* tmpNode = Get(ID);
	if (!tmpNode) return false;

	// 1. �θ𿡼� ���?����
	long unsigned int parentID{ tmpNode->parentID };
	if (parentID != 0)
	{
		UIBase* parentNode = Get(parentID);
		if (parentNode != nullptr)
		{
			auto& ChildVect = parentNode->childIDStorage;
			ChildVect.erase(
				std::remove(ChildVect.begin(), ChildVect.end(), ID),
				ChildVect.end()
			);
		}
	}
	else
	{   // ��Ʈ�����?���?
		m_rootID.erase(
			std::remove(m_rootID.begin(), m_rootID.end(), ID),
			m_rootID.end()
		);
	}

	// 2. ������Ʈ�� �Բ� ����
	RemoveTransformComponent(ID);
	RemoveImageComponent(ID);
	RemoveScriptComponent(ID);

	// 3. ����Ʈ�� ��ü ���� (���������?�ڽĵ鵵 ����)
	return DeleteChildObjects(ID);
}

bool UIWorld::DeleteChildObjects(long unsigned int ID)
{
	UIBase* tmpNode = Get(ID);
	if (!tmpNode) return false;

	// �ڽĵ��� ���� ���� (���纻 ���?- ������ �����ǹǷ�)
	auto children = tmpNode->childIDStorage; // ����
	for (auto cid : children)
	{
		// �� �ڽ��� ������Ʈ�� ����
		RemoveTransformComponent(cid);
		RemoveImageComponent(cid);
		RemoveScriptComponent(cid);
		DeleteChildObjects(cid);
	}

	// ��ƼƼ ����
	pUIObjStorage.erase(ID);
	return true;
}

void UIWorld::Clear()
{
	// ���?��ƼƼ ����
	pUIObjStorage.clear();
	m_rootID.clear();

	// ���?������Ʈ ����
	// m_compStorage.clear(); // UITextComponent ?�음
	m_transformStorage.clear();
	m_imageComponentStorage.clear();
	m_scriptComponentStorage.clear();

	// World Epoch ���� (���� �ڵ����?��ȿȭ��)
	m_worldEpoch++;

	// nowInteger�� reset���� ���� (���� ��å ����)
}

// ============================================================================
// UILayoutSystem ����
// ============================================================================
void UILayoutSystem::UpdateTransforms(UIWorld& world)
{
	// ��Ʈ UI �����?���� 
	for (auto rootID : world.GetRootIDs())
	{
		if (auto* root = world.Get(rootID))
			UpdateTransformChild(world, root, D2D1::Matrix3x2F::Identity());
	}
}

void UILayoutSystem::UpdateTransformChild(UIWorld& world, UIBase* node, const D2D1::Matrix3x2F& parentWorld)
{
	// Transform?� UIBase 캐시�??�근 (?�성 직후 1??부�??�책)
	auto& tr = node->GetTransform();
	D2D1::Matrix3x2F worldMat = tr.WorldMatrix(parentWorld);

	for (auto childID : node->childIDStorage)
	{
		if (auto* child = world.Get(childID))
			UpdateTransformChild(world, child, worldMat);
	}
}

void UILayoutSystem::UpdateUI(UIWorld& world, float deltaTime)
{
	for (auto rootID : world.GetRootIDs())
	{
		auto* root = world.Get(rootID);
		ALICE_LOG_INFO("[Root] id=%lu ptr=%p type=%s", rootID, root, root ? typeid(*root).name() : "null");
		
		if (root)
		{
			root->Update(deltaTime);
			UpdateUIChild(world, root, deltaTime);
		}
	}
}

void UILayoutSystem::UpdateUIChild(UIWorld& world, UIBase* node, float deltaTime)
{
	if (!node) return;

	// 모든 ?�식???�???��??�으�?Update ?�출
	for (auto childID : node->childIDStorage)
	{
		if (auto* child = world.Get(childID))
		{
			child->Update(deltaTime);
			UpdateUIChild(world, child, deltaTime);
		}
	}
}

// ============================================================================
// UIHitTestSystem ����
// ============================================================================
long unsigned UIHitTestSystem::FindUIUnderPointer(UIWorld& world, XMFLOAT2 MousePos, UIRenderStruct* renderStruct)
{
	std::vector<long unsigned> hitMouseID;
	hitMouseID.clear();

	// MousePos??Unity 좌표�?(중앙 0,0 / ?��? ?�수, ?�래가 ?�수)
	XMFLOAT2 unityMousePos = MousePos;

	std::vector<long unsigned> tmpStorage;
	long unsigned tmpID{ 0 };

	// AABB(ȸ�� ���� ��ü AABB)
	for (auto rootID : world.GetRootIDs())
	{
		if (auto* root = world.Get(rootID))
		{
			if (!root->IsMouseOverUIAABB(unityMousePos, tmpStorage))
				continue;

		}
	}

	// ȸ���ִ� ��ü �˻�
	for (auto rootID : tmpStorage)
	{
		if (auto* root = world.Get(rootID))
		{
			// IsMouseOverUIRot??Unity 좌표계�? 받음
			if (root->IsMouseOverUIRot(unityMousePos))
			{
				hitMouseID.push_back(root->getID());
			}

			UIRotRoot(world, unityMousePos, root, hitMouseID);
		}
	}

	if (hitMouseID.size() != 0)
		tmpID = hitMouseID.back();

	return tmpID;
}

void UIHitTestSystem::AABBRoot(UIWorld& world, XMFLOAT2 MousePos, UIBase* node, std::vector<long unsigned>& IDStorage)
{
	for (auto childID : node->childIDStorage)
	{
		if (auto* child = world.Get(childID))
		{
			if (!child->IsMouseOverUIAABB(MousePos, IDStorage))
				continue;

			AABBRoot(world, MousePos, child, IDStorage);
		}
	}
}

void UIHitTestSystem::UIRotRoot(UIWorld& world, XMFLOAT2 MousePos, UIBase* node, std::vector<long unsigned>& hitMouseID)
{
	// MousePos??Unity 좌표�?(IsMouseOverUIRot가 Unity 좌표계�? 받음)
	for (auto childID : node->childIDStorage)
	{
		if (auto* child = world.Get(childID))
		{
			if (!child->IsMouseOverUIRot(MousePos))
				continue;
			hitMouseID.push_back(child->getID());
			UIRotRoot(world, MousePos, child, hitMouseID);
		}
	}
}

// ============================================================================
// UIEventSystem ����
// ============================================================================
// ?�적 ?�태 변??(?�레??�??�태 ?��?)
static unsigned long s_hoveredID = 0;
static unsigned long s_pressedID = 0;
static unsigned long s_dragID = 0;
static DirectX::XMFLOAT2 s_dragStartPos{ 0.0f, 0.0f };
static bool s_isDragging = false;

float UIEventSystem::Distance(const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b)
{
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	return std::sqrt(dx * dx + dy * dy);
}

void UIEventSystem::UpdateMouseEvents(UIWorld& world, Alice::InputSystem* inputSystem, UIRenderStruct* renderStruct,
	UINT viewportWidth, UINT viewportHeight, bool editorMode)
{
	if (!inputSystem || !renderStruct) return;

	// 마우???�치 가?�오�?(?�도???�라?�언??좌표)
	POINT mousePosScreen = inputSystem->GetMousePosition();
	DirectX::XMFLOAT2 mousePos{ static_cast<float>(mousePosScreen.x), static_cast<float>(mousePosScreen.y) };

	// 뷰포???�프??보정: 마우??좌표�?뷰포??로컬 좌표�?변??(=RenderTarget 좌표)
	float localX = mousePos.x - renderStruct->m_viewportX;
	float localY = mousePos.y - renderStruct->m_viewportY;

	// 뷰포??밖이�?UI ?�정?�서 ?�외
	if (localX < 0.0f || localY < 0.0f || 
	    localX > renderStruct->m_viewportWidth || localY > renderStruct->m_viewportHeight)
	{
		s_hoveredID = 0;
		return; // 뷰포??밖이�??�벤??처리 중단
	}

	// RenderTarget 좌표�?Unity 좌표�?변??
	// Unity 좌표: 중앙 (0, 0), ?��? ?�수, ?�래가 ?�수
	DirectX::XMFLOAT2 unityMousePos = {
		localX - renderStruct->m_viewportWidth * 0.5f,
		-(localY - renderStruct->m_viewportHeight * 0.5f)  // Y�?반전: ?��? ?�수, ?�래가 ?�수
	};

	// HitTest: 마우???�래 UI 찾기 (최상???�선) - Unity 좌표�??�용
	unsigned long hoveredID = UIHitTestSystem::FindUIUnderPointer(world, unityMousePos, renderStruct);
	s_hoveredID = hoveredID;

	// 마우??버튼 ?�태 ?�인
	const int LEFT_MOUSE_BUTTON = 0;
	bool isLeftButtonDown = inputSystem->IsLeftButtonDown();
	bool wasLeftButtonPressed = inputSystem->IsMouseButtonPressed(LEFT_MOUSE_BUTTON);
	bool wasLeftButtonReleased = inputSystem->IsMouseButtonReleased(LEFT_MOUSE_BUTTON);
	POINT mouseDelta = inputSystem->GetMouseDelta();

	// MouseDown 처리
	if (wasLeftButtonPressed)
	{
		s_pressedID = hoveredID;
		s_dragID = hoveredID;
		s_dragStartPos = unityMousePos;
		s_isDragging = false;

		if (s_pressedID != 0)
		{
			// UI_InputComponent 찾아??OnPressed ?�출
			if (auto* uiBase = world.Get(s_pressedID))
			{
				if (auto* inputComp = world.TryGetComponent<UI_InputComponent>(s_pressedID))
				{
					if (inputComp->OnPressed)
					{
						inputComp->OnPressed();
					}
				}
			}
		}
	}

	// MouseMove �??�래�??�정
	if (s_pressedID != 0 && isLeftButtonDown)
	{
		float dragDistance = Distance(unityMousePos, s_dragStartPos);
		
		if (!s_isDragging && dragDistance > DRAG_THRESHOLD)
		{
			// DragBegin
			s_isDragging = true;
			if (s_dragID != 0)
			{
				if (auto* uiBase = world.Get(s_dragID))
				{
					if (auto* inputComp = world.TryGetComponent<UI_InputComponent>(s_dragID))
					{
						if (inputComp->OnDragBegin)
						{
							inputComp->OnDragBegin(s_dragStartPos);
						}
					}
				}
			}
		}

		if (s_isDragging)
		{
			// Dragging
			DirectX::XMFLOAT2 deltaPos{ static_cast<float>(mouseDelta.x), static_cast<float>(-mouseDelta.y) };
			if (s_dragID != 0)
			{
				if (auto* uiBase = world.Get(s_dragID))
				{
					if (auto* inputComp = world.TryGetComponent<UI_InputComponent>(s_dragID))
					{
						if (inputComp->OnDrag)
						{
							inputComp->OnDrag(deltaPos, unityMousePos);
						}
					}
				}
			}
		}
	}

	// MouseUp 처리
	if (wasLeftButtonReleased)
	{
		if (s_isDragging)
		{
			// DragEnd
			if (s_dragID != 0)
			{
				if (auto* uiBase = world.Get(s_dragID))
				{
					if (auto* inputComp = world.TryGetComponent<UI_InputComponent>(s_dragID))
					{
						if (inputComp->OnDragEnd)
						{
							inputComp->OnDragEnd(unityMousePos);
						}
					}
				}
			}
		}
		else
		{
			// Click ?�정: pressedID == hoveredID ?�면 Click
			if (s_pressedID != 0 && s_pressedID == hoveredID)
			{
				if (auto* uiBase = world.Get(s_pressedID))
				{
					if (auto* inputComp = world.TryGetComponent<UI_InputComponent>(s_pressedID))
					{
						if (inputComp->OnClicked)
						{
							inputComp->OnClicked();
						}
					}
				}
			}

			// OnMouseUp (OnReleased)
			if (s_pressedID != 0)
			{
				if (auto* uiBase = world.Get(s_pressedID))
				{
					if (auto* inputComp = world.TryGetComponent<UI_InputComponent>(s_pressedID))
					{
						if (inputComp->OnReleased)
						{
							inputComp->OnReleased();
						}
					}
				}
			}
		}

		// ?�태 초기??
		s_pressedID = 0;
		s_dragID = 0;
		s_isDragging = false;
	}
}

// ============================================================================
// UIRenderSystem ����
// ============================================================================
void UIRenderSystem::Render(UIWorld& world, UIRenderStruct* renderStruct)
{
	renderStruct->m_d2DdevCon->BeginDraw();
	renderStruct->m_d2DdevCon->Clear(D2D1::ColorF(0, 0, 0, 0));

	RenderRoot(world, renderStruct);

	renderStruct->m_d2DdevCon->EndDraw();
}

void UIRenderSystem::RenderRoot(UIWorld& world, UIRenderStruct* renderStruct)
{
	// ��Ʈ UI �����?���� 
	for (auto rootID : world.GetRootIDs())
	{
		if (auto* root = world.Get(rootID))
		{
			root->Render();
			RenderRootChild(world, root);
		}
	}
}

void UIRenderSystem::RenderRootChild(UIWorld& world, UIBase* node)
{
	for (auto childID : node->childIDStorage)
	{
		if (auto* child = world.Get(childID))
		{
			child->Render();
			RenderRootChild(world, child);
		}
	}
}

// ============================================================================
// UISceneManager ����
// ============================================================================
void UISceneManager::initalize(ID3D11Device* Dev, ID3D11DeviceContext* DevCon, UIRenderStruct* UIRst, Alice::InputSystem* tmpInput)
{
	pDev = Dev;
	pDevCon = DevCon;
	m_UIRenderStruct = UIRst;
	m_InputSystem = tmpInput;

	// UIWorld �ʱ�ȭ
	m_world.Initialize(UIRst);
}

void UISceneManager::Update(float deltaTime)
{
	// Layout System: Transform ?�데?�트 (먼�? ?�행 - UI Update ?�에 Transform??계산?�어????
	UILayoutSystem::UpdateTransforms(m_world);

	// Layout System: UI Update
	UILayoutSystem::UpdateUI(m_world, deltaTime);

	// Script System: UI_ScriptComponent ?�데?�트
	UIScriptSystem::Tick(m_world, deltaTime);

	// Image System: UI_ImageComponent ?�데?�트
	UIImageSystem::Update(m_world);

	// Input System: UI_InputComponent ?�데?�트 (Hover/Click ?�정)
	UIInputSystem::Update(m_world, *m_InputSystem);

	// UIButton??InputComponent ?�데?�트 (AddComponent�??�용?��? ?��? 경우)
	// 모든 루트 ?�티?��? ?�식?�을 ?�회?�면??UIButton??찾아 UpdateInput ?�출
	for (auto rootID : m_world.GetRootIDs())
	{
		if (auto* root = m_world.Get(rootID))
		{
			UpdateButtonInputRecursive(root);
		}
	}

	// Event System: ���콺 �Է� ó��
	// Event System?� UIWorldManager::Update?�서 ?�출??(viewport size �?editorMode ?�달)
}

void UISceneManager::Render()
{
	if (!m_UIRenderStruct || !m_UIRenderStruct->m_d2DdevCon) 
	{
		ALICE_LOG_WARN("[UISceneManager] Render skipped: invalid render struct");
		return;
	}

	m_UIRenderStruct->m_d2DdevCon->BeginDraw();
	m_UIRenderStruct->m_d2DdevCon->Clear(D2D1::ColorF(0, 0, 0, 0));
	
	UIImageSystem::Render(m_world, m_UIRenderStruct);

	HRESULT hr = m_UIRenderStruct->m_d2DdevCon->EndDraw();
	
	if (FAILED(hr))
	{
		ALICE_LOG_ERRORF("[UISceneManager] EndDraw failed: HRESULT=0x%08X", hr);
		
		if (hr == D2DERR_RECREATE_TARGET)
		{
			ALICE_LOG_WARN("[UISceneManager] D2DERR_RECREATE_TARGET - target needs recreation");
		}
		
		// �??�레???�시??(1?�만)
		static bool s_retryAttempted = false;
		if (!s_retryAttempted && m_UIRenderStruct->m_d2dTargetBitmap)
		{
			s_retryAttempted = true;
			m_UIRenderStruct->m_d2DdevCon->SetTarget(m_UIRenderStruct->m_d2dTargetBitmap.Get());
			hr = m_UIRenderStruct->m_d2DdevCon->EndDraw();
			if (SUCCEEDED(hr))
			{
				ALICE_LOG_INFO("[UISceneManager] EndDraw retry succeeded");
			}
			else
			{
				ALICE_LOG_ERRORF("[UISceneManager] EndDraw retry failed: HRESULT=0x%08X", hr);
			}
		}
	}
}

// ============================================================================
// UIImageSystem 구현
// ============================================================================
void UIImageSystem::Update(UIWorld& world)
{
	UpdateRoot(world);
}

void UIImageSystem::UpdateRoot(UIWorld& world)
{
	// 루트 UI ?�티?�들 ?�회
	for (auto rootID : world.GetRootIDs())
	{
		if (auto* root = world.Get(rootID))
		{
			// ImageComponent ?�데?�트
			if (auto* imageComp = root->TryGetComponent<UI_ImageComponent>())
			{
				imageComp->Update();
			}
			// ?�식?�도 ?��??�으�??�데?�트
			UpdateRootChild(world, root);
		}
	}
}

void UIImageSystem::UpdateRootChild(UIWorld& world, UIBase* node)
{
	for (auto childID : node->childIDStorage)
	{
		if (auto* child = world.Get(childID))
		{
			// ImageComponent ?�데?�트
			if (auto* imageComp = child->TryGetComponent<UI_ImageComponent>())
			{
				imageComp->Update();
			}
			// ?��??�으�??�식?�도 ?�데?�트
			UpdateRootChild(world, child);
		}
	}
}

void UIImageSystem::Render(UIWorld& world, UIRenderStruct* renderStruct)
{
	ALICE_LOG_INFO("[UIImageSystem::Render] Render called: renderStruct=%p", renderStruct);
	if (!renderStruct) 
	{
		ALICE_LOG_WARN("[UIImageSystem::Render] Render skipped: renderStruct is null");
		return;
	}
	ALICE_LOG_INFO("[UIImageSystem::Render] Calling RenderRoot");
	RenderRoot(world, renderStruct);
	ALICE_LOG_INFO("[UIImageSystem::Render] RenderRoot completed");
}

void UIImageSystem::RenderRoot(UIWorld& world, UIRenderStruct* renderStruct)
{
	const auto& rootIDs = world.GetRootIDs();
	ALICE_LOG_INFO("[UIImageSystem::RenderRoot] RenderRoot called: rootIDs.size()=%zu", rootIDs.size());
	
	// 루트 UI ?�티?�들 ?�회
	for (size_t i = 0; i < rootIDs.size(); ++i)
	{
		auto rootID = rootIDs[i];
		ALICE_LOG_INFO("[UIImageSystem::RenderRoot] Processing rootID[%zu]=%lu", i, rootID);
		
		if (auto* root = world.Get(rootID))
		{
		
			// ImageComponent ?�더�?
			if (auto* imageComp = root->TryGetComponent<UI_ImageComponent>())
			{
				
				imageComp->Render();
				
			}

			// ?�식?�도 ?��??�으�??�더�?
			RenderRootChild(world, root, renderStruct);
		}

	}
	
}

void UIImageSystem::RenderRootChild(UIWorld& world, UIBase* node, UIRenderStruct* /*renderStruct*/)
{
	for (auto childID : node->childIDStorage)
	{
		if (auto* child = world.Get(childID))
		{
			// ImageComponent ?�더�?
			if (auto* imageComp = child->TryGetComponent<UI_ImageComponent>())
			{
				imageComp->Render();
				
			}
			// ?��??�으�??�식?�도 ?�더�?
			RenderRootChild(world, child, nullptr);
		}
	}
}

// ============================================================================
// UIInputSystem 구현
// ============================================================================
void UIInputSystem::Update(UIWorld& world, Alice::InputSystem& input)
{
	UpdateRoot(world, input);
}

void UIInputSystem::UpdateRoot(UIWorld& world, Alice::InputSystem& input)
{
	// 루트 UI ?�티?�들 ?�회
	for (auto rootID : world.GetRootIDs())
	{
		if (auto* root = world.Get(rootID))
		{
			// InputComponent ?�데?�트
			if (auto* inputComp = root->TryGetComponent<UI_InputComponent>())
			{
				inputComp->Update(world, input);
			}
			// ?�식?�도 ?��??�으�??�데?�트
			UpdateRootChild(world, root, input);
		}
	}
}

void UIInputSystem::UpdateRootChild(UIWorld& world, UIBase* node, Alice::InputSystem& input)
{
	for (auto childID : node->childIDStorage)
	{
		if (auto* child = world.Get(childID))
		{
			// InputComponent ?�데?�트
			if (auto* inputComp = child->TryGetComponent<UI_InputComponent>())
			{
				inputComp->Update(world, input);
			}
			// ?��??�으�??�식?�도 ?�데?�트
			UpdateRootChild(world, child, input);
		}
	}
}

// ============================================================================
// UIButton Input ?�데?�트 ?�퍼 (UISceneManager ?��? ?�수)
// ============================================================================
void UISceneManager::UpdateButtonInputRecursive(UIBase* node)
{
	if (!node) return;
	
	// #region agent log
	FILE* logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
	if (logFile) {
		fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"I\",\"location\":\"UISceneManager.cpp:1191\",\"message\":\"UpdateButtonInputRecursive called\",\"data\":{\"nodeID\":%lu,\"nodeType\":\"%s\"},\"timestamp\":%lld}\n",
			node->getID(), typeid(*node).name(), (long long)time(nullptr) * 1000);
		fclose(logFile);
	}
	// #endregion agent log

	// ?�재 ?�드가 UIButton?��? ?�인
	// DLL 경계 문제로 dynamic_cast 대신 GetTypeName 사용
	const char* typeName = node->GetTypeName();
	if (typeName && strcmp(typeName, "UIButton") == 0)
	{
		UIButton* button = static_cast<UIButton*>(node);
		
		// #region agent log
		logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
		if (logFile) {
			fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"J\",\"location\":\"UISceneManager.cpp:1330\",\"message\":\"UIButton found, calling UpdateInput\",\"data\":{\"buttonID\":%lu,\"m_InputSystem\":%p,\"button\":%p},\"timestamp\":%lld}\n",
				button->getID(), m_InputSystem, button, (long long)time(nullptr) * 1000);
			fclose(logFile);
		}
		// #endregion agent log
		
		ALICE_LOG_INFO("[UISceneManager] UpdateButtonInputRecursive: UIButton found, ID=%lu", button->getID());
		
		if (m_InputSystem)
		{
			button->UpdateInput(m_world, *m_InputSystem);
		}
		else
		{
			// #region agent log
			logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
			if (logFile) {
				fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"K\",\"location\":\"UISceneManager.cpp:1200\",\"message\":\"m_InputSystem is null!\",\"data\":{\"buttonID\":%lu},\"timestamp\":%lld}\n",
					button->getID(), (long long)time(nullptr) * 1000);
				fclose(logFile);
			}
			// #endregion agent log
		}
	}

	// ?�식?�도 ?��??�으�?처리
	for (auto childID : node->childIDStorage)
	{
		if (auto* child = m_world.Get(childID))
		{
			UpdateButtonInputRecursive(child);
		}
	}
}

// ============================================================================
// UIWorld ?�름 관�??�수 구현
// ============================================================================

bool UIWorld::RegisterName(unsigned long id, const std::string& name)
{
	if (name.empty())
		return false;

	// 기존 ?�름???�으�??�거
	if (auto it = m_idToName.find(id); it != m_idToName.end())
	{
		m_nameToId.erase(it->second);
	}

	// 중복 ?�름 처리: ?�동 ?�니?�화
	std::string uniqueName = name;
	int suffix = 1;
	while (m_nameToId.find(uniqueName) != m_nameToId.end())
	{
		uniqueName = name + "_" + std::to_string(suffix);
		suffix++;
	}

	// ?�록
	m_idToName[id] = uniqueName;
	m_nameToId[uniqueName] = id;

	return true;
}

bool UIWorld::Rename(unsigned long id, const std::string& newName)
{
	if (newName.empty())
		return false;

	// 기존 ?�름 ?�거
	if (auto it = m_idToName.find(id); it != m_idToName.end())
	{
		m_nameToId.erase(it->second);
	}

	// ???�름 ?�록 (중복 처리 ?�함)
	return RegisterName(id, newName);
}

const std::string* UIWorld::GetNameById(unsigned long id) const
{
	auto it = m_idToName.find(id);
	if (it != m_idToName.end())
		return &it->second;
	return nullptr;
}

unsigned long UIWorld::GetIdByName(const std::string& name) const
{
	auto it = m_nameToId.find(name);
	if (it != m_nameToId.end())
		return it->second;
	return 0;
}

UIBase* UIWorld::GetByName(const std::string& name)
{
	// #region agent log
	FILE* logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
	if (logFile) {
		fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"K\",\"location\":\"UISceneManager.cpp:1365\",\"message\":\"GetByName entry\",\"data\":{\"name\":\"%s\",\"this\":%p},\"timestamp\":%lld}\n",
			name.c_str(), this, (long long)time(nullptr) * 1000);
		fclose(logFile);
	}
	// #endregion agent log
	
	unsigned long id = GetIdByName(name);
	
	// #region agent log
	logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
	if (logFile) {
		fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"L\",\"location\":\"UISceneManager.cpp:1367\",\"message\":\"After GetIdByName\",\"data\":{\"id\":%lu},\"timestamp\":%lld}\n",
			id, (long long)time(nullptr) * 1000);
		fclose(logFile);
	}
	// #endregion agent log
	
	if (id == 0)
		return nullptr;
	
	UIBase* result = Get(id);
	
	// #region agent log
	logFile = fopen("d:\\4Q\\4q__AliceRenderer\\.cursor\\debug.log", "a");
	if (logFile) {
		fprintf(logFile, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"M\",\"location\":\"UISceneManager.cpp:1370\",\"message\":\"After Get\",\"data\":{\"result\":%p},\"timestamp\":%lld}\n",
			result, (long long)time(nullptr) * 1000);
		fclose(logFile);
	}
	// #endregion agent log
	
	return result;
}

const UIBase* UIWorld::GetByName(const std::string& name) const
{
	unsigned long id = GetIdByName(name);
	if (id == 0)
		return nullptr;
	return Get(id);
}
