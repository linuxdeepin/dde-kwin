/*****************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2006 Lubos Lunak <l.lunak@kde.org>

You can Freely distribute this program under the GNU General Public
License. See the file "COPYING" for the exact licensing terms.
******************************************************************/

#include "effects.h"

#include "toplevel.h"
#include "client.h"
#include "scene.h"

namespace KWinInternal
{

//****************************************
// Effect
//****************************************

Effect::~Effect()
    {
    }

void Effect::windowUserMovedResized( Toplevel* , bool, bool )
    {
    }

void Effect::windowAdded( Toplevel* )
    {
    }

void Effect::windowDeleted( Toplevel* )
    {
    }

void Effect::prePaintScreen( int* mask, QRegion* region, int time )
    {
    effects->prePaintScreen( mask, region, time );
    }

void Effect::paintScreen( int mask, QRegion region, ScreenPaintData& data )
    {
    effects->paintScreen( mask, region, data );
    }
    
void Effect::postPaintScreen()
    {
    effects->postPaintScreen();
    }

void Effect::prePaintWindow( Scene::Window* w, int* mask, QRegion* region, int time )
    {
    effects->prePaintWindow( w, mask, region, time );
    }

void Effect::paintWindow( Scene::Window* w, int mask, QRegion region, WindowPaintData& data )
    {
    effects->paintWindow( w, mask, region, data );
    }

void Effect::postPaintWindow( Scene::Window* w )
    {
    effects->postPaintWindow( w );
    }

void MakeHalfTransparent::prePaintWindow( Scene::Window* w, int* mask, QRegion* region, int time )
    {
    const Client* c = dynamic_cast< const Client* >( w->window());
    if(( c != NULL && ( c->isMove() || c->isResize())) || w->window()->isDialog())
        {
        *mask |= Scene::PAINT_WINDOW_TRANSLUCENT;
        *mask &= ~Scene::PAINT_WINDOW_OPAQUE;
        }
    effects->prePaintWindow( w, mask, region, time );
    }

void MakeHalfTransparent::paintWindow( Scene::Window* w, int mask, QRegion region, WindowPaintData& data )
    {
    const Client* c = dynamic_cast< const Client* >( w->window());
    if( w->window()->isDialog())
        data.opacity *= 0.8;
    if( c->isMove() || c->isResize())
        data.opacity *= 0.5;
    effects->paintWindow( w, mask, region, data );
    }

void MakeHalfTransparent::windowUserMovedResized( Toplevel* c, bool first, bool last )
    {
    if( first || last )
        c->addDamageFull();
    }

ShakyMove::ShakyMove()
    {
    connect( &timer, SIGNAL( timeout()), SLOT( tick()));
    }

static const int shaky_diff[] = { 0, 1, 2, 3, 2, 1, 0, -1, -2, -3, -2, -1 };
static const int SHAKY_MAX = sizeof( shaky_diff ) / sizeof( shaky_diff[ 0 ] );

void ShakyMove::prePaintScreen( int* mask, QRegion* region, int time )
    {
    if( !windows.isEmpty())
        *mask |= Scene::PAINT_WINDOW_TRANSFORMED;
    effects->prePaintScreen( mask, region, time );
    }

void ShakyMove::prePaintWindow( Scene::Window* w, int* mask, QRegion* region, int time )
    {
    if( windows.contains( w->window()))
        *mask |= Scene::PAINT_WINDOW_TRANSFORMED;
    effects->prePaintWindow( w, mask, region, time );
    }

void ShakyMove::paintWindow( Scene::Window* w, int mask, QRegion region, WindowPaintData& data )
    {
    if( windows.contains( w->window()))
        data.xTranslate += shaky_diff[ windows[ w->window() ]];
    effects->paintWindow( w, mask, region, data );
    }

void ShakyMove::windowUserMovedResized( Toplevel* c, bool first, bool last )
    {
    if( first )
        {
        if( windows.isEmpty())
            timer.start( 50 );
        windows[ c ] = 0;
        }
    else if( last )
        {
        windows.remove( c );
        // just damage whole screen, transformation is involved
        c->workspace()->addDamageFull();
        if( windows.isEmpty())
            timer.stop();
        }
    }

void ShakyMove::windowDeleted( Toplevel* c )
    {
    windows.remove( c );
    if( windows.isEmpty())
        timer.stop();
    }

void ShakyMove::tick()
    {
    for( QMap< const Toplevel*, int >::Iterator it = windows.begin();
         it != windows.end();
         ++it )
        {
        if( *it == SHAKY_MAX - 1 )
            *it = 0;
        else
            ++(*it);
        // just damage whole screen, transformation is involved
        it.key()->workspace()->addDamageFull();
        }
    }

#if 0
void GrowMove::transformWindow( Toplevel* c, Matrix& matrix, EffectData& )
    {
    if( Client* c2 = dynamic_cast< Client* >( c ))
        if( c2->isMove())
            {
            Matrix m;
            m.m[ 0 ][ 0 ] = 1.2;
            m.m[ 1 ][ 1 ] = 1.4;
            matrix *= m;
            }
    }

void GrowMove::windowUserMovedResized( Toplevel* c, bool first, bool last )
    {
    if( first || last )
        {
        // TODO damage whole screen, transformation is involved
        c->workspace()->addDamage( c->geometry());
        }
    }
#endif

ShiftWorkspaceUp::ShiftWorkspaceUp( Workspace* ws )
    : up( false )
    , diff( 0 )
    , wspace( ws )
    {
    connect( &timer, SIGNAL( timeout()), SLOT( tick()));
    timer.start( 2000 );
    }

void ShiftWorkspaceUp::prePaintScreen( int* mask, QRegion* region, int time )
    {
    if( up && diff < 1000 )
        diff = qBound( 0, diff + time, 1000 ); // KDE3: note this differs from KCLAMP
    if( !up && diff > 0 )
        diff = qBound( 0, diff - time, 1000 );
    if( diff != 0 )
        *mask |= Scene::PAINT_SCREEN_TRANSFORMED;
    effects->prePaintScreen( mask, region, time );
    }

void ShiftWorkspaceUp::paintScreen( int mask, QRegion region, ScreenPaintData& data )
    {
    if( diff != 0 )
        data.yTranslate -= diff / 100;
    effects->paintScreen( mask, region, data );
    }

void ShiftWorkspaceUp::postPaintScreen()
    {
    if( up ? diff < 1000 : diff > 0 )
        wspace->addDamageFull(); // trigger next animation repaint
    effects->postPaintScreen();
    }

void ShiftWorkspaceUp::tick()
    {
    up = !up;
    wspace->addDamageFull();
    }


void FadeIn::prePaintWindow( Scene::Window* w, int* mask, QRegion* region, int time )
    {
    if( windows.contains( w->window()))
        {
        windows[ w->window() ] += time / 1000.; // complete change in 1000ms
        if( windows[ w->window() ] < 1 )
            {
            *mask |= Scene::PAINT_WINDOW_TRANSLUCENT;
            *mask &= ~Scene::PAINT_WINDOW_OPAQUE;
            }
        else
            windows.remove( w->window());
        }
    effects->prePaintWindow( w, mask, region, time );
    }

void FadeIn::paintWindow( Scene::Window* w, int mask, QRegion region, WindowPaintData& data )
    {
    if( windows.contains( w->window()))
        {
        data.opacity *= windows[ w->window()];
        }
    effects->paintWindow( w, mask, region, data );
    }

void FadeIn::postPaintWindow( Scene::Window* w )
    {
    if( windows.contains( w->window()))
        w->window()->addDamageFull(); // trigger next animation repaint
    effects->postPaintWindow( w );
    }

void FadeIn::windowAdded( Toplevel* c )
    {
    Client* cc = dynamic_cast< Client* >( c );
    if( cc == NULL || cc->isOnCurrentDesktop())
        {
        windows[ c ] = 0;
        c->addDamageFull();
        }
    }

void FadeIn::windowDeleted( Toplevel* c )
    {
    windows.remove( c );
    }


void ScaleIn::prePaintWindow( Scene::Window* w, int* mask, QRegion* region, int time )
    {
    if( windows.contains( w->window()))
        {
        windows[ w->window() ] += time / 500.; // complete change in 500ms
        if( windows[ w->window() ] < 1 )
            *mask |= Scene::PAINT_WINDOW_TRANSFORMED;
        else
            windows.remove( w->window());
        }
    effects->prePaintWindow( w, mask, region, time );
    }

void ScaleIn::paintWindow( Scene::Window* w, int mask, QRegion region, WindowPaintData& data )
    {
    if( windows.contains( w->window()))
        {
        data.xScale *= windows[ w->window()];
        data.yScale *= windows[ w->window()];
        data.xTranslate += int( w->window()->width() / 2 * ( 1 - windows[ w->window()] ));
        data.yTranslate += int( w->window()->height() / 2 * ( 1 - windows[ w->window()] ));
        }
    effects->paintWindow( w, mask, region, data );
    }

void ScaleIn::postPaintWindow( Scene::Window* w )
    {
    if( windows.contains( w->window()))
        w->window()->addDamageFull(); // trigger next animation repaint
    effects->postPaintWindow( w );
    }

void ScaleIn::windowAdded( Toplevel* c )
    {
    Client* cc = dynamic_cast< Client* >( c );
    if( cc == NULL || cc->isOnCurrentDesktop())
        {
        windows[ c ] = 0;
        c->addDamageFull();
        }
    }

void ScaleIn::windowDeleted( Toplevel* c )
    {
    windows.remove( c );
    }

//****************************************
// EffectsHandler
//****************************************

EffectsHandler::EffectsHandler( Workspace* ws )
    : current_paint_window( 0 )
    , current_paint_screen( 0 )
    {
    if( !compositing())
        return;
//    effects.append( new MakeHalfTransparent );
//    effects.append( new ShakyMove );
//    effects.append( new GrowMove );
//    effects.append( new ShiftWorkspaceUp( ws ));
    effects.append( new FadeIn );
    effects.append( new ScaleIn );
    }

EffectsHandler::~EffectsHandler()
    {
    foreach( Effect* e, effects )
        delete e;
    }

void EffectsHandler::windowUserMovedResized( Toplevel* c, bool first, bool last )
    {
    foreach( Effect* e, effects )
        e->windowUserMovedResized( c, first, last );
    }

void EffectsHandler::windowAdded( Toplevel* c )
    {
    foreach( Effect* e, effects )
        e->windowAdded( c );
    }

void EffectsHandler::windowDeleted( Toplevel* c )
    {
    foreach( Effect* e, effects )
        e->windowDeleted( c );
    }

// start another painting pass
void EffectsHandler::startPaint()
    {
    assert( current_paint_screen == 0 );
    assert( current_paint_window == 0 );
    }

// the idea is that effects call this function again which calls the next one
void EffectsHandler::prePaintScreen( int* mask, QRegion* region, int time )
    {
    if( current_paint_screen < effects.size())
        {
        effects[ current_paint_screen++ ]->prePaintScreen( mask, region, time );
        --current_paint_screen;
        }
    // no special final code
    }

void EffectsHandler::paintScreen( int mask, QRegion region, ScreenPaintData& data )
    {
    if( current_paint_screen < effects.size())
        {
        effects[ current_paint_screen++ ]->paintScreen( mask, region, data );
        --current_paint_screen;
        }
    else
        scene->finalPaintScreen( mask, region, data );
    }

void EffectsHandler::postPaintScreen()
    {
    if( current_paint_screen < effects.size())
        {
        effects[ current_paint_screen++ ]->postPaintScreen();
        --current_paint_screen;
        }
    // no special final code
    }

void EffectsHandler::prePaintWindow( Scene::Window* w, int* mask, QRegion* region, int time )
    {
    if( current_paint_window < effects.size())
        {
        effects[ current_paint_window++ ]->prePaintWindow( w, mask, region, time );
        --current_paint_window;
        }
    // no special final code
    }

void EffectsHandler::paintWindow( Scene::Window* w, int mask, QRegion region, WindowPaintData& data )
    {
    if( current_paint_window < effects.size())
        {
        effects[ current_paint_window++ ]->paintWindow( w, mask, region, data );
        --current_paint_window;
        }
    else
        scene->finalPaintWindow( w, mask, region, data );
    }

void EffectsHandler::postPaintWindow( Scene::Window* w )
    {
    if( current_paint_window < effects.size())
        {
        effects[ current_paint_window++ ]->postPaintWindow( w );
        --current_paint_window;
        }
    // no special final code
    }

EffectsHandler* effects;

} // namespace

#include "effects.moc"
