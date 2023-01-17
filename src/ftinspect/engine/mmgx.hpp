// mmgx.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include <vector>

#include <QString>


class Engine;

enum class MMGXState
{
  NoMMGX,
  MM, // Adobe MM
  GX_OVF, // GX or OpenType variable fonts
};


struct MMGXAxisInfo
{
  QString name;
  unsigned long tag;

  double minimum;
  double maximum;
  double def;

  bool hidden;
  bool isMM;

  static MMGXState get(Engine* engine,
                       std::vector<MMGXAxisInfo>& infos);


  friend bool
  operator==(const MMGXAxisInfo& lhs,
             const MMGXAxisInfo& rhs)
  {
    return lhs.name == rhs.name
           && lhs.tag == rhs.tag
           && lhs.minimum == rhs.minimum
           && lhs.maximum == rhs.maximum
           && lhs.def == rhs.def
           && lhs.hidden == rhs.hidden
           && lhs.isMM == rhs.isMM;
  }


  friend bool
  operator!=(const MMGXAxisInfo& lhs,
             const MMGXAxisInfo& rhs)
  {
    return !(lhs == rhs);
  }
};


// end of mmgx.hpp
