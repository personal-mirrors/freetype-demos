// charmapcombobox.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "charmapcombobox.hpp"
#include "../engine/engine.hpp"


CharMapComboBox::CharMapComboBox(QWidget* parent,
                                 Engine* engine,
                                 bool haveGlyphOrder)
: QComboBox(parent),
  haveGlyphOrder_(haveGlyphOrder),
  engine_(engine)
{
  setToolTip("Set current charmap.");
  connect(this, QOverload<int>::of(&CharMapComboBox::currentIndexChanged),
          this, &CharMapComboBox::updateToolTip);
}


int
CharMapComboBox::currentCharMapIndex()
{
  auto index = haveGlyphOrder_ ? currentIndex() - 1 : currentIndex();
  if (index < 0 || charMaps_.size() <= static_cast<unsigned>(index))
    return -1;
  return index;
}


int
CharMapComboBox::defaultFirstGlyphIndex()
{
  auto newIndex = currentCharMapIndex();
  if (newIndex < 0)
    return 0;
  if (charMaps_[newIndex].maxIndex <= 20)
    return charMaps_[newIndex].maxIndex - 1;
  return 0x20;
}


void
CharMapComboBox::repopulate()
{
  repopulate(engine_->currentFontCharMaps());
}


#define EncodingRole (Qt::UserRole + 10)

void
CharMapComboBox::repopulate(std::vector<CharMapInfo>& charMaps)
{
  if (charMaps_ == charMaps)
  {
    charMaps_ = charMaps; // Still need to substitute because ptr may differ.
    return;
  }
  charMaps_ = charMaps;
  int oldIndex = currentIndex();
  unsigned oldEncoding = 0u;

  // Using additional UserRole to store encoding ID.
  auto oldEncodingV = itemData(oldIndex, EncodingRole);
  if (oldEncodingV.isValid() && oldEncodingV.canConvert<unsigned>())
    oldEncoding = oldEncodingV.value<unsigned>();

  { // This brace isn't for the `if` statement!
    // Suppress events during updating.
    QSignalBlocker selectorBlocker(this);

    clear();
    if (haveGlyphOrder_)
    {
      addItem(tr("Glyph Order"));
      setItemData(0, 0u, EncodingRole);
    }

    int newIndex = 0;
    for (auto& map : charMaps)
    {
      auto i = count();
      auto str = tr("%1: %2 (platform %3, encoding %4)")
                   .arg(i)
                   .arg(*map.encodingName)
                   .arg(map.platformID)
                   .arg(map.encodingID);
      addItem(str);
      setItemData(i, str, Qt::ToolTipRole);

      auto encoding = static_cast<unsigned>(map.encoding);
      setItemData(i, encoding, EncodingRole);

      if (encoding == oldEncoding && i == oldIndex)
        newIndex = i;
    }

    // This shouldn't emit any event either because force repainting
    // will happen later, so embrace it into blocker block.
    setCurrentIndex(newIndex);
    updateToolTip();
  }

  emit forceUpdateLimitIndex();
}


void
CharMapComboBox::updateToolTip()
{
  auto index = currentIndex();
  if (index >= 0 && index < count())
    setToolTip(this->currentText());
}


// end of charmapcombobox.cpp
