// abstracttab.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

// This is an pure abstract interface for a ftinspect "tab".
// The interface itself does not inherit from `QWidget`, but should be used as
// the second base class.
class AbstractTab
{
public:
  virtual ~AbstractTab() = default; // must be `virtual` for `dynamic_cast`

  virtual void syncSettings() = 0;
  virtual void setDefaults() = 0;
  virtual void repaintGlyph() = 0;
  virtual void reloadFont() = 0;
};


// end of abstracttab.hpp
