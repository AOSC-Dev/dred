
typedef struct
{
    drgui_element* pInternalTB;
    dred_timer* pTimer;
} dred_textbox_data;

drgui_element* dred_textbox__get_internal_tb(dred_textbox* pTextBox)
{
    dred_textbox_data* data = dred_control_get_extra_data(pTextBox);
    if (data == NULL) {
        return NULL;
    }

    return data->pInternalTB;
}


void dred_textbox__on_timer(dred_timer* pTimer, void* pUserData)
{
    dred_textbox* pTextBox = (dred_textbox*)pUserData;
    assert(pTextBox != NULL);

    dred_textbox_data* data = (dred_textbox_data*)dred_control_get_extra_data(pTextBox);
    assert(data != NULL);

    drgui_textbox_step(data->pInternalTB, 100);
}

void dred_textbox__on_capture_keyboard(dred_textbox* pTextBox, drgui_element* pPrevCapturedElement)
{
    (void*)pPrevCapturedElement;

    // The internal text box needs to be given the keyboard focus whenever a dred_textbox receives it.
    dred_textbox_data* data = (dred_textbox_data*)dred_control_get_extra_data(pTextBox);
    if (data == NULL) {
        return;
    }

    drgui_capture_keyboard(data->pInternalTB);
}


void dred_textbox_innertb__on_key_down(drgui_element* pInternalTB, drgui_key key, int stateFlags)
{
    (void)stateFlags;

    dred_textbox* pTextBox = dred_control_get_parent(pInternalTB);
    assert(pTextBox != NULL);

    if (pTextBox->onKeyDown) {
        pTextBox->onKeyDown(pTextBox, key, stateFlags);
    } else {
        drgui_textbox_on_key_down(pInternalTB, key, stateFlags);
    }
}

void dred_textbox_innertb__on_key_up(drgui_element* pInternalTB, drgui_key key, int stateFlags)
{
    (void)stateFlags;

    dred_textbox* pTextBox = dred_control_get_parent(pInternalTB);
    assert(pTextBox != NULL);

    if (pTextBox->onKeyUp) {
        pTextBox->onKeyUp(pTextBox, key, stateFlags);
    } else {
        drgui_textbox_on_key_up(pInternalTB, key, stateFlags);
    }
}

void dred_textbox_innertb__on_printable_key_down(drgui_element* pInternalTB, uint32_t utf32, int stateFlags)
{
    (void)stateFlags;

    dred_textbox* pTextBox = dred_control_get_parent(pInternalTB);
    assert(pTextBox != NULL);

    if (pTextBox->onPrintableKeyDown) {
        pTextBox->onPrintableKeyDown(pTextBox, utf32, stateFlags);
    } else {
        drgui_textbox_on_printable_key_down(pInternalTB, utf32, stateFlags);
    }
}

void dred_textbox_innertb__on_capture_keyboard(drgui_element* pInternalTB, drgui_element* pPrevCapturedElement)
{
    (void)pPrevCapturedElement;

    dred_textbox* pTextBox = dred_control_get_parent(pInternalTB);
    assert(pTextBox != NULL);

    dred_textbox_data* data = (dred_textbox_data*)dred_control_get_extra_data(pTextBox);
    assert(data != NULL);

    if (data->pTimer == NULL) {
        data->pTimer = dred_timer_create(100, dred_textbox__on_timer, pTextBox);
    }

    drgui_textbox_on_capture_keyboard(pInternalTB, pPrevCapturedElement);
}

void dred_textbox_innertb__on_release_keyboard(drgui_element* pInternalTB, drgui_element* pNewCapturedElement)
{
    dred_textbox* pTextBox = dred_control_get_parent(pInternalTB);
    assert(pTextBox != NULL);

    dred_textbox_data* data = (dred_textbox_data*)dred_control_get_extra_data(pTextBox);
    assert(data != NULL);

    if (data->pTimer != NULL) {
        dred_timer_delete(data->pTimer);
        data->pTimer = NULL;
    }

    drgui_textbox_on_release_keyboard(pInternalTB, pNewCapturedElement);
}


dred_textbox* dred_textbox_create(dred_context* pDred, dred_control* pParent)
{
    dred_textbox* pTextBox = dred_control_create(pDred, pParent, DRED_CONTROL_TYPE_TEXTBOX, sizeof(dred_textbox_data));
    if (pTextBox == NULL) {
        return NULL;
    }

    dred_textbox_data* data = (dred_textbox_data*)dred_control_get_extra_data(pTextBox);
    assert(data != NULL);

    data->pInternalTB = drgui_create_textbox(pDred->pGUI, pTextBox, 0, NULL);
    if (data->pInternalTB == NULL) {
        dred_control_delete(pTextBox);
        return NULL;
    }

    data->pTimer = NULL;

    drgui_textbox_set_background_color(data->pInternalTB, drgui_rgb(64, 64, 64));
    drgui_textbox_set_active_line_background_color(data->pInternalTB, drgui_rgb(64, 64, 64));
    drgui_textbox_set_text_color(data->pInternalTB, drgui_rgb(224, 224, 224));
    drgui_textbox_set_cursor_color(data->pInternalTB, drgui_rgb(224, 224, 224));
    drgui_textbox_set_font(data->pInternalTB, dred_font_acquire_subfont(pDred->pGUIFont, 1));
    drgui_textbox_set_border_width(data->pInternalTB, 0);

    // Events.
    dred_control_set_on_size(pTextBox, drgui_on_size_fit_children_to_parent);
    dred_control_set_on_capture_keyboard(pTextBox, dred_textbox__on_capture_keyboard);

    // Internal text box overrides.
    drgui_set_on_key_down(data->pInternalTB, dred_textbox_innertb__on_key_down);
    drgui_set_on_key_up(data->pInternalTB, dred_textbox_innertb__on_key_up);
    drgui_set_on_printable_key_down(data->pInternalTB, dred_textbox_innertb__on_printable_key_down);
    drgui_set_on_capture_keyboard(data->pInternalTB, dred_textbox_innertb__on_capture_keyboard);
    drgui_set_on_release_keyboard(data->pInternalTB, dred_textbox_innertb__on_release_keyboard);

    return pTextBox;
}

void dred_textbox_delete(dred_textbox* pTextBox)
{
    if (pTextBox == NULL) {
        return;
    }

    dred_textbox_data* data = (dred_textbox_data*)dred_control_get_extra_data(pTextBox);
    if (data != NULL) {
        drgui_delete_textbox(data->pInternalTB);
    }

    dred_control_delete(pTextBox);
}


void dred_textbox_set_text(dred_textbox* pTextBox, const char* text)
{
    dred_textbox_data* data = dred_control_get_extra_data(pTextBox);
    if (data == NULL) {
        return;
    }

    drgui_textbox_set_text(data->pInternalTB, text);
}

size_t dred_textbox_get_text(dred_textbox* pTextBox, char* textOut, size_t textOutSize)
{
    dred_textbox_data* data = dred_control_get_extra_data(pTextBox);
    if (data == NULL) {
        return 0;
    }

    return drgui_textbox_get_text(data->pInternalTB, textOut, textOutSize);
}



void dred_textbox_set_font(dred_textbox* pTextBox, drgui_font* pFont)
{
    drgui_textbox_set_font(dred_textbox__get_internal_tb(pTextBox), pFont);
}

void dred_textbox_set_text_color(dred_textbox* pTextBox, drgui_color color)
{
    drgui_textbox_set_text_color(dred_textbox__get_internal_tb(pTextBox), color);
}

void dred_textbox_set_background_color(dred_textbox* pTextBox, drgui_color color)
{
    drgui_textbox_set_background_color(dred_textbox__get_internal_tb(pTextBox), color);
}

void dred_textbox_set_active_line_background_color(dred_textbox* pTextBox, drgui_color color)
{
    drgui_textbox_set_active_line_background_color(dred_textbox__get_internal_tb(pTextBox), color);
}

void dred_textbox_set_cursor_color(dred_textbox* pTextBox, drgui_color color)
{
    drgui_textbox_set_cursor_color(dred_textbox__get_internal_tb(pTextBox), color);
}

void dred_textbox_set_border_color(dred_textbox* pTextBox, drgui_color color)
{
    drgui_textbox_set_border_color(dred_textbox__get_internal_tb(pTextBox), color);
}

void dred_textbox_set_border_width(dred_textbox* pTextBox, float borderWidth)
{
    drgui_textbox_set_border_width(dred_textbox__get_internal_tb(pTextBox), borderWidth);
}

void dred_textbox_set_padding(dred_textbox* pTextBox, float padding)
{
    drgui_textbox_set_padding(dred_textbox__get_internal_tb(pTextBox), padding);
}

void dred_textbox_set_vertical_align(dred_textbox* pTextBox, drgui_text_engine_alignment align)
{
    drgui_textbox_set_vertical_align(dred_textbox__get_internal_tb(pTextBox), align);
}

void dred_textbox_set_horizontal_align(dred_textbox* pTextBox, drgui_text_engine_alignment align)
{
    drgui_textbox_set_horizontal_align(dred_textbox__get_internal_tb(pTextBox), align);
}


void dred_textbox_set_cursor_blink_rate(dred_textbox* pTextBox, unsigned int blinkRateInMilliseconds)
{
    drgui_textbox_set_cursor_blink_rate(dred_textbox__get_internal_tb(pTextBox), blinkRateInMilliseconds);
}

void dred_textbox_move_cursor_to_end_of_text(dred_textbox* pTextBox)
{
    drgui_textbox_move_cursor_to_end_of_text(dred_textbox__get_internal_tb(pTextBox));
}

void dred_textbox_move_cursor_to_start_of_line_by_index(dred_textbox* pTextBox, size_t iLine)
{
    drgui_textbox_move_cursor_to_start_of_line_by_index(dred_textbox__get_internal_tb(pTextBox), iLine);
}

bool dred_textbox_is_anything_selected(dred_textbox* pTextBox)
{
    return drgui_textbox_is_anything_selected(dred_textbox__get_internal_tb(pTextBox));
}

void dred_textbox_select_all(dred_textbox* pTextBox)
{
    drgui_textbox_select_all(dred_textbox__get_internal_tb(pTextBox));
}

void dred_textbox_deselect_all(dred_textbox* pTextBox)
{
    drgui_textbox_deselect_all(dred_textbox__get_internal_tb(pTextBox));
}

size_t dred_textbox_get_selected_text(dred_textbox* pTextBox, char* textOut, size_t textOutLength)
{
    return drgui_textbox_get_selected_text(dred_textbox__get_internal_tb(pTextBox), textOut, textOutLength);
}

bool dred_textbox_delete_character_to_right_of_cursor(dred_textbox* pTextBox)
{
    return drgui_textbox_delete_character_to_right_of_cursor(dred_textbox__get_internal_tb(pTextBox));
}

bool dred_textbox_delete_selected_text(dred_textbox* pTextBox)
{
    return drgui_textbox_delete_selected_text(dred_textbox__get_internal_tb(pTextBox));
}

bool dred_textbox_insert_text_at_cursor(dred_textbox* pTextBox, const char* text)
{
    return drgui_textbox_insert_text_at_cursor(dred_textbox__get_internal_tb(pTextBox), text);
}

bool dred_textbox_undo(dred_textbox* pTextBox)
{
    return drgui_textbox_undo(dred_textbox__get_internal_tb(pTextBox));
}

bool dred_textbox_redo(dred_textbox* pTextBox)
{
    return drgui_textbox_redo(dred_textbox__get_internal_tb(pTextBox));
}

unsigned int dred_textbox_get_undo_points_remaining_count(dred_textbox* pTextBox)
{
    return drgui_textbox_get_undo_points_remaining_count(dred_textbox__get_internal_tb(pTextBox));
}

unsigned int dred_textbox_get_redo_points_remaining_count(dred_textbox* pTextBox)
{
    return drgui_textbox_get_redo_points_remaining_count(dred_textbox__get_internal_tb(pTextBox));
}

size_t dred_textbox_get_cursor_line(dred_textbox* pTextBox)
{
    return drgui_textbox_get_cursor_line(dred_textbox__get_internal_tb(pTextBox));
}

size_t dred_textbox_get_cursor_column(dred_textbox* pTextBox)
{
    return drgui_textbox_get_cursor_column(dred_textbox__get_internal_tb(pTextBox));
}

size_t dred_textbox_get_line_count(dred_textbox* pTextBox)
{
    return drgui_textbox_get_line_count(dred_textbox__get_internal_tb(pTextBox));
}


bool dred_textbox_find_and_select_next(dred_textbox* pTextBox, const char* text)
{
    return drgui_textbox_find_and_select_next(dred_textbox__get_internal_tb(pTextBox), text);
}

bool dred_textbox_find_and_replace_next(dred_textbox* pTextBox, const char* text, const char* replacement)
{
    return drgui_textbox_find_and_replace_next(dred_textbox__get_internal_tb(pTextBox), text, replacement);
}

bool dred_textbox_find_and_replace_all(dred_textbox* pTextBox, const char* text, const char* replacement)
{
    return drgui_textbox_find_and_replace_all(dred_textbox__get_internal_tb(pTextBox), text, replacement);
}


void dred_textbox_show_line_numbers(dred_textbox* pTextBox)
{
    drgui_textbox_show_line_numbers(dred_textbox__get_internal_tb(pTextBox));
}

void dred_textbox_hide_line_numbers(dred_textbox* pTextBox)
{
    drgui_textbox_hide_line_numbers(dred_textbox__get_internal_tb(pTextBox));
}


void dred_textbox_disable_vertical_scrollbar(dred_textbox* pTextBox)
{
    drgui_textbox_disable_vertical_scrollbar(dred_textbox__get_internal_tb(pTextBox));
}

void dred_textbox_enable_vertical_scrollbar(dred_textbox* pTextBox)
{
    drgui_textbox_enable_vertical_scrollbar(dred_textbox__get_internal_tb(pTextBox));
}

void dred_textbox_disable_horizontal_scrollbar(dred_textbox* pTextBox)
{
    drgui_textbox_disable_horizontal_scrollbar(dred_textbox__get_internal_tb(pTextBox));
}

void dred_textbox_enable_horizontal_scrollbar(dred_textbox* pTextBox)
{
    drgui_textbox_enable_horizontal_scrollbar(dred_textbox__get_internal_tb(pTextBox));
}



void dred_textbox_on_key_down(dred_textbox* pTextBox, drgui_key key, int stateFlags)
{
    dred_textbox_data* data = dred_control_get_extra_data(pTextBox);
    if (data == NULL) {
        return;
    }

    drgui_textbox_on_key_down(data->pInternalTB, key, stateFlags);
}

void dred_textbox_on_key_up(dred_textbox* pTextBox, drgui_key key, int stateFlags)
{
    dred_textbox_data* data = dred_control_get_extra_data(pTextBox);
    if (data == NULL) {
        return;
    }

    drgui_textbox_on_key_up(data->pInternalTB, key, stateFlags);
}

void dred_textbox_on_printable_key_down(dred_textbox* pTextBox, uint32_t utf32, int stateFlags)
{
    dred_textbox_data* data = dred_control_get_extra_data(pTextBox);
    if (data == NULL) {
        return;
    }

    drgui_textbox_on_printable_key_down(data->pInternalTB, utf32, stateFlags);
}
