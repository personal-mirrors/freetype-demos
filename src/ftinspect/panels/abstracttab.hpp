// abstracttab.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once


// This is a pure, abstract interface for a 'tab' used within `ftinspect`.
// The interface itself does not inherit from `QWidget` but should be used as
// the second base class.
class AbstractTab
{
public:
  virtual ~AbstractTab() = default; // Must be `virtual` for `dynamic_cast`.

  virtual void repaintGlyph() = 0;
  virtual void reloadFont() = 0;
};


// end of abstracttab.hpp
