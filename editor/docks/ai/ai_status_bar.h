#pragma once
#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/progress_bar.h"

class AIStatusBar : public HBoxContainer {
    GDCLASS(AIStatusBar, HBoxContainer);

public:
    enum StatusState {
        STATUS_IDLE             = 0,
        STATUS_THINKING         = 1,
        STATUS_EXECUTING_TOOL   = 2,
        STATUS_INDEXING         = 3,
        STATUS_ERROR            = 4,
    };

    AIStatusBar();

    void set_status(StatusState p_state, const String &p_message = "");
    void set_progress(float p_value);
    void set_token_count(int p_count);
    void set_context_usage(float p_percentage);

protected:
    void _ready() override;
    void _process(double p_delta) override;
    static void _bind_methods();

private:
    StatusState current_state     = STATUS_IDLE;
    Label      *status_label      = nullptr;
    Label      *message_label     = nullptr;
    ProgressBar *progress_bar     = nullptr;
    Label      *token_label       = nullptr;
    Label      *context_usage_label = nullptr;

    float pulse_time = 0.0f;

    void _update_display();
};
