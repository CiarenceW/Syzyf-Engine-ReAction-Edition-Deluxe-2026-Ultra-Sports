#pragma once

#include <string>
#include <InputSystem.h>
#include <unordered_map>
#include <unordered_set>
#include <Scene.h>
#include <TimeSystem.h>
#include <stdexcept>
#include <imgui.h>
#include <bitset>
#include <bits/stdc++.h>

#include <GLFW/glfw3.h>

class ButtonAction {
	friend class ReAction;
public:
	enum class Modifier : unsigned char
	{
		None   = 0,
		LShift = 1 << 0,
		LCtrl  = 1 << 1,
		LMeta  = 1 << 2,
		LAlt   = 1 << 3,
		RAlt   = 1 << 4,
		RMeta  = 1 << 5,
		RCtrl  = 1 << 6,
		RShift = 1 << 7
	};

	enum class Conditional : unsigned char
	{
		None       = 0,
		/// @brief Was the action pressed during this frame?
		Press      = 1 << 0,
		/// @brief Was the action pressed and held?
		LongPress  = 1 << 1,
		/// @brief Was the action released during this frame?
		Release    = 1 << 2,
		/// @brief Is the action being held?
		Continuous = 1 << 3,
		/// @brief Was the action pressed and quickly released?
		Tap        = 1 << 4,
		/// @brief Was the action tapped twice in quick succession?
		DoubleTap  = 1 << 5,
		/// @brief Is the action being continuously tapped?
		Mash       = 1 << 6,
		/// @brief Is the action toggled?
		Toggle     = 1 << 7
	};

	struct Bind {
		friend class ReAction;
	public:
		Bind(Key key, Modifier modifiers, Conditional conditional, float timeOut = .5f);

		Bind();

		~Bind();

		Key GetKey() const;
		void SetKey(Key keyMeow);

		Modifier GetModifiers() const;
		void SetModifiers(Modifier modifiers);

		Conditional GetConditional() const;
		void SetConditional(Conditional conditional);
		
		/// @brief Depending on the Conditional, determines if it's active before, or after the set time
		float timeOut;

	private:
		/// @brief thanks to this, Bind is only 64bits big!!
		unsigned int _internalBitmask;

		bool GetDoubleTapped() const;
		void SetDoubleTapped(bool value);

		bool GetTapped() const;
		void SetTapped(bool value);

		bool GetLongPressed() const;
		void SetLongPressed(bool value);
	};

	std::string name;

	Conditional allowedConditionals = Conditional::None;

	const Bind* GetPrimary() const;
	const Bind* GetSecondary() const;

	std::string GetSet() const;

	bool GetEnabled() const;
	void SetEnabled(bool value);

	bool GetActive() const;

	bool GetConditionState(Conditional conditional) const;

	inline operator bool() const
	{
		return GetActive();
	}
	
	ButtonAction();
	
	~ButtonAction();
//fuck you c++
private:

	Bind m_Primary;
	Bind m_Secondary;

	std::string set = "general";

	bool enabled;

	Conditional conditionalsState;

	ButtonAction(const std::string& name, const Bind& primary, const Bind& secondary, const std::string& set = "general", bool enabled = true, Conditional forceConditionals = Conditional::None);
};

class ReAction : public SceneComponent, public ImGuiDrawable {
	friend class ButtonAction;
	friend class Engine;
private:
	static std::vector<Key> _pressedButtons;
	static std::vector<Key> _releasedButtons;
	static std::unordered_set<std::shared_ptr<ButtonAction>> _allActions;
	static std::unordered_set<std::shared_ptr<ButtonAction>> _enabledActions;
	static std::unordered_set<std::string> _sets;
	static std::unordered_map<ButtonAction::Bind*, float> _tappedButtons;
	static std::unordered_set<std::string> _activeSets;

	static ButtonAction::Modifier activeModifiers;

	#warning "remove this shit when InputSystem gets moved to a static system and we don't have to rely on the fuck ass Dick Head scene anymore"
	static Scene* scene;

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	static void RegisterButtonAction(std::shared_ptr<ButtonAction> buttonAction);
	static void UnregisterButtonAction(std::shared_ptr<ButtonAction> buttonAction);

	static void UpdateActionEnabled(std::shared_ptr<ButtonAction> buttonAction);
	
	void RefreshActionLists();

protected: 
	virtual void OnPostUpdate();
	virtual int Order();
	virtual void DrawImGui();

public:
	ReAction(Scene* scene);

	/// @brief Gets a list of all actions
	/// @return A copy of the list of all actions
	static std::vector<ButtonAction> GetAllActions();

	/// @brief Gets a list of all enabled actions
	/// @return A copy of the list of all enabled actions
	static std::vector<ButtonAction> GetEnabledActions();

	/// @brief Gets a list of all the active sets
	/// @return A copy of the list of all active sets
	static std::vector<std::string> GetActiveSets();

	/// @brief Creates a new action, and returns it
	/// @param name The name of the action
	/// @param primary The primary bind
	/// @param secondary The secondary bind
	/// @param set The name of the set the action should belong to
	/// @param enabled Whether or not the action should be enabled
	/// @param allowedConditionals Use this for UI and stuff, if you wanted the user to only be able to choose between Press or Long Pressed, you'd make this Press | LongPress and check ButtonAction.allowedConditionals in your UI code
	/// @return The newly created action
	static ButtonAction CreateAction(const std::string& name, const ButtonAction::Bind& primary, const ButtonAction::Bind& secondary, const std::string& set = "general", bool enabled = true, ButtonAction::Conditional forceConditionals = ButtonAction::Conditional::None);

	/// @brief Gets an action by name
	/// @param actionName The name of the action
	/// @param throwOnMissing Whether or not to throw an error if the action is not found
	/// @return The action that you wanted to get, or nothing if the action wasn't found
	static ButtonAction GetAction(const std::string& actionName, bool throwOnMissing = false);

	/// @brief Makes a set active, or not
	/// @param setName The name of the set
	/// @param active Whether or not the set should be
	static void SetActionSetActive(const std::string& setName, bool active);
};

namespace std {
	template <>
	struct hash<ButtonAction::Bind> {
		size_t operator()(const ButtonAction::Bind &p) const {
			size_t keyHash = hash<int>{}((int)p.GetKey());
			size_t conditionalHash = hash<unsigned char>{}((unsigned char)p.GetConditional());
			size_t modifierHash = hash<unsigned char>{}((unsigned char)p.GetModifiers());
			size_t floatHash = hash<float>{}(p.timeOut);
			
			return keyHash ^ (conditionalHash << 1) ^ (modifierHash << 2) ^ (floatHash << 3);
		}
	};
}

namespace std {
	template <>
	struct hash<ButtonAction> {
		size_t operator()(const ButtonAction &p) const {
			size_t nameHash = hash<std::string>{}(p.name);
			size_t setHash = hash<std::string>{}(p.GetSet());
			size_t primaryBindHash = hash<ButtonAction::Bind>{}(*p.GetPrimary());
			size_t secondaryBindHash = hash<ButtonAction::Bind>{}(*p.GetSecondary());

			return nameHash ^ (setHash << 1) ^ (primaryBindHash << 2) ^ (secondaryBindHash << 3);
		}
	};
}

inline constexpr ButtonAction::Modifier operator~(ButtonAction::Modifier a) {
	return (ButtonAction::Modifier)~(unsigned char)a;
}

inline constexpr ButtonAction::Modifier operator&(ButtonAction::Modifier a, ButtonAction::Modifier b) {
	return (ButtonAction::Modifier)((unsigned char)a & (unsigned char)b);
}

inline constexpr ButtonAction::Modifier operator|(ButtonAction::Modifier a, ButtonAction::Modifier b) {
	return (ButtonAction::Modifier)((unsigned char)a | (unsigned char)b);
}

inline constexpr ButtonAction::Modifier operator^(ButtonAction::Modifier a, ButtonAction::Modifier b) {
	return (ButtonAction::Modifier)((unsigned char)a ^ (unsigned char)b);
}

inline constexpr ButtonAction::Modifier& operator&=(ButtonAction::Modifier& a, ButtonAction::Modifier b) {
	a = a & b;
	return a;
}

inline constexpr ButtonAction::Modifier& operator|=(ButtonAction::Modifier& a, ButtonAction::Modifier b) {
	a = a | b;
	return a;
}

inline constexpr ButtonAction::Modifier& operator^=(ButtonAction::Modifier& a, ButtonAction::Modifier b) {
	a = a ^ b;
	return a;
}

inline constexpr ButtonAction::Conditional operator~(ButtonAction::Conditional a) {
	return (ButtonAction::Conditional)~(unsigned char)a;
}

inline constexpr ButtonAction::Conditional operator&(ButtonAction::Conditional a, ButtonAction::Conditional b) {
	return (ButtonAction::Conditional)((unsigned char)a & (unsigned char)b);
}

inline constexpr ButtonAction::Conditional operator|(ButtonAction::Conditional a, ButtonAction::Conditional b) {
	return (ButtonAction::Conditional)((unsigned char)a | (unsigned char)b);
}

inline constexpr ButtonAction::Conditional operator^(ButtonAction::Conditional a, ButtonAction::Conditional b) {
	return (ButtonAction::Conditional)((unsigned char)a ^ (unsigned char)b);
}

inline constexpr ButtonAction::Conditional& operator&=(ButtonAction::Conditional& a, ButtonAction::Conditional b) {
	a = a & b;
	return a;
}

inline constexpr ButtonAction::Conditional& operator|=(ButtonAction::Conditional& a, ButtonAction::Conditional b) {
	a = a | b;
	return a;
}

inline constexpr ButtonAction::Conditional& operator^=(ButtonAction::Conditional& a, ButtonAction::Conditional b) {
	a = a ^ b;
	return a;
}