//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <data/field/text.hpp>
#include <util/tagged_string.hpp>
#include <script/script.hpp>

// ----------------------------------------------------------------------------- : TextField

TextField::TextField()
  : multi_line(false)
  , default_name(_("Default"))
{}

IMPLEMENT_FIELD_TYPE(Text, "text");

void TextField::initDependencies(Context& ctx, const Dependency& dep) const {
  Field        ::initDependencies(ctx, dep);
  script        .initDependencies(ctx, dep);
  default_script.initDependencies(ctx, dep);
}


IMPLEMENT_REFLECTION(TextField) {
  REFLECT_BASE(Field);
  REFLECT(multi_line);
  REFLECT(script);
  REFLECT_N("default", default_script);
  REFLECT(default_name);
}

// ----------------------------------------------------------------------------- : TextStyle

TextLayoutP dummy_layout() {
  auto layout = make_intrusive<TextLayout>();
  auto line = make_intrusive<LineLayout>(0, 0, 0, LineLayout::Type::LINE);
  auto paragraph = make_intrusive<LineLayout>(0, 0, 0, LineLayout::Type::PARAGRAPH);
  auto block = make_intrusive<LineLayout>(0, 0, 0, LineLayout::Type::BLOCK);
  paragraph->lines.push_back(line);
  block->lines.push_back(line);
  layout->lines.push_back(line);
  block->paragraphs.push_back(paragraph);
  layout->paragraphs.push_back(paragraph);
  layout->blocks.push_back(block);
  return layout;
}

TextStyle::TextStyle(const TextFieldP& field)
  : Style(field)
  , always_symbol(false), allow_formating(true)
  , alignment(ALIGN_TOP_LEFT)
  , padding_left  (0), padding_left_min  (10000)
  , padding_right (0), padding_right_min (10000)
  , padding_top   (0), padding_top_min   (10000)
  , padding_bottom(0), padding_bottom_min(10000)
  , line_height_soft(1.0)
  , line_height_hard(1.0)
  , line_height_line(1.0)
  , line_height_soft_max(0.0)
  , line_height_hard_max(0.0)
  , line_height_line_max(0.0)
  , paragraph_height(-1)
  , direction(LEFT_TO_RIGHT)
  , layout(dummy_layout())
{}

double TextStyle::getStretch() const {
  if (layout->width > 0 && (alignment() & ALIGN_STRETCH)) {
    double factor = (width - padding_left - padding_right) / layout->width;
    if (!(alignment() & ALIGN_IF_OVERFLOW) || factor < 1.0) {
      return factor;
    }
  }
  return 1.0;
}

int TextStyle::update(Context& ctx) {
  int changes = Style::update(ctx);
  changes |= font       .update(ctx) * CHANGE_OTHER;
  changes |= symbol_font.update(ctx) * CHANGE_OTHER;
  changes |= alignment  .update(ctx) * CHANGE_OTHER;
  changes |= padding_left        .update(ctx) * CHANGE_OTHER;
  changes |= padding_left_min    .update(ctx) * CHANGE_OTHER;
  changes |= padding_right       .update(ctx) * CHANGE_OTHER;
  changes |= padding_right_min   .update(ctx) * CHANGE_OTHER;
  changes |= padding_top         .update(ctx) * CHANGE_OTHER;
  changes |= padding_top_min     .update(ctx) * CHANGE_OTHER;
  changes |= padding_bottom      .update(ctx) * CHANGE_OTHER;
  changes |= padding_bottom_min  .update(ctx) * CHANGE_OTHER;
  changes |= line_height_soft    .update(ctx) * CHANGE_OTHER;
  changes |= line_height_hard    .update(ctx) * CHANGE_OTHER;
  changes |= line_height_line    .update(ctx) * CHANGE_OTHER;
  changes |= line_height_soft_max.update(ctx) * CHANGE_OTHER;
  changes |= line_height_hard_max.update(ctx) * CHANGE_OTHER;
  changes |= line_height_line_max.update(ctx) * CHANGE_OTHER;
  return changes;
}
void TextStyle::initDependencies(Context& ctx, const Dependency& dep) const {
  Style     ::initDependencies(ctx, dep);
//  font       .initDependencies(ctx, dep);
//  symbol_font.initDependencies(ctx, dep);
}
void TextStyle::checkContentDependencies(Context& ctx, const Dependency& dep) const {
  Style   ::checkContentDependencies(ctx, dep);
  alignment.initDependencies(ctx, dep);
}

template <typename T> void reflect_layout(T& handler,         const TextLayoutP& layout) {}
template <>           void reflect_layout(GetMember& handler, const TextLayoutP& layout) {
  assert(layout);
  REFLECT_N("layout", layout);
  REFLECT_N("content_width",  layout->width);
  REFLECT_N("content_height", layout->height);
  REFLECT_N("content_lines",  layout->lines.size());
}

template <> void GetMember::handle(LineLayout const& obj) { obj.reflect(*this); }
template <> void GetDefaultMember::handle(LineLayout const& obj) {}
void LineLayout::reflect(GetMember& handler) const {
  REFLECT(width);
  REFLECT(top);
  REFLECT(height);
  REFLECT_N("bottom", bottom());
  REFLECT_N("middle", top + height/2);
  if (type > Type::LINE) REFLECT(lines);
  if (type > Type::PARAGRAPH) REFLECT(paragraphs);
  if (type > Type::BLOCK) REFLECT(blocks);
}

template <> void GetMember::handle(TextLayout const& obj) { obj.reflect(*this); }
template <> void GetDefaultMember::handle(TextLayout const& obj) {}
void TextLayout::reflect(GetMember& handler) const {
  REFLECT_BASE(LineLayout);
  REFLECT(separators);
}

IMPLEMENT_REFLECTION(TextStyle) {
  REFLECT_BASE(Style);
  REFLECT(font);
  REFLECT(symbol_font);
  REFLECT(always_symbol);
  REFLECT(allow_formating);
  REFLECT(alignment);
  REFLECT(padding_left);
  REFLECT(padding_right);
  REFLECT(padding_top);
  REFLECT(padding_bottom);
  REFLECT(padding_left_min);
  REFLECT(padding_right_min);
  REFLECT(padding_top_min);
  REFLECT(padding_bottom_min);
  REFLECT(line_height_soft);
  REFLECT(line_height_hard);
  REFLECT(line_height_line);
  REFLECT(line_height_soft_max);
  REFLECT(line_height_hard_max);
  REFLECT(line_height_line_max);
  REFLECT(paragraph_height);
  REFLECT(direction);
  reflect_layout(handler, layout);
}

// ----------------------------------------------------------------------------- : TextValue

String TextValue::toString() const {
  return untag_hide_sep(value());
}
bool TextValue::update(Context& ctx) {
  updateAge();
  WITH_DYNAMIC_ARG(last_update_age,     last_update.get());
  WITH_DYNAMIC_ARG(value_being_updated, this);
  bool change = field().default_script.invokeOnDefault(ctx, value)
              | field().        script.invokeOn(ctx, value);
  if (change) last_update.update();
  updateSortValue(ctx);
  return change;
}

IMPLEMENT_REFLECTION_NAMELESS(TextValue) {
  if (fieldP->save_value || !handler.isWriting) REFLECT_NAMELESS(value);
}

// ----------------------------------------------------------------------------- : FakeTextValue

FakeTextValue::FakeTextValue(const TextFieldP& field, String* underlying, bool editable, bool untagged)
  : TextValue(field), underlying(underlying)
  , editable(editable), untagged(untagged)
{}

void FakeTextValue::store() {
  if (underlying) {
    if (editable) {
      *underlying = untagged ? untag(value) : value();
    } else {
      retrieve();
    }
  }
}
void FakeTextValue::retrieve() {
  if (underlying) {
    value.assign(untagged ? escape(*underlying) : *underlying);
  } else {
    value.assign(wxEmptyString);
  }
}

void FakeTextValue::onAction(Action& a, bool undone) {
  store();
}

bool FakeTextValue::equals(const Value* that) {
  if (this == that) return true;
  if (!underlying)  return false;
  const FakeTextValue* thatT = dynamic_cast<const FakeTextValue*>(that);
  if (!thatT || underlying != thatT->underlying) return false;
  // update the value
  retrieve();
  return true;
}
