#include "string_button.hpp"

#include <aurora/rmlui.hpp>

#if defined(__ANDROID__)
#include "dusk/vr/vr_main.hpp"

#include <SDL3/SDL_system.h>
#include <jni.h>

#include <optional>
#include <string>
#endif

namespace dusk::ui {

#if defined(__ANDROID__)
namespace {

// While the game is an immersive VR app the Android keyboard can't show over
// it, so text fields open DuskActivity's 2D text entry panel instead
// (TextInputActivity) and poll for its result.
bool use_text_input_panel() {
    return dusk::vr::isActive();
}

bool request_text_input_panel(const Rml::String& text, bool number, int maxLength) {
    auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    auto activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (env == nullptr || activity == nullptr) {
        return false;
    }
    bool ok = false;
    jclass activityClass = env->GetObjectClass(activity);
    jmethodID method =
        env->GetMethodID(activityClass, "requestTextInput", "(Ljava/lang/String;ZI)V");
    if (method == nullptr) {
        env->ExceptionClear();
    } else {
        jstring jtext = env->NewStringUTF(text.c_str());
        env->CallVoidMethod(activity, method, jtext, static_cast<jboolean>(number),
                            static_cast<jint>(maxLength));
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        } else {
            ok = true;
        }
        if (jtext != nullptr) {
            env->DeleteLocalRef(jtext);
        }
    }
    env->DeleteLocalRef(activityClass);
    env->DeleteLocalRef(activity);
    return ok;
}

using PanelResult = std::optional<std::optional<Rml::String>>;

PanelResult panel_cancelled() {
    return PanelResult{std::in_place, std::nullopt};
}

// Empty while still waiting; otherwise the text (accepted) or nullopt (cancelled).
PanelResult poll_text_input_panel() {
    auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    auto activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (env == nullptr || activity == nullptr) {
        return panel_cancelled();  // can't poll: treat as cancelled
    }
    PanelResult result;
    jclass activityClass = env->GetObjectClass(activity);
    jmethodID method = env->GetMethodID(activityClass, "pollTextInput", "()Ljava/lang/String;");
    if (method == nullptr) {
        env->ExceptionClear();
        result = panel_cancelled();
    } else if (auto str = static_cast<jstring>(env->CallObjectMethod(activity, method))) {
        if (const char* chars = env->GetStringUTFChars(str, nullptr)) {
            const std::string value(chars);
            env->ReleaseStringUTFChars(str, chars);
            if (!value.empty() && value[0] == '1') {
                result = PanelResult{std::in_place, value.substr(1)};
            } else {
                result = panel_cancelled();
            }
        }
        env->DeleteLocalRef(str);
    } else if (env->ExceptionCheck()) {
        env->ExceptionClear();
        result = panel_cancelled();
    }
    env->DeleteLocalRef(activityClass);
    env->DeleteLocalRef(activity);
    return result;
}

}  // namespace
#endif

BaseStringButton::BaseStringButton(Rml::Element* parent, Props props)
    : BaseControlledSelectButton(parent, {std::move(props.key)}), mType(std::move(props.type)),
      mMaxLength(props.maxLength), mSetOnChange(props.setOnChange) {
    mInputListeners.reserve(5);
}

void BaseStringButton::update() {
#if defined(__ANDROID__)
    if (mWaitingForPanelInput && is_editing()) {
        if (auto result = poll_text_input_panel()) {
            mWaitingForPanelInput = false;
            if (*result) {
                mInputElem->SetValue(**result);
            }
            request_stop_editing(result->has_value(), true);
        }
    }
#endif
    if (mPendingStopEditing) {
        stop_editing(mPendingCommit, mPendingRefocusRoot);
    }
    if (mPendingInputFocusFrames > 0) {
        --mPendingInputFocusFrames;
        if (mPendingInputFocusFrames == 0) {
            focus_input();
        }
    }
    BaseControlledSelectButton::update();
}

void BaseStringButton::start_editing() {
    if (is_editing()) {
        return;
    }

    // Create input element
    auto* doc = mRoot->GetOwnerDocument();
    auto elemPtr = doc->CreateElement("input");
    mInputElem = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(elemPtr.get());
    if (mInputElem == nullptr) {
        return;
    }
    mOriginalValue = input_value();
    mInputElem->SetAttribute("type", mType);
    mInputElem->SetAttribute("value", mOriginalValue);
    if (mMaxLength > -1) {
        mInputElem->SetAttribute("maxlength", mMaxLength);
    }
    mRoot->AppendChild(std::move(elemPtr));

    // Hide value element
    mValueElem->SetProperty(Rml::PropertyId::Visibility, Rml::Style::Visibility::Hidden);

    // RmlUi lays out the new input during render. Wait one full frame before focusing it so
    // mobile keyboard placement gets a valid caret rectangle.
    mPendingInputFocusFrames = 2;

    // Dispatch a submit event so the pane can handle item selection
    // However, mark it as "handled" to ensure that we don't steal focus away
    mRoot->DispatchEvent(Rml::EventId::Submit, {{"handled", Rml::Variant{true}}});

    // Register input listeners
    mInputListeners.emplace_back(std::make_unique<ScopedEventListener>(
        mInputElem, Rml::EventId::Textinput, [this](Rml::Event& event) {
            if (event.GetTargetElement() == mInputElem) {
                const Rml::String text = event.GetParameter("text", Rml::String{});
                if (!text.empty() &&
                    std::ranges::all_of(
                        text, [](const char c) { return c == '\r' || c == '\n' || c == '\t'; }))
                {
                    event.StopImmediatePropagation();
                }
            }
        }));
    mInputListeners.emplace_back(std::make_unique<ScopedEventListener>(
        mInputElem, Rml::EventId::Keydown, [this](Rml::Event& event) {
            const auto cmd = map_nav_event(event);
            if (cmd == NavCommand::Confirm) {
                request_stop_editing(true, true);
                event.StopImmediatePropagation();
            } else if (cmd == NavCommand::Cancel) {
                request_stop_editing(false, true);
                event.StopImmediatePropagation();
            }
        }));
    mInputListeners.emplace_back(std::make_unique<ScopedEventListener>(
        mInputElem, Rml::EventId::Click, [](Rml::Event& event) { event.StopPropagation(); }));
    mInputListeners.emplace_back(std::make_unique<ScopedEventListener>(mInputElem,
        Rml::EventId::Blur, [this](Rml::Event&) { request_stop_editing(true, false); }));
    if (mSetOnChange) {
        mInputListeners.emplace_back(std::make_unique<ScopedEventListener>(
            mInputElem, Rml::EventId::Change, [this](Rml::Event& event) {
                if (event.GetTargetElement() == mInputElem) {
                    set_value(mInputElem->GetValue());
                }
            }));
    }
}

void BaseStringButton::request_stop_editing(bool commit, bool refocusRoot) {
    mPendingStopEditing = true;
    mPendingCommit = commit;
    mPendingRefocusRoot = refocusRoot;
}

bool BaseStringButton::handle_nav_command(NavCommand cmd) {
    if (cmd == NavCommand::Confirm) {
        if (!is_editing()) {
            start_editing();
        } else {
            request_stop_editing(true, true);
        }
        return true;
    } else if (cmd == NavCommand::Cancel) {
        if (is_editing()) {
            request_stop_editing(false, true);
            return true;
        }
    }
    return false;
}

void BaseStringButton::focus_input() {
    if (!is_editing()) {
        return;
    }

#if defined(__ANDROID__)
    if (use_text_input_panel() &&
        request_text_input_panel(mInputElem->GetValue(), mType == "number", mMaxLength))
    {
        mWaitingForPanelInput = true;
        return;
    }
#endif

    aurora::rmlui::set_input_type(
        mType == "number" ? aurora::rmlui::InputType::Number : aurora::rmlui::InputType::Text);

    if (mInputElem->Focus(true)) {
        const int end = static_cast<int>(Rml::StringUtilities::LengthUTF8(mInputElem->GetValue()));
        mInputElem->SetSelectionRange(0, end);
    }
}

void BaseStringButton::stop_editing(bool commit, bool refocusRoot) {
    mPendingStopEditing = false;
    mPendingInputFocusFrames = 0;
    mWaitingForPanelInput = false;
    if (!is_editing()) {
        return;
    }
    if (!mSetOnChange && commit) {
        set_value(mInputElem->GetValue());
    } else if (mSetOnChange && !commit) {
        set_value(mOriginalValue);
    }
    mInputListeners.clear();
    mRoot->RemoveChild(mInputElem);
    mInputElem = nullptr;
    mOriginalValue.clear();

    // Restore value element
    mValueElem->SetProperty(Rml::PropertyId::Visibility, Rml::Style::Visibility::Visible);

    set_selected(false);
    if (refocusRoot) {
        mRoot->Focus(true);
    }
}

StringButton::StringButton(Rml::Element* parent, Props props)
    : BaseStringButton(parent,
          {
              .key = std::move(props.key),
              .maxLength = props.maxLength,
              .setOnChange = props.setOnChange,
          }),
      mGetValue(std::move(props.getValue)), mSetValue(std::move(props.setValue)),
      mIsDisabled(std::move(props.isDisabled)), mIsModified(std::move(props.isModified)) {}

bool StringButton::modified() const {
    if (mIsModified) {
        return mIsModified();
    }
    return BaseStringButton::modified();
}

bool StringButton::disabled() const {
    if (mIsDisabled) {
        return mIsDisabled();
    }
    return BaseStringButton::disabled();
}

}  // namespace dusk::ui
