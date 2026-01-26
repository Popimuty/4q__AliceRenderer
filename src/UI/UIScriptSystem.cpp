#include "UIScriptSystem.h"
#include "UI/IUIScript.h"
#include "UI/UIScriptFactory.h"
#include "UI/UISceneManager.h"
#include "UI/UIBase.h"
#include "UI/UI_ScriptComponent.h"
#include "Core/Logger.h"
#include <ctime>
#include <cstdio>
#include <windows.h>  // __try/__except, GetExceptionCode

DynamicUIScriptFactory UIScriptSystem::s_factory = nullptr;
DynamicUIScriptCountFunc UIScriptSystem::s_dynCount = nullptr;
DynamicUIScriptGetNameFunc UIScriptSystem::s_dynGetName = nullptr;

void UIScriptSystem::SetFactory(DynamicUIScriptFactory factory)
{
	s_factory = factory;
	ALICE_LOG_INFO("[UIScriptSystem] SetFactory called - s_factory=%p", s_factory);
}

void UIScriptSystem::SetDynamicUIScriptFunctions(DynamicUIScriptCountFunc countFn, DynamicUIScriptGetNameFunc getNameFn)
{
	s_dynCount = countFn;
	s_dynGetName = getNameFn;
	ALICE_LOG_INFO("[UIScriptSystem] SetDynamicUIScriptFunctions called - countFn=%p, getNameFn=%p", countFn, getNameFn);
}

std::vector<std::string> UIScriptSystem::GetRegisteredUIScriptNames()
{
	std::vector<std::string> result;

	// 1) DLL에서 등록된 스크립트 이름 목록 (동적)
	if (s_dynCount && s_dynGetName)
	{
		int count = s_dynCount();
		char nameBuffer[256] = {};
		for (int i = 0; i < count; ++i)
		{
			if (s_dynGetName(i, nameBuffer, sizeof(nameBuffer)))
			{
				result.push_back(nameBuffer);
			}
		}
	}

	// 2) EXE에서 등록된 스크립트 이름 목록 (정적)
	auto exeNames = UIScriptFactory::GetRegisteredUIScriptNames();
	result.insert(result.end(), exeNames.begin(), exeNames.end());

	return result;
}

std::unique_ptr<IUIScript> UIScriptSystem::CreateUIScriptInstance(const std::string& name)
{
	// 1) DLL 팩토리 먼저 시도 (DLL 스크립트)
	if (s_factory)
	{
		auto instance = s_factory(name);
		if (instance)
		{
			return instance;
		}
	}

	// 2) EXE 레지스트리 시도 (EXE 등록 스크립트)
	auto instance = UIScriptFactory::Create(name.c_str());
	if (instance)
	{
		ALICE_LOG_INFO("[UIScriptSystem] CreateUIScriptInstance: EXE registry SUCCESS for '%s' (instance=%p)", name.c_str(), instance.get());
	}
	return instance;
}

void UIScriptSystem::Tick(UIWorld& world, float dt)
{
	//ALICE_LOG_INFO("[UIScriptSystem::Tick] Called (dt=%.3f)", dt);
	TickRoot(world, dt);
}

void UIScriptSystem::TickRoot(UIWorld& world, float dt)
{
	for (auto rootID : world.GetRootIDs())
	{
		if (auto* root = world.Get(rootID))
		{
			TickNode(world, root, dt);
		}
	}
}

void UIScriptSystem::TickNode(UIWorld& world, UIBase* node, float dt)
{

	// UI_ScriptComponent (레거시, 단일 스크립트)
	if (auto* comp = world.FindScriptComponent(node->ID))
	{
		TickComponent(*comp, dt);
	}

	// UIScriptEntry (여러 스크립트 지원)
	auto* scripts = world.GetUIScripts(node->ID);
	if (scripts)
	{

		for (auto& entry : *scripts)
		{
			TickUIScriptEntry(world, node, entry, dt);
		}
	}

	// 자식 노드 재귀 호출
	for (auto childID : node->childIDStorage)
	{
		if (auto* child = world.Get(childID))
		{
			TickNode(world, child, dt);
		}
	}
}

void UIScriptSystem::TickComponent(UI_ScriptComponent& comp, float dt)
{
	if (!comp.enabled || comp.scriptName.empty())
	{
		return;
	}

	if (!comp.instance)
	{
		EnsureInstance(comp);
	}

	auto* inst = comp.instance.get();
	if (!inst)
	{
		//ALICE_LOG_WARN("[UIScriptSystem::TickComponent] Instance is still null after EnsureInstance for: %s", comp.scriptName.c_str());
		return;
	}

	// Awake phase
	if (!comp.awoken)
	{
		comp.awoken = true;
		if (comp.Owner)
		{
			//ALICE_LOG_INFO("[UIScriptSystem::TickComponent] Calling OnAdded for: %s", comp.scriptName.c_str());
			inst->OnAdded(*comp.Owner);
		}
	}

	// Start phase
	if (!comp.started)
	{
		comp.started = true;
		inst->OnStart();

	}

	// Update 호출 전 Owner 유효성 검사 (가드 코드)
	if (!inst->Owner)
	{

		// 포인터 재주입 시도
		inst->Owner = comp.Owner;
		inst->OwnerID = comp.OwnerID;
		inst->World = comp.World;
		
		if (!inst->Owner)
		{
			ALICE_LOG_ERRORF("[UIScriptSystem] TickComponent: Cannot recover - Owner is still null, skipping Update for '%s'", 
				comp.scriptName.c_str());
			return;
		}
	}

	inst->Update(dt);
}

void UIScriptSystem::EnsureInstance(UI_ScriptComponent& comp)
{
	if (comp.instance || comp.scriptName.empty())
	{
		if (comp.scriptName.empty())
		{
			//ALICE_LOG_WARN("[UIScriptSystem::EnsureInstance] scriptName is empty!");
			//ALICE_LOG_INFO("Registered count=%d", (int)comp.scriptName.size());
		}
		return;
	}

	
	
	if (s_factory)
	{
		
		comp.instance = s_factory(comp.scriptName);
		if (comp.instance)
		{
			// 포인터 주입: Owner, OwnerID, World (3개 모두 필수)
			comp.instance->Owner = comp.Owner;
			comp.instance->OwnerID = comp.OwnerID;
			comp.instance->World = comp.World; // IUIComponent의 World 포인터 주입
			
			// 즉시 실행: OnAdded 호출 (다음 프레임까지 기다리지 않음)
			if (comp.Owner)
			{	
				// __try 블록 내에서는 RAII 객체를 사용할 수 없으므로 raw 포인터 추출
				IUIScript* rawInst = comp.instance.get();
				UIBase* rawOwner = comp.Owner;
				
					rawInst->OnAdded(*rawOwner);
				
			}
			
		}
	}
	// EXE 레지스트리 시도 (DLL 팩토리 실패 시)
	if (!comp.instance)
	{

		comp.instance = UIScriptFactory::Create(comp.scriptName.c_str());
		if (comp.instance)
		{
			// 포인터 주입: Owner, OwnerID, World (3개 모두 필수)
			comp.instance->Owner = comp.Owner;
			comp.instance->OwnerID = comp.OwnerID;
			comp.instance->World = comp.World;
			
			
			// 즉시 실행: OnAdded 호출
			if (comp.Owner)
			{
		
				// __try 블록 내에서는 RAII 객체를 사용할 수 없으므로 raw 포인터 추출
				IUIScript* rawInst = comp.instance.get();
				UIBase* rawOwner = comp.Owner;
				
				rawInst->OnAdded(*rawOwner);
			}
		}
	}
}

void UIScriptSystem::TickUIScriptEntry(UIWorld& world, UIBase* owner, UIScriptEntry& entry, float dt)
{
	if (!entry.enabled)
	{
		return;
	}

	if (!entry.instance)
	{
		EnsureUIScriptInstance(world, owner, entry);
	}

	auto* inst = entry.instance.get();


	// Awake if (!entry.awoken)
	{

		if (inst) {
			void** vtable = *(void***)inst;
		}
		entry.awoken = true;
		if (owner) {
			// DLL 경계 문제를 피하기 위해 예외 처리
			// 주의: void* instPtr = inst; 같은 불필요한 변수 선언은 컴파일러 최적화를 방해할 수 있으므로 제거
			inst->OnAdded(*owner);
		}

	}

	// Start if (!entry.started)
	{
		entry.started = true;
		inst->OnStart();

	}

	// Update 호출 (예외 처리로 보호)
		inst->Update(dt);

}

void UIScriptSystem::EnsureUIScriptInstance(UIWorld& world, UIBase* owner, UIScriptEntry& entry)
{
	// instance가 이미 있으면 재생성하지 않음 (단, Reload 후에는 명시적으로 reset() 후 호출해야 함)
	
	if (entry.scriptName.empty())
	{
		ALICE_LOG_WARN("[UIScriptSystem] EnsureUIScriptInstance: scriptName is empty, cannot create instance");
		return;
	}

	// 공통 

	// 공통 헬퍼 함수 사용: DLL 팩토리 최우선, 실패 시 EXE 레지스트리
	// s_factory를 최우선으로 하도록 CreateUIScriptInstance에서 강제
	entry.instance = CreateUIScriptInstance(entry.scriptName);
	if (entry.instance)
	{
		// 포인터 주입: Owner, OwnerID, World (3개 모두 필수)
		entry.instance->Owner = owner;
		entry.instance->OwnerID = owner ? owner->ID : 0;
		entry.instance->World = &world; // World 포인터 주입
		
		ALICE_LOG_INFO("[UIScriptSystem] EnsureUIScriptInstance: Instance created successfully - instance=%p, Owner=%p, OwnerID=%lu, World=%p", 
			entry.instance.get(), entry.instance->Owner, entry.instance->OwnerID, entry.instance->World);
		
		// 즉시 실행: OnAdded 호출 (다음 프레임까지 기다리지 않음)
		if (owner)
		{
		
			entry.awoken = true; // 중복 호출 방지
			
			// __try 블록 내에서는 RAII 객체를 사용할 수 없으므로 raw 포인터 추출
			IUIScript* rawInst = entry.instance.get();
			UIBase* rawOwner = owner;
			
				rawInst->OnAdded(*rawOwner);
		}
	}
}
