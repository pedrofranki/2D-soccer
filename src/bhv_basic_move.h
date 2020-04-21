// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:

   Z----------------------------------Z
   |    Fragments of the                              |
   |    file Created By: Amir Tavafi  |
   |                                  |
   |    Date Created:    2009/11/30,  |
   |                     1388/08/09   |
   |                                  |
   Z----------------------------------Z

 */

/////////////////////////////////////////////////////////////////////

#ifndef BHV_BASIC_MOVE_H
#define BHV_BASIC_MOVE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

#include <vector>

namespace rcsc {
class WorldModel;
}


class Bhv_BasicMove
    : public rcsc::SoccerBehavior {
public:
    Bhv_BasicMove()
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:
    double getDashPower( const rcsc::PlayerAgent * agent );
};



class Bhv_MarlikBlock
    : public rcsc::SoccerBehavior {
private:

public:

  static bool isInBlockPoint;
  static int timeAtBlockPoint;
  static rcsc::Vector2D opp_static_pos;

    bool execute( rcsc::PlayerAgent * agent );

private:
    bool doInterceptBall2011( rcsc::PlayerAgent * agent );
    bool doBlockMove( rcsc::PlayerAgent * agent );
    rcsc::Vector2D getBlockPoint( rcsc::PlayerAgent * agent );
};


#endif
