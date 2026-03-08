#include <ReAction.h>

constexpr uint32_t k_KeyMask          = 0b00000000'00000000'00000001'11111111u;

constexpr int32_t k_ModifiersBitOffset = 9;
constexpr uint32_t k_ModifierMask     = 0b00000000'00000001'11111110'00000000u;

constexpr int32_t k_ConditionalBitOffset = 17;
constexpr uint32_t k_ConditionalMask  = 0b00000001'11111110'00000000'00000000u;

constexpr int32_t k_TappedBitOffset = 29;
constexpr uint32_t k_TappedMask       = 0b00100000'00000000'00000000'00000000u;

constexpr int32_t k_LongPressedBitOffset = 30;
constexpr uint32_t k_LongPressedMask  = 0b01000000'00000000'00000000'00000000u;

constexpr int32_t k_DoubleTappedBitOffset = 31;
constexpr uint32_t k_DoubleTappedMask = 0b10000000'00000000'00000000'00000000u;

ButtonAction::Bind::Bind(Key key, Modifier modifiers, Conditional conditional, float timeOut):
timeOut(timeOut) {
	SetKey(key);
	SetModifiers(modifiers);
	SetConditional(conditional);
}

ButtonAction::Bind::Bind():
timeOut(), _internalBitmask()
{ };

ButtonAction::Bind::~Bind() { };

Key ButtonAction::Bind::GetKey() const{
	return (Key)(_internalBitmask & k_KeyMask);
}

void ButtonAction::Bind::SetKey(Key keyMeow) {
	_internalBitmask = ((_internalBitmask & ~k_KeyMask) | (uint32_t)keyMeow);
}

ButtonAction::Modifier ButtonAction::Bind::GetModifiers() const{
	return (Modifier)((_internalBitmask & k_ModifierMask) >> k_ModifiersBitOffset);
}

void ButtonAction::Bind::SetModifiers(Modifier modifiers) {
	_internalBitmask = (_internalBitmask & ~k_ModifierMask) | ((uint32_t)modifiers << k_ModifiersBitOffset);
}

ButtonAction::Conditional ButtonAction::Bind::GetConditional() const{
	return (Conditional)((_internalBitmask & k_ConditionalMask) >> k_ConditionalBitOffset);
}

void ButtonAction::Bind::SetConditional(Conditional conditional) {
	_internalBitmask = (_internalBitmask & ~k_ConditionalMask) | ((uint32_t)conditional << k_ConditionalBitOffset);
}

bool ButtonAction::Bind::GetDoubleTapped() const{
	return _internalBitmask & k_DoubleTappedMask;
}

void ButtonAction::Bind::SetDoubleTapped(bool value) {
	_internalBitmask = (_internalBitmask & ~k_DoubleTappedMask) | ((uint32_t)value << k_DoubleTappedBitOffset);
}

bool ButtonAction::Bind::GetTapped() const{
	return _internalBitmask & k_TappedMask;
}

void ButtonAction::Bind::SetTapped(bool value){
	_internalBitmask = (_internalBitmask & ~k_TappedMask) | ((uint32_t)value << k_TappedBitOffset);
}

bool ButtonAction::Bind::GetLongPressed() const{
	return _internalBitmask & k_LongPressedMask;
}

void ButtonAction::Bind::SetLongPressed(bool value) {
	_internalBitmask = (_internalBitmask & ~k_LongPressedMask) | ((uint32_t)value << k_LongPressedBitOffset);
}

const ButtonAction::Bind* ButtonAction::GetPrimary() const{
	return &m_Primary;
}

const ButtonAction::Bind* ButtonAction::GetSecondary() const{
	return &m_Secondary;
}

std::string ButtonAction::GetSet() const{
	return set;
}

bool ButtonAction::GetEnabled() const{
	return enabled;
}

void ButtonAction::SetEnabled(bool value) {
	enabled = true;

	ReAction::UpdateActionEnabled(std::shared_ptr<ButtonAction>(this));
}

bool ButtonAction::GetActive() const{
	return 
	enabled &&
	((conditionalsState & m_Primary.GetConditional()) != Conditional::None && (m_Primary.GetModifiers() == Modifier::None || (ReAction::activeModifiers & m_Primary.GetModifiers()) != Modifier::None)) ||
	((conditionalsState & m_Secondary.GetConditional()) != Conditional::None && (m_Secondary.GetModifiers() == Modifier::None || (ReAction::activeModifiers & m_Secondary.GetModifiers()) != Modifier::None))
	;
}

bool ButtonAction::GetConditionState(Conditional conditional) const{
	return (conditionalsState & conditional) != Conditional::None;
}

ButtonAction::ButtonAction(const std::string &name, const Bind &primary, const Bind &secondary, const std::string &set, bool enabled, Conditional forceConditionals):
m_Primary(primary), m_Secondary(secondary), name(name), set(set), enabled(enabled), forceConditionals(forceConditionals) {
}

ButtonAction::ButtonAction():
m_Primary(), m_Secondary(), name(), set(), enabled(), forceConditionals() {
}

ButtonAction::~ButtonAction() {
}

ButtonAction::Modifier ReAction::activeModifiers = ButtonAction::Modifier::None;

Scene* ReAction::scene = nullptr;

std::vector<Key> ReAction::_pressedButtons;
std::vector<Key> ReAction::_releasedButtons;
std::unordered_set<std::shared_ptr<ButtonAction>> ReAction::_allActions;
std::unordered_set<std::shared_ptr<ButtonAction>> ReAction::_enabledActions;
std::unordered_set<std::string> ReAction::_sets;
std::unordered_map<ButtonAction::Bind*, float> ReAction::_tappedButtons;
std::unordered_set<std::string> ReAction::_activeSets = { "general" };

void ReAction::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_REPEAT) {
		return;
	}

	auto checkPressedKey = [](std::shared_ptr<ButtonAction> action, ButtonAction::Bind* bind) {
		action->conditionalsState |= ButtonAction::Conditional::Press;
		
		action->conditionalsState |= ButtonAction::Conditional::Continuous;

		if (ReAction::scene->Input()->KeyReleasedTime(bind->GetKey()) < bind->timeOut) {
			action->conditionalsState |= ButtonAction::Conditional::Mash;
		}

		action->conditionalsState ^= ButtonAction::Conditional::Toggle;
	};

	auto checkReleasedKey = [](std::shared_ptr<ButtonAction> action, ButtonAction::Bind* bind) {
		action->conditionalsState |= ButtonAction::Conditional::Release;

		bind->SetLongPressed(false);

		if (ReAction::scene->Input()->KeyPressedTime(bind->GetKey()) < bind->timeOut) {
			action->conditionalsState |= ButtonAction::Conditional::Tap;

			auto tapped = _tappedButtons.find(bind);

			if (tapped != _tappedButtons.end()) {
				if (Time::Current() - tapped->second < bind->timeOut && !bind->GetDoubleTapped()) {
					action->conditionalsState |= ButtonAction::Conditional::DoubleTap;
					
					bind->SetDoubleTapped(true);
				}
				else {
					tapped->second = Time::Current();
					
					bind->SetDoubleTapped(false);
				}
			}
			else {
				_tappedButtons.emplace(bind, Time::Current());
			}
		}

		action->conditionalsState &= ~ButtonAction::Conditional::Continuous;
	};

	if (action == GLFW_PRESS) {
		switch ((Key)key) {
			case Key::LeftShift:
				activeModifiers |= ButtonAction::Modifier::LShift;
				break;
			case Key::LeftCtrl:
				activeModifiers |= ButtonAction::Modifier::LCtrl;
				break;
			case Key::LeftSuper:
				activeModifiers |= ButtonAction::Modifier::LMeta;
				break;
			case Key::LeftAlt:
				activeModifiers |= ButtonAction::Modifier::LAlt;
				break;
			case Key::RightAlt:
				activeModifiers |= ButtonAction::Modifier::RAlt;
				break;
			case Key::RightSuper:
				activeModifiers |= ButtonAction::Modifier::RMeta;
				break;
			case Key::RightCtrl:
				activeModifiers |= ButtonAction::Modifier::RCtrl;
				break;
			case Key::RightShift:
				activeModifiers |= ButtonAction::Modifier::RShift;
				break;
			default:
				break;
		}

		for (auto action : _enabledActions) {
			if (action->m_Primary.GetKey() == (Key)key) {
				checkPressedKey(action, &action->m_Primary);
			}

			if (action->m_Secondary.GetKey() == (Key)key) {
				checkPressedKey(action, &action->m_Secondary);
			}
		}
	}
	else {
		switch ((Key)key) {
			case Key::LeftShift:
				activeModifiers &= ~ButtonAction::Modifier::LShift;
				break;
			case Key::LeftCtrl:
				activeModifiers &= ~ButtonAction::Modifier::LCtrl;
				break;
			case Key::LeftSuper:
				activeModifiers &= ~ButtonAction::Modifier::LMeta;
				break;
			case Key::LeftAlt:
				activeModifiers &= ~ButtonAction::Modifier::LAlt;
				break;
			case Key::RightAlt:
				activeModifiers &= ~ButtonAction::Modifier::RAlt;
				break;
			case Key::RightSuper:
				activeModifiers &= ~ButtonAction::Modifier::RMeta;
				break;
			case Key::RightCtrl:
				activeModifiers &= ~ButtonAction::Modifier::RCtrl;
				break;
			case Key::RightShift:
				activeModifiers &= ~ButtonAction::Modifier::RShift;
				break;
			default:
				break;
		}

		for (auto action : _enabledActions) {
			if (action->m_Primary.GetKey() == (Key)key) {
				checkReleasedKey(action, &action->m_Primary);
			}

			if (action->m_Secondary.GetKey() == (Key)key) {
				checkReleasedKey(action, &action->m_Secondary);
			}
		}
	}
}

void ReAction::RegisterButtonAction(std::shared_ptr<ButtonAction> buttonAction) {
	for (auto action : _allActions) {
		//we don't want multiple actions with the same name, meow
		if (buttonAction->name == action->name)
			return;
	}

	_allActions.emplace(buttonAction);

	auto set = buttonAction->set;

	_sets.emplace(set);

	UpdateActionEnabled(buttonAction);
}

void ReAction::UnregisterButtonAction(std::shared_ptr<ButtonAction> buttonAction) {
	_allActions.erase(buttonAction);
	_enabledActions.erase(buttonAction);
}

void ReAction::UpdateActionEnabled(std::shared_ptr<ButtonAction> buttonAction) {
	if (buttonAction->enabled) {
		if (_activeSets.contains(buttonAction->set)) {
			_enabledActions.emplace(buttonAction);
		}
	}
	else {
		_enabledActions.erase(buttonAction);

		buttonAction->conditionalsState = ButtonAction::Conditional::None;
	}
}

void ReAction::RefreshActionLists() {
	auto checkLongPress = [this](std::shared_ptr<ButtonAction> action, ButtonAction::Bind* bind) {
		if (bind->timeOut == 0) {
			return;
		}

		if (this->GetScene()->Input()->KeyPressedTime(bind->GetKey()) >= bind->timeOut && !bind->GetLongPressed()) {
			action->conditionalsState |= ButtonAction::Conditional::LongPress;

			bind->SetLongPressed(true);
		}
	};

	auto checkMashed = [this](std::shared_ptr<ButtonAction> action, ButtonAction::Bind* bind) {
		if (bind->timeOut == 0) {
			return;
		}

		if (Time::Current() - this->GetScene()->Input()->KeyChangedState(bind->GetKey()) >= bind->timeOut) {
			action->conditionalsState &= ~ButtonAction::Conditional::Mash;
		}
	};

	for (auto action : _enabledActions) {
		action->conditionalsState &= ~ButtonAction::Conditional::Press;

		action->conditionalsState &= ~ButtonAction::Conditional::Release;

		action->conditionalsState &= ~ButtonAction::Conditional::Tap;

		action->conditionalsState &= ~ButtonAction::Conditional::LongPress;

		action->conditionalsState &= ~ButtonAction::Conditional::DoubleTap;

		checkMashed(action, &action->m_Primary);
		checkMashed(action, &action->m_Secondary);

		checkLongPress(action, &action->m_Primary);
		checkLongPress(action, &action->m_Secondary);
	}
}

void ReAction::CreateAction(const std::string& name, const ButtonAction::Bind& primary, const ButtonAction::Bind& secondary, const std::string& set, bool enabled, ButtonAction::Conditional forceConditionals) {
	RegisterButtonAction(std::shared_ptr<ButtonAction>(new ButtonAction(name, primary, secondary, set, enabled, forceConditionals)));
}

void ReAction::OnPostUpdate() {
	RefreshActionLists();
}

//Dick Head
int ReAction::Order() {
	return INT_MAX;
}

void ReAction::DrawImGui() {
	if (ImGui::TreeNode("ReAction Debug")) {
		for (auto action : _allActions) {
			if (ImGui::TreeNode(action->name.c_str())) {
				ImGui::Text(action->GetActive() ? "true" : "false");
				ImGui::Text("conditional state: %b", (unsigned char)action->conditionalsState);
				
				if (ImGui::TreeNode("primary")) {
					ImGui::Text("key: %i", action->m_Primary.GetKey());
					ImGui::Text("modifiers: %b", (unsigned char)action->m_Primary.GetModifiers());
					ImGui::Text("conditionals: %b", (unsigned char)action->m_Primary.GetConditional());
					ImGui::Text("timeout: %f", action->m_Primary.timeOut);

					ImGui::TreePop();
				}
				
				if (ImGui::TreeNode("secondary")) {
					ImGui::Text("key: %i", action->m_Secondary.GetKey());
					ImGui::Text("modifiers: %b", (unsigned char)action->m_Secondary.GetModifiers());
					ImGui::Text("conditionals: %b", (unsigned char)action->m_Secondary.GetConditional());
					ImGui::Text("timeout: %f", action->m_Secondary.timeOut);

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}
}
	
	ReAction::ReAction(Scene* scene):
SceneComponent(scene) {
	this->scene = scene;
	_sets = { "general" };
}

std::vector<ButtonAction> ReAction::GetAllActions() {
	std::vector<ButtonAction> array(_allActions.size());

	int i = 0;

	for (auto action : _allActions) {
		array[i++] = *action;
	}

	return array;
}

std::vector<ButtonAction> ReAction::GetEnabledActions() {
	std::vector<ButtonAction> array(_enabledActions.size());
	
	int i = 0;

	for (auto action : _enabledActions) {
		array[i++] = *action;
	}

	return array;
}

std::vector<std::string> ReAction::GetActiveSets() {
	std::vector<std::string> array(_activeSets.size());

	int i = 0;

	for (auto set : _activeSets) {
		array[i++] = set;
	}

	return array;
}

ButtonAction ReAction::GetAction(const std::string &actionName, bool throwOnMissing) {
	bool foundAction = false;

	for (auto action : _allActions) {
		if (action->name == actionName) {
			foundAction = true;
			return *action;
		}
	}

	if (throwOnMissing) {
		throw std::runtime_error(actionName + " does not exist");
	}

	return {};
}

void ReAction::SetActionSetActive(const std::string &setName, bool active) {
	if (setName != "general") {
		if (_sets.contains(setName)) {
			if (active) {
				_activeSets.emplace(setName);

				for (auto action : _allActions) {
					if (action->set == setName) {
						_enabledActions.emplace(action);
					}
				}
			}
			else {
				_activeSets.erase(setName);

				for (auto action : _allActions) {
					if (action->set == setName) {
						_enabledActions.erase(action);
					}
				}
			}
		}
	}
}
