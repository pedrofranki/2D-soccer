// -*-c++-*-

/*
 *Copyright:

 Gliders2d
 Modified by Mikhail Prokopenko, Peter Wang

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_basic_move.h"
#include "strategy.h"
#include "bhv_basic_tackle.h"
#include "neck_offensive_intercept_neck.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/soccer_intention.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

using namespace rcsc;

int Bhv_MarlikBlock::timeAtBlockPoint = 0;
rcsc::Vector2D Bhv_MarlikBlock::opp_static_pos = rcsc::Vector2D( -30.0,0.0 );


/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_BasicMove::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_BasicMove" );

    const WorldModel & wm = agent->world();

    //-----------------------------------------------
    // tackle

// G2d: tackle probability
    double doTackleProb = 0.8;
    if (wm.ball().pos().x < 0.0)
    {
      doTackleProb = 0.5;
    }

    if ( Bhv_BasicTackle( doTackleProb, 80.0 ).execute( agent ) )
    {
        return true;
    }

    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

// G2d: to retrieve opp team name
    bool helios2018 = false;
    if (wm.opponentTeamName().find("HELIOS2018") != std::string::npos)
	helios2018 = true;

// G2d: role
    int role = Strategy::i().roleNumber( wm.self().unum() );


// G2D: blocking

    Vector2D ball = wm.ball().pos();

    double block_d = -10.0;

    Vector2D me = wm.self().pos();
    Vector2D homePos = target_point;
    int num = role;

    const PlayerPtrCont & opps = wm.opponentsFromBall();
    const PlayerObject * nearest_opp
        = ( opps.empty()
            ? static_cast< PlayerObject * >( 0 )
            : opps.front() );
    const double nearest_opp_dist = ( nearest_opp
                                      ? nearest_opp->distFromSelf()
                                      : 1000.0 );

if (ball.x < block_d)
{
  double block_line = -38.0;

    if (helios2018)
	block_line = -48.0;

// acknowledgement: fragments of Marlik-2012 code
    if( (num == 2 || num == 3) && homePos.x < block_line &&
        !( num == 2 && ball.x < -46.0 && ball.y > -18.0 && ball.y < -6.0 &&
          opp_min <= 3 && opp_min <= mate_min && ball.dist(me) < 9.5 ) &&
        !( num == 3 && ball.x < -46.0 && ball.y <  18.0 && ball.y >  6.0  &&
          opp_min <= 3 && opp_min <= mate_min && ball.dist(me) < 9.5 ) ) // do not block in this situation
    {
        // do nothing
    }
    else if( !helios2018 && (num == 2 || num == 3) && fabs(wm.ball().pos().y) > 22.0 )
    {
        // do nothing
    }
    else if( Bhv_MarlikBlock().execute( agent ) )
    {
       return true;
    }

} // end of block




// G2d: pressing
    int pressing = 13;

    if ( role >= 6 && role <= 8 && wm.ball().pos().x > -30.0 && wm.self().pos().x < 10.0 )
        pressing = 7;

    if (fabs(wm.ball().pos().y) > 22.0 && wm.ball().pos().x < 0.0 && wm.ball().pos().x > -36.5 && (role == 4 || role == 5) ) 
        pressing = 23;

    if (helios2018)
	pressing = 4;

    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min <= mate_min
                   && self_min < opp_min + pressing )
              )
         )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": intercept" );
        Body_Intercept().execute( agent );
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );

        return true;
    }


// G2D : offside trap
   double first = 0.0, second = 0.0;

        const PlayerPtrCont::const_iterator t3_end = wm.teammatesFromSelf().end();
        for ( PlayerPtrCont::const_iterator it = wm.teammatesFromSelf().begin();
              it != t3_end;
              ++it )
        {
            double x = (*it)->pos().x;
            if ( x < second )
            {
                second = x;
                if ( second < first )
                {
                    std::swap( first, second );
                }
            }
        }

   double buf1 = 3.5;
   double buf2 = 4.5;

   if( me.x < -37.0 && opp_min < mate_min && 
       (homePos.x > -37.5 || wm.ball().inertiaPoint(opp_min).x > -36.0 ) &&
         second + buf1 > me.x && wm.ball().pos().x > me.x + buf2)    
   {
        Body_GoToPoint( rcsc::Vector2D( me.x + 15.0, me.y ),
                        0.5, ServerParam::i().maxDashPower(), // maximum dash power
                        ServerParam::i().maxDashPower(),     // preferred dash speed
                        2,                                  // preferred reach cycle
                        true,                              // save recovery
                        5.0 ).execute( agent );

       if( wm.existKickableOpponent()
           && wm.ball().distFromSelf() < 12.0 )
             agent->setNeckAction( new Neck_TurnToBall() );
       else
             agent->setNeckAction( new Neck_TurnToBallOrScan() );
       return true;
   }



    const double dash_power = Strategy::get_normal_dash_power( wm );

    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_BasicMove target=(%.1f %.1f) dist_thr=%.2f",
                  target_point.x, target_point.y,
                  dist_thr );

    agent->debugClient().addMessage( "BasicMove%.0f", dash_power );
    agent->debugClient().setTarget( target_point );
    agent->debugClient().addCircle( target_point, dist_thr );

    if ( ! Body_GoToPoint( target_point, dist_thr, dash_power
                           ).execute( agent ) )
    {
        Body_TurnToBall().execute( agent );
    }

    if ( wm.existKickableOpponent()
         && wm.ball().distFromSelf() < 18.0 )
    {
        agent->setNeckAction( new Neck_TurnToBall() );
    }
    else
    {
        agent->setNeckAction( new Neck_TurnToBallOrScan() );
    }

    return true;
}


/*

   Z----------------------------------Z
   |    Fragments of the                              |
   |    file Created By: Amir Tavafi  |
   |                                  |
   |    Date Created:    2009/11/30,  |
   |                     1388/08/09   |
   |                                  |
   Z----------------------------------Z

*/


/// Execute Block Action
bool Bhv_MarlikBlock::execute( rcsc::PlayerAgent * agent )
{
  const rcsc::WorldModel & wm = agent->world();

  rcsc::Vector2D me = wm.self().pos();
  rcsc::Vector2D ball = wm.ball().pos();
  
  int myCycles = wm.interceptTable()->selfReachCycle();
  int tmmCycles = wm.interceptTable()->teammateReachCycle();
  int oppCycles = wm.interceptTable()->opponentReachCycle();

    int num = Strategy::i().roleNumber( wm.self().unum() );

  if( ( num == 2 && ball.x < -46.0 && ball.y > -18.0 && ball.y < -6.0 &&
        oppCycles <= 3 && oppCycles <= tmmCycles && ball.dist(me) < 9.0 ) ||
      ( num == 3 && ball.x < -46.0 && ball.y <  18.0 && ball.y >  6.0  &&
        oppCycles <= 3 && oppCycles <= tmmCycles && ball.dist(me) < 9.0 ) )
        if( doBlockMove( agent ) )
          return true;

  if (wm.opponentsFromBall().empty()) return false;

  rcsc::Vector2D opp = wm.opponentsFromBall().front()->pos();

  float maxDist = 11; // DutchOpen2012: was 14
  
  if( ball.x < -10 )
    maxDist=7; // DutchOpen2012: was 10
  
  if( doInterceptBall2011( agent ) )
     return true;

  double tackle_prob = 0.9;

  if( Bhv_BasicTackle( tackle_prob, 60.0 ).execute( agent ) )
     return true;

  if( (num == 2 || num == 3) && ball.x < -30.0 && ball.absY() > 16.0 )
     return false;

  if( num < 6 && ball.x < -30.0 && ball.x > -40.0 &&
      wm.countTeammatesIn( rcsc::Circle2D( rcsc::Vector2D(opp.x-3.5,opp.y), 3.5 ), 5, false ) > 1 )
    return false;

  if( (num == 2 || num == 3) && ball.x > wm.ourDefenseLineX() + maxDist && // for IO2012 changed from + 6.0 to 10.0
      ball.x > -30.0 && ball.x < 0.0 )
     return false;

  if( num == 6 && ball.x > wm.ourDefenseLineX() + 23.0 )
     return false;

  if( num == 2 && ball.x < wm.ourDefenseLineX() + 7.0 && ball.absY() < 12.0 &&
      ball.y < wm.self().pos().y + 7.5 && ball.y > wm.self().pos().y - 3.0 && ball.x > -38.0 && oppCycles < tmmCycles )
        if( doBlockMove( agent ) )
          return true;

  if( num == 6 && ball.x < -40.0 && ball.absY() < 6.0 && me.dist(ball) < 5.0 && oppCycles <= tmmCycles  )
        if( doBlockMove( agent ) )
          return true;

  if( num < 6 && ball.x < wm.ourDefenseLineX() + 5.0 &&
      ball.dist(wm.self().pos()) < 4.0 && ball.x > -38.0 && oppCycles < tmmCycles ) // IO2011: < 6.5
        if( doBlockMove( agent ) )
          return true;

  if( num == 3 && ball.x < wm.ourDefenseLineX() + 7.0 && ball.absY() < 12.0 &&
      ball.y > wm.self().pos().y + 7.0 && ball.y < wm.self().pos().y + 3.0 && ball.x > -38.0  && oppCycles < tmmCycles )
        if( doBlockMove( agent ) )
          return true;

  if( ( (num > 5 && num <= 8 && ball.x < 25.0) ||
      ( (num <=5 && ball.x < -30.0) ||
        (ball.x < wm.ourDefenseLineX() + maxDist + 2.0 && ball.x > -30.0 && ball.x < 0.0) ) ) && oppCycles < tmmCycles ) // for IO2012 changed from + 8.5 to 12.0
  {

    if( ball.x < -36 && ball.absY() < 7 &&
        myCycles > oppCycles && myCycles <= tmmCycles )
    {
      if( doBlockMove( agent ) )
        return true;
    }
    if( myCycles > oppCycles && myCycles < tmmCycles + 1 )
    {
      if( doBlockMove( agent ) )
        return true;
    }

  }

  timeAtBlockPoint = 0;

return false;
}

/// Do Intercept if Possible
bool Bhv_MarlikBlock::doInterceptBall2011( rcsc::PlayerAgent * agent )
{

    const rcsc::WorldModel & wm = agent->world();

    rcsc::Vector2D ball = wm.ball().pos();
    rcsc::Vector2D me = wm.self().pos();

    int myCycles = wm.interceptTable()->selfReachCycle();
    int tmmCycles = wm.interceptTable()->teammateReachCycle();
    int oppCycles = wm.interceptTable()->opponentReachCycle();

    if ( myCycles < oppCycles && myCycles < tmmCycles )
    {
        rcsc::Body_Intercept(true,ball).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }

    return false;
}

/// Going to Block Point & related Body Turns
bool Bhv_MarlikBlock::doBlockMove( rcsc::PlayerAgent * agent )
{
  const rcsc::WorldModel & wm = agent->world();

  int num = Strategy::i().roleNumber( wm.self().unum() );

  static bool isChaporasting;
  
  rcsc::Vector2D ball = wm.ball().pos();
  rcsc::Vector2D me = wm.self().pos() + wm.self().vel();

  int myCycles = wm.interceptTable()->selfReachCycle();
  int tmmCycles = wm.interceptTable()->teammateReachCycle();
  int oppCycles = wm.interceptTable()->opponentReachCycle();

  if (wm.opponentsFromBall().empty()) return false;

  rcsc::Vector2D blockPos = getBlockPoint( agent );
  rcsc::Vector2D opp = wm.opponentsFromBall().front()->pos() + wm.opponentsFromBall().front()->vel();
  rcsc::Vector2D oppNoVel = wm.opponentsFromBall().front()->pos();
  rcsc::Vector2D goal = rcsc::Vector2D(-52.0, 0.0);

  double dashPower = rcsc::ServerParam::i().maxDashPower();

  rcsc::Vector2D oppPos = wm.opponentsFromBall().front()->pos();
  rcsc::Vector2D oppVel = wm.opponentsFromBall().front()->vel();
  
  rcsc::Vector2D oppTarget = rcsc::Vector2D( -52.5, oppPos.y*0.95 );

   if( num == 7 || num == 9 )
        oppTarget = rcsc::Vector2D( -36.0, -12.0 );
   if( num == 8 || num == 10 )
        oppTarget = rcsc::Vector2D( -36.0, 12.0 );
     
   if( ball.x < -34.0 && ball.x > -40.0 )
        oppTarget = rcsc::Vector2D( -48.0, 0.0 );

   if( ball.x < -30 && ball.x > -40.0 && ball.absY() < 15 )
   {
     if( num == 2 )
        oppTarget = rcsc::Vector2D( -52.5, -4.0 );
     if( num == 3 )
        oppTarget = rcsc::Vector2D( -52.5, 4.0 );
   }
   
   if( ball.dist(goal) < 14 && ball.absY() > 8 )
        oppTarget = rcsc::Vector2D( -51.0, 0.0 );

   if( ball.x < -40.0 )
        oppTarget = rcsc::Vector2D( -49.0, 0.0 );
   
   if( ball.x < -47.0 )
        oppTarget = rcsc::Vector2D( -50.0, 0.0 );

   rcsc::Line2D targetLine = rcsc::Line2D( oppTarget, oppPos+oppVel );
     
   if( targetLine.dist(opp_static_pos) >= 0.2 )
   {
      if( oppVel.y > 0.0 )
         oppTarget = rcsc::Vector2D(-52.5, 9);
      if( oppVel.y < 0.0 )
         oppTarget = rcsc::Vector2D(-52.5, -9);
   }

  const rcsc::PlayerObject * opponent = wm.interceptTable()->fastestOpponent();
  rcsc::AngleDeg opp_body;

   if( ball.x < -36.0 && ball.absY() > 12.0 && me.dist(goal) > opp.dist(goal) + 5.0 )
     return false;

   int minOppDist = 3.5;

    if( ball.x < -36.0 && ball.absY() < 12.0 )
         minOppDist = 4.0;

  int minInterceptCycles = 2;

    if( ball.x < -36.0 && ball.absY() > 12.0 && me.dist(goal) < opp.dist(goal) )
         minInterceptCycles = 3; // 3 bud!
    if( ball.x < -36.0 && ball.absY() < 13.0 )
         minInterceptCycles = 4;
    if( ball.dist(goal) < 13 )
         minInterceptCycles = 5;

    if( ball.x < -36.0 && num == 6 )
      minInterceptCycles = 5;

    if( num == 7 || num == 8 )
      minInterceptCycles = 7;
    
    
     rcsc::Vector2D oppTarget22 = rcsc::Vector2D( -52.5, oppPos.y*0.95 ); // oppPos.y*0.975

   if( num == 7 || num == 9 )
        oppTarget22 = rcsc::Vector2D( -36.0, -12.0 );
   if( num == 8 || num == 10 )
        oppTarget22 = rcsc::Vector2D( -36.0, 12.0 );
     
   if( ball.x < -34.0 && ball.x > -40.0 )
        oppTarget22 = rcsc::Vector2D( -48.0, 0.0 );

   if( ball.x < -30 && ball.x > -40.0 && ball.absY() < 15 )
   {
     if( num == 2 )
        oppTarget22 = rcsc::Vector2D( -52.5, -4.0 );
     if( num == 3 )
        oppTarget22 = rcsc::Vector2D( -52.5, 4.0 );
   }

     if( ball.x < -40.0 )
        oppTarget22 = rcsc::Vector2D( -49.0, 0.0 );

     if( ball.x < -47.0 )
        oppTarget22 = rcsc::Vector2D( -50.0, 0.0 );

  static rcsc::Vector2D opp_static_pos = opp;

    if( myCycles <= 1 )
    {
      isChaporasting = false;
      rcsc::Body_Intercept(true,ball).execute( agent );
      agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
      return true;
    }
    if( myCycles < 20 && ( myCycles < tmmCycles || ( myCycles < tmmCycles + 2 && tmmCycles > 3 ) ) &&
         myCycles <= oppCycles )
    {
      isChaporasting = false;
      rcsc::Body_Intercept(true,ball).execute( agent );
      agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
      return true;
    }
    if( ball.x < -49.0 && ( (ball.y < 14 && ball.y > 7.0) || (ball.y > -14 && ball.y < -7.0) ) && myCycles < 5 ) // AmirZ RC2012
    {
      isChaporasting = false;
      rcsc::Body_Intercept(true,ball).execute( agent );
      agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
      return true;
    }
    if( num > 5 && ball.dist(goal) < 13.0 && myCycles < 5 && me.dist(goal) < ball.dist(goal) ) // AmirZ RC2012
    {
      isChaporasting = false;
      rcsc::Body_Intercept(true,ball).execute( agent );
      agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
      return true;
    }

    if( myCycles < tmmCycles && oppCycles >= 2 && myCycles <= oppCycles  )
    {
      isChaporasting = false;
      rcsc::Body_Intercept(true,ball).execute( agent );
      agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
      return true;
    }

   if( me.dist( blockPos ) < 1.7 && targetLine.dist(me) < 1.25 && myCycles <= minInterceptCycles &&
       targetLine.dist(opp_static_pos) < 0.2 )
   {
      isChaporasting = false;
      timeAtBlockPoint = 0;
      rcsc::Body_Intercept(true,ball).execute( agent );
      agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
      return true;
   }
   else if( (num==7 || num==8) && me.dist( blockPos ) < 2.0 && targetLine.dist(me) < 2 &&
            myCycles <= minInterceptCycles && targetLine.dist(opp_static_pos) < 0.5 )
   {
      isChaporasting = false;
      timeAtBlockPoint = 0;
      rcsc::Body_Intercept(true,ball).execute( agent );
      agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
      return true;
   }
   else if( ball.x < -36.0 && ball.absY() > 14.0 && me.dist(goal) + 1.5 < opp.dist(goal) && targetLine.dist(me) < 2.0 &&
            me.dist(opp) < 2.0 && me.dist(blockPos) < 3.0 && oppCycles < 3 && targetLine.dist(opp_static_pos) < 0.5 )
   // RC2012: for cross area
   {
      isChaporasting = false;
      timeAtBlockPoint = 0;
      rcsc::Body_Intercept(true,ball).execute( agent );
      agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
//      std::cout<<"\n\nxXxXxXxXxXxXxXx CYCLE "<<wm.time().cycle()<<" Block Intercepting while cross 2012 xXxXxXxXx\n\n";
      return true;
   }
   else if( (me.dist( blockPos ) < 3.5 || me.dist(opp) < 2.5 ) && me.dist( opp ) < minOppDist && ball.dist( oppNoVel ) < 1.2 &&
            me.dist(goal)+0.8 < opp.dist(goal) && targetLine.dist(me) < 0.9 &&
            targetLine.dist(opp_static_pos) < 0.2 ) // vele harif age tu masire targetesh nabud, nare tu in if!
   {
     
      if( !rcsc::Body_GoToPoint( opp, 0.4, dashPower, 1, false, true ).execute( agent ) ) // dist_thr 0.5 bud
      {
         rcsc::AngleDeg bodyAngle = agent->world().ball().angleFromSelf();
         if( me.y < 0.0 )
                bodyAngle -= 90.0;
         else
                bodyAngle += 90.0;

         rcsc::Body_TurnToAngle( bodyAngle ).execute( agent );
      }
     isChaporasting = false;

   }
   else if( (me.dist( blockPos ) < 4.0 || me.dist(opp) < 2.2 ) && me.dist( opp ) < 4.0 && ball.dist( oppNoVel ) < 1.2 &&
            me.dist(goal)+0.8 < opp.dist(goal) && wm.opponentsFromBall().front()->vel().r() < 0.1 &&
            targetLine.dist(me) < 0.9 )
   {
     
      if( !rcsc::Body_GoToPoint( opp, 0.4, dashPower, 1, false, true ).execute( agent ) ) // dist_thr 0.5 bud
      {
         rcsc::AngleDeg bodyAngle = agent->world().ball().angleFromSelf();
         if( me.y < 0.0 )
                bodyAngle -= 90.0;
         else
                bodyAngle += 90.0;

         rcsc::Body_TurnToAngle( bodyAngle ).execute( agent );
      }
     isChaporasting = false;

   }
   else if( (me.dist(blockPos) < 2.0 && me.dist(goal) < opp.dist(goal) && ball.dist( opp ) < 1.3
            /*&& timeAtBlockPoint > 0*/) || isChaporasting )
   {
     if( me.dist(goal) + 1 > opp.dist(goal) )
       isChaporasting = false;
     
     if( me.dist(goal) + 1 < blockPos.dist(goal) )
       isChaporasting = false;
     
     if( ball.dist(oppNoVel) > 1.1 )
       isChaporasting = false;

     if( me.dist(blockPos) > 4.0 )
       isChaporasting = false;
    
     if( ball.x < -37.0 && ball.absY() < 10.0 && me.dist(blockPos) < 3.0 )
       isChaporasting = false;

     if( opp.dist(me) > 3.0 )
       isChaporasting = false;
     
     double power = 100;

      if( opp.dist(opp_static_pos) > 0.3 )
        power = 100;
      else if( opp.dist(opp_static_pos) > 0.2 )
        power = 80;
      else
	power = 60;
     
//      rcsc::AngleDeg bodyAngle = agent->world().ball().angleFromSelf();
     rcsc::AngleDeg bodyAngle = (opp-oppTarget22).dir();
	
     if( me.y < 0 )
        bodyAngle -= 90.0;
     else
        bodyAngle += 90.0;

      
     if( std::fabs(wm.self().body().degree() - bodyAngle.degree()) < 10 )
     {
      rcsc::Line2D oppLine(opp,oppTarget22);

      float myXOnLine= oppLine.getX(me.y);
      float myYOnLine= oppLine.getY(me.x);
       
      if( ball.x < -30 && ball.absY() > 15 )
      {

       if( me.y > 0 )
       {
	if( wm.self().stamina()>4000 )
	{
	 if( me.x < myXOnLine )
	   agent->doDash(-power);
	 else
	   agent->doDash(power);
	}
       }
       else
       {
	if( wm.self().stamina()>4000 )
	{
	 if( me.x > myXOnLine )
	   agent->doDash(power);
	 else
	   agent->doDash(-power);
	}
       }
      }
      else
      {

       if( me.y > 0 )
       {
	if( wm.self().stamina()>4000 )
	{
	 if( me.y < myYOnLine )
	   agent->doDash(power);
	 else
	   agent->doDash(-power);
	}
       }
       else
       {
	if( wm.self().stamina()>4000 )
	{
	 if( me.y > myYOnLine )
	   agent->doDash(power);
	 else
	   agent->doDash(-power);
	}
       }
      }
      
       timeAtBlockPoint++;
     }
     else
     {
       rcsc::Body_TurnToAngle( bodyAngle ).execute( agent );
       timeAtBlockPoint++;
     }
    
     
     isChaporasting = true;
     //std::cout<<"\n******** CYCLE: "<<wm.time().cycle()<<" No. "<<wm.self().unum()<<" isChaporasting = TRUE \n";
     
   }   
   else if( !rcsc::Body_GoToPoint( blockPos, 0.4, dashPower, 1, 100, true, 30.0 ).execute( agent ) ) // dist_thr 0.5 bud
   {
//        rcsc::AngleDeg bodyAngle = agent->world().ball().angleFromSelf();
// 
//        if ( bodyAngle.degree() < 0.0 )
//               bodyAngle -= 90.0;
//        else
//               bodyAngle += 90.0;

     rcsc::AngleDeg bodyAngle = (opp-oppTarget22).dir();

     if( me.y < 0 )
        bodyAngle -= 90.0;
     else
        bodyAngle += 90.0;

       rcsc::Body_TurnToAngle( bodyAngle ).execute( agent );
       timeAtBlockPoint++;
       isChaporasting=false;
   }
   else
     timeAtBlockPoint = 0;

   opp_static_pos = opp;

   isChaporasting=false;

   if ( wm.ball().distFromSelf() < 20.0 &&
        ( wm.existKickableOpponent() || oppCycles <= 3 ) )
       agent->setNeckAction( new rcsc::Neck_TurnToBall() );
   else
       agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );

return true;
}

/// get Opponent BlockPoint for Moving to
rcsc::Vector2D Bhv_MarlikBlock::getBlockPoint( rcsc::PlayerAgent * agent )
{
  const rcsc::WorldModel & wm = agent->world();

  int num = Strategy::i().roleNumber( wm.self().unum() );

  rcsc::Vector2D ball = wm.ball().pos();
  rcsc::Vector2D me = wm.self().pos();
  rcsc::Vector2D blockPos = ball;

//   int myCycles = wm.interceptTable()->selfReachCycle();
//   int tmmCycles = wm.interceptTable()->teammateReachCycle();
  int oppCycles = wm.interceptTable()->opponentReachCycle();

  float oppDribbleSpeed = 0.675; // 0.75 bud!

    bool mt = false;
    if (wm.opponentTeamName().find("MT2018") != std::string::npos)
	mt = true;

  if (ball.x < -36.5 || me.x < -36.5) 
  {
         if ( fabs(ball.y) < 20.0 || fabs (me.y) < 20.0 )
	 {
	    if (mt)
		oppDribbleSpeed = 0.5;
	 }
         else
                oppDribbleSpeed = 0.575;
  }

  rcsc::Vector2D ballPred = ball;

   if( !wm.existKickableOpponent() )
        ballPred = wm.ball().inertiaPoint( oppCycles );

  rcsc::Vector2D oppPos = wm.opponentsFromBall().front()->pos();
  rcsc::Vector2D oppVel = wm.opponentsFromBall().front()->vel();

  rcsc::Vector2D oppTarget = rcsc::Vector2D( -52.5, oppPos.y*0.95 ); // oppPos.y*0.975

   if( num == 7 || num == 9 )
        oppTarget = rcsc::Vector2D( -36.0, -12.0 );
   if( num == 8 || num == 10 )
        oppTarget = rcsc::Vector2D( -36.0, 12.0 );
     
   if( ball.x < -34.0 && ball.x > -40.0 )
        oppTarget = rcsc::Vector2D( -48.0, 0.0 );

   if( ball.x < -30 && ball.x > -40.0 && ball.absY() < 15 )
   {
     if( num == 2 )
        oppTarget = rcsc::Vector2D( -52.5, -4.0 );
     if( num == 3 )
        oppTarget = rcsc::Vector2D( -52.5, 4.0 );
   }
   
   if( ball.dist(rcsc::Vector2D(-52.5,0.0)) < 14 && ball.absY() > 8 )
        oppTarget = rcsc::Vector2D( -51.0, 0.0 );

   if( ball.x < -40.0 )
        oppTarget = rcsc::Vector2D( -49.0, 0.0 );

   if( ball.x < -47.0 )
        oppTarget = rcsc::Vector2D( -50.0, 0.0 );
    
   rcsc::Vector2D nextBallPos = ballPred;
   nextBallPos += rcsc::Vector2D::polar2vector( oppDribbleSpeed, (oppTarget - ballPred).dir() );

  int oppTime;
   if( wm.existKickableOpponent() )
      oppTime = 0;
   else
      oppTime = oppCycles;

  while( wm.self().playerType().cyclesToReachDistance( me.dist(nextBallPos) ) > oppTime && oppTime < 50 )
  {
       if( nextBallPos.x > 36.0 && (num == 7 || num == 9) )
        oppTarget = rcsc::Vector2D( -36.0, -12.0 );
       if( nextBallPos.x > 30.0 && num == 11 )
        oppTarget = rcsc::Vector2D( -30.0, 0.0 );
       if( nextBallPos.x < -34.0 && nextBallPos.x > -40.0 )
        oppTarget = rcsc::Vector2D( -48.0, 0.0 );
       if( nextBallPos.x < -40.0 )
        oppTarget = rcsc::Vector2D( -48.0, 0.0 );
    
       nextBallPos += rcsc::Vector2D::polar2vector( oppDribbleSpeed, (oppTarget - ballPred).dir() );
       oppTime++;
  }

  if( oppTime >= 50 )
       blockPos = rcsc::Vector2D( -48.0, ball.y );
  else
       blockPos = nextBallPos;

  if( num < 6 && blockPos.x > 0.0 )
       blockPos.x = 10.0;

return blockPos;
}

