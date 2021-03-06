#include "OptionMenu.h"
#include "ResourceManager.h"
#include "Font.h"

namespace rhythmus
{

// --------------------------- class OptionItem

void OptionItem::Load(const MetricGroup &metric)
{
  option_name.SetFont("SystemFont");  // XXX: fix later
  WheelItem::Load(metric);
}

// --------------------------- class OptionMenu

OptionMenu::OptionMenu()
{
  set_item_min_index(0);
  set_item_max_index(10);
}

void OptionMenu::Load(const MetricGroup &metric)
{
  Wheel::Load(metric);
}

WheelItem* OptionMenu::CreateMenuItem()
{
  return new OptionItem();
}

}